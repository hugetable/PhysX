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
| **RigidBody** | 刚体动力学 | ✅ 已完成 | RigidBodyContactHandler, RigidBodyTrigger, RigidBodyCCD, ContactModifier, GyroscopicForces, AggregateManager |
| **Joint** | 关节和约束 | ✅ 已完成 | JointManager |
| **Articulation** | 关节链系统 | ✅ 已完成 | ArticulationManager |
| **Deformable** | 软体模拟（需要GPU） | ✅ 已完成 | DeformableVolumeManager |
| **Particle** | 粒子系统（PBD） | ✅ 已完成 | PBDFluidManager |
| **Character** | 角色控制器 | ✅ 已完成 | CharacterController |
| **Vehicle** | 车辆物理 | ✅ 已完成 | VehicleManager |
| **Query** | 场景查询（射线投射等） | ✅ 已完成 | GeometryQuery, FrustumQuery, PointDistanceQuery |
| **Utility** | 工具类（网格创建等） | ✅ 已完成 | ConvexMeshBuilder, TriangleMeshBuilder, RigidBodyMassCalculator, SerializationManager, BVHBuilder, CollectionLoader, MaterialLibrary, PhysicsRecorder, PerformanceProfiler |
| **Debug** | 调试可视化 | ✅ 已完成 | DebugDrawer |

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
│   ├── Utility/           # 工具类
│   └── Debug/             # 调试可视化
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
- ✅ `example_debug.cpp` - 物理调试可视化（演示DebugDrawer绘制形状、关节、速度、AABB、质心、自定义图元）
- ✅ `example_bvh.cpp` - BVH空间加速结构（演示BVHBuilder创建BVH、复合球体、盒子网格、性能优化）
- ✅ `example_gyroscopic.cpp` - 陀螺力效果（演示GyroscopicForces展示Dzhanibekov效应、网球拍定理、能量守恒）
- ✅ `example_collection.cpp` - 集合批量加载（演示CollectionLoader加载序列化集合、文件/内存加载、多集合管理）
- ✅ `example_character.cpp` - 角色控制器（演示CharacterController实现角色移动、跳跃、障碍导航、爬楼梯、斜坡行走）
- ✅ `example_aggregate.cpp` - 聚合管理器（演示AggregateManager实现actor分组、ragdoll、debris、vehicle、性能优化）
- ✅ `example_vehicle.cpp` - 车辆管理器（演示VehicleManager实现车辆物理、油门/刹车/转向、4WD/FWD/RWD、多车辆）
- ✅ `example_material.cpp` - 材质库（演示MaterialLibrary使用预定义材质、自定义材质、材质混合、摩擦/弹性对比）
- ✅ `example_recorder.cpp` - 物理录制器（演示PhysicsRecorder录制和回放物理模拟、时间控制、历史记录）
- ✅ `example_profiler.cpp` - 性能分析器（演示PerformanceProfiler监控性能、统计分析、瓶颈检测、导出报告）
- 📅 更多示例添加中...

## 🧪 测试

PhysXWrapper 包含全面的测试套件，确保代码质量和稳定性。

### 测试结构

```
PhysXWrapper/tests/
├── unit/                      # 单元测试
│   ├── test_physxcore.cpp       # PhysXCore初始化和基础功能测试
│   ├── test_rigidbody.cpp       # RigidBody工具类测试
│   ├── test_geometry.cpp        # 几何体构建和查询测试
│   ├── test_joints.cpp          # 关节和关节链系统测试
│   ├── test_queries.cpp         # 查询系统测试（Frustum、PointDistance、BVH）
│   └── test_utilities.cpp       # 工具类测试（Material、Recorder、Profiler等）
└── integration/               # 集成测试
    ├── test_integration_basic.cpp       # 复杂场景集成测试
    └── test_integration_performance.cpp # 性能和可扩展性测试
```

### 运行测试

#### 1. 安装 Google Test

```bash
# Ubuntu/Debian
sudo apt-get install libgtest-dev

# 如果系统未提供，CMake会自动从GitHub下载
```

#### 2. 编译测试

