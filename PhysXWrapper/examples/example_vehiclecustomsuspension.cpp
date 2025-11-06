/**
 * PhysX Snippet: VehicleCustomSuspension
 *
 * 演示自定义悬挂系统（Custom Suspension Systems）
 *
 * 核心功能:
 * 1. 多种悬挂类型
 * 2. 自适应阻尼
 * 3. 主动悬挂控制
 * 4. 气动悬挂
 *
 * 物理背景:
 *
 * 悬挂系统（Suspension System）:
 * 车辆悬挂系统连接车身和车轮，主要功能：
 * - 吸收路面冲击
 * - 保持轮胎接地
 * - 控制车身姿态
 * - 提供乘坐舒适性
 *
 * 悬挂力学模型:
 *
 * 基础弹簧-阻尼器模型:
 * F_suspension = -k × (x - x0) - c × ẋ
 * 其中:
 * - k: 弹簧刚度（N/m）
 * - c: 阻尼系数（N·s/m）
 * - x: 当前压缩量
 * - x0: 静态平衡位置
 * - ẋ: 压缩速度
 *
 * 阻尼比（Damping Ratio）:
 * ζ = c / (2 × √(k × m))
 * - ζ < 1: 欠阻尼（震荡）
 * - ζ = 1: 临界阻尼（最优）
 * - ζ > 1: 过阻尼（缓慢）
 *
 * 固有频率（Natural Frequency）:
 * ω_n = √(k / m)
 * 乘用车典型值: 1.0-1.5 Hz
 * 赛车典型值: 2.0-3.0 Hz
 *
 * 悬挂类型:
 *
 * 1. 被动悬挂（Passive Suspension）:
 *    - 固定刚度和阻尼
 *    - 简单可靠
 *    - 性能折衷
 *
 * 2. 半主动悬挂（Semi-Active Suspension）:
 *    - 可变阻尼（CDC, MRC）
 *    - 实时调整
 *    - 能耗低
 *    公式: c(t) = c_min + (c_max - c_min) × control(t)
 *
 * 3. 主动悬挂（Active Suspension）:
 *    - 电液执行器
 *    - 主动施力
 *    - 性能最优
 *    公式: F_active = F_spring + F_damper + F_control
 *
 * 4. 气动悬挂（Air Suspension）:
 *    - 压缩空气弹簧
 *    - 可调高度
 *    - 载荷自适应
 *    气压-力关系: F = A × P
 *    其中 A 是活塞面积，P 是气压
 *
 * 自适应控制策略:
 *
 * 天钩阻尼（Skyhook Damping）:
 * 目标：车身绝对速度阻尼
 * c_skyhook = {
 *     c_max,  if ż_body × (ż_body - ż_wheel) > 0
 *     c_min,  otherwise
 * }
 *
 * 地钩阻尼（Groundhook Damping）:
 * 目标：轮胎接地力最大化
 * c_groundhook = {
 *     c_max,  if ż_wheel × (ż_body - ż_wheel) > 0
 *     c_min,  otherwise
 * }
 *
 * 混合控制（Hybrid Control）:
 * c = α × c_skyhook + (1-α) × c_groundhook
 * α 根据驾驶模式调整（舒适 vs 运动）
 *
 * 主动防侧倾（Active Roll Control）:
 * T_roll = k_roll × φ + c_roll × φ̇
 * 其中 φ 是侧倾角
 *
 * 自适应高度调节（Adaptive Ride Height）:
 * h_target = h_normal - Δh_aero × v²
 * 高速时降低车身，减小空气阻力
 *
 * 性能指标:
 * 1. 车身加速度RMS（舒适性）
 * 2. 悬挂行程利用率
 * 3. 轮胎接地力变化
 * 4. 侧倾角和俯仰角
 *
 * 应用场景:
 * 1. 豪华车舒适性
 * 2. 赛车操控性
 * 3. 越野车适应性
 * 4. 商用车载荷管理
 *
 * 注意:
 * ⚠️ 主动悬挂能耗较高
 * ⚠️ 半主动系统需要传感器和控制器
 * ⚠️ 气动悬挂维护成本高
 */

