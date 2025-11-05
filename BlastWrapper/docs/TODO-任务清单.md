# BlastWrapper 封装任务清单

> **目标**: 将 NVIDIA Blast 破坏系统封装成易用的 C++ 类库
> **原则**: 头文件与实现分离，面向对象设计，隐藏底层复杂性
> **参考**: `blast/include/` 和 `blast/source/test/`

---

## 📋 任务分类

### ⭐ 优先级说明
- ⭐⭐⭐⭐⭐ - 核心功能，最高优先级
- ⭐⭐⭐⭐ - 重要功能，高优先级
- ⭐⭐⭐ - 常用功能，中优先级
- ⭐⭐ - 辅助功能，低优先级
- ⭐ - 高级功能，可选

---

## 📦 一、核心基础类 (LowLevel Core)

### 1.1 BlastCore - 核心管理类

- [ ] **BlastCore** - Blast 核心管理器
  - **源文件**: `blast/include/lowlevel/NvBlast.h`
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `BlastCore`
  - **功能**:
    - 初始化和关闭 Blast 系统
    - 日志管理
    - 全局配置
  - **头文件**: `BlastWrapper/include/LowLevel/BlastCore.h`
  - **实现文件**: `BlastWrapper/src/LowLevel/BlastCore.cpp`
  - **关键 API**:
    ```cpp
    class BlastCore {
    public:
        static BlastCore& getInstance();
        void initialize(NvBlastLog logFn = nullptr);
        void shutdown();
        void setLogLevel(LogLevel level);
    private:
        NvBlastLog mLogFunction;
    };
    ```

---

## 🏗️ 二、资产管理类 (Asset Management)

### 2.1 BlastAsset - 资产类

- [ ] **BlastAsset** - 破坏资产管理
  - **源文件**: `blast/include/lowlevel/NvBlast.h` (NvBlastAsset 相关)
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `BlastAsset`
  - **功能**:
    - 从描述符创建资产
    - 资产序列化/反序列化
    - 查询资产信息（块数量、键数量）
    - 支持图访问
  - **头文件**: `BlastWrapper/include/LowLevel/BlastAsset.h`
  - **实现文件**: `BlastWrapper/src/LowLevel/BlastAsset.cpp`
  - **关键 API**:
    ```cpp
    class BlastAsset {
    public:
        static BlastAsset* create(const BlastAssetDesc& desc);
        ~BlastAsset();

        uint32_t getChunkCount() const;
        uint32_t getBondCount() const;
        uint32_t getSupportChunkCount() const;
        const BlastSupportGraph& getSupportGraph() const;

        bool save(const char* filename) const;
        static BlastAsset* load(const char* filename);
    private:
        NvBlastAsset* mAsset;
        void* mMemory;
    };
    ```

### 2.2 BlastAssetBuilder - 资产构建器

- [ ] **BlastAssetBuilder** - 资产构建辅助类
  - **源文件**: `blast/include/lowlevel/NvBlast.h` (Helper functions)
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `BlastAssetBuilder`
  - **功能**:
    - 简化资产创建流程
    - 自动处理块重排序
    - 自动支持覆盖检查
    - Builder 模式
  - **头文件**: `BlastWrapper/include/LowLevel/BlastAssetBuilder.h`
  - **实现文件**: `BlastWrapper/src/LowLevel/BlastAssetBuilder.cpp`
  - **关键 API**:
    ```cpp
    class BlastAssetBuilder {
    public:
        BlastAssetBuilder& setChunks(const std::vector<ChunkDesc>& chunks);
        BlastAssetBuilder& setBonds(const std::vector<BondDesc>& bonds);
        BlastAssetBuilder& autoReorder(bool enable = true);
        BlastAsset* build();
    private:
        std::vector<NvBlastChunkDesc> mChunks;
        std::vector<NvBlastBondDesc> mBonds;
        bool mAutoReorder = true;
    };
    ```

---

## 👥 三、Family 和 Actor 管理类

### 3.1 BlastFamily - 家族管理类

- [ ] **BlastFamily** - 家族管理
  - **源文件**: `blast/include/lowlevel/NvBlast.h` (NvBlastFamily 相关)
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `BlastFamily`
  - **功能**:
    - 从资产创建家族
    - 管理 Actor 生命周期
    - 获取活跃 Actor 列表
    - 序列化/反序列化
  - **头文件**: `BlastWrapper/include/LowLevel/BlastFamily.h`
  - **实现文件**: `BlastWrapper/src/LowLevel/BlastFamily.cpp`
  - **关键 API**:
    ```cpp
    class BlastFamily {
    public:
        static BlastFamily* create(BlastAsset* asset);
        ~BlastFamily();

        BlastActor* createFirstActor(const ActorDesc& desc);
        uint32_t getActorCount() const;
        std::vector<BlastActor*> getActors() const;
        BlastActor* getActorByIndex(uint32_t index);

        bool save(const char* filename) const;
        static BlastFamily* load(const char* filename, BlastAsset* asset);
    private:
        NvBlastFamily* mFamily;
        BlastAsset* mAsset;
        void* mMemory;
    };
    ```

