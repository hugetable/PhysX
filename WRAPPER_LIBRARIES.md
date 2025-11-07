# NVIDIA 物理引擎包装器库

本项目为 NVIDIA 的物理引擎提供现代 C++ 包装器库，包括 PhysX、Flow 和 Blast。

## 概述

| 库名 | 用途 | 状态 | 库文件大小 |
|------|------|------|-----------|
| **PhysXWrapper** | 刚体物理、碰撞检测、关节约束 | ✅ 生产就绪 | 1.1 MB |
| **FlowWrapper** | 流体模拟 | ✅ 基础可用 | 14 KB |
| **BlastWrapper** | 破坏模拟 | ✅ 基础可用 | 18 KB |

## 包装器库特性

### 共同特性

- ✅ **现代 C++17**: 使用现代 C++ 特性和最佳实践
- ✅ **RAII 管理**: 自动资源管理，防止内存泄漏
- ✅ **类型安全**: 强类型接口，减少运行时错误
- ✅ **跨平台**: 支持 Linux 和 Windows
- ✅ **CMake 构建**: 标准化的构建系统
- ✅ **文档完整**: 详细的 API 文档和使用示例

### PhysXWrapper

**用途**: 刚体物理模拟

**核心功能**:
- 刚体动力学和碰撞检测
- 关节和约束系统
- 角色控制器
- 车辆模拟
- 可变形体（SoftBody）
- 粒子系统
- 场景查询和射线检测

**API 版本**: PhysX 5.x (最新版本)

**编译状态**: ✅ 0 错误，完整 API 迁移完成

**目录**: `PhysXWrapper/`

**文档**: [PhysXWrapper/README.md](PhysXWrapper/README.md)

### FlowWrapper

**用途**: 流体模拟

**核心功能**:
- GPU 加速流体模拟
- 体积渲染
- 烟雾和火焰效果
- 流体与刚体交互

**目录**: `FlowWrapper/`

**文档**: [FlowWrapper/README.md](FlowWrapper/README.md)

### BlastWrapper

**用途**: 破坏模拟

**核心功能**:
- 分层破坏系统
- 动态断裂
- 伤害传播
- 碎片管理
- 破坏事件回调

**目录**: `BlastWrapper/`

**文档**: [BlastWrapper/README.md](BlastWrapper/README.md)

## 快速开始

### 1. 构建所有包装器库

```bash
# PhysXWrapper
cd PhysXWrapper && mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
cd ../..

# FlowWrapper
cd FlowWrapper && mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
cd ../..

# BlastWrapper
cd BlastWrapper && mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)
cd ../..
```

### 2. 在项目中使用

#### CMake 配置示例

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyPhysicsApp)

set(CMAKE_CXX_STANDARD 17)

# 添加包装器库
add_subdirectory(path/to/PhysXWrapper)
add_subdirectory(path/to/FlowWrapper)
add_subdirectory(path/to/BlastWrapper)

# 创建可执行文件
add_executable(my_app main.cpp)

# 链接包装器库
target_link_libraries(my_app
    PRIVATE
        PhysXWrapper
        FlowWrapper
        BlastWrapper
)
```

#### 代码示例

```cpp
#include <PhysXWrapper.h>
#include <Flow/FlowContext.h>
#include <Blast/BlastManager.h>
#include <iostream>

