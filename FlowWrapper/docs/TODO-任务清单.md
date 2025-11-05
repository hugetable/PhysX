# FlowWrapper 封装任务清单

> **目标**: 将 NVIDIA Flow 流体模拟系统封装成易用的 C++ 类库
> **原则**: 头文件与实现分离，面向对象设计，简化操作图使用
> **参考**: `flow/include/nvflow/` 和 `flow/include/nvflowext/`

---

## 📋 任务分类

### ⭐ 优先级说明
- ⭐⭐⭐⭐⭐ - 核心功能，最高优先级
- ⭐⭐⭐⭐ - 重要功能，高优先级
- ⭐⭐⭐ - 常用功能，中优先级
- ⭐⭐ - 辅助功能，低优先级
- ⭐ - 高级功能，可选

---

## 📦 一、核心上下文类 (Context Management)

### 1.1 FlowContext - 计算上下文类

- [ ] **FlowContext** - Flow 计算环境
  - **源文件**: `flow/include/nvflow/NvFlowContext.h`
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `FlowContext`
  - **功能**:
    - 管理 GPU/CPU 计算后端
    - Buffer 和 Texture 资源创建
    - 计算管道编译
  - **头文件**: `FlowWrapper/include/Core/FlowContext.h`
  - **实现文件**: `FlowWrapper/src/Core/FlowContext.cpp`
  - **关键 API**:
    ```cpp
    class FlowContext {
    public:
        enum class Backend { Vulkan, CPU };

        static FlowContext* create(Backend backend);
        ~FlowContext();

        // Buffer 管理
        FlowBuffer* createBuffer(const BufferDesc& desc);
        void destroyBuffer(FlowBuffer* buffer);

        // Texture 管理
        FlowTexture* createTexture(const TextureDesc& desc);
        void destroyTexture(FlowTexture* texture);

        // 执行
        void execute(FlowOp* op);
        void flush();

    private:
        NvFlowContext* mContext;
        Backend mBackend;
    };
    ```

### 1.2 FlowBuffer - 缓冲区类

- [ ] **FlowBuffer** - GPU/CPU 缓冲区封装
  - **源文件**: `flow/include/nvflow/NvFlowContext.h` (NvFlowBuffer)
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `FlowBuffer`
  - **功能**:
    - 结构化缓冲区
    - 常量缓冲区
    - 间接缓冲区
    - Upload/Readback
  - **头文件**: `FlowWrapper/include/Core/FlowBuffer.h`
  - **实现文件**: `FlowWrapper/src/Core/FlowBuffer.cpp`
  - **关键 API**:
    ```cpp
    class FlowBuffer {
    public:
        enum class Type {
            Constant,
            Structured,
            RWStructured,
            Indirect
        };

        void* map();
        void unmap();
        void upload(const void* data, size_t size);
        void download(void* data, size_t size);

        size_t getSize() const;
        Type getType() const;

    private:
        NvFlowBuffer* mBuffer;
        FlowContext* mContext;
        friend class FlowContext;
    };
    ```

### 1.3 FlowTexture - 纹理类

- [ ] **FlowTexture** - 3D 纹理封装
  - **源文件**: `flow/include/nvflow/NvFlowContext.h` (NvFlowTexture)
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `FlowTexture`
  - **功能**:
    - 3D 体积纹理
    - 读写纹理
    - Mipmap 支持
  - **头文件**: `FlowWrapper/include/Core/FlowTexture.h`
  - **实现文件**: `FlowWrapper/src/Core/FlowTexture.cpp`
  - **关键 API**:
    ```cpp
    class FlowTexture {
    public:
        enum class Format {
            RGBA16F,
            RGBA32F,
            R16F,
            R32F
        };

        uint32_t getWidth() const;
        uint32_t getHeight() const;
        uint32_t getDepth() const;
        Format getFormat() const;

    private:
        NvFlowTexture* mTexture;
        FlowContext* mContext;
        friend class FlowContext;
    };
    ```

---

## 🌊 二、稀疏体积类 (Sparse Volume)

### 2.1 FlowSparse - 稀疏结构类

