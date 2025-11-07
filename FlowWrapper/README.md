# FlowWrapper

FlowWrapper 是 NVIDIA Flow 的现代 C++ 包装器库，为流体模拟提供简化的接口。

## 特性

- **简化的 C++ API**: 易于使用的面向对象接口
- **RAII 管理**: 自动资源管理，防止内存泄漏
- **跨平台**: 支持 Linux 和 Windows
- **类型安全**: 强类型接口，减少运行时错误
- **现代 C++17**: 使用现代 C++ 特性

## 构建

### 前提条件

- CMake 3.16 或更高版本
- C++17 兼容编译器 (GCC 7+, Clang 5+, MSVC 2017+)
- NVIDIA Flow 库（已包含在项目中）

### 编译步骤

```bash
cd FlowWrapper
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 编译选项

- `FLOWWRAPPER_BUILD_EXAMPLES`: 构建示例程序（默认：ON）
- `FLOWWRAPPER_BUILD_TESTS`: 构建测试（默认：ON）
- `FLOW_ROOT_DIR`: Flow 库根目录（默认：../flow）

## 使用方法

### 基本示例

```cpp
#include <Flow/FlowContext.h>
#include <iostream>

int main() {
    // 创建 Flow 上下文配置
    FlowWrapper::FlowContextConfig config;
    config.apiType = "Vulkan";
    config.enableDebug = true;
    config.memoryBudgetMB = 512;

    // 创建 Flow 上下文
    auto flowContext = FlowWrapper::FlowContext::create(config);

    // 初始化
    if (!flowContext->initialize()) {
        std::cerr << "Failed to initialize Flow: "
                  << flowContext->getLastError() << std::endl;
        return 1;
    }

    // 主循环
    float deltaTime = 1.0f / 60.0f; // 60 FPS
    for (int i = 0; i < 100; ++i) {
        flowContext->update(deltaTime);
    }

    // 清理（自动调用）
    flowContext->shutdown();

    return 0;
}
```

### 在 CMake 项目中使用

```cmake
# 在你的 CMakeLists.txt 中添加
add_subdirectory(path/to/FlowWrapper)

# 链接到你的目标
target_link_libraries(your_target PRIVATE FlowWrapper)
```

## API 参考

### FlowContext

主要的 Flow 上下文管理类。

#### 方法

- `static std::unique_ptr<FlowContext> create(const FlowContextConfig&)` - 创建 Flow 上下文
- `bool initialize()` - 初始化上下文
- `void shutdown()` - 关闭上下文
- `bool isInitialized() const` - 检查是否已初始化
- `void update(float deltaTime)` - 更新模拟
- `std::string getLastError() const` - 获取最后的错误信息

### FlowContextConfig

Flow 上下文配置结构。

#### 字段

- `std::string apiType` - API 类型（Vulkan, D3D12 等）
- `bool enableDebug` - 启用调试模式
- `int deviceIndex` - 设备索引
- `size_t memoryBudgetMB` - 内存预算（MB）

## 编译结果

成功编译后将生成：

- **库文件**: `libFlowWrapper.a` (~14 KB)
- **头文件**: `include/Flow/FlowContext.h`

## 项目结构

```
FlowWrapper/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 本文件
├── include/                # 公共头文件
│   └── Flow/
│       └── FlowContext.h
├── src/                    # 实现文件
│   └── Flow/
│       └── FlowContext.cpp
├── examples/               # 示例程序
├── tests/                  # 单元测试
└── docs/                   # 文档
```

## 许可证

Copyright (c) 2025. All rights reserved.

## 贡献

欢迎贡献！请参阅项目根目录的 CONTRIBUTING.md。

## 支持

如有问题或建议，请在项目仓库中提交 issue。
