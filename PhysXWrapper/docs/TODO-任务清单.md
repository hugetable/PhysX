# PhysXWrapper 项目任务清单

## 项目概述

本项目目标是将 PhysX 的核心功能和示例（snippets）封装成一个易用的C++类库，方便其他项目调用。

## 目录结构

```
PhysXWrapper/
├── include/              # 头文件
│   ├── Core/              # 核心封装类
│   ├── RigidBody/         # 刚体相关
│   ├── Deformable/        # 可变形体
│   ├── Particle/          # 粒子系统
│   ├── Vehicle/           # 车辆模拟
│   ├── Joint/             # 关节和连接
│   ├── Query/             # 查询系统
│   └── Utility/           # 工具类
├── src/                  # 实现文件
│   ├── Core/
│   ├── RigidBody/
│   ├── Deformable/
│   ├── Particle/
│   ├── Vehicle/
│   ├── Joint/
│   ├── Query/
│   └── Utility/
├── examples/             # 使用示例
├── docs/                 # 文档
└── cmake/                # CMake配置
```

---

## Snippets 分类清单

### 📦 一、核心基础类 (Core)

#### 1.1 基础示例
- [ ] **SnippetHelloWorld** - 最基本的PhysX使用
  - 源文件: `physx/snippets/snippethelloworld/`
  - 功能: 创建场景、添加简单物体、运行模拟
  - 优先级: ⭐⭐⭐⭐⭐ (最高)
  - 封装类名: `PhysXCore`

- [ ] **SnippetHelloGRB** - GPU刚体基础
  - 源文件: `physx/snippets/snippethellogrb/`
  - 功能: GPU加速刚体模拟入门
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `PhysXGPUCore`

- [ ] **SnippetMultithreading** - 多线程示例
  - 源文件: `physx/snippets/snippetmultithreading/`
  - 功能: 多线程物理模拟
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `PhysXMultiThreadManager`

- [ ] **SnippetSplitSim** - 分离模拟
  - 源文件: `physx/snippets/snippetsplitsim/`
  - 功能: 碰撞检测和动力学分离执行
  - 优先级: ⭐⭐⭐
  - 封装类名: `PhysXSplitSimulation`

- [ ] **SnippetSplitFetchResults** - 分离结果获取
  - 源文件: `physx/snippets/snippetsplitfetchresults/`
  - 功能: 异步获取模拟结果
  - 优先级: ⭐⭐⭐
  - 封装类名: `PhysXAsyncFetch`

- [ ] **SnippetStepper** - 步进控制
  - 源文件: `physx/snippets/snippetstepper/`
  - 功能: 精确控制模拟步长
  - 优先级: ⭐⭐⭐
  - 封装类名: `PhysXStepper`

#### 1.2 工具和配置
- [ ] **SnippetToleranceScale** - 容差缩放
  - 源文件: `physx/snippets/snippettolerancescale/`
  - 功能: 不同尺度场景的容差设置
  - 优先级: ⭐⭐⭐
  - 封装类名: `PhysXToleranceManager`

- [ ] **SnippetCustomProfiler** - 自定义性能分析器
  - 源文件: `physx/snippets/snippetcustomprofiler/`
  - 功能: 性能监控和分析
  - 优先级: ⭐⭐
  - 封装类名: `PhysXProfiler`

- [ ] **SnippetOmniPvd** - Omniverse PVD调试
  - 源文件: `physx/snippets/snippetomnipvd/`
  - 功能: 可视化调试工具
  - 优先级: ⭐⭐
  - 封装类名: `PhysXDebugVisualizer`

- [ ] **SnippetDelayLoadHook** - 延迟加载钩子
  - 源文件: `physx/snippets/snippetdelayloadhook/`
  - 功能: 动态库延迟加载
  - 优先级: ⭐
  - 封装类名: `PhysXDynamicLoader`

---

### 🎯 二、刚体动力学类 (RigidBody)

#### 2.1 基础刚体
- [ ] **SnippetContactReport** - 接触报告
  - 源文件: `physx/snippets/snippetcontactreport/`
  - 功能: 碰撞接触事件回调
  - 优先级: ⭐⭐⭐⭐⭐
  - 封装类名: `RigidBodyContactHandler`