### 3.2 BlastActor - 演员类

- [ ] **BlastActor** - Actor 管理
  - **源文件**: `blast/include/lowlevel/NvBlast.h` (NvBlastActor 相关)
  - **优先级**: ⭐⭐⭐⭐⭐
  - **封装类名**: `BlastActor`
  - **功能**:
    - 可见块管理
    - 伤害应用
    - 分裂检测和执行
    - 健康值查询
  - **头文件**: `BlastWrapper/include/LowLevel/BlastActor.h`
  - **实现文件**: `BlastWrapper/src/LowLevel/BlastActor.cpp`
  - **关键 API**:
    ```cpp
    class BlastActor {
    public:
        // 查询
        uint32_t getVisibleChunkCount() const;
        std::vector<uint32_t> getVisibleChunkIndices() const;
        std::vector<float> getBondHealths() const;

        // 破坏操作
        void applyDamage(const DamageProgram& program, const void* params);
        void applyFracture(const FractureBuffers& commands);

        // 分裂
        bool isSplitRequired() const;
        std::vector<BlastActor*> split();

        // 生命周期
        void deactivate();
        bool isActive() const;

    private:
        NvBlastActor* mActor;
        BlastFamily* mFamily;
        friend class BlastFamily;
    };
    ```

---

## 💥 四、伤害和破坏类 (Damage & Fracture)

### 4.1 BlastDamageProgram - 伤害程序类

- [ ] **BlastDamageProgram** - 伤害程序封装
  - **源文件**: `blast/include/lowlevel/NvBlastTypes.h` (NvBlastDamageProgram)
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `BlastDamageProgram`
  - **功能**:
    - 定义伤害计算逻辑
    - 支持自定义伤害着色器
    - 材质函数
  - **头文件**: `BlastWrapper/include/LowLevel/BlastDamageProgram.h`
  - **实现文件**: `BlastWrapper/src/LowLevel/BlastDamageProgram.cpp`
  - **关键 API**:
    ```cpp
    class BlastDamageProgram {
    public:
        using GraphShaderFunction = void(*)(NvBlastFractureBuffers*,
                                            const NvBlastGraphShaderActor*,
                                            const void*);
        using SubgraphShaderFunction = void(*)(NvBlastFractureBuffers*,
                                               const NvBlastSubgraphShaderActor*,
                                               const void*);

        void setGraphShader(GraphShaderFunction func);
        void setSubgraphShader(SubgraphShaderFunction func);

        NvBlastDamageProgram getNativeProgram() const;
    private:
        NvBlastDamageProgram mProgram;
    };
    ```

### 4.2 BlastFractureBuffer - 破坏缓冲区类

- [ ] **BlastFractureBuffer** - 破坏数据管理
  - **源文件**: `blast/include/lowlevel/NvBlastTypes.h`
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `BlastFractureBuffer`
  - **功能**:
    - 管理破坏命令和事件
    - 自动内存分配
    - RAII 封装
  - **头文件**: `BlastWrapper/include/LowLevel/BlastFractureBuffer.h`
  - **实现文件**: `BlastWrapper/src/LowLevel/BlastFractureBuffer.cpp`
  - **关键 API**:
    ```cpp
    class BlastFractureBuffer {
    public:
        BlastFractureBuffer(uint32_t chunkCapacity, uint32_t bondCapacity);
        ~BlastFractureBuffer();

        void resize(uint32_t chunkCapacity, uint32_t bondCapacity);
        void clear();

        NvBlastFractureBuffers* getNativeBuffers();
        const NvBlastFractureBuffers* getNativeBuffers() const;

        uint32_t getChunkCount() const;
        uint32_t getBondCount() const;
    private:
        NvBlastFractureBuffers mBuffers;
        std::vector<NvBlastChunkFractureData> mChunkData;
        std::vector<NvBlastBondFractureData> mBondData;
    };
    ```

---

## 🛠️ 五、Toolkit 高级类 (TkFramework)

### 5.1 TkFramework - 框架管理类

