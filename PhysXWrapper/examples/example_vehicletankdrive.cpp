/**
 * PhysX Snippet: VehicleTankDrive
 *
 * 演示坦克式驱动系统（Tank Drive / Tracked Vehicle）
 *
 * 核心功能:
 * 1. 履带式车辆物理
 * 2. 差速转向（Differential Steering）
 * 3. 独立履带控制
 * 4. 坦克转向模型
 *
 * 物理背景:
 *
 * 坦克式驱动（Tank Drive）:
 * 通过左右履带的速度差实现转向，无需方向盘。
 * 常见于坦克、推土机、工程机械、履带机器人。
 *
 * 差速转向（Differential Steering）:
 *
 * 转向原理:
 * v_left ≠ v_right  =>  车辆转向
 *
 * 瞬时转向中心（Instantaneous Center of Rotation, ICR）:
 * R = L × (v_left + v_right) / (2 × (v_right - v_left))
 * 其中:
 * - L: 履带间距（轮距）
 * - v_left, v_right: 左右履带速度
 *
 * 角速度:
 * ω = (v_right - v_left) / L
 *
 * 特殊情况:
 * 1. v_left = v_right: 直线前进
 * 2. v_left = -v_right: 原地转向（R = 0）
 * 3. v_left = 0, v_right > 0: 绕左履带转向
 * 4. v_left > 0, v_right = 0: 绕右履带转向
 *
 * 履带-地面接触模型:
 *
 * 履带简化为多个接地点（Ground Contact Points）
 * 每个接地点施加:
 * 1. 法向力（悬挂）: F_n = k × δ - c × v_n
 * 2. 切向力（驱动）: F_t = τ_track / r_sprocket
 * 3. 摩擦力（横向）: F_friction = μ × F_n
 *
 * 履带张力（Track Tension）:
 * T = F_drive / 2
 * 每个接地点承受的张力分量
 *
 * 地面压强（Ground Pressure）:
 * P = W / (L_track × W_track)
 * 其中:
 * - W: 车辆重量
 * - L_track: 履带接地长度
 * - W_track: 履带宽度
 *
 * 优点:
 * 1. 优秀的越野能力
 * 2. 原地转向能力
 * 3. 低地面压强
 * 4. 高牵引力
 *
 * 缺点:
 * 1. 能耗高
 * 2. 速度慢
 * 3. 转向阻力大
 * 4. 维护复杂
 *
 * 应用场景:
 * 1. 军用坦克
 * 2. 工程机械（推土机、挖掘机）
 * 3. 农业机械
 * 4. 履带式机器人
 *
 * 注意:
 * ⚠️ 履带物理比轮式车辆复杂，需要多接地点模拟
 * ⚠️ 原地转向时摩擦力矩很大，需要高扭矩
 * ⚠️ 地面材质对履带性能影响显著
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
 * 履带配置
 */
struct TrackConfig {
    PxReal trackWidth;           // 履带宽度（米）
    PxReal trackLength;          // 履带接地长度（米）
    PxU32 numContactPoints;      // 接地点数量
    PxReal sprocketRadius;       // 主动轮半径（米）
    PxReal frictionCoefficient;  // 摩擦系数
    PxReal lateralFriction;      // 横向摩擦系数

    TrackConfig()
        : trackWidth(0.5f)
        , trackLength(3.0f)
        , numContactPoints(6)
        , sprocketRadius(0.4f)
        , frictionCoefficient(0.9f)
        , lateralFriction(1.5f) {}
};

/**
 * 坦克车辆配置
 */
struct TankVehicleConfig {
    // 车身参数
    PxReal chassisMass;          // 车身质量（kg）
    PxVec3 chassisDims;          // 车身尺寸（长x宽x高，米）
    PxVec3 chassisMOI;           // 转动惯量（kg·m²）

    // 履带参数
    PxReal trackSeparation;      // 履带间距（米）
    TrackConfig leftTrack;
    TrackConfig rightTrack;

    // 悬挂参数
    PxReal suspensionSpringStrength;     // 弹簧刚度（N/m）
    PxReal suspensionSpringDamping;      // 阻尼系数（N·s/m）
    PxReal suspensionMaxTravel;          // 最大行程（米）

