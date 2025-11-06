/**
 * PhysX Snippet: VehicleDirectDrive
 *
 * 演示直驱车辆系统（Direct Drive Vehicle）
 *
 * 核心功能:
 * 1. 无引擎/变速箱的直接驱动车辆
 * 2. 直接控制车轮扭矩
 * 3. 电动车辆模拟
 * 4. 简化的车辆动力学
 *
 * 物理背景:
 *
 * 直驱车辆（Direct Drive）:
 * 与传统车辆（引擎+变速箱+传动系统）不同，直驱车辆直接对车轮施加扭矩。
 * 常见于电动车、轮毂电机车辆、简化的游戏车辆。
 *
 * 传统车辆 vs 直驱车辆:
 *
 * 传统车辆动力链:
 * 引擎 -> 离合器 -> 变速箱 -> 差速器 -> 车轮
 * τ_wheel = τ_engine × gear_ratio × differential_ratio × η
 *
 * 直驱车辆动力链:
 * 控制输入 -> 车轮扭矩
 * τ_wheel = f(throttle, brake)
 *
 * 关键方程:
 *
 * 车轮扭矩:
 * τ_wheel = τ_drive - τ_brake - τ_friction
 *
 * 车轮角加速度:
 * α = τ_wheel / I_wheel
 * 其中 I_wheel 是车轮转动惯量
 *
 * 纵向力:
 * F_long = τ_wheel / r_wheel
 * 其中 r_wheel 是车轮半径
 *
 * 轮胎滑移率:
 * κ = (ω × r - v_wheel) / max(|v_wheel|, v_min)
 * 其中:
 * - ω: 车轮角速度
 * - r: 车轮半径
 * - v_wheel: 车轮接触点速度
 *
 * 轮胎力模型（简化Pacejka）:
 * F_x = F_z × D × sin(C × arctan(B × κ))
 * 其中:
 * - F_z: 垂直载荷
 * - B, C, D: Pacejka魔术公式参数
 *
 * 优点:
 * 1. 实现简单，无需复杂的传动系统
 * 2. 响应快速，无换挡延迟
 * 3. 易于调试和调整
 * 4. 适合电动车辆仿真
 *
 * 缺点:
 * 1. 缺少传统车辆的真实感（引擎声、换挡）
 * 2. 无法模拟变速箱、离合器等组件
 *
 * 应用场景:
 * 1. 电动汽车仿真
 * 2. 简化的游戏车辆
 * 3. 轮毂电机车辆
 * 4. 原型设计和快速迭代
 *
 * 注意:
 * ⚠️ PhysX 5.x 需要配置PxVehicleDrive驱动模式为eDIRECT
 * ⚠️ 需要正确设置车轮悬挂、轮胎参数
 * ⚠️ 扭矩限制应根据电机功率设定
 */

#include <PxPhysicsAPI.h>
#include <vehicle2/PxVehicleAPI.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace physx;
using namespace physx::vehicle2;

// ============================================================================
// 车辆参数配置
// ============================================================================

/**
 * 直驱车辆配置
 */
struct DirectDriveVehicleConfig {
    // 车身参数
    PxReal chassisMass;          // 车身质量（kg）
    PxVec3 chassisDims;          // 车身尺寸（长x宽x高，米）
    PxVec3 chassisMOI;           // 转动惯量（kg·m²）

    // 车轮参数
    PxReal wheelRadius;          // 车轮半径（米）
    PxReal wheelWidth;           // 车轮宽度（米）
    PxReal wheelMass;            // 车轮质量（kg）
    PxReal wheelMOI;             // 车轮转动惯量（kg·m²）

    // 悬挂参数
    PxReal suspensionSpringStrength;     // 弹簧刚度（N/m）
    PxReal suspensionSpringDamping;      // 阻尼系数（N·s/m）
    PxReal suspensionMaxDroop;           // 最大下沉（米）
    PxReal suspensionMaxCompression;     // 最大压缩（米）

