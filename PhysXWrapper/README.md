# PhysXWrapper - PhysX 物理引擎封装库

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
[![PhysX Version](https://img.shields.io/badge/PhysX-5.6.1-green.svg)](https://github.com/NVIDIA-Omniverse/PhysX)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

**PhysXWrapper** 是对 NVIDIA PhysX 物理引擎的高级C++封装库，旨在简化PhysX的使用，提供更加友好和易用的接口。

## ✨ 特性

- 🚀 **简化的API** - 封装复杂的PhysX初始化和配置流程
- 📦 **模块化设计** - 按功能分类，可按需引入使用
- 🎯 **基于官方示例** - 所有类都基于PhysX官方Snippets改编
- 📚 **完善的文档** - 中英文文档和详细注释
- 🛡️ **资源管理** - RAII模式自动管理PhysX对象生命周期
- ⚡ **性能优化** - 保留PhysX的高性能特性
- 🎮 **丰富的功能** - 支持刚体、软体、粒子、车辆等

## 📋 功能模块

| 模块 | 功能 | 状态 |
|------|------|------|
| **Core** | 核心初始化、场景管理 | 🚧 开发中 |
| **RigidBody** | 刚体动力学 | 🚧 开发中 |
| **Joint** | 关节和约束 | 📅 计划中 |
| **Deformable** | 软体模拟 | 📅 计划中 |
| **Particle** | 粒子系统（PBD） | 📅 计划中 |
| **Vehicle** | 车辆物理 | 📅 计划中 |
| **Query** | 场景查询（射线投射等） | 📅 计划中 |
| **Utility** | 工具类（网格创建等） | 📅 计划中 |

## 🚀 快速开始

### 最简单的示例

```cpp
#include <PhysXWrapper/Core/PhysXCore.h>

int main() {
    // 1. 初始化PhysX
    PhysXCore physx;
    physx.initialize();

    // 2. 创建场景
    PhysXCore::SceneConfig config;
    config.gravity = {0.0f, -9.81f, 0.0f};
    physx.createScene(config);

    // 3. 添加地面
    physx.addGroundPlane(0.0f);

    // 4. 添加一个下落的球体
    PhysXCore::RigidBodyConfig ball;
    ball.position = {0.0f, 10.0f, 0.0f};
    ball.shape = PhysXCore::Shape::Sphere;
    ball.radius = 1.0f;
    ball.mass = 1.0f;

    int ballId = physx.addRigidBody(ball);

    // 5. 模拟
    for (int i = 0; i < 100; ++i) {
        physx.update(1.0f / 60.0f);

        auto transform = physx.getTransform(ballId);
        std::cout << "Y position: " << transform.position.y << std::endl;
    }

    return 0;
}
```

### 编译和运行

```bash
# 1. 编译PhysX核心库（如果还没编译）
cd physx
./generate_projects.sh linux
cd compiler/linux-release && make -j8

# 2. 编译PhysXWrapper
cd ../../PhysXWrapper
mkdir build && cd build
cmake .. -DPHYSX_ROOT=/path/to/physx
make -j8

# 3. 编译您的项目
g++ -std=c++17 main.cpp \
    -I../PhysXWrapper/include \
    -I../physx/include \
    -L../PhysXWrapper/build \
    -L../physx/bin/linux.clang/release \
    -lPhysXWrapper -lPhysX_64 -lPhysXCommon_64 \
    -lPhysXFoundation_64 -lPhysXCooking_64 \
    -o MyApp

# 4. 运行
export LD_LIBRARY_PATH=../PhysXWrapper/build:../physx/bin/linux.clang/release:$LD_LIBRARY_PATH
./MyApp
```

## 📖 文档

- [使用说明](docs/使用说明.md) - 详细的编译、配置和使用指南
- [TODO清单](docs/TODO-任务清单.md) - 所有Snippets分类和开发计划
- [API文档](docs/API.md) - API参考手册（开发中）

## 🔧 依赖

- **PhysX 5.6.1+** - NVIDIA PhysX SDK
- **CMake 3.15+** - 构建系统
- **C++17编译器** - GCC 7+, Clang 6+, MSVC 2017+
- **CUDA 11.0+** (可选) - GPU加速功能

## 📂 项目结构

```
PhysXWrapper/
├── include/              # 公共头文件
│   ├── Core/              # 核心功能
│   ├── RigidBody/         # 刚体相关
│   ├── Joint/             # 关节系统
│   ├── Deformable/        # 可变形体
│   ├── Particle/          # 粒子系统
│   ├── Vehicle/           # 车辆模拟
│   ├── Query/             # 查询系统
│   └── Utility/           # 工具类
├── src/                  # 实现文件（与include结构对应）
├── examples/             # 使用示例
├── docs/                 # 文档
├── cmake/                # CMake模块
├── CMakeLists.txt        # CMake配置
└── README.md             # 本文件
```

## 💡 示例代码

查看 `examples/` 目录获取更多示例：

- `examples/01_HelloWorld/` - 最基础的使用示例
- `examples/02_Collision/` - 碰撞检测和回调
- `examples/03_Joints/` - 关节系统使用
- `examples/04_SoftBody/` - 软体模拟（需要GPU）
- `examples/05_Particles/` - 粒子系统（需要GPU）
- `examples/06_Vehicle/` - 车辆物理模拟
- 更多示例添加中...

## 🗺️ 开发路线图

### 第一阶段 (当前)
- [x] 项目架构设计
- [x] 文档编写
- [ ] PhysXCore基础类
- [ ] RigidBodyContactHandler
- [ ] 基础示例

### 第二阶段
- [ ] 完整的刚体系统
- [ ] 关节系统
- [ ] 查询系统
- [ ] 工具类

### 第三阶段
- [ ] 软体模拟
- [ ] 粒子系统
- [ ] 车辆系统
- [ ] GPU加速

### 第四阶段
- [ ] 性能优化
- [ ] 完善文档
- [ ] 单元测试
- [ ] 发布v1.0

## 🤝 贡献

欢迎贡献！请查看 [CONTRIBUTING.md](CONTRIBUTING.md) 了解如何参与项目开发。

### 贡献方式

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📄 许可证

本项目采用 BSD 3-Clause 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

PhysX SDK 采用 BSD 3-Clause 许可证，版权归 NVIDIA Corporation 所有。

## 🙏 致谢

- [NVIDIA PhysX](https://github.com/NVIDIA-Omniverse/PhysX) - 优秀的物理引擎
- PhysX 官方Snippets - 本项目的灵感来源
- 所有贡献者

## 📞 联系方式

- Issue Tracker: (您的仓库Issue页面)
- Email: (您的邮箱)
- Discord/论坛: (如果有)

## ⭐ Star History

如果这个项目对您有帮助，请给一个Star支持我们！

---

**Made with ❤️ for the PhysX community**
