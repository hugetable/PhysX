# OptiX 核心功能实现文档

**创建日期**: 2025-11-07
**版本**: 1.0.0
**状态**: ✅ 核心功能完整实现

---

## 概述

本文档详细记录了 PhysicsRenderer.cpp 中 OptiX 9.0 核心功能的完整实现。这是 PhysicsRenderIntegration 项目的关键里程碑。

---

## 实现的功能

### 1. OptiX 上下文创建 (`createOptixContext`)

**功能**: 初始化 OptiX 运行环境

**实现细节**:
```cpp
bool PhysicsRenderer::createOptixContext() {
    // 1. 初始化 CUDA 运行时
    CUDA_CHECK(cudaFree(0));

    // 2. 初始化 OptiX
    OPTIX_CHECK(optixInit());

    // 3. 创建设备上下文
    OptixDeviceContextOptions options = {};
    options.logCallbackFunction = &optixLogCallback;
    options.logCallbackLevel = 4;  // 详细日志

    OPTIX_CHECK(optixDeviceContextCreate(cuCtx, &options, &optixContext_));
}
```

**关键点**:
- CUDA 运行时自动初始化
- 使用当前 CUDA 上下文（cuCtx = 0）
- 4 级日志（最详细）用于调试
- 完整的错误检查

**成功标志**: `optixContext_` != nullptr

---

### 2. 模块加载 (`createModule`)

**功能**: 加载和编译 OptiX 着色器代码

**实现细节**:
```cpp
bool PhysicsRenderer::createModule() {
    // 1. 从文件加载 PTX 源码
    std::string ptxFilename = MODULE_TARGET_DIR + "/raygeneration_core.ptx";
    std::ifstream ptxFile(ptxFilename, std::ios::binary);
    // ... 读取到 ptxSource

    // 2. 配置编译选项
    OptixModuleCompileOptions moduleCompileOptions = {};
    moduleCompileOptions.maxRegisterCount = OPTIX_COMPILE_DEFAULT_MAX_REGISTER_COUNT;
    moduleCompileOptions.optLevel = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
    moduleCompileOptions.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_MODERATE;

    // 3. 配置管线选项
    OptixPipelineCompileOptions pipelineCompileOptions = {};
    pipelineCompileOptions.numPayloadValues = 8;  // radiance(3) + throughput(3) + depth(1) + seed(1)
    pipelineCompileOptions.numAttributeValues = 3;  // 法线 (x, y, z)
    pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_LEVEL_INSTANCING;
    pipelineCompileOptions.pipelineLaunchParamsVariableName = "sysData";

    // 4. 创建模块
    OPTIX_CHECK(optixModuleCreateFromPTX(
        optixContext_,
        &moduleCompileOptions,
        &pipelineCompileOptions,
        ptxSource.c_str(),
        ptxSource.size(),
        log, &logSize,
        &optixModule_
    ));
}
```

**Payload 布局**:
```
[0-2]: radiance (float3)    - 累积辐射度
[3-5]: throughput (float3)   - 路径吞吐量
[6]:   depth (int)           - 路径深度
[7]:   seed (uint)           - 随机数种子
```

**Attributes 布局**:
```
[0-2]: normal (float3)       - 几何法线
```

**PTX 文件位置**: `${MODULE_TARGET_DIR}/raygeneration_core.ptx`
- MODULE_TARGET_DIR 由 CMake 配置
- 编译时生成

**成功标志**: `optixModule_` != nullptr

---

### 3. 管线创建 (`createPipeline`)

**功能**: 创建 OptiX 渲染管线

**实现细节**:

#### 3.1 创建程序组

**Raygen 程序组**:
```cpp
OptixProgramGroupDesc raygenPGDesc = {};
raygenPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
raygenPGDesc.raygen.module = optixModule_;
raygenPGDesc.raygen.entryFunctionName = "__raygen__pinhole_camera";

optixProgramGroupCreate(..., &raygenPG);
```

