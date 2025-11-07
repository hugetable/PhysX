# BlastWrapper

BlastWrapper 是 NVIDIA Blast 的现代 C++ 包装器库，为破坏模拟提供简化的接口。

## 特性

- **简化的 C++ API**: 易于使用的面向对象接口
- **RAII 管理**: 自动资源管理，防止内存泄漏
- **跨平台**: 支持 Linux 和 Windows
- **类型安全**: 强类型接口，减少运行时错误
- **现代 C++17**: 使用现代 C++ 特性
- **事件系统**: 完整的破坏事件回调支持

## 构建

### 前提条件

- CMake 3.16 或更高版本
- C++17 兼容编译器 (GCC 7+, Clang 5+, MSVC 2017+)
- NVIDIA Blast 库（已包含在项目中）

### 编译步骤

```bash
cd BlastWrapper
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 编译选项

- `BLASTWRAPPER_BUILD_EXAMPLES`: 构建示例程序（默认：ON）
- `BLASTWRAPPER_BUILD_TESTS`: 构建测试（默认：ON）
- `BLAST_ROOT_DIR`: Blast 库根目录（默认：../blast）

## 使用方法

### 基本示例

```cpp
#include <Blast/BlastManager.h>
#include <iostream>

int main() {
    // 创建 Blast 管理器配置
    BlastWrapper::BlastConfig config;
    config.enableProfiling = true;
    config.maxActors = 1024;
    config.maxChunksPerActor = 256;
    config.enableDebugRender = false;

    // 创建 Blast 管理器
    auto blastManager = BlastWrapper::BlastManager::create(config);

    // 初始化
    if (!blastManager->initialize()) {
        std::cerr << "Failed to initialize Blast: "
                  << blastManager->getLastError() << std::endl;
        return 1;
    }

    // 应用伤害
    uint32_t actorId = 0;
    float damagePosition[3] = {0.0f, 1.0f, 0.0f};
    float damageAmount = 100.0f;
    float damageRadius = 2.0f;

    uint32_t numEvents = blastManager->applyDamage(
        actorId, damageAmount, damagePosition, damageRadius
    );

    std::cout << "Generated " << numEvents << " fracture events" << std::endl;

    // 获取破坏事件
    auto events = blastManager->getFractureEvents();
    for (const auto& event : events) {
        std::cout << "Actor " << event.actorId
                  << " chunk " << event.chunkIndex
                  << " damaged: " << event.damage
                  << " (split: " << event.isSplit << ")" << std::endl;
    }

    // 主循环
    float deltaTime = 1.0f / 60.0f; // 60 FPS
    for (int i = 0; i < 100; ++i) {
        blastManager->update(deltaTime);
    }

    // 清理（自动调用）
    blastManager->shutdown();

    return 0;
}
```

### 在 CMake 项目中使用

```cmake
# 在你的 CMakeLists.txt 中添加
add_subdirectory(path/to/BlastWrapper)

# 链接到你的目标
target_link_libraries(your_target PRIVATE BlastWrapper)
```

## API 参考

### BlastManager

主要的 Blast 管理器类。

#### 方法

- `static std::unique_ptr<BlastManager> create(const BlastConfig&)` - 创建 Blast 管理器
- `bool initialize()` - 初始化管理器
- `void shutdown()` - 关闭管理器
- `bool isInitialized() const` - 检查是否已初始化
- `void update(float deltaTime)` - 更新模拟
- `uint32_t applyDamage(...)` - 应用伤害
- `std::vector<FractureEvent> getFractureEvents() const` - 获取破坏事件
- `std::string getLastError() const` - 获取最后的错误信息

### BlastConfig

Blast 管理器配置结构。

#### 字段

- `bool enableProfiling` - 启用性能分析
- `uint32_t maxActors` - 最大 actor 数量
- `uint32_t maxChunksPerActor` - 每个 actor 的最大块数
- `bool enableDebugRender` - 启用调试渲染

### FractureEvent

破坏事件数据结构。

#### 字段

- `uint32_t actorId` - Actor 标识符
- `uint32_t chunkIndex` - 块索引
- `float damage` - 伤害量
- `bool isSplit` - 是否分裂

## 编译结果

成功编译后将生成：

- **库文件**: `libBlastWrapper.a` (~18 KB)
- **头文件**: `include/Blast/BlastManager.h`

## 项目结构

```
BlastWrapper/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 本文件
├── include/                # 公共头文件
│   └── Blast/
│       └── BlastManager.h
├── src/                    # 实现文件
│   └── Blast/
│       └── BlastManager.cpp
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