- [ ] **SnippetContactModification** - 接触修改
  - 源文件: `physx/snippets/snippetcontactmodification/`
  - 功能: 运行时修改接触点
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `RigidBodyContactModifier`

- [ ] **SnippetTriggers** - 触发器
  - 源文件: `physx/snippets/snippettriggers/`
  - 功能: 触发区域检测
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `RigidBodyTrigger`

- [ ] **SnippetCCD** - 连续碰撞检测
  - 源文件: `physx/snippets/snippetccd/`
  - 功能: 防止高速物体穿透
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `RigidBodyCCD`

- [ ] **SnippetContactReportCCD** - CCD接触报告
  - 源文件: `physx/snippets/snippetcontactreportccd/`
  - 功能: CCD碰撞事件
  - 优先级: ⭐⭐⭐
  - 封装类名: `RigidBodyCCDContact`

- [ ] **SnippetMassProperties** - 质量属性
  - 源文件: `physx/snippets/snippetmassproperties/`
  - 功能: 计算和设置质量、惯性
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `RigidBodyMassCalculator`

- [ ] **SnippetGyroscopic** - 陀螺效应
  - 源文件: `physx/snippets/snippetgyroscopic/`
  - 功能: 陀螺仪力学效果
  - 优先级: ⭐⭐
  - 封装类名: `RigidBodyGyroscopic`

#### 2.2 GPU刚体
- [ ] **SnippetRBDirectGPUAPI** - 刚体GPU直接API
  - 源文件: `physx/snippets/snippetrbdirectgpuapi/`
  - 功能: GPU刚体直接控制
  - 优先级: ⭐⭐⭐
  - 封装类名: `GPURigidBodyManager`

---

### 🔗 三、关节和约束类 (Joint)

#### 3.1 基础关节
- [ ] **SnippetJoint** - 基本关节
  - 源文件: `physx/snippets/snippetjoint/`
  - 功能: 各类关节创建和使用
  - 优先级: ⭐⭐⭐⭐⭐
  - 封装类名: `JointManager`

- [ ] **SnippetJointDrive** - 关节驱动
  - 源文件: `physx/snippets/snippetjointdrive/`
  - 功能: 主动驱动关节
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `JointDriveController`

- [ ] **SnippetCustomJoint** - 自定义关节
  - 源文件: `physx/snippets/snippetcustomjoint/`
  - 功能: 创建用户自定义关节类型
  - 优先级: ⭐⭐⭐
  - 封装类名: `CustomJointBuilder`

- [ ] **SnippetGearJoint** - 齿轮关节
  - 源文件: `physx/snippets/snippetgearjoint/`
  - 功能: 齿轮传动关节
  - 优先级: ⭐⭐
  - 封装类名: `GearJoint`

- [ ] **SnippetRackJoint** - 齿条关节
  - 源文件: `physx/snippets/snippetrackjoint/`
  - 功能: 齿条齿轮关节
  - 优先级: ⭐⭐
  - 封装类名: `RackJoint`

#### 3.2 关节链 (Articulation)
- [ ] **SnippetArticulationRC** - 减少坐标关节链
  - 源文件: `physx/snippets/snippetarticulationrc/`
  - 功能: 机器人、骨骼系统
  - 优先级: ⭐⭐⭐⭐⭐
  - 封装类名: `ArticulationManager`

- [ ] **SnippetImmediateArticulation** - 即时关节链
  - 源文件: `physx/snippets/snippetimmediatearticulation/`
  - 功能: 低级关节链API
  - 优先级: ⭐⭐⭐
  - 封装类名: `ImmediateArticulation`

- [ ] **SnippetDirectGPUAPIArticulation** - GPU关节链
  - 源文件: `physx/snippets/snippetdirectgpuapiarticulation/`
  - 功能: GPU加速关节链
  - 优先级: ⭐⭐⭐
  - 封装类名: `GPUArticulation`

