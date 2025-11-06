/**
 * PhysX Snippet: DeformableVolumeKinematic
 *
 * 演示可变形体积与运动学物体的交互
 *
 * 核心功能:
 * 1. 可变形体与运动学刚体的碰撞
 * 2. 运动学对象作为驱动器
 * 3. 单向驱动（运动学不受软体影响）
 * 4. 复杂运动路径交互
 *
 * 物理背景:
 *
 * 运动学物体（Kinematic Bodies）:
 * - 不受力和碰撞影响
 * - 由脚本/动画直接控制
 * - 单向影响动态物体
 * - 用于移动平台、门、电梯等
 *
 * 软体-运动学交互:
 * 运动学物体位置: p_k(t) = f_trajectory(t)
 * 软体受到约束: C = ||p_soft - p_k|| - d ≥ 0
 *
 * 当软体与运动学物体碰撞时:
 * 1. 检测碰撞: SDF查询或连续碰撞检测
 * 2. 计算穿透: δ = d - ||p_soft - p_k||
 * 3. 约束求解: 投影软体顶点到运动学表面
 * 4. 施加反作用力: F = -k·δ·n (但不影响运动学物体)
 *
 * 关键特性:
 * - **单向耦合**: 运动学驱动软体，软体不影响运动学
 * - **精确路径**: 运动学物体沿预定义轨迹移动
 * - **高效求解**: 运动学约束优先级高，求解简单
 * - **动画控制**: 适合脚本控制的交互场景
 *
 * 应用场景:
 * 1. 移动平台压缩软体
 * 2. 旋转刀片切割软体
 * 3. 电梯与软体乘客
 * 4. 传送带运输软体物体
 *
 * 实现要点:
 * - 使用PxRigidDynamic + setKinematicTarget()设置运动学目标
 * - 或使用PxRigidStatic + 手动更新位置
 * - 运动学物体不参与动力学求解
 * - 碰撞法线始终从运动学指向软体
 *
 * 注意:
 * ⚠️ 本示例需要GPU/CUDA支持才能运行FEM软体模拟
 * ⚠️ 运动学速度过快可能导致穿透（需要CCD）
 * ⚠️ 大变形可能需要重新网格化
 */

#include <PxPhysicsAPI.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace physx;

// ============================================================================
// 数据结构
// ============================================================================

/**
 * 运动学轨迹类型
 */
enum class TrajectoryType {
    Linear,          // 直线运动
    Circular,        // 圆周运动
    Oscillating,     // 振荡运动
    Custom           // 自定义路径
};

/**
 * 运动学物体配置
 */
struct KinematicConfig {
    TrajectoryType trajectoryType;
    PxVec3 startPos;
    PxVec3 endPos;              // 用于直线运动
    PxVec3 axis;                // 用于圆周/振荡运动
    PxReal radius;              // 用于圆周运动
    PxReal frequency;           // 用于振荡运动（Hz）
    PxReal speed;               // 运动速度

    KinematicConfig()
        : trajectoryType(TrajectoryType::Linear)
        , startPos(0, 0, 0)
        , endPos(0, 0, 0)
        , axis(0, 1, 0)
        , radius(1.0f)
        , frequency(1.0f)
        , speed(1.0f) {}
};

/**
 * 软体数据结构（简化版FEM）
 */
struct DeformableVolume {
    std::vector<PxVec3> vertices;       // 顶点位置
    std::vector<PxVec3> velocities;     // 顶点速度
    std::vector<PxU32> tetrahedra;      // 四面体索引（每4个索引一个四面体）
    PxReal stiffness;                   // 刚度
    PxReal damping;                     // 阻尼
    PxReal mass;                        // 总质量

    DeformableVolume() : stiffness(1000.0f), damping(0.1f), mass(1.0f) {}
};

/**
 * 运动学-软体交互对
 */
struct KinematicSoftBodyPair {
    PxRigidDynamic* kinematicActor;     // 运动学刚体
    DeformableVolume softBody;          // 软体
    KinematicConfig config;             // 运动配置
    PxReal currentTime;                 // 当前时间

    KinematicSoftBodyPair() : kinematicActor(nullptr), currentTime(0.0f) {}
};

// ============================================================================
// 几何生成工具
// ============================================================================

/**
 * 生成立方体软体网格
 */