```bash
cd PhysXWrapper
mkdir build && cd build

# 配置并启用测试
cmake .. -DPHYSX_ROOT=/path/to/physx -DPHYSXWRAPPER_BUILD_TESTS=ON

# 编译
make -j8

# 编译测试
make -j8
```

#### 3. 运行所有测试

```bash
# 使用 CTest 运行所有测试
ctest --output-on-failure

# 或者手动运行各个测试
cd tests

# 运行单元测试
./test_physxcore
./test_rigidbody
./test_geometry
./test_joints
./test_queries
./test_utilities

# 运行集成测试
./test_integration_basic
./test_integration_performance
```

#### 4. 运行特定测试

```bash
# 运行特定测试套件
./test_physxcore --gtest_filter=PhysXCoreTest.*

# 运行特定测试用例
./test_rigidbody --gtest_filter=RigidBodyTest.ContactHandlerCallbacks

# 查看详细输出
./test_geometry --gtest_verbose
```

### 测试覆盖

**单元测试 (6个测试文件, 100+ 测试用例)**
- ✅ **PhysXCore测试** (20个测试)
  - 初始化/清理、多线程配置
  - 自定义重力、CCD、GPU动态
  - Actor管理、场景仿真

- ✅ **RigidBody测试** (25个测试)
  - ContactHandler回调和多重接触
  - Trigger体积和形状
  - CCD防止穿透
  - MassCalculator质量和惯性计算

- ✅ **Geometry测试** (18个测试)
  - ConvexMesh创建和标准形状
  - TriangleMesh地形生成
  - GeometryQuery射线投射、扫描、重叠

- ✅ **Joint测试** (20个测试)
  - 所有关节类型（球形、固定、铰链、滑动、距离、D6）
  - 关节链和断裂测试
  - Articulation系统和机器人手臂
  - Ragdoll创建和驱动

- ✅ **Query测试** (18个测试)
  - FrustumQuery视锥体剔除
  - PointDistanceQuery最近点和半径查询
  - BVH构建和加速查询

- ✅ **Utility测试** (22个测试)
  - MaterialLibrary预定义和自定义材质
  - PhysicsRecorder录制和回放
  - PerformanceProfiler性能监控和导出
  - SerializationManager场景序列化
  - DebugDrawer可视化调试

**集成测试 (2个测试文件, 15+ 场景)**
- ✅ **基础集成测试** (8个复杂场景)
  - 完整场景设置（地形、材质、性能分析）
  - 接触处理与关节组合
  - Trigger与Articulation交互
  - 多系统协作（所有功能整合）

- ✅ **性能测试** (7个性能场景)
  - Actor数量扩展性（10/500/1000个actors）
  - Aggregate性能优化对比
  - BVH查询性能
  - 大规模射线投射性能
  - 内存使用跟踪
  - 变量时间步长性能
  - Profiler开销测试

### 测试命令参考

```bash
# 运行所有测试并生成报告
ctest --output-on-failure --verbose

# 并行运行测试
ctest -j8

# 只运行特定类别的测试
ctest -R unit        # 只运行单元测试
ctest -R integration # 只运行集成测试

# 运行性能测试（需要较长时间）
./tests/test_integration_performance

# 测试结果输出到文件
ctest --output-on-failure > test_results.txt 2>&1
```

### 持续集成

测试套件设计用于持续集成环境：

```yaml
# GitHub Actions 示例
- name: Run Tests
  run: |
    cd build
    ctest --output-on-failure -j2
```

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
- [x] **SerializationManager** - 场景序列化系统 (⭐⭐⭐⭐)
- [x] **DebugDrawer** - 物理调试可视化 (⭐⭐⭐⭐)
- [x] **example_debug** - 调试可视化示例程序
- [x] **BVHBuilder** - BVH空间加速结构 (⭐⭐⭐)
- [x] **example_bvh** - BVH构建示例程序
- [x] **GyroscopicForces** - 陀螺力效果 (⭐⭐)
- [x] **example_gyroscopic** - 陀螺力示例程序
- [x] **CollectionLoader** - 集合批量加载 (⭐⭐⭐)
- [x] **example_collection** - 集合加载示例程序
- [x] **CharacterController** - 角色控制器 (⭐⭐⭐⭐⭐)
- [x] **example_character** - 角色控制器示例程序
- [x] **AggregateManager** - 聚合管理器 (⭐⭐⭐⭐)
- [x] **example_aggregate** - 聚合管理器示例程序
- [x] **VehicleManager** - 车辆管理器 (⭐⭐⭐⭐⭐)
- [x] **example_vehicle** - 车辆管理器示例程序
- [x] **MaterialLibrary** - 材质库 (⭐⭐⭐⭐)
- [x] **example_material** - 材质库示例程序
- [x] **PhysicsRecorder** - 物理录制器 (⭐⭐⭐)
- [x] **example_recorder** - 录制器示例程序
- [x] **PerformanceProfiler** - 性能分析器 (⭐⭐⭐⭐⭐)
- [x] **example_profiler** - 性能分析器示例程序