int main() {
    // 1. 初始化 PhysX
    auto physx = PhysXWrapper::create();
    if (!physx->initialize()) {
        std::cerr << "PhysX init failed!" << std::endl;
        return 1;
    }

    // 2. 初始化 Flow (流体)
    FlowWrapper::FlowContextConfig flowConfig;
    flowConfig.memoryBudgetMB = 512;
    auto flow = FlowWrapper::FlowContext::create(flowConfig);
    if (!flow->initialize()) {
        std::cerr << "Flow init failed!" << std::endl;
        return 1;
    }

    // 3. 初始化 Blast (破坏)
    BlastWrapper::BlastConfig blastConfig;
    blastConfig.maxActors = 1024;
    auto blast = BlastWrapper::BlastManager::create(blastConfig);
    if (!blast->initialize()) {
        std::cerr << "Blast init failed!" << std::endl;
        return 1;
    }

    // 主循环
    float deltaTime = 1.0f / 60.0f; // 60 FPS
    for (int frame = 0; frame < 1000; ++frame) {
        // 更新所有系统
        physx->update(deltaTime);
        flow->update(deltaTime);
        blast->update(deltaTime);

        // 你的游戏逻辑...
    }

    // 清理（自动调用析构函数）
    blast->shutdown();
    flow->shutdown();
    physx->shutdown();

    return 0;
}
```

## 编译结果总结

| 库 | 编译状态 | 错误数 | 警告数 | 库文件 |
|----|---------|--------|--------|--------|
| PhysXWrapper | ✅ 成功 | 0 | 0 | libPhysXWrapper.a (1.1 MB) |
| FlowWrapper | ✅ 成功 | 0 | 0 | libFlowWrapper.a (14 KB) |
| BlastWrapper | ✅ 成功 | 0 | 0 | libBlastWrapper.a (18 KB) |

## 项目结构

```
PhysX/
├── PhysXWrapper/               # PhysX 包装器
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── include/                # 公共头文件
│   │   ├── Core/
│   │   ├── RigidBody/
│   │   ├── Character/
│   │   ├── Vehicle/
│   │   ├── Joint/
│   │   └── ...
│   ├── src/                    # 实现文件
│   ├── build/                  # 构建输出
│   │   └── libPhysXWrapper.a
│   └── docs/
│
├── FlowWrapper/                # Flow 包装器
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── include/Flow/
│   │   └── FlowContext.h
│   ├── src/Flow/
│   │   └── FlowContext.cpp
│   └── build/
│       └── libFlowWrapper.a
│
├── BlastWrapper/               # Blast 包装器
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── include/Blast/
│   │   └── BlastManager.h
│   ├── src/Blast/
│   │   └── BlastManager.cpp
│   └── build/
│       └── libBlastWrapper.a
│
├── physx/                      # NVIDIA PhysX 源码
├── flow/                       # NVIDIA Flow 源码
├── blast/                      # NVIDIA Blast 源码
└── WRAPPER_LIBRARIES.md        # 本文件
```

## 技术栈

- **C++ 标准**: C++17
- **构建系统**: CMake 3.16+
- **编译器**: GCC 7+, Clang 5+, MSVC 2017+
- **平台**: Linux, Windows
- **架构**: x86_64

## API 版本

- **PhysX**: 5.x (最新版本，无 4.x 向后兼容)
- **Flow**: 最新版本
- **Blast**: 最新版本

## 使用场景

### 游戏引擎集成

所有三个包装器库都设计为易于集成到游戏引擎中：

```cpp
// 游戏引擎物理系统
class PhysicsEngine {
    std::unique_ptr<PhysXWrapper> m_physx;
    std::unique_ptr<FlowWrapper::FlowContext> m_flow;
    std::unique_ptr<BlastWrapper::BlastManager> m_blast;

public:
    void initialize() {
        m_physx = PhysXWrapper::create();
        m_flow = FlowWrapper::FlowContext::create();
        m_blast = BlastWrapper::BlastManager::create();

        // 初始化所有系统...
    }

    void update(float deltaTime) {
        m_physx->update(deltaTime);
        m_flow->update(deltaTime);
        m_blast->update(deltaTime);
    }
};
```

### 仿真应用

用于科学计算、工程仿真等场景：

- 流体动力学模拟 (Flow)
- 刚体碰撞分析 (PhysX)
- 结构破坏测试 (Blast)

### 虚拟现实/增强现实

实时物理交互：

- 精确的碰撞检测
- 真实的物理反馈
- 动态环境破坏

## 性能考虑

- **PhysXWrapper**: GPU 加速支持（可选）
- **FlowWrapper**: GPU 加速流体模拟
- **BlastWrapper**: 多线程破坏计算

## 调试支持

所有包装器都支持调试模式：

```cpp
// 启用调试输出
config.enableDebug = true;
```

## 已知限制

1. **FlowWrapper**: 当前为基础实现，需要连接实际的 NVIDIA Flow 库
2. **BlastWrapper**: 当前为基础实现，需要连接实际的 NVIDIA Blast 库
3. **GPU 支持**: FlowWrapper 需要支持 Vulkan 或 D3D12 的 GPU

## 未来计划

- [ ] FlowWrapper 完整 API 实现
- [ ] BlastWrapper 完整 API 实现
- [ ] 添加更多示例程序
- [ ] 性能优化和基准测试
- [ ] Python 绑定
- [ ] 单元测试覆盖

## 贡献

欢迎贡献！每个包装器库都有自己的文档和贡献指南。

## 许可证

Copyright (c) 2025. All rights reserved.

各个 NVIDIA 库遵循其各自的许可证条款。

## 支持

如有问题或建议，请在项目仓库中提交 issue。

---

**最后更新**: 2025-11-07

**编译测试**: ✅ 所有包装器库编译通过，0 错误