DeformableVolume createCubeSoftBody(const PxVec3& center, PxReal size, PxU32 resolution) {
    DeformableVolume volume;

    // 生成网格顶点
    PxReal step = size / static_cast<PxReal>(resolution - 1);
    for (PxU32 z = 0; z < resolution; ++z) {
        for (PxU32 y = 0; y < resolution; ++y) {
            for (PxU32 x = 0; x < resolution; ++x) {
                PxVec3 pos = center + PxVec3(
                    (x - (resolution-1) * 0.5f) * step,
                    (y - (resolution-1) * 0.5f) * step,
                    (z - (resolution-1) * 0.5f) * step
                );
                volume.vertices.push_back(pos);
                volume.velocities.push_back(PxVec3(0, 0, 0));
            }
        }
    }

    // 生成四面体（简化：每个小立方体分成5个四面体）
    auto getIndex = [resolution](PxU32 x, PxU32 y, PxU32 z) -> PxU32 {
        return x + y * resolution + z * resolution * resolution;
    };

    for (PxU32 z = 0; z < resolution - 1; ++z) {
        for (PxU32 y = 0; y < resolution - 1; ++y) {
            for (PxU32 x = 0; x < resolution - 1; ++x) {
                // 立方体8个顶点
                PxU32 i000 = getIndex(x,   y,   z);
                PxU32 i100 = getIndex(x+1, y,   z);
                PxU32 i010 = getIndex(x,   y+1, z);
                PxU32 i110 = getIndex(x+1, y+1, z);
                PxU32 i001 = getIndex(x,   y,   z+1);
                PxU32 i101 = getIndex(x+1, y,   z+1);
                PxU32 i011 = getIndex(x,   y+1, z+1);
                PxU32 i111 = getIndex(x+1, y+1, z+1);

                // 分成5个四面体
                volume.tetrahedra.push_back(i000); volume.tetrahedra.push_back(i100);
                volume.tetrahedra.push_back(i010); volume.tetrahedra.push_back(i001);

                volume.tetrahedra.push_back(i100); volume.tetrahedra.push_back(i110);
                volume.tetrahedra.push_back(i010); volume.tetrahedra.push_back(i111);

                volume.tetrahedra.push_back(i100); volume.tetrahedra.push_back(i101);
                volume.tetrahedra.push_back(i001); volume.tetrahedra.push_back(i111);

                volume.tetrahedra.push_back(i010); volume.tetrahedra.push_back(i011);
                volume.tetrahedra.push_back(i001); volume.tetrahedra.push_back(i111);

                volume.tetrahedra.push_back(i100); volume.tetrahedra.push_back(i010);
                volume.tetrahedra.push_back(i001); volume.tetrahedra.push_back(i111);
            }
        }
    }

    return volume;
}

// ============================================================================
// 运动学轨迹计算
// ============================================================================

/**
 * 计算给定时间的运动学物体位置
 */
PxTransform computeKinematicTransform(const KinematicConfig& config, PxReal time) {
    PxTransform transform(PxIdentity);

    switch (config.trajectoryType) {
        case TrajectoryType::Linear: {
            // 直线插值
            PxReal t = PxClamp(time * config.speed, 0.0f, 1.0f);
            transform.p = config.startPos + (config.endPos - config.startPos) * t;
            break;
        }

        case TrajectoryType::Circular: {
            // 圆周运动
            PxReal angle = time * config.speed;
            PxVec3 perpendicular1, perpendicular2;

            // 构建垂直于轴的正交基
            if (PxAbs(config.axis.x) < 0.9f) {
                perpendicular1 = config.axis.cross(PxVec3(1, 0, 0)).getNormalized();
            } else {
                perpendicular1 = config.axis.cross(PxVec3(0, 1, 0)).getNormalized();
            }
            perpendicular2 = config.axis.cross(perpendicular1);

            PxVec3 offset = (perpendicular1 * PxCos(angle) + perpendicular2 * PxSin(angle)) * config.radius;
            transform.p = config.startPos + offset;
            transform.q = PxQuat(angle, config.axis);
            break;
        }

        case TrajectoryType::Oscillating: {
            // 简谐振荡
            PxReal omega = 2.0f * PxPi * config.frequency;
            PxReal displacement = PxSin(omega * time) * config.radius;
            transform.p = config.startPos + config.axis * displacement;
            break;
        }

        case TrajectoryType::Custom:
            // 自定义轨迹（此处简化为静止）
            transform.p = config.startPos;
            break;
    }

    return transform;
}

// ============================================================================
// 碰撞检测和响应
// ============================================================================

/**
 * 检测点与盒体的碰撞
 */