    // 驱动参数
    PxReal maxTorquePerTrack;    // 每条履带最大扭矩（N·m）

    TankVehicleConfig() {
        // 默认值（中型坦克）
        chassisMass = 40000.0f;  // 40吨
        chassisDims = PxVec3(6.0f, 3.5f, 2.5f);
        chassisMOI = PxVec3(50000.0f, 80000.0f, 50000.0f);

        trackSeparation = 3.0f;
        maxTorquePerTrack = 15000.0f;

        suspensionSpringStrength = 200000.0f;
        suspensionSpringDamping = 20000.0f;
        suspensionMaxTravel = 0.3f;
    }
};

/**
 * 履带接地点
 */
struct TrackContactPoint {
    PxVec3 localPosition;        // 相对车身的位置
    PxReal verticalForce;        // 垂直力（N）
    PxReal tangentialForce;      // 切向力（N）
};

/**
 * 简化的坦克车辆类
 */
class SimpleTankVehicle {
public:
    PxRigidDynamic* chassis;
    TankVehicleConfig config;

    std::vector<TrackContactPoint> leftTrackContacts;
    std::vector<TrackContactPoint> rightTrackContacts;

    // 控制输入
    PxReal leftTrackThrottle;    // -1到1（负值倒退）
    PxReal rightTrackThrottle;   // -1到1

    SimpleTankVehicle() : chassis(nullptr), leftTrackThrottle(0), rightTrackThrottle(0) {}

    void create(PxPhysics* physics, PxScene* scene, const PxTransform& initialPose,
                const TankVehicleConfig& cfg) {
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

        // 初始化履带接地点
        initializeTrackContacts();
    }

    void setControls(PxReal leftThrottle, PxReal rightThrottle) {
        leftTrackThrottle = PxClamp(leftThrottle, -1.0f, 1.0f);
        rightTrackThrottle = PxClamp(rightThrottle, -1.0f, 1.0f);
    }

    void update(PxReal dt) {
        if (!chassis) return;

        // 更新左履带
        updateTrack(leftTrackContacts, -config.trackSeparation * 0.5f, leftTrackThrottle, dt);

        // 更新右履带
        updateTrack(rightTrackContacts, config.trackSeparation * 0.5f, rightTrackThrottle, dt);
    }

    // 计算瞬时转向半径
    PxReal calculateTurnRadius() const {
        PxVec3 velocity = chassis->getLinearVelocity();
        PxReal forwardSpeed = velocity.magnitude();

        if (forwardSpeed < 0.1f) {
            return 0.0f;  // 静止或原地转向
        }

        PxReal avgSpeed = (leftTrackThrottle + rightTrackThrottle) * 0.5f;
        PxReal speedDiff = rightTrackThrottle - leftTrackThrottle;

        if (PxAbs(speedDiff) < 0.01f) {
            return PX_MAX_F32;  // 直线前进
        }

        PxReal R = config.trackSeparation * avgSpeed / speedDiff;
        return R;
    }

    // 计算角速度（rad/s）
    PxReal calculateAngularVelocity() const {
        PxReal maxSpeed = 10.0f;  // 假设最大速度10 m/s
        PxReal leftSpeed = leftTrackThrottle * maxSpeed;
        PxReal rightSpeed = rightTrackThrottle * maxSpeed;

        return (rightSpeed - leftSpeed) / config.trackSeparation;
    }

private:
    void initializeTrackContacts() {
        // 左履带接地点
        PxReal trackLength = config.leftTrack.trackLength;
        PxReal stepLength = trackLength / (config.leftTrack.numContactPoints - 1);

        for (PxU32 i = 0; i < config.leftTrack.numContactPoints; ++i) {
            TrackContactPoint point;
            point.localPosition = PxVec3(
                trackLength * 0.5f - i * stepLength,
                -config.trackSeparation * 0.5f,
                -config.chassisDims.z * 0.5f
            );
            point.verticalForce = 0.0f;
            point.tangentialForce = 0.0f;
            leftTrackContacts.push_back(point);
        }

        // 右履带接地点
        for (PxU32 i = 0; i < config.rightTrack.numContactPoints; ++i) {
            TrackContactPoint point;
            point.localPosition = PxVec3(
                trackLength * 0.5f - i * stepLength,
                config.trackSeparation * 0.5f,
                -config.chassisDims.z * 0.5f
            );
            point.verticalForce = 0.0f;
            point.tangentialForce = 0.0f;
            rightTrackContacts.push_back(point);
        }
    }