- [ ] **TkFramework** - Toolkit 框架
  - **源文件**: `blast/include/toolkit/NvBlastTkFramework.h`
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `TkFramework`
  - **功能**:
    - 管理高级对象生命周期
    - 提供内存分配器
    - 事件分发器
  - **头文件**: `BlastWrapper/include/Toolkit/TkFramework.h`
  - **实现文件**: `BlastWrapper/src/Toolkit/TkFramework.cpp`
  - **关键 API**:
    ```cpp
    class TkFramework {
    public:
        static TkFramework& create();
        void release();

        TkAsset* createAsset(const AssetDesc& desc);
        TkGroup* createGroup();

        void addEventListener(TkEventListener* listener);
        void removeEventListener(TkEventListener* listener);
    private:
        Nv::Blast::TkFramework* mFramework;
    };
    ```

### 5.2 TkAsset - 高级资产类

- [ ] **TkAsset** - Toolkit 资产
  - **源文件**: `blast/include/toolkit/NvBlastTkAsset.h`
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `TkAsset`
  - **功能**:
    - C++ 风格的资产接口
    - 自动内存管理
    - RAII 封装
  - **头文件**: `BlastWrapper/include/Toolkit/TkAsset.h`
  - **实现文件**: `BlastWrapper/src/Toolkit/TkAsset.cpp`

### 5.3 TkActor - 高级演员类

- [ ] **TkActor** - Toolkit Actor
  - **源文件**: `blast/include/toolkit/NvBlastTkActor.h`
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `TkActor`
  - **功能**:
    - 事件回调
    - 物理引擎集成接口
    - 用户数据管理
  - **头文件**: `BlastWrapper/include/Toolkit/TkActor.h`
  - **实现文件**: `BlastWrapper/src/Toolkit/TkActor.cpp`

### 5.4 TkFamily - 高级家族类

- [ ] **TkFamily** - Toolkit Family
  - **源文件**: `blast/include/toolkit/NvBlastTkFamily.h`
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `TkFamily`
  - **功能**:
    - 管理 Actor 集合
    - 分裂事件处理
  - **头文件**: `BlastWrapper/include/Toolkit/TkFamily.h`
  - **实现文件**: `BlastWrapper/src/Toolkit/TkFamily.cpp`

---

## 🔗 六、Joint 连接类

### 6.1 TkJoint - 关节类

- [ ] **TkJoint** - Actor 连接
  - **源文件**: `blast/include/toolkit/NvBlastTkJoint.h`
  - **优先级**: ⭐⭐⭐
  - **封装类名**: `TkJoint`
  - **功能**:
    - Actor 之间的约束
    - 破坏时断开
  - **头文件**: `BlastWrapper/include/Toolkit/TkJoint.h`
  - **实现文件**: `BlastWrapper/src/Toolkit/TkJoint.cpp`

---

## 👥 七、Group 分组类

### 7.1 TkGroup - 分组管理类

- [ ] **TkGroup** - 批量处理
  - **源文件**: `blast/include/toolkit/NvBlastTkGroup.h`
  - **优先级**: ⭐⭐⭐
  - **封装类名**: `TkGroup`
  - **功能**:
    - 批量破坏处理
    - 并行化支持
  - **头文件**: `BlastWrapper/include/Toolkit/TkGroup.h`
  - **实现文件**: `BlastWrapper/src/Toolkit/TkGroup.cpp`

---

## 🔌 八、Extensions 扩展类

### 8.1 BlastSerializer - 序列化类

- [ ] **BlastSerializer** - 序列化/反序列化
  - **源文件**: `blast/include/extensions/serialization/`
  - **优先级**: ⭐⭐⭐
  - **封装类名**: `BlastSerializer`
  - **功能**:
    - Asset 序列化
    - Family 序列化
    - 二进制格式
  - **头文件**: `BlastWrapper/include/Extensions/BlastSerializer.h`
  - **实现文件**: `BlastWrapper/src/Extensions/BlastSerializer.cpp`
  - **关键 API**:
    ```cpp
    class BlastSerializer {
    public:
        static bool saveAsset(const BlastAsset* asset, const char* filename);
        static BlastAsset* loadAsset(const char* filename);

        static bool saveFamily(const BlastFamily* family, const char* filename);
        static BlastFamily* loadFamily(const char* filename, BlastAsset* asset);
    };
    ```

### 8.2 BlastAuthoring - 创作工具类

- [ ] **BlastAuthoring** - 资产创作
  - **源文件**: `blast/include/extensions/authoring/`
  - **优先级**: ⭐⭐
  - **封装类名**: `BlastAuthoring`
  - **功能**:
    - Voronoi 碎片生成
    - 程序化破坏模式
  - **头文件**: `BlastWrapper/include/Extensions/BlastAuthoring.h`
  - **实现文件**: `BlastWrapper/src/Extensions/BlastAuthoring.cpp`