bool detectPointBoxCollision(const PxVec3& point, const PxTransform& boxTransform,
                              const PxVec3& boxHalfExtents, PxVec3& outNormal, PxReal& outDepth) {
    // 将点转换到盒体局部空间
    PxVec3 localPoint = boxTransform.transformInv(point);

    // 计算最近点（在盒体表面）
    PxVec3 closestPoint = PxVec3(
        PxClamp(localPoint.x, -boxHalfExtents.x, boxHalfExtents.x),
        PxClamp(localPoint.y, -boxHalfExtents.y, boxHalfExtents.y),
        PxClamp(localPoint.z, -boxHalfExtents.z, boxHalfExtents.z)
    );

    PxVec3 delta = localPoint - closestPoint;
    PxReal distSq = delta.magnitudeSquared();

    // 如果点在盒体内部
    if (distSq < 1e-6f) {
        // 计算到各面的距离
        PxReal dists[6] = {
            boxHalfExtents.x + localPoint.x,  // -X face
            boxHalfExtents.x - localPoint.x,  // +X face
            boxHalfExtents.y + localPoint.y,  // -Y face
            boxHalfExtents.y - localPoint.y,  // +Y face
            boxHalfExtents.z + localPoint.z,  // -Z face
            boxHalfExtents.z - localPoint.z   // +Z face
        };

        PxVec3 normals[6] = {
            PxVec3(-1, 0, 0), PxVec3(1, 0, 0),
            PxVec3(0, -1, 0), PxVec3(0, 1, 0),
            PxVec3(0, 0, -1), PxVec3(0, 0, 1)
        };

        // 找到最近的面
        PxU32 minIdx = 0;
        for (PxU32 i = 1; i < 6; ++i) {
            if (dists[i] < dists[minIdx]) {
                minIdx = i;
            }
        }

        outDepth = dists[minIdx];
        outNormal = boxTransform.rotate(normals[minIdx]);
        return true;
    }

    return false;
}

/**
 * 应用碰撞响应到软体顶点
 */
void applyKinematicCollision(DeformableVolume& volume, const PxTransform& kinematicTransform,
                              const PxVec3& boxHalfExtents, PxReal dt) {
    for (size_t i = 0; i < volume.vertices.size(); ++i) {
        PxVec3 normal;
        PxReal depth;

        if (detectPointBoxCollision(volume.vertices[i], kinematicTransform, boxHalfExtents, normal, depth)) {
            // 位置修正（投影到表面）
            volume.vertices[i] += normal * depth;

            // 速度修正（移除法向分量 + 摩擦）
            PxVec3& vel = volume.velocities[i];
            PxReal normalVel = vel.dot(normal);

            if (normalVel < 0.0f) {
                // 移除法向速度（完全非弹性碰撞）
                vel -= normal * normalVel;

                // 应用摩擦
                PxReal friction = 0.5f;
                vel *= (1.0f - friction * dt * 60.0f);  // 近似摩擦
            }
        }
    }
}

// ============================================================================
// 软体模拟
// ============================================================================

/**
 * 简化的FEM软体模拟步骤
 */
void simulateSoftBody(DeformableVolume& volume, PxReal dt) {
    // 1. 施加重力
    PxVec3 gravity(0, -9.81f, 0);
    for (auto& vel : volume.velocities) {
        vel += gravity * dt;
    }

    // 2. 弹性力（简化：只考虑边的拉伸）
    for (size_t i = 0; i < volume.tetrahedra.size(); i += 4) {
        PxU32 idx0 = volume.tetrahedra[i];
        PxU32 idx1 = volume.tetrahedra[i+1];
        PxU32 idx2 = volume.tetrahedra[i+2];
        PxU32 idx3 = volume.tetrahedra[i+3];

        // 四面体6条边
        PxU32 edges[6][2] = {{idx0,idx1}, {idx0,idx2}, {idx0,idx3}, {idx1,idx2}, {idx1,idx3}, {idx2,idx3}};

        for (const auto& edge : edges) {
            PxVec3& p0 = volume.vertices[edge[0]];
            PxVec3& p1 = volume.vertices[edge[1]];
            PxVec3& v0 = volume.velocities[edge[0]];
            PxVec3& v1 = volume.velocities[edge[1]];

            PxVec3 delta = p1 - p0;
            PxReal currentLength = delta.magnitude();
            PxReal restLength = 0.5f;  // 假设的静止长度

            if (currentLength > 1e-6f) {
                PxVec3 dir = delta / currentLength;
                PxReal strain = (currentLength - restLength) / restLength;

                // 弹性力: F = k * strain
                PxVec3 force = dir * (volume.stiffness * strain);

                // 阻尼力: F_d = -d * v_rel
                PxVec3 relVel = v1 - v0;
                PxVec3 dampingForce = relVel * (-volume.damping);

                PxVec3 totalForce = force + dampingForce;

                // 应用牛顿第三定律
                PxReal vertexMass = volume.mass / volume.vertices.size();
                v0 += totalForce * (dt / vertexMass);
                v1 -= totalForce * (dt / vertexMass);
            }
        }
    }

    // 3. 更新位置（显式欧拉积分）
    for (size_t i = 0; i < volume.vertices.size(); ++i) {
        volume.vertices[i] += volume.velocities[i] * dt;
    }
}

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 移动平台压缩软体
 */