#include <PxPhysicsAPI.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace physx;

// ============================================================================
// 数据结构
// ============================================================================

/**
 * 悬挂类型
 */
enum class SuspensionType {
    Passive,            // 被动悬挂
    SemiActiveSkyhook,  // 半主动（天钩）
    SemiActiveGround,   // 半主动（地钩）
    Active,             // 主动悬挂
    AirSuspension       // 气动悬挂
};

/**
 * 悬挂配置
 */
struct SuspensionConfig {
    SuspensionType type;

    // 基础参数
    PxReal springStiffness;      // 弹簧刚度（N/m）
    PxReal damperCoeffMin;       // 最小阻尼系数（N·s/m）
    PxReal damperCoeffMax;       // 最大阻尼系数（N·s/m）
    PxReal maxTravel;            // 最大行程（米）
    PxReal restLength;           // 静态长度（米）

    // 主动悬挂参数
    PxReal activeGain;           // 主动控制增益
    PxReal maxActiveForce;       // 最大主动力（N）

    // 气动悬挂参数
    PxReal airSpringArea;        // 气囊面积（m²）
    PxReal airPressureMin;       // 最小气压（Pa）
    PxReal airPressureMax;       // 最大气压（Pa）
    PxReal currentAirPressure;   // 当前气压（Pa）

    SuspensionConfig() {
        type = SuspensionType::Passive;
        springStiffness = 35000.0f;
        damperCoeffMin = 2000.0f;
        damperCoeffMax = 8000.0f;
        maxTravel = 0.3f;
        restLength = 0.4f;
        activeGain = 5000.0f;
        maxActiveForce = 3000.0f;
        airSpringArea = 0.05f;  // 500 cm²
        airPressureMin = 200000.0f;  // 2 bar
        airPressureMax = 800000.0f;  // 8 bar
        currentAirPressure = 400000.0f;  // 4 bar
    }
};

/**
 * 悬挂状态
 */
struct SuspensionState {
    PxReal compression;          // 当前压缩量
    PxReal compressionVel;       // 压缩速度
    PxReal bodyVelZ;             // 车身垂直速度
    PxReal wheelVelZ;            // 车轮垂直速度
    PxReal currentDamping;       // 当前阻尼系数
    PxReal currentForce;         // 当前悬挂力

    SuspensionState() : compression(0), compressionVel(0), bodyVelZ(0),
                        wheelVelZ(0), currentDamping(0), currentForce(0) {}
};

/**
 * 简化的车辆（用于测试悬挂）
 */
class VehicleWithCustomSuspension {
public:
    PxRigidDynamic* chassis;
    std::vector<SuspensionConfig> suspensions;  // 4个车轮的悬挂
    std::vector<SuspensionState> suspensionStates;
    std::vector<PxVec3> wheelPositions;  // 车轮相对位置

    PxReal chassisMass;
    PxVec3 chassisDims;

    VehicleWithCustomSuspension() : chassis(nullptr), chassisMass(1500.0f),
                                     chassisDims(4.5f, 2.0f, 1.5f) {}

    void create(PxPhysics* physics, PxScene* scene, const PxTransform& initialPose,
                SuspensionType suspensionType) {
        // 创建车身
        PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.1f);
        PxBoxGeometry geom(chassisDims * 0.5f);

        chassis = physics->createRigidDynamic(initialPose);
        PxShape* shape = physics->createShape(geom, *material);
        chassis->attachShape(*shape);

        PxRigidBodyExt::setMassAndUpdateInertia(*chassis, chassisMass);
        scene->addActor(*chassis);

        shape->release();
        material->release();

        // 初始化4个悬挂
        PxReal halfLength = chassisDims.x * 0.4f;
        PxReal halfWidth = chassisDims.y * 0.5f;
        PxReal wheelHeight = -chassisDims.z * 0.5f;

        wheelPositions.push_back(PxVec3(halfLength, -halfWidth, wheelHeight));   // FL
        wheelPositions.push_back(PxVec3(halfLength, halfWidth, wheelHeight));    // FR
        wheelPositions.push_back(PxVec3(-halfLength, -halfWidth, wheelHeight));  // RL
        wheelPositions.push_back(PxVec3(-halfLength, halfWidth, wheelHeight));   // RR