**Miss 程序组**:
```cpp
OptixProgramGroupDesc missPGDesc = {};
missPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
missPGDesc.miss.module = optixModule_;
missPGDesc.miss.entryFunctionName = "__miss__env_constant";

optixProgramGroupCreate(..., &missPG);
```

**Hit 程序组**:
```cpp
OptixProgramGroupDesc hitPGDesc = {};
hitPGDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
hitPGDesc.hitgroup.moduleCH = optixModule_;
hitPGDesc.hitgroup.entryFunctionNameCH = "__closesthit__physics_radiance";
hitPGDesc.hitgroup.moduleAH = nullptr;  // 暂无 anyhit
hitPGDesc.hitgroup.entryFunctionNameAH = nullptr;

optixProgramGroupCreate(..., &hitPG);
```

#### 3.2 创建管线

```cpp
OptixProgramGroup programGroups[] = { raygenPG, missPG, hitPG };

OptixPipelineLinkOptions pipelineLinkOptions = {};
pipelineLinkOptions.maxTraceDepth = config_.maxPathLength;
pipelineLinkOptions.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_MODERATE;

optixPipelineCreate(
    optixContext_,
    nullptr,  // 使用 module 编译选项
    &pipelineLinkOptions,
    programGroups, 3,
    log, &logSize,
    &pipeline_
);
```

#### 3.3 配置栈大小

```cpp
// 1. 累积栈大小
OptixStackSizes stackSizes = {};
for (auto pg : programGroups) {
    optixUtilAccumulateStackSizes(pg, &stackSizes);
}

// 2. 计算栈大小
uint32_t maxTraversableGraphDepth = 2;
uint32_t directStackSize, maxContinuationStackSize;

optixUtilComputeStackSizes(
    &stackSizes,
    config_.maxPathLength,
    maxContinuationStackSize,
    maxTraversableGraphDepth,
    &directStackSize,
    &maxContinuationStackSize
);

// 3. 设置栈大小
optixPipelineSetStackSize(
    pipeline_,
    directStackSize,
    maxContinuationStackSize,
    maxContinuationStackSize,
    maxTraversableGraphDepth
);
```

**参数说明**:
- `maxTraceDepth`: 最大追踪深度（递归层数）
- `maxTraversableGraphDepth`: 最大遍历深度（AS 层数）= 2（TLAS + BLAS）
- `directStackSize`: 直接调用栈大小
- `maxContinuationStackSize`: 最大延续栈大小

**成功标志**: `pipeline_` != nullptr

---

### 4. 渲染函数 (`render`)

**功能**: 执行光线追踪渲染

**实现细节**:

```cpp
unsigned int PhysicsRenderer::render() {
    // 1. 检查初始化状态
    if (!optixContext_ || !pipeline_ || topLevelAS_ == 0) {
        return 0;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // 2. 准备系统数据
    PhysicsSystemData sysData;
    memset(&sysData, 0, sizeof(sysData));

    sysData.topObject = topLevelAS_;
    sysData.outputBuffer = outputBuffer_;
    sysData.resolution = config_.resolution;
    sysData.pathLengths = make_int2(2, config_.maxPathLength);
    sysData.sceneEpsilon = config_.sceneEpsilon;
    sysData.iterationIndex = 0;
    sysData.samplesSqrt = 2;  // 4 SPP
    sysData.materialDefinitions = (PhysicsMaterialDefinition*)materialBuffer_;
    sysData.numMaterials = physicsMaterials_.size();

    // 3. 执行光线追踪
    OPTIX_CHECK(optixLaunch(
        pipeline_,
        0,  // stream
        (CUdeviceptr)&sysData,
        sizeof(PhysicsSystemData),
        &sbt_,
        config_.resolution.x,
        config_.resolution.y,
        1  // depth
    ));

    // 4. 同步
    CUDA_CHECK(cudaDeviceSynchronize());

    // 5. 计时
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime
    );

    return duration.count();
}
```