KinematicSoftBodyPair createMovingPlatformScene(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景1: 移动平台压缩软体 ===" << std::endl;
    std::cout << "运动学平台从上方下降，压缩下方的软立方体" << std::endl;

    KinematicSoftBodyPair pair;

    // 创建运动学平台（盒体）
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.1f);
    PxShape* shape = physics->createShape(PxBoxGeometry(2.0f, 0.2f, 2.0f), *material);

    pair.kinematicActor = physics->createRigidDynamic(PxTransform(PxVec3(0, 5, 0)));
    pair.kinematicActor->attachShape(*shape);
    pair.kinematicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    scene->addActor(*pair.kinematicActor);

    shape->release();
    material->release();

    // 配置直线下降轨迹
    pair.config.trajectoryType = TrajectoryType::Linear;
    pair.config.startPos = PxVec3(0, 5, 0);
    pair.config.endPos = PxVec3(0, 1, 0);
    pair.config.speed = 0.5f;  // 0.5单位/秒

    // 创建软立方体
    pair.softBody = createCubeSoftBody(PxVec3(0, 0, 0), 1.5f, 4);
    pair.softBody.stiffness = 2000.0f;
    pair.softBody.damping = 0.2f;

    std::cout << "平台将在2秒内从y=5下降到y=1" << std::endl;
    std::cout << "软体: " << pair.softBody.vertices.size() << "个顶点, "
              << pair.softBody.tetrahedra.size()/4 << "个四面体" << std::endl;

    return pair;
}

/**
 * 场景2: 旋转刀片
 */
KinematicSoftBodyPair createRotatingBladeScene(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景2: 旋转刀片切割软体 ===" << std::endl;
    std::cout << "薄盒体沿圆周路径旋转，切割中心的软立方体" << std::endl;

    KinematicSoftBodyPair pair;

    // 创建旋转刀片（薄盒体）
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.1f);
    PxShape* shape = physics->createShape(PxBoxGeometry(2.0f, 0.1f, 0.1f), *material);

    pair.kinematicActor = physics->createRigidDynamic(PxTransform(PxVec3(2, 0, 0)));
    pair.kinematicActor->attachShape(*shape);
    pair.kinematicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    scene->addActor(*pair.kinematicActor);

    shape->release();
    material->release();

    // 配置圆周运动
    pair.config.trajectoryType = TrajectoryType::Circular;
    pair.config.startPos = PxVec3(0, 0, 0);  // 圆心
    pair.config.axis = PxVec3(0, 1, 0);      // Y轴旋转
    pair.config.radius = 2.0f;
    pair.config.speed = 2.0f;  // 2 rad/s

    // 创建软立方体
    pair.softBody = createCubeSoftBody(PxVec3(0, 0, 0), 1.0f, 3);
    pair.softBody.stiffness = 1500.0f;
    pair.softBody.damping = 0.15f;

    std::cout << "刀片沿半径2.0的圆周旋转，角速度2.0 rad/s" << std::endl;
    std::cout << "周期: " << 2*PxPi/2.0f << "秒" << std::endl;

    return pair;
}

/**
 * 场景3: 振荡锤
 */
KinematicSoftBodyPair createOscillatingHammerScene(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景3: 振荡锤敲击软体 ===" << std::endl;
    std::cout << "盒体沿Y轴简谐振荡，周期性敲击软体" << std::endl;

    KinematicSoftBodyPair pair;

    // 创建振荡锤
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.3f);
    PxShape* shape = physics->createShape(PxBoxGeometry(1.0f, 0.3f, 1.0f), *material);

    pair.kinematicActor = physics->createRigidDynamic(PxTransform(PxVec3(0, 3, 0)));
    pair.kinematicActor->attachShape(*shape);
    pair.kinematicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    scene->addActor(*pair.kinematicActor);

    shape->release();
    material->release();

    // 配置简谐振荡
    pair.config.trajectoryType = TrajectoryType::Oscillating;
    pair.config.startPos = PxVec3(0, 3, 0);
    pair.config.axis = PxVec3(0, -1, 0);     // 向下振荡
    pair.config.frequency = 1.0f;            // 1 Hz
    pair.config.radius = 2.0f;               // 振幅2.0

    // 创建软立方体
    pair.softBody = createCubeSoftBody(PxVec3(0, 0, 0), 1.2f, 4);
    pair.softBody.stiffness = 1800.0f;
    pair.softBody.damping = 0.25f;

    std::cout << "振荡频率: 1.0 Hz" << std::endl;
    std::cout << "振幅: 2.0单位（y范围: [1, 5]）" << std::endl;
    std::cout << "最低点y=1时接触软体顶部" << std::endl;

    return pair;
}