        for (int i = 0; i < 4; ++i) {
            SuspensionConfig config;
            config.type = suspensionType;
            suspensions.push_back(config);
            suspensionStates.push_back(SuspensionState());
        }
    }

    void update(PxReal dt) {
        if (!chassis) return;

        PxScene* scene;
        chassis->getScene(scene);

        for (size_t i = 0; i < 4; ++i) {
            updateSuspension(i, dt, scene);
        }
    }

    // 计算车身加速度RMS（舒适性指标）
    PxReal calculateBodyAccelerationRMS(int numSamples) const {
        // 简化：使用当前加速度
        PxVec3 accel = chassis->getLinearVelocity();  // 简化
        return accel.z * accel.z;
    }

    // 设置气动悬挂高度
    void setAirSuspensionHeight(PxReal targetHeight) {
        for (auto& susp : suspensions) {
            if (susp.type == SuspensionType::AirSuspension) {
                // 调整气压以达到目标高度
                PxReal heightDiff = targetHeight - susp.restLength;
                PxReal pressureDelta = (chassisMass * 9.81f / (4.0f * susp.airSpringArea)) * heightDiff;
                susp.currentAirPressure = PxClamp(
                    susp.currentAirPressure + pressureDelta,
                    susp.airPressureMin,
                    susp.airPressureMax
                );
            }
        }
    }

private:
    void updateSuspension(size_t wheelIdx, PxReal dt, PxScene* scene) {
        PxTransform chassisTransform = chassis->getGlobalPose();
        PxVec3 wheelWorldPos = chassisTransform.transform(wheelPositions[wheelIdx]);

        // 射线检测地面
        PxVec3 rayStart = wheelWorldPos + PxVec3(0, 0, 1.0f);
        PxVec3 rayDir(0, 0, -1);
        PxReal rayLength = suspensions[wheelIdx].maxTravel + 2.0f;

        PxRaycastBuffer hit;
        bool hasContact = scene->raycast(rayStart, rayDir, rayLength, hit);

        if (!hasContact) {
            suspensionStates[wheelIdx].currentForce = 0.0f;
            return;
        }

        // 计算压缩量
        PxReal compression = suspensions[wheelIdx].maxTravel + 1.0f - (hit.block.distance - 1.0f);
        compression = PxClamp(compression, 0.0f, suspensions[wheelIdx].maxTravel);

        // 计算速度
        PxVec3 chassisVelAtWheel = chassis->getLinearVelocity() +
                                    chassis->getAngularVelocity().cross(wheelPositions[wheelIdx]);

        PxReal compressionVel = chassisVelAtWheel.z;

        // 更新状态
        suspensionStates[wheelIdx].compression = compression;
        suspensionStates[wheelIdx].compressionVel = compressionVel;
        suspensionStates[wheelIdx].bodyVelZ = chassis->getLinearVelocity().z;
        suspensionStates[wheelIdx].wheelVelZ = 0.0f;  // 简化：假设车轮速度为0

        // 根据悬挂类型计算力
        PxReal force = calculateSuspensionForce(wheelIdx);
        suspensionStates[wheelIdx].currentForce = force;

        // 施加悬挂力
        chassis->addForceAtPos(PxVec3(0, 0, force), wheelWorldPos);
    }

    PxReal calculateSuspensionForce(size_t wheelIdx) {
        const auto& config = suspensions[wheelIdx];
        const auto& state = suspensionStates[wheelIdx];

        PxReal springForce = 0.0f;
        PxReal damperForce = 0.0f;
        PxReal activeForce = 0.0f;

        switch (config.type) {
            case SuspensionType::Passive:
                springForce = config.springStiffness * state.compression;
                damperForce = config.damperCoeffMin * state.compressionVel;
                suspensionStates[wheelIdx].currentDamping = config.damperCoeffMin;
                break;

            case SuspensionType::SemiActiveSkyhook:
                springForce = config.springStiffness * state.compression;
                // 天钩控制
                if (state.bodyVelZ * (state.bodyVelZ - state.wheelVelZ) > 0) {
                    suspensionStates[wheelIdx].currentDamping = config.damperCoeffMax;
                } else {
                    suspensionStates[wheelIdx].currentDamping = config.damperCoeffMin;
                }
                damperForce = suspensionStates[wheelIdx].currentDamping * state.compressionVel;
                break;

            case SuspensionType::SemiActiveGround:
                springForce = config.springStiffness * state.compression;
                // 地钩控制
                if (state.wheelVelZ * (state.bodyVelZ - state.wheelVelZ) > 0) {
                    suspensionStates[wheelIdx].currentDamping = config.damperCoeffMax;
                } else {
                    suspensionStates[wheelIdx].currentDamping = config.damperCoeffMin;
                }
                damperForce = suspensionStates[wheelIdx].currentDamping * state.compressionVel;
                break;

            case SuspensionType::Active:
                springForce = config.springStiffness * state.compression;
                damperForce = config.damperCoeffMax * state.compressionVel;
                // 主动控制：抑制车身垂直速度
                activeForce = -config.activeGain * state.bodyVelZ;
                activeForce = PxClamp(activeForce, -config.maxActiveForce, config.maxActiveForce);
                suspensionStates[wheelIdx].currentDamping = config.damperCoeffMax;
                break;

            case SuspensionType::AirSuspension:
                // 气动弹簧力：F = A × P
                springForce = config.airSpringArea * config.currentAirPressure *
                              (state.compression / config.maxTravel);
                damperForce = config.damperCoeffMin * state.compressionVel;
                suspensionStates[wheelIdx].currentDamping = config.damperCoeffMin;
                break;
        }

        return springForce + damperForce + activeForce;
    }
};

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 悬挂类型对比
 */