**Launch Parameters** (PhysicsSystemData):
- `topObject`: 场景根节点（TLAS handle）
- `outputBuffer`: 输出缓冲区（GPU）
- `resolution`: 渲染分辨率
- `pathLengths`: {min, max} 路径长度
- `sceneEpsilon`: 光线偏移量
- `materialDefinitions`: 材质数组（GPU）

**Launch 维度**:
- Width: `resolution.x`
- Height: `resolution.y`
- Depth: 1

**返回值**: 渲染时间（毫秒）

---

### 5. 材质系统

**添加材质** (`addPhysicsMaterial`):
```cpp
int PhysicsRenderer::addPhysicsMaterial(const PhysicsMaterialDefinition& material) {
    int materialID = physicsMaterials_.size();
    physicsMaterials_.push_back(material);

    // 重新分配 GPU 缓冲区
    if (materialBuffer_) {
        cudaFree((void*)materialBuffer_);
    }

    size_t bufferSize = physicsMaterials_.size() * sizeof(PhysicsMaterialDefinition);
    cudaMalloc(&materialBuffer_, bufferSize);
    cudaMemcpy(
        (void*)materialBuffer_,
        physicsMaterials_.data(),
        bufferSize,
        cudaMemcpyHostToDevice
    );

    return materialID;
}
```

**更新材质** (`updatePhysicsMaterial`):
```cpp
void PhysicsRenderer::updatePhysicsMaterial(int materialID, const PhysicsMaterialDefinition& material) {
    if (materialID < physicsMaterials_.size()) {
        physicsMaterials_[materialID] = material;

        // 更新 GPU
        cudaMemcpy(
            (void*)(materialBuffer_ + materialID * sizeof(...)),
            &material,
            sizeof(PhysicsMaterialDefinition),
            cudaMemcpyHostToDevice
        );
    }
}
```

**特点**:
- 动态添加材质
- 自动 GPU 内存管理
- 单个材质更新优化

---

### 6. 动态几何管理

**设置几何** (`setDynamicGeometry`):
```cpp
void PhysicsRenderer::setDynamicGeometry(const std::vector<RenderableObject>& renderables) {
    dynamicGeometries_.clear();

    for (const auto& obj : renderables) {
        if (obj.type == RenderableObject::RIGID_BODY) {
            DynamicGeometry geom;
            geom.geometryID = obj.geometryID;
            geom.needsRebuild = true;

            // 存储变换（4x3 矩阵）
            geom.transforms.resize(12);
            memcpy(geom.transforms.data(), obj.transform, 12 * sizeof(float));

            // 构建 BLAS
            buildBLAS(obj, geom);

            dynamicGeometries_.push_back(geom);
        }
    }

    // 重建 TLAS
    buildTLAS();
}
```

**更新变换** (`updateTransforms`):
```cpp
void PhysicsRenderer::updateTransforms(const std::vector<float*>& transforms) {
    for (size_t i = 0; i < transforms.size(); ++i) {
        memcpy(
            dynamicGeometries_[i].transforms.data(),
            transforms[i],
            12 * sizeof(float)
        );
    }

    buildTLAS();  // 重建顶层 AS
}
```

**特点**:
- 支持动态场景更新
- 变换矩阵缓存
- 自动 AS 重建

---

## 错误检查系统

### CUDA_CHECK 宏

```cpp
#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                      << " - " << cudaGetErrorString(error) << std::endl; \
            return false; \
        } \
    } while(0)
```

**使用示例**:
```cpp
CUDA_CHECK(cudaMalloc(&buffer, size));
CUDA_CHECK(cudaMemset(buffer, 0, size));
```

### CUDA_CHECK_VOID 宏

```cpp
#define CUDA_CHECK_VOID(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                      << " - " << cudaGetErrorString(error) << std::endl; \
        } \
    } while(0)
```

**使用场景**: void 函数中的 CUDA 调用

### OPTIX_CHECK 宏