- [ ] **SnippetFixedTendon** - 固定肌腱
  - 源文件: `physx/snippets/snippetfixedtendon/`
  - 功能: 关节链肌腱约束
  - 优先级: ⭐⭐
  - 封装类名: `FixedTendon`

- [ ] **SnippetSpatialTendon** - 空间肌腱
  - 源文件: `physx/snippets/snippetspatialtendon/`
  - 功能: 空间肌腱约束
  - 优先级: ⭐⭐
  - 封装类名: `SpatialTendon`

- [ ] **SnippetMimicJoint** - 模仿关节
  - 源文件: `physx/snippets/snippetmimicjoint/`
  - 功能: 从动关节
  - 优先级: ⭐⭐
  - 封装类名: `MimicJoint`

---

### 🧊 四、可变形体类 (Deformable)

#### 4.1 可变形网格
- [ ] **SnippetDeformableMesh** - 可变形网格基础
  - 源文件: `physx/snippets/snippetdeformablemesh/`
  - 功能: 基础可变形体
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `DeformableMesh`

- [ ] **SnippetDeformableVolume** - 可变形体积
  - 源文件: `physx/snippets/snippetdeformablevolume/`
  - 功能: 软体模拟
  - 优先级: ⭐⭐⭐⭐⭐
  - 封装类名: `DeformableVolume`

- [ ] **SnippetDeformableVolumeKinematic** - 运动学可变形体
  - 源文件: `physx/snippets/snippetdeformablevolumekinematic/`
  - 功能: 运动学控制软体
  - 优先级: ⭐⭐⭐
  - 封装类名: `KinematicDeformable`

- [ ] **SnippetDeformableVolumeAttachment** - 可变形体附件
  - 源文件: `physx/snippets/snippetdeformablevolumeattachment/`
  - 功能: 软体与刚体附着
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `DeformableAttachment`

- [ ] **SnippetDeformableVolumeSkinning** - 可变形体蒙皮
  - 源文件: `physx/snippets/snippetdeformablevolumeskinning/`
  - 功能: 软体蒙皮绑定
  - 优先级: ⭐⭐⭐
  - 封装类名: `DeformableSkinning`

#### 4.2 可变形表面
- [ ] **SnippetDeformableSurface** - 可变形表面
  - 源文件: `physx/snippets/snippetdeformablesurface/`
  - 功能: 布料、薄膜模拟
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `DeformableSurface`

- [ ] **SnippetDeformableSurfaceSkinning** - 表面蒙皮
  - 源文件: `physx/snippets/snippetdeformablesurfaceskinning/`
  - 功能: 布料蒙皮
  - 优先级: ⭐⭐⭐
  - 封装类名: `SurfaceSkinning`

---

### 💧 五、粒子系统类 (Particle)

#### 5.1 PBD粒子
- [ ] **SnippetPBF** - 基于位置的流体
  - 源文件: `physx/snippets/snippetpbf/`
  - 功能: PBD流体模拟
  - 优先级: ⭐⭐⭐⭐⭐
  - 封装类名: `PBDFluid`

- [ ] **SnippetPBFMultimat** - 多材质PBF
  - 源文件: `physx/snippets/snippetpbfmultimat/`
  - 功能: 多材质流体交互
  - 优先级: ⭐⭐⭐
  - 封装类名: `MultiMaterialFluid`

- [ ] **SnippetPBDCloth** - PBD布料
  - 源文件: `physx/snippets/snippetpbdcloth/`
  - 功能: 基于位置的布料
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `PBDCloth`

- [ ] **SnippetPBDInflatable** - PBD充气物体
  - 源文件: `physx/snippets/snippetpbdinflatable/`
  - 功能: 充气物体模拟
  - 优先级: ⭐⭐⭐
  - 封装类名: `PBDInflatable`

#### 5.2 高级粒子
- [ ] **SnippetSDF** - 符号距离场
  - 源文件: `physx/snippets/snippetsdf/`
  - 功能: SDF粒子碰撞
  - 优先级: ⭐⭐
  - 封装类名: `SDFParticle`

