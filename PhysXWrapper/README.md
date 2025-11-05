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

| 模块 | 功能 | 状态 | 已实现类 |
|------|------|------|----------|
| **Core** | 核心初始化、场景管理 | ✅ 已完成 | PhysXCore |
| **RigidBody** | 刚体动力学 | ✅ 已完成 | RigidBodyContactHandler, RigidBodyTrigger, RigidBodyCCD, ContactModifier |
| **Joint** | 关节和约束 | ✅ 已完成 | JointManager |
| **Articulation** | 关节链系统 | ✅ 已完成 | ArticulationManager |
| **Deformable** | 软体模拟（需要GPU） | ✅ 已完成 | DeformableVolumeManager |
| **Particle** | 粒子系统（PBD） | ✅ 已完成 | PBDFluidManager |
| **Vehicle** | 车辆物理 | 📅 计划中 | - |
| **Query** | 场景查询（射线投射等） | ✅ 已完成 | GeometryQuery, FrustumQuery, PointDistanceQuery |
| **Utility** | 工具类（网格创建等） | ✅ 已完成 | ConvexMeshBuilder, TriangleMeshBuilder, RigidBodyMassCalculator |

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

- ✅ `example_helloworld.cpp` - 最基础的使用示例（演示PhysXCore基本功能）
- ✅ `example_contactreport.cpp` - 碰撞检测和回调（演示RigidBodyContactHandler）
- ✅ `example_geometryquery.cpp` - 场景查询（演示GeometryQuery射线投射、扫描、重叠检测）
- ✅ `example_convexmesh.cpp` - 凸网格创建（演示ConvexMeshBuilder从点云创建网格）
- ✅ `example_trianglemesh.cpp` - 三角网格创建（演示TriangleMeshBuilder创建地形等静态几何体）
- ✅ `example_trigger.cpp` - 触发器体积（演示RigidBodyTrigger创建触发区域和事件检测）
- ✅ `example_ccd.cpp` - 连续碰撞检测（演示RigidBodyCCD防止高速物体穿透）
- ✅ `example_joint.cpp` - 关节系统（演示JointManager创建各种关节类型：球形、固定、铰链、滑动、距离、D6、关节链）
- ✅ `example_articulation.cpp` - 关节链系统（演示ArticulationManager创建机器人手臂、灵活链条、自定义关节链）
- ✅ `example_deformable.cpp` - 软体变形体（演示DeformableVolumeManager创建软体、GPU加速、材料配置）**需要GPU/CUDA**
- ✅ `example_contactmodifier.cpp` - 运行时接触修改（演示ContactModifier调整质量比率、摩擦力、恢复系数、自定义修改）
- ✅ `example_pbdfluid.cpp` - PBD流体模拟（演示PBDFluidManager创建水、油、蜜等流体、多材料、扩散粒子效果）**需要GPU/CUDA**
- ✅ `example_frustum.cpp` - 视锥体剔除（演示FrustumQuery进行可见性剔除、BVH加速、相机视锥体查询）
- ✅ `example_pointdistance.cpp` - 点距离查询（演示PointDistanceQuery查找最近点、批量查询、场景查询、距离场生成）
- 📅 更多示例添加中...

## 🗺️ 开发路线图

### 第一阶段：核心基础 (当前 - 2025年11月)
- [x] 项目架构设计
- [x] 完整文档编写（研究文档、TODO清单、使用指南）
- [x] CMake构建系统
- [x] **PhysXCore基础类** - Foundation/Physics/Scene管理
- [x] **example_helloworld** - HelloWorld示例程序
- [x] **RigidBodyContactHandler** - 碰撞回调系统 (⭐⭐⭐⭐⭐)
- [x] **example_contactreport** - 碰撞事件示例程序
- [x] **GeometryQuery** - 射线投射和查询 (⭐⭐⭐⭐⭐)
- [x] **example_geometryquery** - 场景查询示例程序
- [x] **ConvexMeshBuilder** - 凸网格创建 (⭐⭐⭐⭐)
- [x] **example_convexmesh** - 凸网格创建示例程序
- [x] **TriangleMeshBuilder** - 三角网格创建 (⭐⭐⭐⭐)
- [x] **example_trianglemesh** - 三角网格创建示例程序
- [x] **RigidBodyTrigger** - 触发器区域 (⭐⭐⭐⭐)
- [x] **example_trigger** - 触发器示例程序
- [x] **RigidBodyCCD** - 连续碰撞检测 (⭐⭐⭐⭐)
- [x] **example_ccd** - CCD示例程序
- [x] **RigidBodyMassCalculator** - 质量属性计算 (⭐⭐⭐⭐)
- [x] **JointManager** - 关节系统 (⭐⭐⭐⭐⭐)
- [x] **example_joint** - 关节系统示例程序
- [x] **ArticulationManager** - 关节链系统 (⭐⭐⭐⭐⭐)
- [x] **example_articulation** - 关节链系统示例程序
- [x] **DeformableVolumeManager** - 软体变形体系统 (⭐⭐⭐⭐⭐)
- [x] **example_deformable** - 软体变形体示例程序
- [x] **ContactModifier** - 运行时接触修改 (⭐⭐⭐⭐)
- [x] **example_contactmodifier** - 接触修改示例程序
- [x] **PBDFluidManager** - 粒子流体系统 (⭐⭐⭐⭐⭐)
- [x] **example_pbdfluid** - PBD流体模拟示例程序
- [x] **FrustumQuery** - 视锥体剔除查询 (⭐⭐⭐)
- [x] **example_frustum** - 视锥体剔除示例程序
- [x] **PointDistanceQuery** - 点距离查询 (⭐⭐⭐)
- [x] **example_pointdistance** - 点距离查询示例程序

### 第二阶段：高级功能 (已完成)
- [x] JointManager - 基本关节系统（球形、固定、铰链、滑动、距离、D6、关节链）
- [x] ArticulationManager - 减少坐标关节链系统（机器人、骨骼系统）
- [x] DeformableVolumeManager - GPU加速软体模拟（需要CUDA）
- [x] ContactModifier - 运行时接触修改（质量比率、摩擦力、恢复系数）
- [x] PBDFluidManager - 粒子流体系统（水、油、蜜、多材料、扩散粒子）

### 第四阶段：完善和优化 (持续)
- [ ] 性能优化和基准测试
- [ ] 完善API文档
- [ ] 单元测试和集成测试
- [ ] 示例程序库
- [ ] 发布v1.0

**当前进度**: 第一阶段 100% 完成！第二阶段 100% 完成 - 已实现15个核心类：
- PhysXCore（核心初始化）
- RigidBodyContactHandler（碰撞回调）
- GeometryQuery（场景查询）
- ConvexMeshBuilder（凸网格）
- TriangleMeshBuilder（三角网格）
- RigidBodyTrigger（触发器）
- RigidBodyCCD（连续碰撞检测）
- RigidBodyMassCalculator（质量计算）
- JointManager（关节系统：球形、固定、铰链、滑动、距离、D6、关节链）
- ArticulationManager（关节链系统：机器人、骨骼、灵活链条）
- DeformableVolumeManager（软体变形体：GPU加速、材料配置、网格生成）
- ContactModifier（运行时接触修改：质量比率、摩擦力、恢复系数、自定义规则）
- PBDFluidManager（粒子流体：水、油、蜜等多材料、扩散粒子效果、GPU加速）
- FrustumQuery（视锥体剔除：相机视锥体查询、BVH加速、场景剔除）
- PointDistanceQuery（点距离查询：最近点计算、批量查询、场景查询、距离场生成）

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