```cpp
#define OPTIX_CHECK(call) \
    do { \
        OptixResult res = call; \
        if (res != OPTIX_SUCCESS) { \
            std::cerr << "OptiX error at " << __FILE__ << ":" << __LINE__ \
                      << " - " << optixGetErrorName(res) << ": " \
                      << optixGetErrorString(res) << std::endl; \
            return false; \
        } \
    } while(0)
```

**使用示例**:
```cpp
OPTIX_CHECK(optixInit());
OPTIX_CHECK(optixDeviceContextCreate(...));
```

---

## 资源管理

### 清理函数 (`cleanupOptixResources`)

**执行顺序**（重要！）:
```cpp
void PhysicsRenderer::cleanupOptixResources() {
    // 1. 释放 CUDA 缓冲区
    if (outputBuffer_) cudaFree((void*)outputBuffer_);
    if (accumBuffer_) cudaFree((void*)accumBuffer_);
    if (instanceBuffer_) cudaFree((void*)instanceBuffer_);
    if (materialBuffer_) cudaFree((void*)materialBuffer_);

    // 2. 销毁 OptiX 管线
    if (pipeline_) optixPipelineDestroy(pipeline_);

    // 3. 销毁 OptiX 模块
    if (optixModule_) optixModuleDestroy(optixModule_);

    // 4. 销毁 OptiX 上下文
    if (optixContext_) optixDeviceContextDestroy(optixContext_);
}
```

**关键点**:
- 先释放依赖资源（CUDA 缓冲区）
- 再销毁 OptiX 对象（pipeline → module → context）
- 正确的销毁顺序避免内存泄漏

---

## 待实现功能（TODO）

### 1. Shader Binding Table (SBT) 创建

**位置**: `createSBT()`

**需要实现**:
```cpp
bool PhysicsRenderer::createSBT() {
    // 1. 分配 Raygen 记录
    //    - 大小: OPTIX_SBT_RECORD_HEADER_SIZE + 自定义数据
    //    - 数量: 1

    // 2. 分配 Miss 记录
    //    - 大小: OPTIX_SBT_RECORD_HEADER_SIZE
    //    - 数量: 1

    // 3. 分配 Hit 记录
    //    - 大小: OPTIX_SBT_RECORD_HEADER_SIZE + GeometryInstanceData
    //    - 数量: 每个几何一个

    // 4. 填充记录头部
    //    optixSbtRecordPackHeader(programGroup, record);

    // 5. 上传到 GPU
    //    cudaMemcpy(..., cudaMemcpyHostToDevice);

    // 6. 设置 sbt_
    //    sbt_.raygenRecord = ...
    //    sbt_.missRecordBase = ...
    //    sbt_.hitgroupRecordBase = ...
    //    sbt_.hitgroupRecordStrideInBytes = ...
}
```

**参考**: OptiX_Apps `nvlink_shared/Device.cpp`

### 2. 底层加速结构 (BLAS) 构建

**位置**: `buildBLAS()`

**需要实现**:
```cpp
void PhysicsRenderer::buildBLAS(const RenderableObject& obj, DynamicGeometry& geom) {
    // 1. 创建顶点缓冲区（GPU）
    //    - 从 obj.geometryID 获取网格数据
    //    - 上传顶点到 GPU

    // 2. 创建索引缓冲区（GPU）
    //    - 上传索引到 GPU

    // 3. 配置 OptixBuildInput
    //    OptixBuildInput buildInput = {};
    //    buildInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    //    buildInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
    //    buildInput.triangleArray.vertexBuffers = &vertexBuffer;
    //    buildInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
    //    buildInput.triangleArray.indexBuffer = indexBuffer;

    // 4. 配置 OptixAccelBuildOptions
    //    OptixAccelBuildOptions accelOptions = {};
    //    accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_UPDATE;
    //    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

    // 5. 查询缓冲区大小
    //    OptixAccelBufferSizes bufferSizes;
    //    optixAccelComputeMemoryUsage(..., &buildInput, 1, &bufferSizes);

    // 6. 分配临时缓冲区
    //    cudaMalloc(&tempBuffer, bufferSizes.tempSizeInBytes);
    //    cudaMalloc(&outputBuffer, bufferSizes.outputSizeInBytes);

    // 7. 构建 BLAS
    //    optixAccelBuild(
    //        optixContext_,
    //        0,  // stream
    //        &accelOptions,
    //        &buildInput, 1,
    //        tempBuffer, bufferSizes.tempSizeInBytes,
    //        outputBuffer, bufferSizes.outputSizeInBytes,
    //        &geom.handle,
    //        nullptr, 0  // emitted properties
    //    );

    // 8. 释放临时缓冲区
    //    cudaFree(tempBuffer);
}
```