- [ ] **SnippetIsoSurface** - 等值面
  - 源文件: `physx/snippets/snippetisosurface/`
  - 功能: 流体表面重建
  - 优先级: ⭐⭐
  - 封装类名: `IsoSurface`

---

### 🚗 六、车辆模拟类 (Vehicle)

- [ ] **SnippetVehicleFourWheelDrive** - 四轮驱动车辆
  - 源文件: `physx/snippets/snippetvehiclefourwheeldrive/`
  - 功能: 标准四轮车
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `VehicleFourWheels`

- [ ] **SnippetVehicleDirectDrive** - 直接驱动车辆
  - 源文件: `physx/snippets/snippetvehicledirectdrive/`
  - 功能: 电动车模拟
  - 优先级: ⭐⭐⭐
  - 封装类名: `VehicleDirectDrive`

- [ ] **SnippetVehicleTankDrive** - 坦克驱动
  - 源文件: `physx/snippets/snippetvehicletankdrive/`
  - 功能: 履带车辆
  - 优先级: ⭐⭐⭐
  - 封装类名: `VehicleTank`

- [ ] **SnippetVehicleTruck** - 卡车
  - 源文件: `physx/snippets/snippetvehicletruck/`
  - 功能: 多轴卡车
  - 优先级: ⭐⭐
  - 封装类名: `VehicleTruck`

- [ ] **SnippetVehicleCustomSuspension** - 自定义悬挂
  - 源文件: `physx/snippets/snippetvehiclecustomsuspension/`
  - 功能: 自定义悬挂系统
  - 优先级: ⭐⭐
  - 封装类名: `VehicleCustomSuspension`

- [ ] **SnippetVehicleCustomTire** - 自定义轮胎
  - 源文件: `physx/snippets/snippetvehiclecustomtire/`
  - 功能: 自定义轮胎模型
  - 优先级: ⭐⭐
  - 封装类名: `VehicleCustomTire`

- [ ] **SnippetVehicleMultithreading** - 车辆多线程
  - 源文件: `physx/snippets/snippetvehiclemultithreading/`
  - 功能: 多线程车辆模拟
  - 优先级: ⭐⭐
  - 封装类名: `VehicleMultiThread`

---

### 🔍 七、查询系统类 (Query)

#### 7.1 几何查询
- [ ] **SnippetGeometryQuery** - 几何查询
  - 源文件: `physx/snippets/snippetgeometryquery/`
  - 功能: 射线投射、扫描、重叠测试
  - 优先级: ⭐⭐⭐⭐⭐
  - 封装类名: `GeometryQuery`

- [ ] **SnippetFrustumQuery** - 视锥体查询
  - 源文件: `physx/snippets/snippetfrustumquery/`
  - 功能: 视锥体裁剪查询
  - 优先级: ⭐⭐⭐
  - 封装类名: `FrustumQuery`

- [ ] **SnippetPointDistanceQuery** - 点距离查询
  - 源文件: `physx/snippets/snippetpointdistancequery/`
  - 功能: 最近点查询
  - 优先级: ⭐⭐⭐
  - 封装类名: `PointDistanceQuery`

#### 7.2 高级查询
- [ ] **SnippetQuerySystemAllQueries** - 全查询系统
  - 源文件: `physx/snippets/snippetquerysystemallqueries/`
  - 功能: 完整查询系统示例
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `QuerySystemAll`

- [ ] **SnippetQuerySystemCustomCompound** - 自定义复合查询
  - 源文件: `physx/snippets/snippetquerysystemcustomcompound/`
  - 功能: 复合形状查询
  - 优先级: ⭐⭐
  - 封装类名: `CustomCompoundQuery`

- [ ] **SnippetStandaloneQuerySystem** - 独立查询系统
  - 源文件: `physx/snippets/snippetstandalonequerysystem/`
  - 功能: 无场景查询系统
  - 优先级: ⭐⭐
  - 封装类名: `StandaloneQuerySystem`

---

### 🔧 八、工具和数据结构类 (Utility)

#### 8.1 几何创建
- [ ] **SnippetConvexMeshCreate** - 凸网格创建
  - 源文件: `physx/snippets/snippetconvexmeshcreate/`
  - 功能: 创建凸包网格
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `ConvexMeshBuilder`