    // 驱动参数
    PxReal maxTorque;            // 最大扭矩（N·m）
    PxReal maxBrakeTorque;       // 最大制动扭矩（N·m）

    // 轮胎参数
    PxReal tireFrictionMultiplier;       // 摩擦倍数

    DirectDriveVehicleConfig() {
        // 默认值（中型电动车）
        chassisMass = 1500.0f;
        chassisDims = PxVec3(4.5f, 2.0f, 1.5f);
        chassisMOI = PxVec3(1000.0f, 1500.0f, 1000.0f);

        wheelRadius = 0.4f;
        wheelWidth = 0.3f;
        wheelMass = 20.0f;
        wheelMOI = wheelMass * wheelRadius * wheelRadius * 0.5f;

        suspensionSpringStrength = 35000.0f;
        suspensionSpringDamping = 4500.0f;
        suspensionMaxDroop = 0.1f;
        suspensionMaxCompression = 0.3f;

        maxTorque = 300.0f;          // 每轮300N·m
        maxBrakeTorque = 2000.0f;    // 制动扭矩

        tireFrictionMultiplier = 1.0f;
    }
};

/**
 * 车轮位置（相对车身中心）
 */
struct WheelPositions {
    PxVec3 frontLeft;
    PxVec3 frontRight;
    PxVec3 rearLeft;
    PxVec3 rearRight;

    static WheelPositions createDefault(const PxVec3& chassisDims) {
        WheelPositions pos;
        PxReal halfLength = chassisDims.x * 0.4f;
        PxReal halfWidth = chassisDims.y * 0.5f;
        PxReal wheelHeight = -chassisDims.z * 0.3f;

        pos.frontLeft = PxVec3(halfLength, -halfWidth, wheelHeight);
        pos.frontRight = PxVec3(halfLength, halfWidth, wheelHeight);
        pos.rearLeft = PxVec3(-halfLength, -halfWidth, wheelHeight);
        pos.rearRight = PxVec3(-halfLength, halfWidth, wheelHeight);

        return pos;
    }
};

// ============================================================================
// 简化的直驱车辆类
// ============================================================================

/**
 * 直驱车辆（简化版，不使用完整的PxVehicle2系统）
 */
class SimpleDirectDriveVehicle {
public:
    PxRigidDynamic* chassis;
    std::vector<PxShape*> wheelShapes;
    std::vector<PxReal> wheelAngularVelocities;  // 车轮角速度（rad/s）
    std::vector<PxVec3> wheelRelativePositions;  // 车轮相对位置

    DirectDriveVehicleConfig config;

    // 控制输入
    PxReal throttle;      // 0到1
    PxReal brake;         // 0到1
    PxReal steer;         // -1到1

    SimpleDirectDriveVehicle() : chassis(nullptr), throttle(0), brake(0), steer(0) {}

    void create(PxPhysics* physics, PxScene* scene, const PxTransform& initialPose,
                const DirectDriveVehicleConfig& cfg) {
        config = cfg;

        // 创建车身
        PxMaterial* chassisMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);
        PxBoxGeometry chassisGeom(config.chassisDims * 0.5f);

        chassis = physics->createRigidDynamic(initialPose);
        PxShape* chassisShape = physics->createShape(chassisGeom, *chassisMaterial);
        chassis->attachShape(*chassisShape);

        PxRigidBodyExt::setMassAndUpdateInertia(*chassis, config.chassisMass);
        chassis->setMassSpaceInertiaTensor(config.chassisMOI);

        scene->addActor(*chassis);
        chassisShape->release();
        chassisMaterial->release();

        // 创建车轮（作为可视化，实际驱动通过扭矩施加）
        WheelPositions wheelPos = WheelPositions::createDefault(config.chassisDims);
        PxVec3 positions[4] = {wheelPos.frontLeft, wheelPos.frontRight, wheelPos.rearLeft, wheelPos.rearRight};

        PxMaterial* wheelMaterial = physics->createMaterial(0.8f, 0.8f, 0.1f);