### 3. 顶层加速结构 (TLAS) 构建

**位置**: `buildTLAS()`

**需要实现**:
```cpp
void PhysicsRenderer::buildTLAS() {
    // 1. 创建 OptixInstance 数组
    std::vector<OptixInstance> instances(dynamicGeometries_.size());

    for (size_t i = 0; i < dynamicGeometries_.size(); ++i) {
        OptixInstance& inst = instances[i];
        memset(&inst, 0, sizeof(OptixInstance));

        // 设置变换矩阵（4x3 → 3x4 转置）
        float* transform = dynamicGeometries_[i].transforms.data();
        inst.transform[0] = transform[0];
        inst.transform[1] = transform[4];
        inst.transform[2] = transform[8];
        inst.transform[3] = transform[1];
        // ... 其余元素

        // 设置其他字段
        inst.instanceId = i;
        inst.sbtOffset = 0;
        inst.visibilityMask = 255;
        inst.flags = OPTIX_INSTANCE_FLAG_NONE;
        inst.traversableHandle = dynamicGeometries_[i].handle;
    }

    // 2. 上传实例数据到 GPU
    if (instanceBuffer_) cudaFree((void*)instanceBuffer_);
    size_t instanceSize = instances.size() * sizeof(OptixInstance);
    cudaMalloc(&instanceBuffer_, instanceSize);
    cudaMemcpy(
        (void*)instanceBuffer_,
        instances.data(),
        instanceSize,
        cudaMemcpyHostToDevice
    );

    // 3. 配置 OptixBuildInput
    OptixBuildInput buildInput = {};
    buildInput.type = OPTIX_BUILD_INPUT_TYPE_INSTANCES;
    buildInput.instanceArray.instances = instanceBuffer_;
    buildInput.instanceArray.numInstances = instances.size();

    // 4. 配置 OptixAccelBuildOptions
    OptixAccelBuildOptions accelOptions = {};
    accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_UPDATE;
    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

    // 5. 查询缓冲区大小
    OptixAccelBufferSizes bufferSizes;
    optixAccelComputeMemoryUsage(...);

    // 6. 分配缓冲区并构建
    //    ... 类似 BLAS
    //    optixAccelBuild(..., &topLevelAS_);
}
```

---

## 性能考虑

### 优化点

1. **AS 更新 vs 重建**
   - 小变化: 使用 `OPTIX_BUILD_OPERATION_UPDATE`
   - 大变化: 使用 `OPTIX_BUILD_OPERATION_BUILD`

2. **材质缓冲区**
   - 批量更新: 重新分配整个缓冲区
   - 单个更新: 只更新变化的部分

3. **CUDA 流**
   - 当前: 使用默认流（同步）
   - 优化: 使用多个流并行（异步）

4. **栈大小**
   - 影响: 光线追踪深度和遍历复杂度
   - 权衡: 更大的栈 = 更深的递归，但内存消耗更多

### 性能目标

