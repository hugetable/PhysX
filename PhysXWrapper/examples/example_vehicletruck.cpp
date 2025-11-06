/**
 * PhysX Snippet: VehicleTruck
 *
 * 演示卡车物理系统（Truck Physics / Heavy Duty Vehicle）
 *
 * 核心功能:
 * 1. 多轴车辆模拟
 * 2. 动态载荷分配
 * 3. 挂车/拖车连接
 * 4. 重型车辆动力学
 *
 * 物理背景:
 *
 * 卡车物理（Truck Physics）:
 * 与小型车辆相比，卡车具有:
 * - 多个车轴（2-5轴）
 * - 高质量和高惯性
 * - 可变载荷（空载 vs 满载）
 * - 更长的制动距离
 * - 更复杂的悬挂系统
 *
 * 多轴车辆动力学:
 *
 * 轴载荷分配（Load Distribution）:
 * 对于N轴车辆：
 * F_zi = (W × g) × (L_total - Σ(d_j)) / L_total + F_dynamic
 * 其中:
 * - F_zi: 第i轴的垂直载荷
 * - W: 总质量
 * - d_j: 到第i轴的距离
 * - F_dynamic: 动态载荷转移
 *
 * 动态载荷转移（Load Transfer）:
 *
 * 纵向载荷转移（加速/制动）:
 * ΔF_z = (m × a × h_cg) / L_wheelbase
 * 其中:
 * - m: 质量
 * - a: 纵向加速度
 * - h_cg: 质心高度
 * - L_wheelbase: 轴距
 *
 * 横向载荷转移（转弯）:
 * ΔF_z = (m × v² × h_cg) / (R × t_track)
 * 其中:
 * - v: 速度
 * - R: 转弯半径
 * - t_track: 轮距
 *
 * 制动力分配:
 *
 * 理想制动力分配曲线:
 * F_brake_rear / F_brake_front = (L_rear / L_front) × (1 - a/g)
 * 其中:
 * - L_rear, L_front: 后轴、前轴到质心的距离
 * - a: 制动减速度
 *
 * 实际卡车常采用固定比例:
 * 前轴: 60-70%
 * 后轴: 30-40%
 *
 * 拖车连接物理:
 *
 * 第五轮（Fifth Wheel）连接:
 * - 允许相对旋转（pitch, yaw）
 * - 限制垂直分离
 * - 传递牵引力和制动力
 *
 * 连接力:
 * F_coupling = k × Δx + c × Δv
 * τ_coupling = k_rot × Δθ + c_rot × Δω
 *
 * 轴重限制（Axle Load Limits）:
 *
 * 各国法规不同，例如（中国标准）:
 * - 单轴: ≤10吨
 * - 双轴: ≤18吨
 * - 三轴: ≤25吨
 *
 * 超载影响:
 * 1. 制动距离增加: d ∝ m
 * 2. 轮胎磨损加剧
 * 3. 悬挂损坏风险
 * 4. 操控性下降
 *
 * 应用场景:
 * 1. 物流运输仿真
 * 2. 驾驶员培训
 * 3. 交通流建模
 * 4. 车辆设计优化
 *
 * 注意:
 * ⚠️ 卡车需要考虑货物质量和质心变化
 * ⚠️ 多轴车辆的转向几何较复杂
 * ⚠️ 制动时容易发生甩尾（尤其空载时）
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
 * 车轴配置
 */
struct AxleConfig {
    PxVec3 position;             // 相对车身的位置
    PxReal trackWidth;           // 轮距
    bool isDriven;               // 是否为驱动轴
    bool canSteer;               // 是否可转向
    PxReal maxSteerAngle;        // 最大转向角（弧度）
    PxReal brakeRatio;           // 制动力分配比例

    AxleConfig()
        : position(0, 0, 0)
        , trackWidth(2.0f)
        , isDriven(false)
        , canSteer(false)
        , maxSteerAngle(0.0f)
        , brakeRatio(0.0f) {}
};

/**
 * 载荷状态
 */
enum class LoadCondition {
    Empty,          // 空载
    HalfLoaded,     // 半载
    FullLoaded      // 满载
};

/**
 * 卡车配置
 */
struct TruckVehicleConfig {
    // 车身参数（空载）
    PxReal emptyMass;            // 空载质量（kg）
    PxReal maxCargoMass;         // 最大货物质量（kg）
    PxVec3 chassisDims;          // 车身尺寸（米）
    PxVec3 emptyCenterOfMass;    // 空载质心
    PxVec3 loadedCenterOfMass;   // 满载质心