        for (int i = 0; i < 4; ++i) {
            wheelRelativePositions.push_back(positions[i]);
            wheelAngularVelocities.push_back(0.0f);

            // 创建车轮形状（圆柱体）
            PxShape* wheelShape = physics->createShape(
                PxCylinderGeometry(config.wheelRadius, config.wheelWidth * 0.5f),
                *wheelMaterial
            );
            wheelShapes.push_back(wheelShape);
        }

        wheelMaterial->release();
    }

    void setControls(PxReal throttleInput, PxReal brakeInput, PxReal steerInput) {
        throttle = PxClamp(throttleInput, 0.0f, 1.0f);
        brake = PxClamp(brakeInput, 0.0f, 1.0f);
        steer = PxClamp(steerInput, -1.0f, 1.0f);
    }

    void update(PxReal dt) {
        if (!chassis) return;

        // 计算每个车轮的扭矩和力
        for (int wheelIdx = 0; wheelIdx < 4; ++wheelIdx) {
            updateWheel(wheelIdx, dt);
        }
    }

private:
    void updateWheel(int wheelIdx, PxReal dt) {
        // 确定是否为驱动轮（这里假设四轮驱动）
        bool isDriveWheel = true;

        // 确定是否为转向轮（前轮）
        bool isSteerWheel = (wheelIdx == 0 || wheelIdx == 1);

        // 计算车轮在世界空间的位置
        PxTransform chassisTransform = chassis->getGlobalPose();
        PxVec3 wheelWorldPos = chassisTransform.transform(wheelRelativePositions[wheelIdx]);

        // 射线检测地面
        PxVec3 rayStart = wheelWorldPos;
        PxVec3 rayDir(0, 0, -1);  // 向下
        PxReal rayLength = config.wheelRadius + config.suspensionMaxDroop;

        PxRaycastBuffer hit;
        PxScene* scene;
        chassis->getScene(scene);

        bool hasContact = scene->raycast(rayStart, rayDir, rayLength, hit);

        if (!hasContact) {
            // 车轮悬空
            return;
        }

        // 计算悬挂压缩量
        PxReal compressionDistance = hit.block.distance - config.wheelRadius;
        PxReal compression = PxClamp(compressionDistance, -config.suspensionMaxCompression, config.suspensionMaxDroop);

        // 悬挂力
        PxReal suspensionForce = -compression * config.suspensionSpringStrength;

        // 阻尼力（简化）
        PxVec3 chassisVelAtWheel = chassis->getLinearVelocity() +
                                    chassis->getAngularVelocity().cross(wheelRelativePositions[wheelIdx]);
        PxReal suspensionVel = chassisVelAtWheel.z;
        suspensionForce -= suspensionVel * config.suspensionSpringDamping;

        // 施加悬挂力
        chassis->addForceAtPos(PxVec3(0, 0, suspensionForce), wheelWorldPos);

        // 计算纵向力（驱动/制动）
        PxReal wheelTorque = 0.0f;

        if (isDriveWheel) {
            // 驱动扭矩
            wheelTorque += throttle * config.maxTorque;

            // 制动扭矩（反向）
            if (wheelAngularVelocities[wheelIdx] > 0.1f) {
                wheelTorque -= brake * config.maxBrakeTorque;
            } else if (wheelAngularVelocities[wheelIdx] < -0.1f) {
                wheelTorque += brake * config.maxBrakeTorque;
            }
        }

        // 更新车轮角速度（简化，忽略轮胎滑移）
        PxReal wheelAngularAccel = wheelTorque / config.wheelMOI;
        wheelAngularVelocities[wheelIdx] += wheelAngularAccel * dt;

        // 计算纵向力
        PxReal longitudinalForce = wheelTorque / config.wheelRadius;

        // 获取车轮前进方向
        PxVec3 wheelForward = chassisTransform.q.rotate(PxVec3(1, 0, 0));

        // 如果是转向轮，应用转向角
        if (isSteerWheel) {
            PxReal steerAngle = steer * PxPi / 6.0f;  // 最大转向30度
            PxQuat steerRotation(steerAngle, PxVec3(0, 0, 1));
            wheelForward = steerRotation.rotate(wheelForward);
        }

        // 施加纵向力
        chassis->addForceAtPos(wheelForward * longitudinalForce * config.tireFrictionMultiplier, wheelWorldPos);

        // 施加横向摩擦力（简化）
        PxVec3 wheelRight = chassisTransform.q.rotate(PxVec3(0, 1, 0));
        PxReal lateralVel = chassisVelAtWheel.dot(wheelRight);
        PxReal lateralFriction = -lateralVel * config.chassisMass * 0.5f;  // 简化的横向摩擦
        chassis->addForceAtPos(wheelRight * lateralFriction, wheelWorldPos);
    }
};

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 直线加速
 */