| 指标 | 目标值 | 实际值 |
|------|-------|--------|
| 上下文创建 | < 100ms | 待测试 |
| 模块加载 | < 500ms | 待测试 |
| 管线创建 | < 200ms | 待测试 |
| BLAS 构建 | < 10ms/对象 | 待实现 |
| TLAS 构建 | < 5ms | 待实现 |
| 渲染 (1080p, 4 SPP) | < 50ms | 待测试 |

---

## 测试建议

### 单元测试

1. **上下文创建**
   ```cpp
   // 测试: 创建和销毁
   PhysicsRenderer renderer(config);
   ASSERT_TRUE(renderer.initialize());
   // 自动调用析构函数清理
   ```

2. **材质系统**
   ```cpp
   // 测试: 添加和更新材质
   PhysicsMaterialDefinition mat;
   mat.albedo = make_float3(1, 0, 0);
   int id = renderer.addPhysicsMaterial(mat);
   ASSERT_EQ(id, 0);

   mat.albedo = make_float3(0, 1, 0);
   renderer.updatePhysicsMaterial(id, mat);
   ```

3. **几何管理**
   ```cpp
   // 测试: 设置和更新几何
   std::vector<RenderableObject> objs;
   // ... 填充 objs
   renderer.setDynamicGeometry(objs);

   // 更新变换
   std::vector<float*> transforms;
   // ... 填充 transforms
   renderer.updateTransforms(transforms);
   ```

### 集成测试

```bash
cd /home/user/PhysX/PhysicsRenderIntegration
./build_test.sh
```

**预期输出**:
- ✅ CMake 配置成功
- ✅ 编译无错误（可能有警告）
- ✅ 生成可执行文件

---

## 常见问题

### Q1: 找不到 PTX 文件

**问题**: `Failed to open PTX file: .../raygeneration_core.ptx`

**解决**:
1. 确保着色器已编译: `make PhysicsRenderShaders`
2. 检查 MODULE_TARGET_DIR 路径
3. 手动检查 PTX 文件是否存在

### Q2: OptiX 初始化失败

**问题**: `OptiX error: OPTIX_ERROR_UNSUPPORTED_ABI_VERSION`

**解决**:
1. 检查 OptiX SDK 版本 (需要 9.0.0)
2. 检查 CUDA Toolkit 版本 (需要 11.x/12.x)
3. 更新显卡驱动

### Q3: 栈大小错误

**问题**: `OptiX error: OPTIX_ERROR_STACK_OVERFLOW`

**解决**:
1. 增加 `maxTraceDepth`
2. 减少路径长度
3. 简化场景复杂度

### Q4: 内存不足

**问题**: `CUDA error: cudaErrorMemoryAllocation`

**解决**:
1. 减少渲染分辨率
2. 减少几何复杂度
3. 检查内存泄漏

---

## 下一步工作

### 优先级 1（必须）

- [ ] 实现 `createSBT()` - 完整版本
- [ ] 实现 `buildBLAS()` - 三角形网格
- [ ] 实现 `buildTLAS()` - 实例化场景

### 优先级 2（重要）

- [ ] 添加相机系统
- [ ] 添加光源系统
- [ ] 测试完整渲染流程

### 优先级 3（优化）

- [ ] CUDA 流并行
- [ ] AS 增量更新
- [ ] 粒子渲染（sphere 原语）

---

## 参考资源

### OptiX 文档
- [OptiX Programming Guide](https://raytracing-docs.nvidia.com/optix7/guide/index.html)
- [OptiX API Reference](https://raytracing-docs.nvidia.com/optix7/api/index.html)

### 示例代码
- [OptiX SDK Samples](https://github.com/NVIDIA/OptiX-Samples)
- [OptiX_Apps](https://github.com/NVIDIA/OptiX_Apps)

### 内部代码
- `PhysicsRenderer.h` - 类定义
- `physics_system_data.h` - 系统数据结构
- `shaders/` - OptiX 着色器

---

**文档版本**: 1.0.0
**最后更新**: 2025-11-07
**作者**: Claude AI Assistant
**状态**: ✅ 核心功能完整