- [ ] **FlowSparse** - 稀疏体积管理
  - **源文件**: `flow/include/nvflow/NvFlow.h` (NvFlowSparse)
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `FlowSparse`
  - **功能**:
    - 层次化稀疏体积
    - 自动块分配/释放
    - LOD 管理
  - **头文件**: `FlowWrapper/include/Core/FlowSparse.h`
  - **实现文件**: `FlowWrapper/src/Core/FlowSparse.cpp`
  - **关键 API**:
    ```cpp
    class FlowSparse {
    public:
        struct Params {
            uint32_t layerCount;
            uint32_t levelCount;
            uint32_t blockDim;
        };

        static FlowSparse* create(FlowContext* context, const Params& params);
        ~FlowSparse();

        void resize(const Params& params);
        void update();

        uint32_t getActiveBlockCount() const;
        const SparseParams& getParams() const;

    private:
        NvFlowSparse* mSparse;
        FlowContext* mContext;
    };
    ```

---

## 🎨 三、操作图类 (Operation Graph)

### 3.1 FlowOp - 操作节点基类

- [ ] **FlowOp** - 操作接口封装
  - **源文件**: `flow/include/nvflow/NvFlow.h` (NvFlowOp)
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `FlowOp`
  - **功能**:
    - 操作节点基类
    - Pin 连接管理
    - 执行接口
  - **头文件**: `FlowWrapper/include/OpGraph/FlowOp.h`
  - **实现文件**: `FlowWrapper/src/OpGraph/FlowOp.cpp`

### 3.2 FlowOpGraph - 操作图类

- [ ] **FlowOpGraph** - 操作图管理
  - **源文件**: `flow/include/nvflow/NvFlow.h`
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `FlowOpGraph`
  - **功能**:
    - 构建数据流图
    - 节点连接
    - 图执行
  - **头文件**: `FlowWrapper/include/OpGraph/FlowOpGraph.h`
  - **实现文件**: `FlowWrapper/src/OpGraph/FlowOpGraph.cpp`
  - **关键 API**:
    ```cpp
    class FlowOpGraph {
    public:
        FlowOpGraph();
        ~FlowOpGraph();

        void addOp(FlowOp* op);
        void removeOp(FlowOp* op);

        void connect(FlowOp* srcOp, const char* srcPin,
                    FlowOp* dstOp, const char* dstPin);
        void disconnect(FlowOp* op, const char* pin);

        void execute(FlowContext* context);

    private:
        NvFlowOpGraph* mGraph;
        std::vector<FlowOp*> mOps;
    };
    ```

---

## 🌐 四、Grid 网格系统类 (NvFlowExt)

### 4.1 FlowGrid - 流体网格类

- [ ] **FlowGrid** - 高级流体网格
  - **源文件**: `flow/include/nvflowext/NvFlowExt.h` (Grid 部分)
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `FlowGrid`
  - **功能**:
    - 管理稀疏体积
    - 流体模拟参数
    - 自动更新
  - **头文件**: `FlowWrapper/include/Extensions/FlowGrid.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowGrid.cpp`
  - **关键 API**:
    ```cpp
    class FlowGrid {
    public:
        struct Desc {
            float virtualCellSize[3];
            uint32_t blockDim;
            bool enableSparseAllocation;
        };

        static FlowGrid* create(FlowContext* context, const Desc& desc);
        ~FlowGrid();

        void update(float deltaTime);
        void emit(const EmitterParams& params);

        // 参数设置
        void setGravity(const float gravity[3]);
        void setVelocityDamping(float damping);
        void setDensityDissipation(float rate);

        // 查询
        FlowSparse* getSparse() const;
        uint32_t getActiveVoxelCount() const;

    private:
        // TODO: 根据实际 NvFlowExt 实现
        void* mGrid;
        FlowContext* mContext;
    };
    ```

### 4.2 FlowGridParams - 网格参数类

- [ ] **FlowGridParams** - 流体参数管理
  - **源文件**: `flow/include/nvflowext/NvFlowExt.h`
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `FlowGridParams`
  - **功能**:
    - 模拟参数集中管理
    - 参数验证
  - **头文件**: `FlowWrapper/include/Extensions/FlowGridParams.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowGridParams.cpp`
  - **关键 API**:
    ```cpp
    class FlowGridParams {
    public:
        // 重力
        glm::vec3 gravity = glm::vec3(0, -9.81f, 0);

        // 粘度
        float viscosity = 0.001f;

        // 扩散
        float densityDissipation = 0.98f;
        float temperatureDissipation = 0.95f;

        // 浮力
        float buoyancyCoefficient = 1.0f;
        float temperatureScale = 1.0f;

        // 涡度约束
        float vorticityConfinement = 0.0f;

        // 压力求解器
        uint32_t pressureIterations = 40;

        void validate();
    };
    ```