### 8.3 StressSolver - 压力求解器类

- [ ] **StressSolver** - 压力模拟
  - **源文件**: `blast/source/shared/stress_solver/`
  - **优先级**: ⭐⭐
  - **封装类名**: `StressSolver`
  - **功能**:
    - 基于物理的破坏预测
    - 压力传播
  - **头文件**: `BlastWrapper/include/Extensions/StressSolver.h`
  - **实现文件**: `BlastWrapper/src/Extensions/StressSolver.cpp`

---

## 📊 九、事件系统类

### 9.1 BlastEventListener - 事件监听器接口

- [ ] **BlastEventListener** - 事件回调接口
  - **源文件**: `blast/include/toolkit/NvBlastTkEvent.h`
  - **优先级**: ⭐⭐⭐⭐
  - **封装类名**: `BlastEventListener`
  - **功能**:
    - 分裂事件
    - 块破坏事件
    - 键断裂事件
  - **头文件**: `BlastWrapper/include/Toolkit/BlastEventListener.h`
  - **实现文件**: `BlastWrapper/src/Toolkit/BlastEventListener.cpp`
  - **关键 API**:
    ```cpp
    class BlastEventListener {
    public:
        virtual ~BlastEventListener() = default;

        virtual void onActorCreated(const TkActor* actor) {}
        virtual void onActorDestroyed(const TkActor* actor) {}
        virtual void onActorSplit(const SplitEvent& event) {}
        virtual void onChunkFractured(const ChunkEvent& event) {}
        virtual void onBondBroken(const BondEvent& event) {}
    };
    ```

---

## 🧰 十、辅助工具类

### 10.1 BlastAllocator - 内存分配器

- [ ] **BlastAllocator** - 自定义内存管理
  - **源文件**: 自定义实现
  - **优先级**: ⭐⭐
  - **封装类名**: `BlastAllocator`
  - **功能**:
    - 内存池
    - 对齐分配
  - **头文件**: `BlastWrapper/include/Shared/BlastAllocator.h`
  - **实现文件**: `BlastWrapper/src/Shared/BlastAllocator.cpp`

### 10.2 BlastLogger - 日志系统

- [ ] **BlastLogger** - 日志管理
  - **源文件**: 自定义实现
  - **优先级**: ⭐⭐⭐
  - **封装类名**: `BlastLogger`
  - **功能**:
    - 多级别日志
    - 文件/控制台输出
  - **头文件**: `BlastWrapper/include/Shared/BlastLogger.h`
  - **实现文件**: `BlastWrapper/src/Shared/BlastLogger.cpp`
  - **关键 API**:
    ```cpp
    class BlastLogger {
    public:
        enum class Level { Error, Warning, Info, Debug };

        static void setLevel(Level level);
        static void setOutputFile(const char* filename);

        static void error(const char* format, ...);
        static void warning(const char* format, ...);
        static void info(const char* format, ...);
        static void debug(const char* format, ...);
    };
    ```

---

## 🎯 十一、示例和测试

### 11.1 示例程序

- [ ] **BasicDestruction** - 基础破坏示例
  - **优先级**: ⭐⭐⭐⭐⭐
  - **文件**: `BlastWrapper/examples/BasicDestruction.cpp`
  - **功能**: 演示完整的破坏流程

- [ ] **MultiActorSplit** - 多 Actor 分裂示例
  - **优先级**: ⭐⭐⭐⭐
  - **文件**: `BlastWrapper/examples/MultiActorSplit.cpp`

- [ ] **DamagePrograms** - 自定义伤害程序示例
  - **优先级**: ⭐⭐⭐
  - **文件**: `BlastWrapper/examples/DamagePrograms.cpp`

### 11.2 单元测试

- [ ] **AssetTests** - 资产测试
  - **参考**: `blast/source/test/src/unit/AssetTests.cpp`
  - **文件**: `BlastWrapper/tests/AssetTests.cpp`

- [ ] **ActorTests** - Actor 测试
  - **参考**: `blast/source/test/src/unit/ActorTests.cpp`
  - **文件**: `BlastWrapper/tests/ActorTests.cpp`

- [ ] **FractureTests** - 破坏测试
  - **参考**: `blast/source/test/src/unit/CoreTests.cpp`
  - **文件**: `BlastWrapper/tests/FractureTests.cpp`

---

## 📦 十二、构建系统

### 12.1 CMake 配置