    // 车轴配置
    std::vector<AxleConfig> axles;

    // 车轮参数
    PxReal wheelRadius;
    PxReal wheelMass;

    // 动力参数
    PxReal maxTorque;
    PxReal maxBrakeTorque;

    // 悬挂参数
    PxReal suspensionSpringStrength;
    PxReal suspensionSpringDamping;
    PxReal suspensionMaxTravel;

    // 当前载荷
    LoadCondition loadCondition;

    TruckVehicleConfig() {
        // 默认值（3轴重型卡车）
        emptyMass = 8000.0f;         // 8吨空载
        maxCargoMass = 25000.0f;     // 25吨货物
        chassisDims = PxVec3(8.0f, 2.5f, 3.0f);
        emptyCenterOfMass = PxVec3(0, 0, 1.2f);
        loadedCenterOfMass = PxVec3(-0.5f, 0, 1.5f);  // 满载时质心后移、上移

        wheelRadius = 0.5f;
        wheelMass = 50.0f;

        maxTorque = 3000.0f;
        maxBrakeTorque = 15000.0f;

        suspensionSpringStrength = 80000.0f;
        suspensionSpringDamping = 8000.0f;
        suspensionMaxTravel = 0.2f;

        loadCondition = LoadCondition::Empty;

        // 配置3轴：前轴（转向） + 中轴（驱动） + 后轴（驱动）
        AxleConfig frontAxle;
        frontAxle.position = PxVec3(3.0f, 0, 0);
        frontAxle.trackWidth = 2.0f;
        frontAxle.isDriven = false;
        frontAxle.canSteer = true;
        frontAxle.maxSteerAngle = PxPi / 6.0f;  // 30度
        frontAxle.brakeRatio = 0.4f;
        axles.push_back(frontAxle);

        AxleConfig middleAxle;
        middleAxle.position = PxVec3(-1.0f, 0, 0);
        middleAxle.trackWidth = 2.0f;
        middleAxle.isDriven = true;
        middleAxle.canSteer = false;
        middleAxle.brakeRatio = 0.3f;
        axles.push_back(middleAxle);

        AxleConfig rearAxle;
        rearAxle.position = PxVec3(-3.0f, 0, 0);
        rearAxle.trackWidth = 2.0f;
        rearAxle.isDriven = true;
        rearAxle.canSteer = false;
        rearAxle.brakeRatio = 0.3f;
        axles.push_back(rearAxle);
    }

    // 获取当前总质量
    PxReal getTotalMass() const {
        PxReal cargoMass = 0.0f;
        switch (loadCondition) {
            case LoadCondition::Empty: cargoMass = 0.0f; break;
            case LoadCondition::HalfLoaded: cargoMass = maxCargoMass * 0.5f; break;
            case LoadCondition::FullLoaded: cargoMass = maxCargoMass; break;
        }
        return emptyMass + cargoMass;
    }

    // 获取当前质心
    PxVec3 getCenterOfMass() const {
        PxReal cargoRatio = 0.0f;
        switch (loadCondition) {
            case LoadCondition::Empty: cargoRatio = 0.0f; break;
            case LoadCondition::HalfLoaded: cargoRatio = 0.5f; break;
            case LoadCondition::FullLoaded: cargoRatio = 1.0f; break;
        }
        return emptyCenterOfMass + (loadedCenterOfMass - emptyCenterOfMass) * cargoRatio;
    }
};

/**
 * 简化的卡车类
 */
class SimpleTruckVehicle {
public:
    PxRigidDynamic* chassis;
    TruckVehicleConfig config;

    // 控制输入
    PxReal throttle;
    PxReal brake;
    PxReal steer;

    SimpleTruckVehicle() : chassis(nullptr), throttle(0), brake(0), steer(0) {}

    void create(PxPhysics* physics, PxScene* scene, const PxTransform& initialPose,
                const TruckVehicleConfig& cfg) {
        config = cfg;

        // 创建车身
        PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.1f);
        PxBoxGeometry geom(config.chassisDims * 0.5f);

        chassis = physics->createRigidDynamic(initialPose);
        PxShape* shape = physics->createShape(geom, *material);
        chassis->attachShape(*shape);

        // 设置质量和质心
        PxRigidBodyExt::setMassAndUpdateInertia(*chassis, config.getTotalMass());
        chassis->setCMassLocalPose(PxTransform(config.getCenterOfMass()));