void demonstrateStraightAcceleration(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景1: 直线加速 ===" << std::endl;
    std::cout << "车辆从静止加速到最大速度" << std::endl;

    DirectDriveVehicleConfig config;
    config.maxTorque = 400.0f;  // 高扭矩电机

    SimpleDirectDriveVehicle vehicle;
    vehicle.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), config);

    // 全油门加速
    vehicle.setControls(1.0f, 0.0f, 0.0f);  // throttle=1, brake=0, steer=0

    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 300;  // 5秒

    std::cout << "\n时间\t速度(m/s)\t速度(km/h)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    for (int step = 0; step < numSteps; ++step) {
        vehicle.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 60 == 0) {
            PxVec3 velocity = vehicle.chassis->getLinearVelocity();
            PxReal speed = velocity.magnitude();
            std::cout << step * dt << "s\t"
                      << speed << "\t\t"
                      << speed * 3.6f << std::endl;
        }
    }

    PxReal finalSpeed = vehicle.chassis->getLinearVelocity().magnitude();
    std::cout << "\n最终速度: " << finalSpeed * 3.6f << " km/h" << std::endl;
    std::cout << "加速性能: 0-" << finalSpeed * 3.6f << " km/h in 5秒" << std::endl;
}

/**
 * 场景2: 紧急制动
 */
void demonstrateEmergencyBraking(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景2: 紧急制动 ===" << std::endl;
    std::cout << "车辆从高速紧急制动至停止" << std::endl;

    DirectDriveVehicleConfig config;
    config.maxBrakeTorque = 3000.0f;  // 强力制动

    SimpleDirectDriveVehicle vehicle;
    vehicle.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), config);

    // 先加速到一定速度
    vehicle.setControls(1.0f, 0.0f, 0.0f);
    const PxReal dt = 1.0f / 60.0f;

    for (int step = 0; step < 180; ++step) {  // 3秒加速
        vehicle.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);
    }

    PxReal initialSpeed = vehicle.chassis->getLinearVelocity().magnitude();
    std::cout << "初始速度: " << initialSpeed * 3.6f << " km/h" << std::endl;

    // 紧急制动
    vehicle.setControls(0.0f, 1.0f, 0.0f);  // throttle=0, brake=1

    std::cout << "\n时间\t速度(m/s)\t距离(m)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    PxVec3 brakeStartPos = vehicle.chassis->getGlobalPose().p;
    int brakingSteps = 0;

    while (vehicle.chassis->getLinearVelocity().magnitude() > 0.1f && brakingSteps < 300) {
        vehicle.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);

        if (brakingSteps % 30 == 0) {
            PxReal speed = vehicle.chassis->getLinearVelocity().magnitude();
            PxReal distance = (vehicle.chassis->getGlobalPose().p - brakeStartPos).magnitude();
            std::cout << brakingSteps * dt << "s\t"
                      << speed << "\t\t"
                      << distance << std::endl;
        }

        brakingSteps++;
    }

    PxReal totalBrakingDistance = (vehicle.chassis->getGlobalPose().p - brakeStartPos).magnitude();
    PxReal brakingTime = brakingSteps * dt;

    std::cout << "\n制动距离: " << totalBrakingDistance << " m" << std::endl;
    std::cout << "制动时间: " << brakingTime << " s" << std::endl;
    std::cout << "平均减速度: " << initialSpeed / brakingTime << " m/s²" << std::endl;
}