---

## 💨 五、Emitter 发射器类

### 5.1 FlowEmitter - 发射器基类

- [ ] **FlowEmitter** - 发射器接口
  - **源文件**: `flow/include/nvflowext/NvFlowExt.h` (Emitter)
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `FlowEmitter`
  - **功能**:
    - 发射器基类
    - 位置、速度、密度
  - **头文件**: `FlowWrapper/include/Extensions/FlowEmitter.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowEmitter.cpp`
  - **关键 API**:
    ```cpp
    class FlowEmitter {
    public:
        virtual ~FlowEmitter() = default;

        // 基础属性
        void setPosition(const glm::vec3& pos) { mPosition = pos; }
        void setVelocity(const glm::vec3& vel) { mVelocity = vel; }

        // 流体属性
        void setDensity(float density) { mDensity = density; }
        void setTemperature(float temp) { mTemperature = temp; }
        void setFuel(float fuel) { mFuel = fuel; }  // 火焰

        // 发射
        virtual void emit(FlowGrid* grid, float deltaTime) = 0;

    protected:
        glm::vec3 mPosition = glm::vec3(0);
        glm::vec3 mVelocity = glm::vec3(0);
        float mDensity = 1.0f;
        float mTemperature = 0.0f;
        float mFuel = 0.0f;
        float mEmissionRate = 1.0f;
    };
    ```

### 5.2 FlowSphereEmitter - 球形发射器

- [ ] **FlowSphereEmitter** - 球形发射器
  - **源文件**: `flow/source/nvflowext/shaders/EmitterParams.h`
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `FlowSphereEmitter`
  - **功能**:
    - 球形 SDF 发射器
  - **头文件**: `FlowWrapper/include/Extensions/FlowSphereEmitter.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowSphereEmitter.cpp`
  - **关键 API**:
    ```cpp
    class FlowSphereEmitter : public FlowEmitter {
    public:
        FlowSphereEmitter(float radius);

        void setRadius(float radius) { mRadius = radius; }
        float getRadius() const { return mRadius; }

        void emit(FlowGrid* grid, float deltaTime) override;

    private:
        float mRadius;
    };
    ```

### 5.3 FlowBoxEmitter - 盒形发射器

- [ ] **FlowBoxEmitter** - 盒形发射器
  - **源文件**: 类似 SphereEmitter
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `FlowBoxEmitter`
  - **头文件**: `FlowWrapper/include/Extensions/FlowBoxEmitter.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowBoxEmitter.cpp`

### 5.4 FlowCapsuleEmitter - 胶囊发射器

- [ ] **FlowCapsuleEmitter** - 胶囊发射器
  - **源文件**: 类似 SphereEmitter
  - **优先级**: ⭐⭐⭐
  - **封装类名**: `FlowCapsuleEmitter`
  - **头文件**: `FlowWrapper/include/Extensions/FlowCapsuleEmitter.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowCapsuleEmitter.cpp`

---

## 🎥 六、Rendering 渲染类

### 6.1 FlowVolumeRenderer - 体积渲染器

- [ ] **FlowVolumeRenderer** - 体积渲染
  - **源文件**: `flow/source/nvflowext/shaders/RayMarchParams.h`
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `FlowVolumeRenderer`
  - **功能**:
    - 光线行进渲染
    - 密度积分
    - 颜色映射
  - **头文件**: `FlowWrapper/include/Extensions/FlowVolumeRenderer.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowVolumeRenderer.cpp`
  - **关键 API**:
    ```cpp
    class FlowVolumeRenderer {
    public:
        struct Params {
            glm::mat4 view;
            glm::mat4 projection;
            float stepSize = 0.1f;
            int maxSteps = 128;
            float densityMultiplier = 1.0f;
        };

        FlowVolumeRenderer(FlowContext* context);
        ~FlowVolumeRenderer();

        void render(FlowGrid* grid, FlowTexture* output, const Params& params);

        void setColorRamp(const std::vector<glm::vec4>& colors);
        void setShadowsEnabled(bool enabled);

    private:
        FlowContext* mContext;
        FlowTexture* mColorRampTexture;
        bool mShadowsEnabled = false;
    };
    ```