        scene->addActor(*chassis);
        shape->release();
        material->release();
    }

    void setControls(PxReal throttleInput, PxReal brakeInput, PxReal steerInput) {
        throttle = PxClamp(throttleInput, 0.0f, 1.0f);
        brake = PxClamp(brakeInput, 0.0f, 1.0f);
        steer = PxClamp(steerInput, -1.0f, 1.0f);
    }

    void setLoadCondition(LoadCondition condition) {
        config.loadCondition = condition;
        // 更新质量和质心
        PxRigidBodyExt::setMassAndUpdateInertia(*chassis, config.getTotalMass());
        chassis->setCMassLocalPose(PxTransform(config.getCenterOfMass()));
    }

    void update(PxReal dt) {
        if (!chassis) return;

        PxScene* scene;
        chassis->getScene(scene);

        // 更新每个车轴
        for (const auto& axle : config.axles) {
            updateAxle(axle, dt, scene);
        }
    }

    // 计算制动距离（估算）
    PxReal estimateBrakingDistance(PxReal initialSpeed) const {
        // d = v² / (2 × μ × g)
        PxReal mu = 0.7f;  // 摩擦系数
        PxReal g = 9.81f;
        return (initialSpeed * initialSpeed) / (2.0f * mu * g);
    }

    // 计算轴载荷
    PxReal calculateAxleLoad(const AxleConfig& axle) const {
        PxReal totalMass = config.getTotalMass();
        PxVec3 cg = config.getCenterOfMass();

        // 静态载荷
        PxReal totalWeight = totalMass * 9.81f;
        PxReal numAxles = config.axles.size();
        PxReal staticLoad = totalWeight / numAxles;  // 简化：均分

        // 动态载荷转移（纵向）
        PxVec3 accel = chassis->getLinearVelocity();  // 简化
        PxReal longitudinalAccel = accel.x;
        PxReal loadTransfer = (totalMass * longitudinalAccel * cg.z) / 6.0f;  // 简化轴距

        // 根据轴位置调整
        if (axle.position.x > 0) {
            // 前轴：制动时增加载荷
            staticLoad += loadTransfer;
        } else {
            // 后轴：制动时减少载荷
            staticLoad -= loadTransfer;
        }

        return PxMax(staticLoad, 0.0f);
    }

private:
    void updateAxle(const AxleConfig& axle, PxReal dt, PxScene* scene) {
        PxTransform chassisTransform = chassis->getGlobalPose();

        // 左右车轮位置
        PxVec3 wheelPositions[2] = {
            axle.position + PxVec3(0, -axle.trackWidth * 0.5f, -config.chassisDims.z * 0.5f),
            axle.position + PxVec3(0, axle.trackWidth * 0.5f, -config.chassisDims.z * 0.5f)
        };

        for (int i = 0; i < 2; ++i) {
            updateWheel(axle, wheelPositions[i], i == 0, dt, scene);
        }
    }

    void updateWheel(const AxleConfig& axle, const PxVec3& localPos, bool isLeft,
                     PxReal dt, PxScene* scene) {
        PxTransform chassisTransform = chassis->getGlobalPose();
        PxVec3 wheelWorldPos = chassisTransform.transform(localPos);

        // 射线检测地面
        PxVec3 rayStart = wheelWorldPos + PxVec3(0, 0, 0.5f);
        PxVec3 rayDir(0, 0, -1);
        PxReal rayLength = config.wheelRadius + config.suspensionMaxTravel + 0.5f;

        PxRaycastBuffer hit;
        bool hasContact = scene->raycast(rayStart, rayDir, rayLength, hit);

        if (!hasContact) return;

        // 计算悬挂力
        PxReal compression = rayLength - hit.block.distance - config.wheelRadius;
        compression = PxClamp(compression, 0.0f, config.suspensionMaxTravel);

        PxReal suspensionForce = compression * config.suspensionSpringStrength;

        PxVec3 chassisVelAtWheel = chassis->getLinearVelocity() +
                                    chassis->getAngularVelocity().cross(localPos);
        PxReal suspensionVel = chassisVelAtWheel.z;
        suspensionForce += suspensionVel * config.suspensionSpringDamping;

        // 施加悬挂力
        chassis->addForceAtPos(PxVec3(0, 0, suspensionForce), wheelWorldPos);

        // 计算车轮方向
        PxVec3 wheelForward = chassisTransform.q.rotate(PxVec3(1, 0, 0));
        PxVec3 wheelRight = chassisTransform.q.rotate(PxVec3(0, 1, 0));

        // 转向
        if (axle.canSteer) {
            PxReal steerAngle = steer * axle.maxSteerAngle;
            PxQuat steerRotation(steerAngle, PxVec3(0, 0, 1));
            wheelForward = steerRotation.rotate(wheelForward);
        }

        // 驱动力
        if (axle.isDriven) {
            PxReal wheelTorque = throttle * config.maxTorque / 2.0f;  // 每轴2个轮
            PxReal driveForce = wheelTorque / config.wheelRadius;
            chassis->addForceAtPos(wheelForward * driveForce, wheelWorldPos);
        }

        // 制动力
        PxReal brakeForce = brake * config.maxBrakeTorque * axle.brakeRatio / config.wheelRadius;
        PxReal wheelSpeed = chassisVelAtWheel.dot(wheelForward);
        if (wheelSpeed > 0.1f) {
            chassis->addForceAtPos(wheelForward * (-brakeForce), wheelWorldPos);
        }

        // 横向摩擦
        PxReal lateralVel = chassisVelAtWheel.dot(wheelRight);
        PxReal lateralFriction = -lateralVel * config.getTotalMass() * 0.1f;
        chassis->addForceAtPos(wheelRight * lateralFriction, wheelWorldPos);
    }
};

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 不同载荷条件下的加速对比
 */