void demonstrateSuspensionComparison(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景1: 悬挂类型对比 ===" << std::endl;
    std::cout << "对比5种悬挂类型在冲击载荷下的响应" << std::endl;

    SuspensionType types[5] = {
        SuspensionType::Passive,
        SuspensionType::SemiActiveSkyhook,
        SuspensionType::SemiActiveGround,
        SuspensionType::Active,
        SuspensionType::AirSuspension
    };

    const char* names[5] = {
        "被动悬挂",
        "半主动（天钩）",
        "半主动（地钩）",
        "主动悬挂",
        "气动悬挂"
    };

    std::vector<VehicleWithCustomSuspension> vehicles;

    // 创建5辆车
    for (int i = 0; i < 5; ++i) {
        VehicleWithCustomSuspension vehicle;
        vehicle.create(physics, scene, PxTransform(PxVec3(i * 8.0f, 0, 5)), types[i]);
        vehicles.push_back(vehicle);
    }

    // 模拟下落
    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 180;  // 3秒

    std::cout << "\n时间\t";
    for (int i = 0; i < 5; ++i) {
        std::cout << names[i] << "\t";
    }
    std::cout << std::endl;
    std::cout << "---------------------------------------------------------------------" << std::endl;

    for (int step = 0; step < numSteps; ++step) {
        for (auto& vehicle : vehicles) {
            vehicle.update(dt);
        }

        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 30 == 0) {
            std::cout << step * dt << "s\t";
            for (const auto& vehicle : vehicles) {
                PxReal height = vehicle.chassis->getGlobalPose().p.z;
                std::cout << height << "\t\t";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "\n最终车身高度:" << std::endl;
    for (size_t i = 0; i < vehicles.size(); ++i) {
        PxReal finalHeight = vehicles[i].chassis->getGlobalPose().p.z;
        std::cout << names[i] << ": " << finalHeight << " m" << std::endl;
    }
}

/**
 * 场景2: 自适应阻尼响应
 */
void demonstrateAdaptiveDamping(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景2: 自适应阻尼响应 ===" << std::endl;
    std::cout << "天钩悬挂在周期性冲击下的阻尼变化" << std::endl;

    VehicleWithCustomSuspension vehicle;
    vehicle.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), SuspensionType::SemiActiveSkyhook);

    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 300;

    std::cout << "\n时间\t车身高度(m)\t阻尼系数(N·s/m)" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    for (int step = 0; step < numSteps; ++step) {
        // 周期性施加向下冲击
        if (step % 60 == 0 && step > 0) {
            vehicle.chassis->addForce(PxVec3(0, 0, -5000.0f));
        }

        vehicle.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 30 == 0) {
            PxReal height = vehicle.chassis->getGlobalPose().p.z;
            PxReal avgDamping = 0.0f;
            for (const auto& state : vehicle.suspensionStates) {
                avgDamping += state.currentDamping;
            }
            avgDamping /= 4.0f;

            std::cout << step * dt << "s\t"
                      << height << "\t\t"
                      << avgDamping << std::endl;
        }
    }

    std::cout << "\n天钩悬挂特点:" << std::endl;
    std::cout << "- 车身下降时：高阻尼（抑制下沉）" << std::endl;
    std::cout << "- 车身上升时：低阻尼（快速恢复）" << std::endl;
    std::cout << "- 效果：减小车身震荡，提高舒适性" << std::endl;
}