    void updateTrack(std::vector<TrackContactPoint>& contacts, PxReal trackOffset,
                     PxReal throttle, PxReal dt) {
        PxTransform chassisTransform = chassis->getGlobalPose();
        PxVec3 forwardDir = chassisTransform.q.rotate(PxVec3(1, 0, 0));
        PxVec3 rightDir = chassisTransform.q.rotate(PxVec3(0, 1, 0));

        PxScene* scene;
        chassis->getScene(scene);

        for (auto& contact : contacts) {
            // 计算接地点世界位置
            PxVec3 contactWorldPos = chassisTransform.transform(contact.localPosition);

            // 射线检测地面
            PxVec3 rayStart = contactWorldPos + PxVec3(0, 0, 0.5f);
            PxVec3 rayDir(0, 0, -1);
            PxReal rayLength = config.suspensionMaxTravel + 1.0f;

            PxRaycastBuffer hit;
            bool hasContact = scene->raycast(rayStart, rayDir, rayLength, hit);

            if (!hasContact) {
                contact.verticalForce = 0.0f;
                contact.tangentialForce = 0.0f;
                continue;
            }

            // 计算悬挂压缩量
            PxReal compressionDistance = hit.block.distance - 0.5f;
            PxReal compression = PxClamp(compressionDistance, 0.0f, config.suspensionMaxTravel);

            // 悬挂力
            PxReal suspensionForce = compression * config.suspensionSpringStrength;

            // 阻尼力
            PxVec3 chassisVelAtContact = chassis->getLinearVelocity() +
                                          chassis->getAngularVelocity().cross(contact.localPosition);
            PxReal suspensionVel = chassisVelAtContact.z;
            suspensionForce += suspensionVel * config.suspensionSpringDamping;

            // 施加垂直力
            chassis->addForceAtPos(PxVec3(0, 0, suspensionForce), contactWorldPos);
            contact.verticalForce = suspensionForce;

            // 计算驱动力
            PxReal trackTorque = throttle * config.maxTorquePerTrack;
            PxReal tangentialForce = trackTorque / config.leftTrack.sprocketRadius;

            // 摩擦力限制
            PxReal maxFriction = suspensionForce * config.leftTrack.frictionCoefficient;
            tangentialForce = PxClamp(tangentialForce, -maxFriction, maxFriction);

            // 施加驱动力（沿车辆前进方向）
            chassis->addForceAtPos(forwardDir * tangentialForce, contactWorldPos);
            contact.tangentialForce = tangentialForce;

            // 施加横向摩擦力（防止侧滑）
            PxReal lateralVel = chassisVelAtContact.dot(rightDir);
            PxReal lateralFriction = -lateralVel * config.chassisMass * 0.2f;
            lateralFriction = PxClamp(lateralFriction,
                                      -suspensionForce * config.leftTrack.lateralFriction,
                                      suspensionForce * config.leftTrack.lateralFriction);
            chassis->addForceAtPos(rightDir * lateralFriction, contactWorldPos);
        }
    }
};

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 直线前进
 */