void demonstrateLoadComparison(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景1: 载荷对比 ===" << std::endl;
    std::cout << "对比空载、半载、满载三种情况下的加速性能" << std::endl;

    LoadCondition conditions[3] = {LoadCondition::Empty, LoadCondition::HalfLoaded, LoadCondition::FullLoaded};
    const char* names[3] = {"空载", "半载", "满载"};

    std::cout << "\n载荷条件\t质量(kg)\t5秒后速度(m/s)" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;

    for (int c = 0; c < 3; ++c) {
        TruckVehicleConfig config;
        config.loadCondition = conditions[c];

        SimpleTruckVehicle truck;
        truck.create(physics, scene, PxTransform(PxVec3(c * 15.0f, 0, 2)), config);
        truck.setControls(1.0f, 0.0f, 0.0f);  // 全油门

        const PxReal dt = 1.0f / 60.0f;
        for (int step = 0; step < 300; ++step) {
            truck.update(dt);
            scene->simulate(dt);
            scene->fetchResults(true);
        }

        PxReal finalSpeed = truck.chassis->getLinearVelocity().magnitude();

        std::cout << names[c] << "\t\t"
                  << config.getTotalMass() << "\t\t"
                  << finalSpeed << std::endl;
    }

    std::cout << "\n结论：载荷越重，加速越慢（质量增加，加速度减小）" << std::endl;
}

/**
 * 场景2: 制动距离测试
 */
void demonstrateBrakingDistance(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景2: 制动距离测试 ===" << std::endl;
    std::cout << "测试不同载荷条件下从20m/s到静止的制动距离" << std::endl;

    LoadCondition conditions[3] = {LoadCondition::Empty, LoadCondition::HalfLoaded, LoadCondition::FullLoaded};
    const char* names[3] = {"空载", "半载", "满载"};

    std::cout << "\n载荷\t理论距离(m)\t实际距离(m)\t制动时间(s)" << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;

    const PxReal dt = 1.0f / 60.0f;

    for (int c = 0; c < 3; ++c) {
        TruckVehicleConfig config;
        config.loadCondition = conditions[c];

        SimpleTruckVehicle truck;
        truck.create(physics, scene, PxTransform(PxVec3(c * 30.0f, 0, 2)), config);

        // 先加速到20m/s
        truck.setControls(1.0f, 0.0f, 0.0f);
        for (int step = 0; step < 200; ++step) {
            truck.update(dt);
            scene->simulate(dt);
            scene->fetchResults(true);
        }

        // 设置初速度为20m/s
        PxVec3 forward = truck.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0));
        truck.chassis->setLinearVelocity(forward * 20.0f);

        PxReal initialSpeed = 20.0f;
        PxReal theoreticalDistance = truck.estimateBrakingDistance(initialSpeed);

        // 紧急制动
        truck.setControls(0.0f, 1.0f, 0.0f);

        PxVec3 brakeStartPos = truck.chassis->getGlobalPose().p;
        int brakeSteps = 0;

        while (truck.chassis->getLinearVelocity().magnitude() > 0.5f && brakeSteps < 600) {
            truck.update(dt);
            scene->simulate(dt);
            scene->fetchResults(true);
            brakeSteps++;
        }

        PxReal actualDistance = (truck.chassis->getGlobalPose().p - brakeStartPos).magnitude();
        PxReal brakeTime = brakeSteps * dt;

        std::cout << names[c] << "\t"
                  << theoreticalDistance << "\t\t"
                  << actualDistance << "\t\t"
                  << brakeTime << std::endl;
    }

    std::cout << "\n结论：载荷越重，制动距离越长（惯性更大）" << std::endl;
}