/**
 * 场景3: 气动悬挂高度调节
 */
void demonstrateAirSuspensionHeightControl(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景3: 气动悬挂高度调节 ===" << std::endl;
    std::cout << "演示气动悬挂的高度自适应" << std::endl;

    VehicleWithCustomSuspension vehicle;
    vehicle.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), SuspensionType::AirSuspension);

    const PxReal dt = 1.0f / 60.0f;

    std::cout << "\n阶段\t\t目标高度(m)\t实际高度(m)\t气压(bar)" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;

    // 阶段1：正常高度
    vehicle.setAirSuspensionHeight(0.4f);
    for (int step = 0; step < 120; ++step) {
        vehicle.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);
    }
    PxReal height1 = vehicle.chassis->getGlobalPose().p.z;
    PxReal pressure1 = vehicle.suspensions[0].currentAirPressure / 100000.0f;
    std::cout << "正常模式\t0.4\t\t" << height1 << "\t\t" << pressure1 << std::endl;

    // 阶段2：降低（高速模式）
    vehicle.setAirSuspensionHeight(0.3f);
    for (int step = 0; step < 120; ++step) {
        vehicle.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);
    }
    PxReal height2 = vehicle.chassis->getGlobalPose().p.z;
    PxReal pressure2 = vehicle.suspensions[0].currentAirPressure / 100000.0f;
    std::cout << "高速模式\t0.3\t\t" << height2 << "\t\t" << pressure2 << std::endl;

    // 阶段3：升高（越野模式）
    vehicle.setAirSuspensionHeight(0.5f);
    for (int step = 0; step < 120; ++step) {
        vehicle.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);
    }
    PxReal height3 = vehicle.chassis->getGlobalPose().p.z;
    PxReal pressure3 = vehicle.suspensions[0].currentAirPressure / 100000.0f;
    std::cout << "越野模式\t0.5\t\t" << height3 << "\t\t" << pressure3 << std::endl;

    std::cout << "\n气动悬挂优势:" << std::endl;
    std::cout << "- 可调高度：适应不同路况和速度" << std::endl;
    std::cout << "- 载荷自适应：保持恒定车身高度" << std::endl;
    std::cout << "- 舒适性好：气囊具有非线性特性" << std::endl;
}

/**
 * 场景4: 主动悬挂性能
 */