void demonstrateStraightMovement(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景1: 直线前进 ===" << std::endl;
    std::cout << "左右履带等速前进" << std::endl;

    TankVehicleConfig config;
    SimpleTankVehicle tank;
    tank.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), config);

    // 双履带全速前进
    tank.setControls(1.0f, 1.0f);

    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 300;  // 5秒

    std::cout << "\n时间\t速度(m/s)\t位移(m)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    PxVec3 startPos = tank.chassis->getGlobalPose().p;

    for (int step = 0; step < numSteps; ++step) {
        tank.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 60 == 0) {
            PxVec3 velocity = tank.chassis->getLinearVelocity();
            PxReal speed = velocity.magnitude();
            PxVec3 currentPos = tank.chassis->getGlobalPose().p;
            PxReal distance = (currentPos - startPos).magnitude();

            std::cout << step * dt << "s\t"
                      << speed << "\t\t"
                      << distance << std::endl;
        }
    }

    PxVec3 finalPos = tank.chassis->getGlobalPose().p;
    PxReal totalDistance = (finalPos - startPos).magnitude();

    std::cout << "\n总位移: " << totalDistance << " m" << std::endl;
    std::cout << "平均速度: " << totalDistance / (numSteps * dt) << " m/s" << std::endl;
}

/**
 * 场景2: 原地转向
 */
void demonstratePivotTurn(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景2: 原地转向 ===" << std::endl;
    std::cout << "左履带后退、右履带前进，实现原地旋转" << std::endl;

    TankVehicleConfig config;
    SimpleTankVehicle tank;
    tank.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), config);

    // 原地转向：左履带-1，右履带+1
    tank.setControls(-0.5f, 0.5f);

    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 300;

    std::cout << "\n时间\t朝向(度)\t角速度(rad/s)" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    PxReal initialHeading = PxAtan2(tank.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0)).y,
                                     tank.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0)).x);

    for (int step = 0; step < numSteps; ++step) {
        tank.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 60 == 0) {
            PxVec3 forward = tank.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0));
            PxReal heading = PxAtan2(forward.y, forward.x);
            PxReal headingDeg = heading * 180.0f / PxPi;

            PxVec3 angularVel = tank.chassis->getAngularVelocity();
            PxReal angularSpeed = angularVel.z;  // Z轴角速度

            std::cout << step * dt << "s\t"
                      << headingDeg << "\t\t"
                      << angularSpeed << std::endl;
        }
    }

    PxVec3 finalForward = tank.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0));
    PxReal finalHeading = PxAtan2(finalForward.y, finalForward.x);
    PxReal totalRotation = (finalHeading - initialHeading) * 180.0f / PxPi;

    std::cout << "\n总旋转角度: " << totalRotation << "度" << std::endl;
    std::cout << "平均角速度: " << totalRotation / (numSteps * dt) << " 度/s" << std::endl;
    std::cout << "理论角速度: " << tank.calculateAngularVelocity() << " rad/s" << std::endl;

    // 验证原地转向（位移应该很小）
    PxVec3 posDisplacement = tank.chassis->getGlobalPose().p - PxVec3(0, 0, 2);
    std::cout << "位置偏移: (" << posDisplacement.x << ", " << posDisplacement.y << ", "
              << posDisplacement.z << ") m" << std::endl;
}

/**
 * 场景3: 差速转向
 */
void demonstrateDifferentialSteering(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景3: 差速转向 ===" << std::endl;
    std::cout << "左履带慢速、右履带快速，实现圆弧转向" << std::endl;

    TankVehicleConfig config;
    SimpleTankVehicle tank;
    tank.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), config);

    // 差速转向：左履带0.3，右履带0.7
    tank.setControls(0.3f, 0.7f);

    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 360;  // 6秒

    PxReal theoreticalRadius = tank.calculateTurnRadius();
    std::cout << "理论转向半径: " << theoreticalRadius << " m" << std::endl;

    std::cout << "\n时间\t位置(x,y)\t\t朝向(度)" << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;

    PxVec3 startPos = tank.chassis->getGlobalPose().p;

    for (int step = 0; step < numSteps; ++step) {
        tank.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 90 == 0) {
            PxVec3 currentPos = tank.chassis->getGlobalPose().p;
            PxVec3 forward = tank.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0));
            PxReal heading = PxAtan2(forward.y, forward.x) * 180.0f / PxPi;

            std::cout << step * dt << "s\t"
                      << "(" << currentPos.x << ", " << currentPos.y << ")\t"
                      << heading << std::endl;
        }
    }

    PxVec3 finalPos = tank.chassis->getGlobalPose().p;
    PxReal pathLength = (finalPos - startPos).magnitude();

    std::cout << "\n路径长度: " << pathLength << " m" << std::endl;
    std::cout << "位移矢量: (" << (finalPos.x - startPos.x) << ", "
              << (finalPos.y - startPos.y) << ") m" << std::endl;
}