/**
 * 场景3: 多轴车辆转向
 */
void demonstrateMultiAxleSteering(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景3: 多轴车辆转向 ===" << std::endl;
    std::cout << "演示3轴卡车的转向性能" << std::endl;

    TruckVehicleConfig config;
    config.loadCondition = LoadCondition::HalfLoaded;

    SimpleTruckVehicle truck;
    truck.create(physics, scene, PxTransform(PxVec3(0, 0, 2)), config);

    // 中速+左转
    truck.setControls(0.5f, 0.0f, -0.8f);

    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 360;

    std::cout << "\n时间\t位置(x,y)\t\t朝向(度)" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;

    for (int step = 0; step < numSteps; ++step) {
        truck.update(dt);
        scene->simulate(dt);
        scene->fetchResults(true);

        if (step % 90 == 0) {
            PxVec3 pos = truck.chassis->getGlobalPose().p;
            PxVec3 forward = truck.chassis->getGlobalPose().q.rotate(PxVec3(1, 0, 0));
            PxReal heading = PxAtan2(forward.y, forward.x) * 180.0f / PxPi;

            std::cout << step * dt << "s\t"
                      << "(" << pos.x << ", " << pos.y << ")\t"
                      << heading << std::endl;
        }
    }

    std::cout << "\n多轴车辆转向特点：" << std::endl;
    std::cout << "- 转向半径较大（轴距长）" << std::endl;
    std::cout << "- 后轴存在侧滑（不能转向）" << std::endl;
    std::cout << "- 需要更大的转向空间" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: VehicleTruck" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "\n演示卡车物理系统" << std::endl;

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

    // 运行3个场景
    demonstrateLoadComparison(physics, scene);
    demonstrateBrakingDistance(physics, scene);
    demonstrateMultiAxleSteering(physics, scene);

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\n卡车物理关键特性:" << std::endl;
    std::cout << "1. 多轴配置：2-5轴，载荷分配复杂" << std::endl;
    std::cout << "2. 可变载荷：空载vs满载质量差异大（2-5倍）" << std::endl;
    std::cout << "3. 质心变化：载荷影响质心高度和位置" << std::endl;
    std::cout << "4. 制动系统：需要更强的制动力和更长距离" << std::endl;

    std::cout << "\n关键公式:" << std::endl;
    std::cout << "纵向载荷转移: ΔF_z = (m × a × h_cg) / L" << std::endl;
    std::cout << "制动距离: d = v² / (2 × μ × g)" << std::endl;
    std::cout << "轴载荷: F_zi = W × d_i / L_total + ΔF_dynamic" << std::endl;

    std::cout << "\n轴重限制（中国标准）:" << std::endl;
    std::cout << "┌────────────┬──────────────┐" << std::endl;
    std::cout << "│ 轴数       │ 最大载重     │" << std::endl;
    std::cout << "├────────────┼──────────────┤" << std::endl;
    std::cout << "│ 单轴       │ ≤ 10吨       │" << std::endl;
    std::cout << "│ 双轴       │ ≤ 18吨       │" << std::endl;
    std::cout << "│ 三轴       │ ≤ 25吨       │" << std::endl;
    std::cout << "│ 四轴       │ ≤ 31吨       │" << std::endl;
    std::cout << "│ 五轴       │ ≤ 43吨       │" << std::endl;
    std::cout << "└────────────┴──────────────┘" << std::endl;

    std::cout << "\n应用场景:" << std::endl;
    std::cout << "1. 物流仿真 - 运输效率、路线规划" << std::endl;
    std::cout << "2. 驾驶培训 - 重型车辆操控技能" << std::endl;
    std::cout << "3. 安全研究 - 制动距离、侧翻风险" << std::endl;
    std::cout << "4. 车辆设计 - 轴配置、悬挂优化" << std::endl;

    // 清理
    scene->release();
    physics->release();
    foundation->release();

    return 0;
}