### 6.2 FlowColorRamp - 颜色映射类

- [ ] **FlowColorRamp** - 颜色渐变
  - **源文件**: 自定义实现
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `FlowColorRamp`
  - **功能**:
    - 温度/密度到颜色映射
    - 预设渐变（火焰、烟雾）
  - **头文件**: `FlowWrapper/include/Extensions/FlowColorRamp.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowColorRamp.cpp`
  - **关键 API**:
    ```cpp
    class FlowColorRamp {
    public:
        enum class Preset {
            Fire,       // 黄橙红火焰
            Smoke,      // 灰白烟雾
            Steam,      // 半透明蒸汽
            Custom
        };

        static FlowColorRamp createPreset(Preset preset);

        void addStop(float position, const glm::vec4& color);
        glm::vec4 sample(float t) const;

        std::vector<glm::vec4> generate1DTexture(uint32_t resolution) const;

    private:
        struct ColorStop {
            float position;
            glm::vec4 color;
        };
        std::vector<ColorStop> mStops;
    };
    ```

---

## 🧮 七、数学和工具类

### 7.1 FlowMath - 数学库

- [ ] **FlowMath** - 数学辅助函数
  - **源文件**: 自定义实现（或使用 GLM）
  - **优先级**: ⭐⭐⭐
  - **封装类名**: `FlowMath`
  - **功能**:
    - 向量/矩阵运算
    - SDF 函数
    - 插值函数
  - **头文件**: `FlowWrapper/include/Utility/FlowMath.h`
  - **实现文件**: `FlowWrapper/src/Utility/FlowMath.cpp`

### 7.2 FlowTimer - 性能计时器

- [ ] **FlowTimer** - 性能分析
  - **源文件**: 自定义实现
  - **优先级**: ⭐⭐
  - **封装类名**: `FlowTimer`
  - **功能**:
    - GPU/CPU 计时
    - 性能统计
  - **头文件**: `FlowWrapper/include/Utility/FlowTimer.h`
  - **实现文件**: `FlowWrapper/src/Utility/FlowTimer.cpp`

---

## 🎬 八、高级功能类

### 8.1 FlowSimulator - 模拟器封装

- [ ] **FlowSimulator** - 一体化模拟器
  - **源文件**: 自定义高级封装
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `FlowSimulator`
  - **功能**:
    - 集成 Grid + Emitters + Renderer
    - 简化 API
    - 常见场景预设
  - **头文件**: `FlowWrapper/include/Extensions/FlowSimulator.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowSimulator.cpp`
  - **关键 API**:
    ```cpp
    class FlowSimulator {
    public:
        FlowSimulator(FlowContext::Backend backend = FlowContext::Backend::Vulkan);
        ~FlowSimulator();

        // 初始化
        void initialize(const glm::ivec3& resolution);

        // 发射器管理
        void addEmitter(FlowEmitter* emitter);
        void removeEmitter(FlowEmitter* emitter);
        void clearEmitters();

        // 模拟
        void update(float deltaTime);

        // 渲染
        void render(const glm::mat4& view, const glm::mat4& projection,
                   FlowTexture* output);

        // 参数
        FlowGridParams& getParams() { return mParams; }
        FlowGrid* getGrid() { return mGrid; }

    private:
        FlowContext* mContext;
        FlowGrid* mGrid;
        FlowVolumeRenderer* mRenderer;
        std::vector<FlowEmitter*> mEmitters;
        FlowGridParams mParams;
    };
    ```

### 8.2 FlowPresets - 预设场景类