/**
 * 场景4: 单侧履带驱动
 */
void demonstrateSingleTrackDrive(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景4: 单侧履带驱动 ===" << std::endl;
    std::cout << "仅右履带驱动，左履带静止，绕左履带转向" << std::endl;

    TankVehicleConfig config;
    SimpleTankVehicle tank;
    tank.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), config);

    // 单侧驱动：左履带0，右履带0.5
    tank.setControls(0.0f, 0.5f);

    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 300;

    std::cout << "理论转向半径（绕左履带）: " << config.trackSeparation * 0.5f << " m" << std::endl;

    std::cout << "\n时间\t位置(x,y)\t\t朝向(度)" << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;

    for (int step = 0; step < numSteps; ++step) {
        tank.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 75 == 0) {
            PxVec3 currentPos = tank.chassis->getGlobalPose().p;
            PxVec3 forward = tank.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0));
            PxReal heading = PxAtan2(forward.y, forward.x) * 180.0f / PxPi;

            std::cout << step * dt << "s\t"
                      << "(" << currentPos.x << ", " << currentPos.y << ")\t"
                      << heading << std::endl;
        }
    }

    std::cout << "\n这种转向方式常用于狭窄空间中的机动" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: VehicleTankDrive" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "\n演示坦克式驱动系统（差速转向）" << std::endl;

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

    // 运行4个场景
    demonstrateStraightMovement(physics, scene);
    demonstratePivotTurn(physics, scene);
    demonstrateDifferentialSteering(physics, scene);
    demonstrateSingleTrackDrive(physics, scene);

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\n差速转向原理:" << std::endl;
    std::cout << "┌────────────────────┬──────────────────────┬──────────────────┐" << std::endl;
    std::cout << "│ 履带状态           │ 转向效果             │ 转向半径         │" << std::endl;
    std::cout << "├────────────────────┼──────────────────────┼──────────────────┤" << std::endl;
    std::cout << "│ v_L = v_R > 0      │ 直线前进             │ R = ∞            │" << std::endl;
    std::cout << "│ v_L = -v_R         │ 原地转向             │ R = 0            │" << std::endl;
    std::cout << "│ v_L = 0, v_R > 0   │ 绕左履带转向         │ R = L/2          │" << std::endl;
    std::cout << "│ v_L < v_R (同号)   │ 圆弧转向（向左）     │ R = L·avg/diff   │" << std::endl;
    std::cout << "│ v_L > v_R (同号)   │ 圆弧转向（向右）     │ R = L·avg/diff   │" << std::endl;
    std::cout << "└────────────────────┴──────────────────────┴──────────────────┘" << std::endl;

    std::cout << "\n关键公式:" << std::endl;
    std::cout << "转向半径: R = L × (v_L + v_R) / (2 × (v_R - v_L))" << std::endl;
    std::cout << "角速度: ω = (v_R - v_L) / L" << std::endl;
    std::cout << "地面压强: P = W / (L_track × W_track)" << std::endl;

    std::cout << "\n坦克式驱动 vs 轮式转向:" << std::endl;
    std::cout << "优点: 原地转向、高牵引力、低地面压强、优秀越野" << std::endl;
    std::cout << "缺点: 能耗高、速度慢、转向阻力大、维护复杂" << std::endl;

    std::cout << "\n应用场景:" << std::endl;
    std::cout << "1. 军用坦克 - 需要强大越野和原地转向能力" << std::endl;
    std::cout << "2. 工程机械 - 推土机、挖掘机等重型设备" << std::endl;
    std::cout << "3. 农业机械 - 收割机、拖拉机（履带版）" << std::endl;
    std::cout << "4. 履带机器人 - 探测、救援、军事用途" << std::endl;

    // 清理
    scene->release();
    physics->release();
    foundation->release();

    return 0;
}