### 第二阶段：高级功能 (已完成)
- [x] JointManager - 基本关节系统（球形、固定、铰链、滑动、距离、D6、关节链）
- [x] ArticulationManager - 减少坐标关节链系统（机器人、骨骼系统）
- [x] DeformableVolumeManager - GPU加速软体模拟（需要CUDA）
- [x] ContactModifier - 运行时接触修改（质量比率、摩擦力、恢复系数）
- [x] PBDFluidManager - 粒子流体系统（水、油、蜜、多材料、扩散粒子）

### 第三阶段：扩展实现 (进行中 - PhysX Snippets完整覆盖)

基于PhysX官方72个Snippets，当前已实现60个(83%) - 🎉 **已达成目标！** 🎉

**高优先级与关节系统 (已完成 ✅)**
- [x] **JointDrive** - D6关节驱动系统 ✅
- [x] **ToleranceScale** - 容差缩放配置 ✅
- [x] **Stepper** - 自定义时间步长控制器 ✅
- [x] **MassProperties** - 质量属性计算示例 ✅
- [x] **MultiThreading** - 多线程优化示例 ✅
- [x] **CustomJoint** - 自定义关节类型 (RopeJoint实现) ✅
- [x] **GearJoint** - 齿轮关节 (齿轮比、齿轮组) ✅
- [x] **RackJoint** - 齿条齿轮关节 (旋转转直线运动) ✅
- [x] **MimicJoint** - 模仿关节 (跟随器、机器人手爪) ✅
- [x] **FixedTendon** - 固定肌腱 (关节链肌腱耦合) ✅
- [x] **SpatialTendon** - 空间肌腱 (3D钢索路径、滑轮系统) ✅
- [x] **ImmediateArticulation** - 即时模式关节链 (FK/ID直接计算) ✅

**软体/流体扩展 (已完成 ✅)**
- [x] **PBDCloth** - PBD布料模拟 (GPU加速粒子布料) ✅
- [x] **PBDInflatable** - PBD充气物体 (体积约束、压力模拟) ✅
- [x] **DeformableMesh** - 可变形网格 (FEM有限元、弹性材料) ✅
- [x] **DeformableSurface** - 可变形表面 (薄壳/膜理论、Kirchhoff-Love) ✅
- [x] **DeformableSurfaceSkinning** - 可变形表面蒙皮 (线性混合、骨骼绑定) ✅
- [x] **DeformableVolumeAttachment** - 软体附着到刚体 (弹簧阻尼、双向耦合) ✅
- [x] **DeformableVolumeKinematic** - 软体与运动学物体交互 (单向驱动、轨迹控制) ✅
- [x] **DeformableVolumeSkinning** - 软体蒙皮 (LBS/DQS、骨骼绑定) ✅

**车辆系统扩展 (已完成 ✅)**
- [x] **VehicleDirectDrive** - 直驱车辆 (电动车、直接扭矩控制) ✅
- [x] **VehicleTankDrive** - 坦克式驱动 (履带车辆、差速转向) ✅
- [x] **VehicleTruck** - 卡车物理 (多轴、载荷分配、制动系统) ✅
- [x] **VehicleCustomSuspension** - 自定义悬挂 (主动/半主动/气动) ✅
- [x] **VehicleCustomTire** - 自定义轮胎模型 (Pacejka公式、温度、磨损) ✅
- [x] **VehicleMultithreading** - 多线程车辆 (并行仿真、Amdahl定律) ✅