- [ ] **SnippetTriangleMeshCreate** - 三角网格创建
  - 源文件: `physx/snippets/snippettrianglemeshcreate/`
  - 功能: 创建三角网格
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `TriangleMeshBuilder`

- [ ] **SnippetCustomGeometry** - 自定义几何
  - 源文件: `physx/snippets/snippetcustomgeometry/`
  - 功能: 用户自定义几何类型
  - 优先级: ⭐⭐⭐
  - 封装类名: `CustomGeometry`

- [ ] **SnippetCustomGeometryCollision** - 自定义几何碰撞
  - 源文件: `physx/snippets/snippetcustomgeometrycollision/`
  - 功能: 自定义几何碰撞检测
  - 优先级: ⭐⭐⭐
  - 封装类名: `CustomGeometryCollider`

- [ ] **SnippetCustomGeometryQueries** - 自定义几何查询
  - 源文件: `physx/snippets/snippetcustomgeometryqueries/`
  - 功能: 自定义几何查询
  - 优先级: ⭐⭐
  - 封装类名: `CustomGeometryQuery`

- [ ] **SnippetCustomConvex** - 自定义凸体
  - 源文件: `physx/snippets/snippetcustomconvex/`
  - 功能: 自定义凸体类型
  - 优先级: ⭐⭐
  - 封装类名: `CustomConvex`

#### 8.2 空间加速结构
- [ ] **SnippetBVHStructure** - BVH结构
  - 源文件: `physx/snippets/snippetbvhstructure/`
  - 功能: 层次包围盒
  - 优先级: ⭐⭐⭐
  - 封装类名: `BVHBuilder`

- [ ] **SnippetStandaloneBVH** - 独立BVH
  - 源文件: `physx/snippets/snippetstandalonebvh/`
  - 功能: 无场景BVH
  - 优先级: ⭐⭐
  - 封装类名: `StandaloneBVH`

- [ ] **SnippetMBP** - 多盒剪枝
  - 源文件: `physx/snippets/snippetmbp/`
  - 功能: MBP广相位算法
  - 优先级: ⭐⭐
  - 封装类名: `MBPBroadPhase`

- [ ] **SnippetStandaloneBroadphase** - 独立广相位
  - 源文件: `physx/snippets/snippetstandalonebroadphase/`
  - 功能: 无场景广相位
  - 优先级: ⭐⭐
  - 封装类名: `StandaloneBroadPhase`

- [ ] **SnippetMultiPruners** - 多剪枝器
  - 源文件: `physx/snippets/snippetmultipruners/`
  - 功能: 多个剪枝结构
  - 优先级: ⭐⭐
  - 封装类名: `MultiPruner`

- [ ] **SnippetPrunerSerialization** - 剪枝器序列化
  - 源文件: `physx/snippets/snippetprunerserialization/`
  - 功能: 保存/加载剪枝结构
  - 优先级: ⭐
  - 封装类名: `PrunerSerializer`

- [ ] **SnippetPathTracing** - 路径追踪
  - 源文件: `physx/snippets/snippetpathtracing/`
  - 功能: 光线追踪演示
  - 优先级: ⭐
  - 封装类名: `PathTracer`

#### 8.3 序列化和集合
- [ ] **SnippetSerialization** - 序列化
  - 源文件: `physx/snippets/snippetserialization/`
  - 功能: 场景序列化
  - 优先级: ⭐⭐⭐⭐
  - 封装类名: `SceneSerializer`

- [ ] **SnippetLoadCollection** - 加载集合
  - 源文件: `physx/snippets/snippetloadcollection/`
  - 功能: 批量加载对象
  - 优先级: ⭐⭐⭐
  - 封装类名: `CollectionLoader`

#### 8.4 即时模式
- [ ] **SnippetImmediateMode** - 即时模式
  - 源文件: `physx/snippets/snippetimmediatemode/`
  - 功能: 低级即时API
  - 优先级: ⭐⭐⭐
  - 封装类名: `ImmediateModePhysics`

---

## 实现优先级说明