- [ ] **FlowPresets** - 常见场景预设
  - **源文件**: 自定义实现
  - **优先级**: ⭐⭐⭐
  - **封装类名**: `FlowPresets`
  - **功能**:
    - 火焰场景
    - 烟雾场景
    - 爆炸效果
  - **头文件**: `FlowWrapper/include/Extensions/FlowPresets.h`
  - **实现文件**: `FlowWrapper/src/Extensions/FlowPresets.cpp`
  - **关键 API**:
    ```cpp
    class FlowPresets {
    public:
        // 创建火焰模拟器
        static FlowSimulator* createFireSimulator();

        // 创建烟雾模拟器
        static FlowSimulator* createSmokeSimulator();

        // 创建爆炸效果
        static void setupExplosion(FlowSimulator* sim, const glm::vec3& position,
                                   float strength);

        // 创建篝火
        static void setupCampfire(FlowSimulator* sim, const glm::vec3& position);
    };
    ```

---

## 🎯 九、示例程序

### 9.1 基础示例

- [ ] **BasicSmoke** - 基础烟雾示例
  - **优先级**: ⭐⭐⭐⭐⭐
  - **文件**: `FlowWrapper/examples/BasicSmoke.cpp`
  - **功能**: 演示烟雾发射和渲染

- [ ] **SimpleFire** - 简单火焰示例
  - **优先级**: ⭐⭐⭐⭐⭐
  - **文件**: `FlowWrapper/examples/SimpleFire.cpp`
  - **功能**: 演示火焰模拟

- [ ] **MultiEmitter** - 多发射器示例
  - **优先级**: ⭐⭐⭐⭐
  - **文件**: `FlowWrapper/examples/MultiEmitter.cpp`
  - **功能**: 多个发射器交互

### 9.2 高级示例

- [ ] **Explosion** - 爆炸效果示例
  - **优先级**: ⭐⭐⭐
  - **文件**: `FlowWrapper/examples/Explosion.cpp`

- [ ] **SparsePerformance** - 稀疏性能测试
  - **优先级**: ⭐⭐
  - **文件**: `FlowWrapper/examples/SparsePerformance.cpp`

---

## 📦 十、构建系统

### 10.1 CMake 配置

- [ ] **CMakeLists.txt** - 根 CMake 文件
  - **位置**: `FlowWrapper/CMakeLists.txt`
  - **优先级**: ⭐⭐⭐⭐⭐
  - **功能**:
    - 查找 Flow SDK
    - Vulkan SDK 检测
    - 配置后端

- [ ] **FindFlow.cmake** - CMake 查找模块
  - **位置**: `FlowWrapper/cmake/FindFlow.cmake`
  - **优先级**: ⭐⭐⭐⭐

### 10.2 示例 CMake

```cmake
# FlowWrapper/CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(FlowWrapper VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(Flow REQUIRED)
find_package(Vulkan)  # 可选
find_package(glm REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${FLOW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
)

# Core 库
add_library(FlowWrapper_Core STATIC
    src/Core/FlowContext.cpp
    src/Core/FlowBuffer.cpp
    src/Core/FlowTexture.cpp
    src/Core/FlowSparse.cpp
)

target_link_libraries(FlowWrapper_Core
    ${FLOW_LIBRARIES}
)

# Extensions 库
add_library(FlowWrapper_Extensions STATIC
    src/Extensions/FlowGrid.cpp
    src/Extensions/FlowEmitter.cpp
    src/Extensions/FlowVolumeRenderer.cpp
    src/Extensions/FlowSimulator.cpp
)

target_link_libraries(FlowWrapper_Extensions
    FlowWrapper_Core
    ${FLOW_EXT_LIBRARIES}
)

# 如果有 Vulkan，编译 Vulkan 后端
if(Vulkan_FOUND)
    target_compile_definitions(FlowWrapper_Core PRIVATE FLOW_VULKAN_BACKEND)
    target_link_libraries(FlowWrapper_Core ${Vulkan_LIBRARIES})
endif()

# 示例
add_executable(BasicSmoke
    examples/BasicSmoke.cpp
)

target_link_libraries(BasicSmoke
    FlowWrapper_Extensions
)
```

---

## 📈 任务统计

| 类别 | 任务数 | 已完成 | 进度 |
|------|--------|--------|------|
| 核心上下文 | 3 | 0 | 0% |
| 稀疏体积 | 1 | 0 | 0% |
| 操作图 | 2 | 0 | 0% |
| Grid 系统 | 2 | 0 | 0% |
| Emitter | 4 | 0 | 0% |
| Rendering | 2 | 0 | 0% |
| 工具类 | 2 | 0 | 0% |
| 高级功能 | 2 | 0 | 0% |
| 示例程序 | 5 | 0 | 0% |
| 构建系统 | 2 | 0 | 0% |
| **总计** | **25** | **0** | **0%** |