void demonstrateActiveSuspension(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景4: 主动悬挂性能 ===" << std::endl;
    std::cout << "对比被动悬挂和主动悬挂在连续路面不平度下的表现" << std::endl;

    VehicleWithCustomSuspension passive;
    passive.create(physics, scene, PxTransform(PxVec3(-5, 0, 2)), SuspensionType::Passive);

    VehicleWithCustomSuspension active;
    active.create(physics, scene, PxTransform(PxVec3(5, 0, 2)), SuspensionType::Active);

    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 240;

    std::cout << "\n时间\t被动悬挂高度(m)\t主动悬挂高度(m)" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    for (int step = 0; step < numSteps; ++step) {
        // 模拟路面不平：周期性冲击
        if (step % 40 == 0) {
            PxReal impulseMag = 3000.0f;
            passive.chassis->addForce(PxVec3(0, 0, -impulseMag));
            active.chassis->addForce(PxVec3(0, 0, -impulseMag));
        }

        passive.update(dt);
        active.update(dt);

        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 40 == 0) {
            PxReal passiveHeight = passive.chassis->getGlobalPose().p.z;
            PxReal activeHeight = active.chassis->getGlobalPose().p.z;

            std::cout << step * dt << "s\t"
                      << passiveHeight << "\t\t\t"
                      << activeHeight << std::endl;
        }
    }

    std::cout << "\n主动悬挂优势:" << std::endl;
    std::cout << "- 快速响应：主动施力抵消冲击" << std::endl;
    std::cout << "- 姿态控制：保持车身水平" << std::endl;
    std::cout << "- 舒适性：减小车身加速度" << std::endl;
    std::cout << "- 操控性：减小侧倾和俯仰" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: VehicleCustomSuspension" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\n演示自定义悬挂系统" << std::endl;

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
    sceneDesc.gravity = PxVec3(0.0f, 0.0f, -9.81f);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    PxScene* scene = physics->createScene(sceneDesc);

    // 添加地面
    PxMaterial* groundMaterial = physics->createMaterial(0.8f, 0.8f, 0.1f);
    PxRigidStatic* ground = physics->createRigidStatic(PxTransform(PxVec3(0, 0, 0)));
    PxShape* groundShape = physics->createShape(PxPlaneGeometry(), *groundMaterial);
    ground->attachShape(*groundShape);
    scene->addActor(*ground);
    groundShape->release();
    groundMaterial->release();

    // 运行4个场景
    demonstrateSuspensionComparison(physics, scene);
    demonstrateAdaptiveDamping(physics, scene);
    demonstrateAirSuspensionHeightControl(physics, scene);
    demonstrateActiveSuspension(physics, scene);

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\n悬挂类型对比:" << std::endl;
    std::cout << "┌────────────────┬──────────┬──────────┬──────────┬──────────┐" << std::endl;
    std::cout << "│ 类型           │ 舒适性   │ 操控性   │ 成本     │ 能耗     │" << std::endl;
    std::cout << "├────────────────┼──────────┼──────────┼──────────┼──────────┤" << std::endl;
    std::cout << "│ 被动悬挂       │ 中       │ 中       │ 低       │ 无       │" << std::endl;
    std::cout << "│ 半主动悬挂     │ 高       │ 高       │ 中       │ 很低     │" << std::endl;
    std::cout << "│ 主动悬挂       │ 很高     │ 很高     │ 很高     │ 高       │" << std::endl;
    std::cout << "│ 气动悬挂       │ 很高     │ 中       │ 高       │ 中       │" << std::endl;
    std::cout << "└────────────────┴──────────┴──────────┴──────────┴──────────┘" << std::endl;

    std::cout << "\n关键公式:" << std::endl;
    std::cout << "悬挂力: F = -k × x - c × ẋ" << std::endl;
    std::cout << "阻尼比: ζ = c / (2 × √(k × m))" << std::endl;
    std::cout << "固有频率: ω_n = √(k / m)" << std::endl;
    std::cout << "天钩阻尼: c = c_max if ż_body × (ż_body - ż_wheel) > 0" << std::endl;
    std::cout << "气动弹簧: F = A × P × (x / x_max)" << std::endl;

    std::cout << "\n应用场景:" << std::endl;
    std::cout << "1. 豪华车 - 主动/半主动悬挂，提升舒适性" << std::endl;
    std::cout << "2. 跑车 - 硬悬挂+主动防侧倾，提升操控" << std::endl;
    std::cout << "3. SUV - 气动悬挂，适应越野和公路" << std::endl;
    std::cout << "4. 商用车 - 气动悬挂，载荷自适应" << std::endl;

    // 清理
    scene->release();
    physics->release();
    foundation->release();

    return 0;
}
