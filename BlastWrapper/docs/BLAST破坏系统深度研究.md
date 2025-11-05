# NVIDIA Blast 破坏系统深度研究

## 1. 项目概述

### 1.1 基本信息

**NVIDIA Blast** 是 NVIDIA 开发的高性能破坏物理系统，专门用于游戏和实时应用中的大规模破坏效果。Blast 提供了一个灵活的、可定制的破坏物理框架。

- **开发者**: NVIDIA Corporation
- **版权年份**: 2016-2024
- **许可证**: BSD 3-Clause
- **主要用途**: 实时破坏、断裂、碎片模拟
- **官方文档**: [Blast SDK Documentation](https://nvidia-omniverse.github.io/PhysX/blast/index.html)

### 1.2 代码规模统计

```
总文件数量:        359 个文件
目录数量:          68 个目录
源文件数量:        251 个 (.h/.cpp)
  - 头文件:        165 个
  - 源文件:        86 个
公共API头文件:     59 个
核心API代码行:     ~3,355 行
```

### 1.3 与 PhysX 的关系

**重要发现**: Blast 曾经依赖 PhysX，但现在已经基本独立。

证据来自 `blast/source/sdk/premake5.lua`:
```lua
-- 以下依赖已被注释掉
-- links { "PhysXFoundation_64", "PhysXTask_static_64" }
```

**现状**:
- Blast 有自己独立的 Foundation 层（NsFoundation）
- Blast 有自己的任务管理系统（NvTask）
- Blast 可以独立编译和使用
- 用户可以选择将 Blast 与任何物理引擎（包括 PhysX）集成

---

## 2. 核心架构

Blast 采用分层架构设计，从低级到高级分为三个主要层次：

### 2.1 架构层次

```
┌─────────────────────────────────────────────┐
│   Extensions 扩展层                          │
│   - Serialization (序列化)                   │
│   - TkSerializers (工具包序列化器)           │
│   - Authoring (创作工具)                     │
│   - Stress Solver (压力求解器)               │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│   Toolkit (Tk) 高级工具包                    │
│   - Actor 管理                               │
│   - Family 管理                              │
│   - Joint 连接                               │
│   - Group 分组                               │
│   - Framework 框架                           │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│   LowLevel 核心层                            │
│   - Asset 资产                               │
│   - Family 家族                              │
│   - Actor 演员                               │
│   - Fracture 破坏                            │
│   - Damage 伤害                              │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│   Shared 共享层                              │
│   - NsFoundation (基础库)                    │
│   - NvTask (任务系统)                        │
│   - StressSolver (压力求解)                  │
└─────────────────────────────────────────────┘
```

---

## 3. 核心模块详解

### 3.1 LowLevel 核心层

#### 3.1.1 主要头文件
- `NvBlast.h` - 核心 API（42,600 行）
- `NvBlastTypes.h` - 基础类型定义（21,138 行）

#### 3.1.2 核心数据结构

##### **NvBlastAsset - 破坏资产**
```cpp
struct NvBlastAsset {
    // 不透明结构体，通过 API 函数访问
};

// 主要API函数：
NvBlastAsset* NvBlastCreateAsset(void* mem, const NvBlastAssetDesc* desc,
                                  void* scratch, NvBlastLog logFn);
uint32_t NvBlastAssetGetChunkCount(const NvBlastAsset* asset, NvBlastLog logFn);
uint32_t NvBlastAssetGetBondCount(const NvBlastAsset* asset, NvBlastLog logFn);
```

**Asset 包含**:
- **Chunks（块）**: 可破坏的层次结构单元
- **Bonds（键）**: 块之间的连接关系
- **Support Graph（支持图）**: 用于破坏计算的图结构

##### **NvBlastChunk - 块结构**
```cpp
struct NvBlastChunk {
    float        centroid[3];           // 质心位置
    uint32_t     userData;              // 用户数据
    float        volume;                // 体积
    uint32_t     parentChunkIndex;      // 父块索引
    uint32_t     firstChildIndex;       // 第一个子块索引
    uint32_t     childIndexStop;        // 子块索引结束
};
```

**Chunk 特性**:
- 层次化结构（父子关系）
- Support Chunks（支撑块）：承受物理力的块
- Subsupport Chunks（子支撑块）：纯视觉块

##### **NvBlastBond - 键连接**
```cpp
struct NvBlastBond {
    float        normal[3];             // 键法线
    float        area;                  // 键面积
    float        centroid[3];           // 键质心
    uint32_t     userData;              // 用户数据
};
```

**Bond 作用**:
- 定义块之间的连接强度
- 伤害计算的基础
- 断裂传播路径

##### **NvBlastFamily - 实例家族**
```cpp
struct NvBlastFamily {
    // 不透明结构体
};

// 从 Asset 创建 Family
NvBlastFamily* NvBlastAssetCreateFamily(void* mem, const NvBlastAsset* asset,
                                        NvBlastLog logFn);
```

**Family 特性**:
- 一个 Asset 可以创建多个 Family
- Family 包含多个 Actor（演员）
- 共享伤害状态（bond health）

##### **NvBlastActor - 演员**
```cpp
struct NvBlastActor {
    // 不透明结构体
};

// 创建第一个Actor
NvBlastActor* NvBlastFamilyCreateFirstActor(NvBlastFamily* family,
                                            const NvBlastActorDesc* desc,
                                            void* scratch, NvBlastLog logFn);
```

**Actor 特性**:
- 代表一个独立的可破坏物体
- 包含可见块（visible chunks）
- 可以分裂成多个子 Actor

#### 3.1.3 破坏流程

```
1. 创建 Asset
   NvBlastCreateAsset()
        ↓
2. 创建 Family (Asset实例)
   NvBlastAssetCreateFamily()
        ↓
3. 创建初始 Actor
   NvBlastFamilyCreateFirstActor()
        ↓
4. 施加伤害
   NvBlastActorGenerateFracture()  // 生成破坏命令
   NvBlastActorApplyFracture()      // 应用伤害
        ↓
5. 分裂检测
   NvBlastActorIsSplitRequired()    // 检查是否需要分裂
        ↓
6. 执行分裂
   NvBlastActorSplit()              // 分裂成多个Actor
        ↓
7. 清理
   NvBlastActorDeactivate()         // 停用Actor
```

---

### 3.2 Toolkit (Tk) 高级层

Toolkit 提供了更高级的 C++ 封装和额外功能。

#### 3.2.1 主要组件

##### **TkFramework - 框架管理**
- `NvBlastTkFramework.h` (12,383 行)
- 管理所有高级对象的生命周期
- 提供内存管理和序列化支持

##### **TkActor - 高级演员**
- `NvBlastTkActor.h` (10,038 行)
- C++ 封装的 Actor
- 支持事件监听器
- 集成物理引擎接口

##### **TkAsset - 高级资产**
- `NvBlastTkAsset.h` (5,579 行)
- C++ Asset 封装
- 提供更友好的 API

##### **TkFamily - 高级家族**
- `NvBlastTkFamily.h` (5,137 行)
- 管理 Actor 集合
- 处理分裂事件

##### **TkJoint - 连接**
- `NvBlastTkJoint.h` (3,284 行)
- Actor 之间的约束连接
- 支持破坏时的连接断开

##### **TkGroup - 分组**
- `NvBlastTkGroup.h` (8,081 行)
- 批量处理多个 Actor
- 并行化破坏计算

---

### 3.3 Extensions 扩展层

#### 3.3.1 Serialization - 序列化
- 资产和 Actor 的保存/加载
- 支持版本控制
- 二进制格式

#### 3.3.2 Authoring - 创作工具
- 程序化生成破坏网格
- Voronoi 碎片生成
- 破坏模式设计

#### 3.3.3 Stress Solver - 压力求解器
- 基于物理的破坏预测
- 压力传播模拟
- 实时断裂计算

**核心文件**:
- `blast/source/shared/stress_solver/stress.h`
- `blast/source/shared/stress_solver/stress.cpp`
- `blast/source/shared/stress_solver/bond.h`
- `blast/source/shared/stress_solver/coupling.h`
- `blast/source/shared/stress_solver/inertia.h`

**数学库**:
- `blast/source/shared/stress_solver/math/cgnr.h` - 共轭梯度法
- `blast/source/shared/stress_solver/simd/simd.h` - SIMD 优化

---

### 3.4 Shared 共享层

#### 3.4.1 NsFoundation - 基础库

Blast 自己的基础库，类似 PhysX Foundation：

**主要组件**:
- `NsArray.h` - 动态数组
- `NsHashMap.h` - 哈希表
- `NsHashSet.h` - 哈希集合
- `NsAlignedMalloc.h` - 对齐内存分配
- `NsFPU.h` - 浮点单元控制
- `NsVecMath*.h` - 向量数学库（AoS/SoA）

#### 3.4.2 NvTask - 任务系统

Blast 的多线程任务调度：

**主要头文件**:
- `NvTaskManager.h` - 任务管理器
- `NvTask.h` - 任务基类
- `NvCpuDispatcher.h` - CPU调度器
- `NvGpuDispatcher.h` - GPU调度器（预留）

---

## 4. 破坏算法原理

### 4.1 Support Graph（支持图）

Blast 使用图结构来表示块之间的连接关系：

```
Nodes (节点):
  - 每个 Support Chunk 对应一个节点
  - 存储块的健康值

Edges (边):
  - 对应 Bonds
  - 存储连接强度（健康值）
  - 记录连接面积和法线
```

### 4.2 Damage Application（伤害应用）

#### 伤害类型
1. **程序化伤害 (Programmatic Damage)**
   - 直接指定哪些 Bonds 受损
   - 精确控制破坏效果

2. **材质伤害 (Material Damage)**
   - 使用伤害程序（Damage Program）
   - 基于材质属性计算伤害

#### 伤害传播
```cpp
void NvBlastActorApplyFracture(
    NvBlastFractureBuffers* eventBuffers,
    NvBlastActor* actor,
    const NvBlastFractureBuffers* commands,
    NvBlastLog logFn,
    NvBlastTimers* timers
);
```

**流程**:
1. 计算每个 Bond 的伤害值
2. 减少 Bond 健康值
3. 当 Bond 健康值 ≤ 0 时，Bond 断裂
4. 递归检查子 Chunk 是否失去支撑
5. 生成破坏事件

### 4.3 Island Detection（岛检测）

当 Bonds 断裂时，Blast 使用图算法检测独立的连通分量（islands）：

```
原始 Actor:
  [========A========]
        ↓ (Bond断裂)
  [===B===]  [===C===]
  (Actor B) (Actor C)
```

**算法**:
- 深度优先搜索（DFS）或广度优先搜索（BFS）
- 识别不连通的子图
- 为每个岛创建新的 Actor

### 4.4 Visibility Update（可见性更新）

```cpp
uint32_t NvBlastActorGetVisibleChunkCount(const NvBlastActor* actor,
                                          NvBlastLog logFn);
```

**规则**:
- 叶子块（Leaf Chunks）总是可见（如果其父块破坏）
- 内部块：只有当所有子块都不存在时才可见
- 根块：只在完全破坏时可见

---

## 5. 性能优化特性

### 5.1 内存管理

**用户控制的内存分配**:
```cpp
// 1. 查询所需内存大小
size_t assetSize = NvBlastGetAssetMemorySize(desc, logFn);

// 2. 用户分配内存（16字节对齐）
void* mem = alignedMalloc(assetSize, 16);

// 3. 原地构造
NvBlastAsset* asset = NvBlastCreateAsset(mem, desc, scratch, logFn);
```

**优点**:
- 零内部分配
- 可预测的内存使用
- 适合自定义内存管理器

### 5.2 Scratch Memory（临时内存）

Blast 需要临时内存进行计算：

```cpp
size_t scratchSize = NvBlastGetRequiredScratchForCreateAsset(desc, logFn);
void* scratch = malloc(scratchSize);
NvBlastCreateAsset(mem, desc, scratch, logFn);
free(scratch);  // 计算完成后可立即释放
```

### 5.3 并行化

#### Task-based Parallelism
- 使用 NvTask 任务系统
- 支持 CPU 多线程
- Group 可批量处理多个 Actor

#### SIMD 优化
- 向量数学使用 SIMD 指令
- AoS (Array of Structures) 和 SoA (Structure of Arrays) 布局

---

## 6. 与物理引擎集成

### 6.1 Blast 的职责

Blast **只负责**:
- ✅ 破坏拓扑管理（哪些块连接，哪些断裂）
- ✅ 伤害计算和传播
- ✅ 岛检测和分裂
- ✅ 可见块管理

Blast **不负责**:
- ❌ 刚体物理模拟（需要外部物理引擎）
- ❌ 碰撞检测
- ❌ 图形渲染
- ❌ 音效

### 6.2 集成模式

#### 典型集成流程
```cpp
// 1. Blast 资产创建
NvBlastAsset* blastAsset = NvBlastCreateAsset(...);
NvBlastFamily* family = NvBlastAssetCreateFamily(...);
NvBlastActor* blastActor = NvBlastFamilyCreateFirstActor(...);

// 2. 物理引擎刚体创建
PxRigidDynamic* physxActor = physics->createRigidDynamic(...);

// 3. 关联
blastActorToPhysxActor[blastActor] = physxActor;

// 4. 碰撞时应用伤害
void onCollision(PxRigidDynamic* actor, PxVec3 impactPoint, float force) {
    NvBlastActor* blastActor = physxActorToBlastActor[actor];

    // 生成伤害
    NvBlastFractureBuffers commands;
    // ... 填充伤害数据

    // 应用破坏
    NvBlastActorApplyFracture(&events, blastActor, &commands, logFn, nullptr);

    // 检查分裂
    if (NvBlastActorIsSplitRequired(blastActor, logFn)) {
        NvBlastActorSplitEvent splitEvent;
        uint32_t count = NvBlastActorSplit(&splitEvent, blastActor,
                                          maxActors, scratch, logFn, nullptr);

        // 为每个新的 Blast Actor 创建物理刚体
        for (uint32_t i = 0; i < count; i++) {
            NvBlastActor* newBlastActor = splitEvent.newActors[i];
            PxRigidDynamic* newPhysxActor = createPhysicsActor(newBlastActor);
            blastActorToPhysxActor[newBlastActor] = newPhysxActor;
        }
    }
}
```

---

## 7. 关键 API 总结

### 7.1 Asset 管理

| API 函数 | 功能 |
|---------|------|
| `NvBlastGetAssetMemorySize()` | 查询 Asset 所需内存 |
| `NvBlastCreateAsset()` | 创建 Asset |
| `NvBlastAssetGetChunkCount()` | 获取块数量 |
| `NvBlastAssetGetBondCount()` | 获取键数量 |
| `NvBlastAssetGetSupportGraph()` | 获取支持图 |

### 7.2 Family 管理

| API 函数 | 功能 |
|---------|------|
| `NvBlastAssetGetFamilyMemorySize()` | 查询 Family 所需内存 |
| `NvBlastAssetCreateFamily()` | 从 Asset 创建 Family |
| `NvBlastFamilyGetActorCount()` | 获取 Actor 数量 |
| `NvBlastFamilyGetActors()` | 获取所有 Actor |

### 7.3 Actor 操作

| API 函数 | 功能 |
|---------|------|
| `NvBlastFamilyCreateFirstActor()` | 创建初始 Actor |
| `NvBlastActorGetVisibleChunkCount()` | 获取可见块数量 |
| `NvBlastActorGetBondHealths()` | 获取键健康值数组 |
| `NvBlastActorDeactivate()` | 停用 Actor |

### 7.4 破坏操作

| API 函数 | 功能 |
|---------|------|
| `NvBlastActorGenerateFracture()` | 生成破坏命令 |
| `NvBlastActorApplyFracture()` | 应用伤害 |
| `NvBlastActorCanFracture()` | 检查是否可破坏 |
| `NvBlastActorIsSplitRequired()` | 检查是否需要分裂 |
| `NvBlastActorSplit()` | 执行分裂 |

---

## 8. 使用场景

### 8.1 适用场景

✅ **大规模建筑破坏**
- 楼房倒塌
- 桥梁断裂
- 墙壁破坏

✅ **可预测的破坏模式**
- Voronoi 碎片
- 层次化破坏
- 艺术指导的破坏

✅ **性能关键应用**
- 大量可破坏物体
- 实时游戏
- VR/AR 应用

### 8.2 不适用场景

❌ **完全程序化破坏**
- 任意形状的实时切割
- 无法预测的破坏模式

❌ **软体破坏**
- 布料撕裂
- 软体变形

---

## 9. 与其他技术对比

### 9.1 Blast vs Havok Destruction

| 特性 | Blast | Havok Destruction |
|------|-------|-------------------|
| 开源 | ✅ 是 | ❌ 否 |
| 独立性 | ✅ 独立 | ⚠️ 与 Havok Physics 绑定 |
| 内存控制 | ✅ 用户控制 | ⚠️ 内部管理 |
| 性能 | ⚡ 非常高 | ⚡ 高 |

### 9.2 Blast vs PhysX Destruction (Legacy)

**历史**: PhysX 曾有 PhysX Destruction（APEX Destruction），现已废弃。Blast 是其精神继承者，但设计更简洁、性能更好。

| 特性 | Blast | PhysX Destruction |
|------|-------|-------------------|
| 状态 | ✅ 活跃维护 | ❌ 已废弃 |
| 架构 | ✅ 模块化 | ⚠️ 紧耦合 PhysX |
| API | ✅ 简洁 | ⚠️ 复杂 |
| 性能 | ⚡ 更快 | ⚡ 快 |

---

## 10. 总结

### 10.1 核心优势

1. **性能极佳**
   - 零内部分配
   - SIMD 优化
   - 多线程支持

2. **灵活集成**
   - 独立于物理引擎
   - 可与任何引擎配合
   - 用户控制内存

3. **可预测性**
   - 确定性算法
   - 艺术家友好
   - 层次化破坏

### 10.2 架构亮点

- **分层设计**: LowLevel → Toolkit → Extensions
- **图算法**: 高效的支持图和岛检测
- **事件驱动**: 清晰的破坏事件回调
- **内存友好**: 用户控制的内存布局

### 10.3 学习建议

1. **入门**: 从 LowLevel API 开始
2. **进阶**: 学习 Toolkit 封装
3. **高级**: 研究 Stress Solver 算法
4. **实践**: 集成到实际项目中

---

## 11. 参考资源

- 官方文档: https://nvidia-omniverse.github.io/PhysX/blast/index.html
- 源码位置: `/home/user/PhysX/blast/`
- 核心头文件:
  - `blast/include/lowlevel/NvBlast.h`
  - `blast/include/lowlevel/NvBlastTypes.h`
  - `blast/include/toolkit/NvBlastTkFramework.h`

---

**文档版本**: 1.0
**创建日期**: 2025-11-05
**作者**: Claude (AI 辅助研究)