### ⭐⭐⭐⭐⭐ 最高优先级（必须实现）
这些是最基础和最常用的功能，应该首先实现：
1. SnippetHelloWorld - 入门基础
2. SnippetContactReport - 碰撞检测
3. SnippetArticulationRC - 关节链
4. SnippetJoint - 基本关节
5. SnippetGeometryQuery - 查询系统
6. SnippetDeformableVolume - 软体
7. SnippetPBF - 流体

### ⭐⭐⭐⭐ 高优先级（核心功能）
这些是常用的高级功能：
- 刚体高级特性 (CCD, 触发器等)
- 车辆模拟
- 网格创建工具
- 序列化

### ⭐⭐⭐ 中优先级（扩展功能）
这些是特定场景需要的功能：
- 自定义几何
- 高级查询
- GPU加速

### ⭐⭐ 低优先级（可选功能）
这些是专业或罕见场景：
- 性能分析工具
- 独立模块
- 特殊效果

### ⭐ 最低优先级（可忽略）
这些可以根据需求决定是否实现：
- 调试工具
- 示例演示

---

## 实现步骤建议

### 第一阶段：核心基础 (2-4周)
1. PhysXCore (HelloWorld)
2. RigidBodyContactHandler (ContactReport)
3. GeometryQuery (GeometryQuery)
4. ConvexMeshBuilder, TriangleMeshBuilder

### 第二阶段：刚体完善 (2-3周)
5. RigidBodyTrigger
6. RigidBodyCCD
7. RigidBodyContactModifier
8. RigidBodyMassCalculator

### 第三阶段：关节系统 (2-3周)
9. JointManager
10. ArticulationManager
11. JointDriveController

### 第四阶段：高级功能 (3-4周)
12. DeformableVolume (软体)
13. PBDFluid (流体)
14. VehicleFourWheels (车辆)
15. SceneSerializer (序列化)

### 第五阶段：GPU和优化 (2-3周)
16. PhysXGPUCore
17. GPURigidBodyManager
18. GPUArticulation

### 第六阶段：完善和测试 (持续)
- 编写单元测试
- 性能优化
- 文档完善
- 示例代码

---

## 类设计规范

### 命名规范
- 所有封装类使用 `PascalCase`
- 私有成员使用 `m_` 前缀
- 静态成员使用 `s_` 前缀
- 常量使用 `k_` 前缀

### 接口设计
每个封装类应包含：
```cpp
class ExampleWrapper {
public:
    // 构造和析构
    ExampleWrapper();
    ~ExampleWrapper();

    // 初始化和清理
    bool initialize(const Config& config);
    void cleanup();

    // 核心功能
    bool update(float deltaTime);

    // 配置和查询
    void setParameter(const Param& param);
    Param getParameter() const;

    // 错误处理
    bool hasError() const;
    std::string getLastError() const;

private:
    // PhysX原生对象
    PxFoundation* m_foundation;
    PxPhysics* m_physics;
    PxScene* m_scene;

    // 内部状态
    std::string m_lastError;
    bool m_initialized;
};
```

---

## 文档要求

每个封装类需要提供：
1. **README.md** - 功能说明、使用示例
2. **API文档** - Doxygen格式注释
3. **示例代码** - examples/ 目录下

---

## 测试要求

每个封装类需要：
1. **单元测试** - Google Test
2. **集成测试** - 与其他模块交互
3. **性能测试** - 基准测试
4. **内存测试** - Valgrind/AddressSanitizer

---

## 当前进度

- [ ] 项目框架搭建
- [ ] CMake配置文件
- [ ] 第一个示例类 (PhysXCore)

---

## 注意事项

1. **保持简洁**：封装应该简化使用，不要过度设计
2. **保留灵活性**：提供访问原生PhysX对象的接口
3. **错误处理**：所有API都应该有完善的错误处理
4. **线程安全**：明确标注哪些接口是线程安全的
5. **资源管理**：使用RAII模式管理PhysX资源
6. **版本兼容**：明确支持的PhysX版本

---

**文档版本**: 1.0
**创建日期**: 2025-11-05
**最后更新**: 2025-11-05