// ============================================================================
// 主函数
// ============================================================================

void updateKinematicSoftBodyPair(KinematicSoftBodyPair& pair, PxReal dt) {
    pair.currentTime += dt;

    // 更新运动学物体位置
    PxTransform newTransform = computeKinematicTransform(pair.config, pair.currentTime);
    pair.kinematicActor->setKinematicTarget(newTransform);

    // 模拟软体
    simulateSoftBody(pair.softBody, dt);

    // 检测并响应碰撞
    PxShape* shape;
    pair.kinematicActor->getShapes(&shape, 1);
    PxBoxGeometry boxGeom;
    shape->getBoxGeometry(boxGeom);

    applyKinematicCollision(pair.softBody, newTransform, boxGeom.halfExtents, dt);
}

int main() {
    std::cout << "PhysX Snippet: DeformableVolumeKinematic" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "\n演示可变形体积与运动学物体的交互" << std::endl;

    // 初始化PhysX
    PxDefaultAllocator allocator;
    PxDefaultErrorCallback errorCallback;

    PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
    if (!foundation) {
        std::cerr << "PxCreateFoundation failed!" << std::endl;
        return 1;
    }

    PxPhysics* physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale());
    if (!physics) {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return 1;
    }

    // 创建场景
    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    PxScene* scene = physics->createScene(sceneDesc);

    // 创建3个场景
    std::vector<KinematicSoftBodyPair> scenes;
    scenes.push_back(createMovingPlatformScene(physics, scene));
    scenes.push_back(createRotatingBladeScene(physics, scene));
    scenes.push_back(createOscillatingHammerScene(physics, scene));

    // 运行模拟
    std::cout << "\n=== 开始模拟 ===" << std::endl;
    const PxReal dt = 1.0f / 60.0f;
    const PxU32 numSteps = 300;  // 5秒

    for (PxU32 step = 0; step < numSteps; ++step) {
        PxReal time = step * dt;

        // 更新所有场景
        for (auto& pair : scenes) {
            updateKinematicSoftBodyPair(pair, dt);
        }

        // 推进PhysX场景（运动学物体）
        scene->simulate(dt);
        scene->fetchResults(true);

        // 每60帧输出一次状态
        if (step % 60 == 0) {
            std::cout << "\n时间: " << time << "s" << std::endl;

            // 场景1状态
            PxTransform t1 = scenes[0].kinematicActor->getGlobalPose();
            std::cout << "场景1 - 平台Y位置: " << t1.p.y << std::endl;

            // 计算软体平均Y坐标（压缩程度）
            PxReal avgY = 0.0f;
            for (const auto& v : scenes[0].softBody.vertices) {
                avgY += v.y;
            }
            avgY /= scenes[0].softBody.vertices.size();
            std::cout << "        软体中心Y: " << avgY << std::endl;

            // 场景2状态
            PxTransform t2 = scenes[1].kinematicActor->getGlobalPose();
            PxReal angle = PxAtan2(t2.p.z, t2.p.x);
            std::cout << "场景2 - 刀片角度: " << angle * 180.0f / PxPi << "度" << std::endl;

            // 场景3状态
            PxTransform t3 = scenes[2].kinematicActor->getGlobalPose();
            std::cout << "场景3 - 锤Y位置: " << t3.p.y << std::endl;
        }
    }

    std::cout << "\n=== 模拟完成 ===" << std::endl;
    std::cout << "\n总结:" << std::endl;
    std::cout << "- 场景1展示了运动学平台如何单向驱动软体压缩" << std::endl;
    std::cout << "- 场景2展示了旋转运动学物体对软体的持续切割作用" << std::endl;
    std::cout << "- 场景3展示了周期性运动学冲击对软体的影响" << std::endl;
    std::cout << "- 运动学物体始终沿预定义轨迹运动，不受软体反作用力影响" << std::endl;

    // 清理
    scene->release();
    physics->release();
    foundation->release();

    return 0;
}