**自定义几何/查询 (已完成 ✅ 9/9 = 100%)**
- [x] **CustomGeometry** - 自定义几何类型 (椭球体实现) ✅
- [x] **CustomGeometryCollision** - 自定义几何碰撞 (GJK/EPA算法) ✅
- [x] **CustomGeometryQueries** - 自定义几何查询 (Raycast/Sweep/Overlap) ✅
- [x] **CustomConvex** - 自定义凸几何 (支撑映射、质量属性) ✅
- [x] **StandaloneBroadphase** - 独立宽相剔除器 (AABB管理、SAP算法) ✅
- [x] **StandaloneQuerySystem** - 独立查询系统 (BVH、查询加速) ✅
- [x] **QuerySystemAllQueries** - 查询系统全功能 (Raycast/Sweep/Overlap) ✅
- [x] **QuerySystemCustomCompound** - 自定义复合查询 (复合几何、过滤回调) ✅
- [x] **PrunerSerialization** - 剔除器序列化 (BVH序列化、文件持久化) ✅

**性能优化专题 (部分完成)**
- [x] **MBP** - Multi Box Pruning宽相剔除 (空间划分、区域SAP、并行优化) ✅
- [ ] MultiPruners - 多剔除器支持
- [ ] SplitSim - 分离仿真模式
- [ ] SplitFetchResults - 分离获取结果
- [ ] ImmediateMode - 即时模式(无Scene低层API)

**GPU/Direct API (5个待实现 - 需要CUDA)**
- [ ] HelloGRB - GPU刚体入门
- [ ] RBDirectGPUAPI - 刚体Direct GPU API
- [ ] DirectGPUAPIArticulation - 关节链Direct GPU API
- [ ] DelayLoadHook - GPU延迟加载钩子
- [ ] OmniPvd - OmniVerse PVD可视化

**其他高级特性 (7个待实现)**
- [ ] Isosurface - 等值面生成
- [ ] SDF - 有向距离场
- [ ] PathTracing - 路径追踪
- [ ] CustomProfiler - 自定义分析器
- [ ] ProfilerConverter - 分析器数据转换
- [ ] ContactReportCCD - CCD接触报告
- [ ] 其他工具类特性

**覆盖率统计**: 60/72 已实现 (83%) | 🎯 **目标达成！** 🎯 | 剩余12个为高级/GPU特性

### 第四阶段：完善和优化 (持续)
- [ ] 性能优化和基准测试
- [ ] 完善API文档
- [x] **单元测试和集成测试** - 8个测试套件，100+测试用例，全面覆盖
- [ ] 示例程序库
- [ ] 发布v1.0

**当前进度**: 第一阶段 100% 完成！第二阶段 100% 完成 - 已实现26个核心类、27个示例程序、8个测试套件(100+测试用例)，约45,000+行代码：
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
- SerializationManager（序列化：场景保存/加载、对象序列化、二进制格式、内存管理）
- DebugDrawer（调试可视化：形状线框、关节、速度矢量、AABB、质心、坐标轴）
- BVHBuilder（BVH加速结构：复合actor优化、场景查询加速、空间划分、性能提升）
- GyroscopicForces（陀螺力效果：Dzhanibekov效应、网球拍定理、角动量守恒、能量分析）
- CollectionLoader（集合加载器：批量加载、场景资产管理、二进制/XML格式、依赖管理）
- CharacterController（角色控制器：运动学角色、跳跃、爬坡、爬楼梯、碰撞检测、胶囊/盒体）
- AggregateManager（聚合管理器：actor分组、性能优化、ragdoll、debris、vehicle）
- VehicleManager（车辆管理器：4轮车辆、悬挂系统、引擎/变速箱、油门/刹车/转向、4WD/FWD/RWD）
- MaterialLibrary（材质库：20+预定义材质、自定义材质、材质混合、真实物理属性）
- PhysicsRecorder（物理录制器：录制/回放、时间控制、帧历史、保存/加载）
- PerformanceProfiler（性能分析器：帧率统计、性能监控、瓶颈检测、CSV/JSON导出）

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