/**
 * 场景3: 转向操控
 */
void demonstrateSteering(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景3: 转向操控 ===" << std::endl;
    std::cout << "车辆在恒定速度下执行圆周转向" << std::endl;

    DirectDriveVehicleConfig config;

    SimpleDirectDriveVehicle vehicle;
    vehicle.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), config);

    // 先加速到中等速度
    vehicle.setControls(0.5f, 0.0f, 0.0f);
    const PxReal dt = 1.0f / 60.0f;

    for (int step = 0; step < 120; ++step) {
        vehicle.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);
    }

    std::cout << "稳定速度: " << vehicle.chassis->getLinearVelocity().magnitude() * 3.6f << " km/h" << std::endl;

    // 执行左转
    vehicle.setControls(0.5f, 0.0f, -1.0f);  // 左转（steer=-1）

    PxVec3 startPos = vehicle.chassis->getGlobalPose().p;
    std::cout << "\n执行左转圆周运动（5秒）..." << std::endl;

    for (int step = 0; step < 300; ++step) {
        vehicle.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 60 == 0) {
            PxVec3 currentPos = vehicle.chassis->getGlobalPose().p;
            PxReal heading = PxAtan2(vehicle.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0)).y,
                                     vehicle.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0)).x);
            std::cout << "时间 " << step * dt << "s: 位置(" << currentPos.x << ", " << currentPos.y
                      << "), 朝向 " << heading * 180.0f / PxPi << "度" << std::endl;
        }
    }

    PxVec3 endPos = vehicle.chassis->getGlobalPose().p;
    PxReal turnRadius = (endPos - startPos).magnitude() * 0.5f;

    std::cout << "\n转弯半径（估计）: " << turnRadius << " m" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: VehicleDirectDrive" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "\n演示直驱车辆系统" << std::endl;

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
    sceneDesc.gravity = PxVec3(0.0f, 0.0f, -9.81f);  // Z向上
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

    // 运行3个场景
    demonstrateStraightAcceleration(physics, scene);
    demonstrateEmergencyBraking(physics, scene);
    demonstrateSteering(physics, scene);

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\n直驱车辆 vs 传统车辆:" << std::endl;
    std::cout << "┌────────────────┬─────────────────────┬──────────────────────┐" << std::endl;
    std::cout << "│ 特性           │ 直驱车辆            │ 传统车辆             │" << std::endl;
    std::cout << "├────────────────┼─────────────────────┼──────────────────────┤" << std::endl;
    std::cout << "│ 动力链         │ 控制→车轮           │ 引擎→变速箱→车轮     │" << std::endl;
    std::cout << "│ 响应速度       │ 即时                │ 有延迟（换挡）       │" << std::endl;
    std::cout << "│ 实现复杂度     │ 简单                │ 复杂                 │" << std::endl;
    std::cout << "│ 真实感         │ 低（无引擎声）      │ 高                   │" << std::endl;
    std::cout << "│ 适用场景       │ 电动车、游戏        │ 燃油车仿真           │" << std::endl;
    std::cout << "└────────────────┴─────────────────────┴──────────────────────┘" << std::endl;

    std::cout << "\n关键公式:" << std::endl;
    std::cout << "车轮扭矩: τ_wheel = τ_drive - τ_brake" << std::endl;
    std::cout << "纵向力: F_long = τ_wheel / r_wheel" << std::endl;
    std::cout << "车轮角加速度: α = τ_wheel / I_wheel" << std::endl;

    std::cout << "\n应用建议:" << std::endl;
    std::cout << "1. 电动车仿真：直驱模式最合适" << std::endl;
    std::cout << "2. 原型开发：快速迭代，易于调试" << std::endl;
    std::cout << "3. 游戏车辆：街机式手感，响应快" << std::endl;
    std::cout << "4. 教学演示：概念清晰，易于理解" << std::endl;

    // 清理
    scene->release();
    physics->release();
    foundation->release();

    return 0;
}