- [ ] **CMakeLists.txt** - 根 CMake 文件
  - **位置**: `BlastWrapper/CMakeLists.txt`
  - **优先级**: ⭐⭐⭐⭐⭐
  - **功能**:
    - 查找 Blast SDK
    - 配置编译选项
    - 链接库设置

- [ ] **FindBlast.cmake** - CMake 查找模块
  - **位置**: `BlastWrapper/cmake/FindBlast.cmake`
  - **优先级**: ⭐⭐⭐⭐

### 12.2 示例 CMake

```cmake
# BlastWrapper/CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(BlastWrapper VERSION 1.0.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Blast SDK
find_package(Blast REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${BLAST_INCLUDE_DIRS}
)

# LowLevel 库
add_library(BlastWrapper_LowLevel STATIC
    src/LowLevel/BlastCore.cpp
    src/LowLevel/BlastAsset.cpp
    src/LowLevel/BlastFamily.cpp
    src/LowLevel/BlastActor.cpp
    # ... 更多源文件
)

target_link_libraries(BlastWrapper_LowLevel
    ${BLAST_LIBRARIES}
)

# Toolkit 库
add_library(BlastWrapper_Toolkit STATIC
    src/Toolkit/TkFramework.cpp
    src/Toolkit/TkAsset.cpp
    src/Toolkit/TkActor.cpp
    # ... 更多源文件
)

target_link_libraries(BlastWrapper_Toolkit
    BlastWrapper_LowLevel
    ${BLAST_TK_LIBRARIES}
)

# 示例
add_executable(BasicDestruction
    examples/BasicDestruction.cpp
)

target_link_libraries(BasicDestruction
    BlastWrapper_Toolkit
)
```

---

## 📈 任务统计

| 类别 | 任务数 | 已完成 | 进度 |
|------|--------|--------|------|
| 核心基础类 | 1 | 0 | 0% |
| 资产管理类 | 2 | 0 | 0% |
| Family/Actor | 2 | 0 | 0% |
| 伤害破坏类 | 2 | 0 | 0% |
| Toolkit 类 | 4 | 0 | 0% |
| Joint 类 | 1 | 0 | 0% |
| Group 类 | 1 | 0 | 0% |
| Extensions | 3 | 0 | 0% |
| 事件系统 | 1 | 0 | 0% |
| 辅助工具 | 2 | 0 | 0% |
| 示例测试 | 6 | 0 | 0% |
| 构建系统 | 2 | 0 | 0% |
| **总计** | **27** | **0** | **0%** |

---

## 🚀 开发顺序建议

### 阶段 1: 核心功能 (Week 1-2)
1. BlastCore
2. BlastAsset
3. BlastFamily
4. BlastActor
5. BasicDestruction 示例

### 阶段 2: 伤害系统 (Week 3)
6. BlastDamageProgram
7. BlastFractureBuffer
8. DamagePrograms 示例

### 阶段 3: Toolkit 封装 (Week 4-5)
9. TkFramework
10. TkAsset
11. TkActor
12. TkFamily
13. BlastEventListener

### 阶段 4: 高级功能 (Week 6)
14. TkJoint
15. TkGroup
16. BlastSerializer
17. MultiActorSplit 示例

### 阶段 5: 扩展和工具 (Week 7)
18. BlastAuthoring
19. StressSolver
20. BlastLogger
21. BlastAllocator

### 阶段 6: 测试和文档 (Week 8)
22. 单元测试
23. 性能测试
24. API 文档
25. 使用教程

---

## 📝 代码风格指南

### 命名约定
- 类名: PascalCase (BlastAsset)
- 成员变量: m 前缀 + PascalCase (mAsset)
- 方法名: camelCase (getChunkCount)
- 常量: UPPER_SNAKE_CASE (MAX_CHUNKS)

### 头文件保护
```cpp
#ifndef BLASTWRAPPER_LOWLEVEL_BLASTASSET_H
#define BLASTWRAPPER_LOWLEVEL_BLASTASSET_H
// ...
#endif // BLASTWRAPPER_LOWLEVEL_BLASTASSET_H
```

### 智能指针使用
- 优先使用 `std::unique_ptr` 管理资源
- 共享所有权使用 `std::shared_ptr`
- 避免裸指针传递所有权

---

## ✅ 验收标准

每个类完成后需要满足：

1. ✅ 头文件和实现文件分离
2. ✅ 完整的 Doxygen 注释
3. ✅ RAII 封装，无内存泄漏
4. ✅ 异常安全
5. ✅ 至少一个工作示例
6. ✅ 单元测试覆盖核心功能
7. ✅ 编译无警告 (-Wall -Wextra)

---

**更新日期**: 2025-11-05
**版本**: 1.0