---

## 🚀 开发顺序建议

### 阶段 1: 核心功能 (Week 1-2)
1. FlowContext (Vulkan/CPU)
2. FlowBuffer
3. FlowTexture
4. FlowSparse
5. BasicSmoke 示例（使用底层 API）

### 阶段 2: Grid 系统 (Week 3)
6. FlowGrid
7. FlowGridParams
8. SimpleFire 示例

### 阶段 3: Emitter 系统 (Week 4)
9. FlowEmitter (基类)
10. FlowSphereEmitter
11. FlowBoxEmitter
12. MultiEmitter 示例

### 阶段 4: Rendering (Week 5)
13. FlowVolumeRenderer
14. FlowColorRamp
15. 更新之前的示例添加渲染

### 阶段 5: 高级封装 (Week 6)
16. FlowSimulator (一体化)
17. FlowPresets (预设)
18. Explosion 示例

### 阶段 6: 工具和优化 (Week 7)
19. FlowMath
20. FlowTimer
21. 性能测试

### 阶段 7: 文档和测试 (Week 8)
22. API 文档
23. 教程
24. 性能基准测试
25. 单元测试

---

## 📝 代码风格指南

### 命名约定
- 类名: PascalCase (FlowContext)
- 成员变量: m 前缀 + PascalCase (mContext)
- 方法名: camelCase (createBuffer)
- 常量: UPPER_SNAKE_CASE (MAX_EMITTERS)

### GLM 使用
```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 使用 glm 类型
glm::vec3 position;
glm::mat4 transform;
```

### 智能指针
- 优先使用 `std::unique_ptr` 管理资源
- Flow 原生对象使用裸指针（由上下文管理）

---

## 🎨 使用示例代码

### 最简单的烟雾模拟

```cpp
#include <FlowWrapper/FlowSimulator.h>
#include <FlowWrapper/FlowSphereEmitter.h>

int main() {
    // 1. 创建模拟器
    FlowSimulator sim;
    sim.initialize(glm::ivec3(128, 128, 128));

    // 2. 创建发射器
    auto emitter = new FlowSphereEmitter(2.0f);
    emitter->setPosition(glm::vec3(0, 0, 0));
    emitter->setVelocity(glm::vec3(0, 5, 0));
    emitter->setDensity(1.0f);
    sim.addEmitter(emitter);

    // 3. 设置参数
    auto& params = sim.getParams();
    params.gravity = glm::vec3(0, -9.81f, 0);
    params.densityDissipation = 0.98f;

    // 4. 模拟循环
    for (int i = 0; i < 100; i++) {
        sim.update(1.0f / 60.0f);

        // 渲染到纹理
        // sim.render(viewMatrix, projMatrix, outputTexture);
    }

    delete emitter;
    return 0;
}
```

### 火焰效果

```cpp
FlowSimulator* fire = FlowPresets::createFireSimulator();

auto emitter = new FlowSphereEmitter(1.0f);
emitter->setPosition(glm::vec3(0, 0, 0));
emitter->setFuel(1.0f);          // 燃料
emitter->setTemperature(1000.0f); // 高温
fire->addEmitter(emitter);

// 更新和渲染
fire->update(deltaTime);
fire->render(view, proj, output);
```

---

## ✅ 验收标准

每个类完成后需要满足：

1. ✅ 头文件和实现分离
2. ✅ 完整的 Doxygen 注释
3. ✅ RAII 资源管理
4. ✅ 至少一个工作示例
5. ✅ 性能测试（FPS 统计）
6. ✅ 编译无警告
7. ✅ 支持 Vulkan 和 CPU 后端（如果适用）

---

## 🔍 依赖检查

### 必需依赖
- ✅ Flow SDK (NvFlow)
- ✅ GLM (数学库)
- ✅ C++17 编译器

### 可选依赖
- ⚪ Vulkan SDK (推荐，用于 GPU 加速)
- ⚪ OpenGL (备选渲染后端)

---

**更新日期**: 2025-11-05
**版本**: 1.0
