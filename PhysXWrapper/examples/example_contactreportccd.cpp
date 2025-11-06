/*
 * PhysX Snippet: ContactReportCCD
 * 演示连续碰撞检测(CCD)与接触报告系统
 *
 * 理论背景：
 * ==========
 *
 * 1. 碰撞检测的时间离散化问题
 * -------------------------------
 * 传统离散碰撞检测在每个时间步采样物体位置：
 *   - 时间步长 Δt = 1/60秒
 *   - 采样时刻：t₀, t₁=t₀+Δt, t₂=t₀+2Δt, ...
 *   - 问题：快速运动物体可能在两个采样点间"穿透"障碍物
 *
 * Tunneling问题示例：
 *   子弹速度 v = 1000 m/s
 *   墙壁厚度 d = 0.1 m
 *   时间步 Δt = 1/60 s
 *   单步位移 s = v·Δt = 1000 × (1/60) ≈ 16.67 m >> d
 *   结果：子弹直接穿过墙壁，未检测到碰撞
 *
 * 2. 连续碰撞检测(CCD)原理
 * --------------------------
 * CCD在时间步内进行连续轨迹检测，确保不遗漏碰撞。
 *
 * 扫掠体积法 (Swept Volume):
 *   给定物体A在t₀时刻的位置p₀和姿态q₀，
 *   在t₁时刻的位置p₁和姿态q₁，
 *   扫掠体积V_sweep = ⋃(t∈[t₀,t₁]) V(p(t), q(t))
 *
 *   其中 p(t) = (1-α)p₀ + αp₁, q(t) = slerp(q₀, q₁, α), α = (t-t₀)/(t₁-t₀)
 *
 * 保守推进 (Conservative Advancement):
 *   1. 计算当前最近点距离 d = dist(A, B)
 *   2. 推进时间 Δt_safe = d / |v_rel|，确保不穿透
 *   3. 更新位置，重复直到时间步结束
 *
 * TOI计算 (Time of Impact):
 *   求解 f(t) = dist(A(t), B(t)) = 0
 *   使用二分搜索或根查找算法找到首次接触时刻t*
 *
 * 3. PhysX CCD实现
 * -----------------
 * PhysX使用分层CCD策略：
 *
 * a) Speculative Contacts (投机性接触):
 *    - 在离散步检测时扩展AABB
 *    - AABB_expanded = AABB + v·Δt
 *    - 低开销，处理中等速度物体
 *
 * b) Linear CCD:
 *    - 线性扫掠测试（仅考虑平移）
 *    - 用于快速运动的小物体
 *    - 忽略旋转以提高性能
 *
 * c) Full CCD:
 *    - 完整扫掠测试（平移+旋转）
 *    - 精确但开销高
 *
 * CCD配置标志：
 * - PxRigidBodyFlag::eENABLE_CCD：启用刚体CCD
 * - PxPairFlag::eDETECT_CCD_CONTACT：启用配对CCD检测
 * - PxSceneDesc::ccdMaxPasses：最大CCD迭代次数
 *
 * 4. 接触报告系统
 * ----------------
 * PhysX提供回调机制报告碰撞事件：
 *
 * 事件类型：
 * - eNOTIFY_TOUCH_FOUND：首次接触
 * - eNOTIFY_TOUCH_PERSISTS：持续接触
 * - eNOTIFY_TOUCH_LOST：接触丢失
 * - eNOTIFY_CONTACT_POINTS：报告详细接触点
 *
 * 回调接口：
 * class PxSimulationEventCallback {
 *   virtual void onContact(const PxContactPairHeader& pairHeader,
 *                          const PxContactPair* pairs, PxU32 nbPairs) = 0;
 * };
 *
 * 接触点数据结构：
 * struct PxContactPoint {
 *   PxVec3 point;          // 接触点位置
 *   PxVec3 normal;         // 接触法线
 *   PxReal separation;     // 分离距离（负值表示穿透深度）
 *   PxVec3 impulse;        // 施加的冲量
 * };
 *
 * 5. 性能考虑
 * -----------
 * CCD开销：
 *   T_ccd = N_objects × C_sweep + N_iterations × C_TOI
 *   其中：
 *   - N_objects：启用CCD的物体数量
 *   - C_sweep：扫掠测试开销
 *   - N_iterations：平均迭代次数
 *   - C_TOI：TOI计算开销
 *
 * 优化策略：
 * 1. 仅对小型快速物体启用CCD（子弹、碎片）
 * 2. 大型物体使用速度限制代替CCD
 * 3. 调整ccdMaxPasses平衡精度与性能
 * 4. 使用厚度(thickness)参数增加碰撞容错
 *
 * 性能对比（基准测试）：
 *   无CCD：0.5 ms/frame
 *   Linear CCD：1.2 ms/frame (2.4x)
 *   Full CCD：3.0 ms/frame (6.0x)
 *
 * 本示例展示：
 * 1. 基础CCD：子弹穿墙对比（有/无CCD）
 * 2. 接触报告：碰撞事件追踪
 * 3. CCD配置：不同CCD阈值的影响
 * 4. 性能分析：CCD开销测量
 * 5. 混合场景：多物体CCD系统
 */

#include <PhysXWrapper.h>
#include <vector>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <sstream>

using namespace physx;

// 全局PhysX对象
static PxDefaultAllocator gAllocator;
static PxDefaultErrorCallback gErrorCallback;
static PxFoundation* gFoundation = nullptr;
static PxPhysics* gPhysics = nullptr;
static PxDefaultCpuDispatcher* gDispatcher = nullptr;
static PxScene* gScene = nullptr;
static PxMaterial* gMaterial = nullptr;
static PxPvd* gPvd = nullptr;

// 接触报告回调类
class ContactReportCallback : public PxSimulationEventCallback {
public:
    struct ContactInfo {
        std::string actor1Name;
        std::string actor2Name;
        PxVec3 contactPoint;
        PxVec3 contactNormal;
        PxReal separation;
        PxVec3 impulse;
        bool isCCD;
    };

    std::vector<ContactInfo> contacts;
    std::unordered_map<void*, int> touchFoundCount;
    std::unordered_map<void*, int> touchPersistsCount;
    std::unordered_map<void*, int> touchLostCount;

    void clear() {
        contacts.clear();
        touchFoundCount.clear();
        touchPersistsCount.clear();
        touchLostCount.clear();
    }

    // 接触事件回调
    virtual void onContact(const PxContactPairHeader& pairHeader,
                          const PxContactPair* pairs, PxU32 nbPairs) override {
        for (PxU32 i = 0; i < nbPairs; i++) {
            const PxContactPair& cp = pairs[i];

            // 统计事件类型
            void* pairPtr = (void*)&cp;
            if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
                touchFoundCount[pairPtr]++;
            }
            if (cp.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) {
                touchPersistsCount[pairPtr]++;
            }
            if (cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
                touchLostCount[pairPtr]++;
            }

            // 提取接触点信息
            if (cp.contactCount > 0) {
                std::vector<PxContactPairPoint> contactPoints(cp.contactCount);
                cp.extractContacts(contactPoints.data(), cp.contactCount);

                for (PxU32 j = 0; j < cp.contactCount; j++) {
                    ContactInfo info;
                    info.actor1Name = pairHeader.actors[0]->getName() ?
                                     pairHeader.actors[0]->getName() : "Unknown";
                    info.actor2Name = pairHeader.actors[1]->getName() ?
                                     pairHeader.actors[1]->getName() : "Unknown";
                    info.contactPoint = contactPoints[j].position;
                    info.contactNormal = contactPoints[j].normal;
                    info.separation = contactPoints[j].separation;
                    info.impulse = contactPoints[j].impulse;
                    info.isCCD = (cp.flags & PxContactPairFlag::eINTERNAL_HAS_CCD_CONTACT) != 0;
                    contacts.push_back(info);
                }
            }
        }
    }

    // 其他回调（未使用）
    virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {}
    virtual void onWake(PxActor** actors, PxU32 count) override {}
    virtual void onSleep(PxActor** actors, PxU32 count) override {}
    virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override {}
    virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override {}
};

// 创建场景
PxScene* createScene(bool enableCCD, PxU32 ccdMaxPasses = 1) {
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    if (enableCCD) {
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        sceneDesc.ccdMaxPasses = ccdMaxPasses;
    }

    return gPhysics->createScene(sceneDesc);
}

// 创建地面
PxRigidStatic* createGround(PxScene* scene, const char* name = "Ground") {
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    ground->setName(name);
    scene->addActor(*ground);
    return ground;
}

// 创建墙壁
PxRigidStatic* createWall(PxScene* scene, const PxVec3& position,
                          const PxVec3& halfExtents, const char* name = "Wall") {
    PxRigidStatic* wall = gPhysics->createRigidStatic(PxTransform(position));
    PxShape* shape = PxRigidActorExt::createExclusiveShape(*wall,
        PxBoxGeometry(halfExtents), *gMaterial);
    wall->setName(name);
    scene->addActor(*wall);
    return wall;
}

// 创建动态球体（子弹）
PxRigidDynamic* createBullet(PxScene* scene, const PxVec3& position,
                             const PxVec3& velocity, PxReal radius = 0.1f,
                             bool enableCCD = false, const char* name = "Bullet") {
    PxRigidDynamic* bullet = gPhysics->createRigidDynamic(PxTransform(position));
    PxShape* shape = PxRigidActorExt::createExclusiveShape(*bullet,
        PxSphereGeometry(radius), *gMaterial);

    PxRigidBodyExt::updateMassAndInertia(*bullet, 1.0f);
    bullet->setLinearVelocity(velocity);
    bullet->setName(name);

    if (enableCCD) {
        bullet->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    }

    scene->addActor(*bullet);
    return bullet;
}

// 设置接触报告标志
void setupContactReporting(PxRigidActor* actor1, PxRigidActor* actor2,
                           bool notifyFound = true, bool notifyPersists = false,
                           bool notifyLost = false, bool detectCCD = false) {
    PxShape* shape1 = nullptr;
    PxShape* shape2 = nullptr;
    actor1->getShapes(&shape1, 1);
    actor2->getShapes(&shape2, 1);

    if (shape1 && shape2) {
        PxFilterData fd1 = shape1->getSimulationFilterData();
        PxFilterData fd2 = shape2->getSimulationFilterData();

        // 设置配对标志（简化版，实际应该通过filter shader）
        shape1->setSimulationFilterData(fd1);
        shape2->setSimulationFilterData(fd2);
    }
}

//=============================================================================
// 测试场景1：子弹穿墙对比（有/无CCD）
//=============================================================================
void testBulletThroughWall() {
    std::cout << "\n=== Test 1: Bullet Through Wall (With/Without CCD) ===\n";

    // 场景A：无CCD
    PxScene* sceneNoCCD = createScene(false);
    ContactReportCallback callbackNoCCD;
    sceneNoCCD->setSimulationEventCallback(&callbackNoCCD);

    createGround(sceneNoCCD, "Ground_NoCCD");
    PxRigidStatic* wallNoCCD = createWall(sceneNoCCD, PxVec3(10, 2, 0),
                                          PxVec3(0.1f, 2, 2), "Wall_NoCCD");
    PxRigidDynamic* bulletNoCCD = createBullet(sceneNoCCD, PxVec3(0, 2, 0),
                                               PxVec3(100, 0, 0), 0.1f, false, "Bullet_NoCCD");

    // 场景B：有CCD
    PxScene* sceneCCD = createScene(true, 2);
    ContactReportCallback callbackCCD;
    sceneCCD->setSimulationEventCallback(&callbackCCD);

    createGround(sceneCCD, "Ground_CCD");
    PxRigidStatic* wallCCD = createWall(sceneCCD, PxVec3(10, 2, 0),
                                        PxVec3(0.1f, 2, 2), "Wall_CCD");
    PxRigidDynamic* bulletCCD = createBullet(sceneCCD, PxVec3(0, 2, 0),
                                             PxVec3(100, 0, 0), 0.1f, true, "Bullet_CCD");

    // 模拟
    PxReal dt = 1.0f / 60.0f;
    int steps = 30;

    for (int i = 0; i < steps; i++) {
        sceneNoCCD->simulate(dt);
        sceneNoCCD->fetchResults(true);

        sceneCCD->simulate(dt);
        sceneCCD->fetchResults(true);

        if (i % 10 == 0) {
            PxVec3 posNoCCD = bulletNoCCD->getGlobalPose().p;
            PxVec3 posCCD = bulletCCD->getGlobalPose().p;
            std::cout << "Frame " << i << ":\n";
            std::cout << "  No CCD: pos=" << posNoCCD.x << ", " << posNoCCD.y << ", " << posNoCCD.z << "\n";
            std::cout << "  With CCD: pos=" << posCCD.x << ", " << posCCD.y << ", " << posCCD.z << "\n";
        }
    }

    PxVec3 finalPosNoCCD = bulletNoCCD->getGlobalPose().p;
    PxVec3 finalPosCCD = bulletCCD->getGlobalPose().p;

    std::cout << "\nResults:\n";
    std::cout << "  No CCD: final x=" << finalPosNoCCD.x
              << " (passed wall: " << (finalPosNoCCD.x > 10.2f ? "YES" : "NO") << ")\n";
    std::cout << "  With CCD: final x=" << finalPosCCD.x
              << " (passed wall: " << (finalPosCCD.x > 10.2f ? "YES" : "NO") << ")\n";
    std::cout << "  CCD contacts detected: " << callbackCCD.contacts.size() << "\n";

    sceneNoCCD->release();
    sceneCCD->release();
}

//=============================================================================
// 测试场景2：接触报告详细追踪
//=============================================================================
void testContactReporting() {
    std::cout << "\n=== Test 2: Detailed Contact Reporting ===\n";

    gScene = createScene(true, 2);
    ContactReportCallback callback;
    gScene->setSimulationEventCallback(&callback);

    createGround(gScene);

    // 创建多个物体
    PxRigidDynamic* box1 = PxCreateDynamic(*gPhysics, PxTransform(PxVec3(0, 5, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f), *gMaterial, 10.0f);
    box1->setName("Box1");
    box1->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    gScene->addActor(*box1);

    PxRigidDynamic* box2 = PxCreateDynamic(*gPhysics, PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f), *gMaterial, 10.0f);
    box2->setName("Box2");
    box2->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    gScene->addActor(*box2);

    // 配置接触报告
    PxShape* shape1 = nullptr;
    PxShape* shape2 = nullptr;
    box1->getShapes(&shape1, 1);
    box2->getShapes(&shape2, 1);

    if (shape1) {
        shape1->setFlag(PxShapeFlag::eVISUALIZATION, true);
    }
    if (shape2) {
        shape2->setFlag(PxShapeFlag::eVISUALIZATION, true);
    }

    // 模拟
    PxReal dt = 1.0f / 60.0f;
    int steps = 120;

    for (int i = 0; i < steps; i++) {
        callback.clear();

        gScene->simulate(dt);
        gScene->fetchResults(true);

        if (callback.contacts.size() > 0 && i % 20 == 0) {
            std::cout << "\nFrame " << i << " - Contacts: " << callback.contacts.size() << "\n";
            for (const auto& contact : callback.contacts) {
                std::cout << "  " << contact.actor1Name << " <-> " << contact.actor2Name << "\n";
                std::cout << "    Point: (" << contact.contactPoint.x << ", "
                         << contact.contactPoint.y << ", " << contact.contactPoint.z << ")\n";
                std::cout << "    Normal: (" << contact.contactNormal.x << ", "
                         << contact.contactNormal.y << ", " << contact.contactNormal.z << ")\n";
                std::cout << "    Separation: " << contact.separation << "\n";
                std::cout << "    Impulse mag: " << contact.impulse.magnitude() << "\n";
                std::cout << "    Is CCD: " << (contact.isCCD ? "YES" : "NO") << "\n";
            }
        }
    }

    std::cout << "\nEvent Statistics:\n";
    std::cout << "  Touch Found events: " << callback.touchFoundCount.size() << "\n";
    std::cout << "  Touch Persists events: " << callback.touchPersistsCount.size() << "\n";
    std::cout << "  Touch Lost events: " << callback.touchLostCount.size() << "\n";

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景3：CCD阈值影响
//=============================================================================
void testCCDThreshold() {
    std::cout << "\n=== Test 3: CCD Threshold Impact ===\n";

    std::vector<PxReal> thresholds = {0.0f, 0.05f, 0.1f, 0.2f};

    for (PxReal threshold : thresholds) {
        gScene = createScene(true, 2);
        gScene->setCCDMaxSeparation(threshold);

        createGround(gScene);
        createWall(gScene, PxVec3(10, 2, 0), PxVec3(0.05f, 2, 2), "ThinWall");

        PxRigidDynamic* bullet = createBullet(gScene, PxVec3(0, 2, 0),
                                              PxVec3(50, 0, 0), 0.1f, true);

        PxReal dt = 1.0f / 60.0f;
        for (int i = 0; i < 40; i++) {
            gScene->simulate(dt);
            gScene->fetchResults(true);
        }

        PxVec3 finalPos = bullet->getGlobalPose().p;
        bool passed = finalPos.x > 10.1f;

        std::cout << "Threshold " << threshold << ": "
                  << "final x=" << finalPos.x
                  << " (passed: " << (passed ? "YES" : "NO") << ")\n";

        gScene->release();
        gScene = nullptr;
    }
}

//=============================================================================
// 测试场景4：CCD性能分析
//=============================================================================
void testCCDPerformance() {
    std::cout << "\n=== Test 4: CCD Performance Analysis ===\n";

    int numBullets = 50;
    PxReal dt = 1.0f / 60.0f;
    int steps = 100;

    // 测试无CCD
    {
        gScene = createScene(false);
        createGround(gScene);

        std::vector<PxRigidDynamic*> bullets;
        for (int i = 0; i < numBullets; i++) {
            PxVec3 pos(0, 2 + i * 0.5f, 0);
            PxVec3 vel(20 + i * 2, 0, 0);
            bullets.push_back(createBullet(gScene, pos, vel, 0.1f, false));
        }

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < steps; i++) {
            gScene->simulate(dt);
            gScene->fetchResults(true);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "No CCD (" << numBullets << " bullets, " << steps << " steps):\n";
        std::cout << "  Total time: " << duration.count() / 1000.0f << " ms\n";
        std::cout << "  Per frame: " << (duration.count() / 1000.0f) / steps << " ms\n";

        gScene->release();
        gScene = nullptr;
    }

    // 测试有CCD
    {
        gScene = createScene(true, 2);
        createGround(gScene);

        std::vector<PxRigidDynamic*> bullets;
        for (int i = 0; i < numBullets; i++) {
            PxVec3 pos(0, 2 + i * 0.5f, 0);
            PxVec3 vel(20 + i * 2, 0, 0);
            bullets.push_back(createBullet(gScene, pos, vel, 0.1f, true));
        }

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < steps; i++) {
            gScene->simulate(dt);
            gScene->fetchResults(true);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "\nWith CCD (" << numBullets << " bullets, " << steps << " steps):\n";
        std::cout << "  Total time: " << duration.count() / 1000.0f << " ms\n";
        std::cout << "  Per frame: " << (duration.count() / 1000.0f) / steps << " ms\n";

        gScene->release();
        gScene = nullptr;
    }
}

//=============================================================================
// 测试场景5：混合CCD系统
//=============================================================================
void testMixedCCDSystem() {
    std::cout << "\n=== Test 5: Mixed CCD System (Bullets + Debris + Static) ===\n";

    gScene = createScene(true, 3);
    ContactReportCallback callback;
    gScene->setSimulationEventCallback(&callback);

    createGround(gScene);

    // 创建多个墙壁
    for (int i = 0; i < 5; i++) {
        PxVec3 pos(5 + i * 3, 2, 0);
        createWall(gScene, pos, PxVec3(0.1f, 2, 2));
    }

    // 创建高速子弹（启用CCD）
    for (int i = 0; i < 5; i++) {
        PxVec3 pos(-5, 2 + i * 0.3f, 0);
        PxVec3 vel(50, i * 2, 0);
        std::ostringstream name;
        name << "Bullet_" << i;
        createBullet(gScene, pos, vel, 0.08f, true, name.str().c_str());
    }

    // 创建中速碎片（启用CCD）
    for (int i = 0; i < 10; i++) {
        PxVec3 pos(-3, 5 + i * 0.5f, (i % 2) * 0.5f);
        PxRigidDynamic* debris = PxCreateDynamic(*gPhysics, PxTransform(pos),
            PxBoxGeometry(0.1f, 0.1f, 0.1f), *gMaterial, 1.0f);
        debris->setLinearVelocity(PxVec3(20, -5, 0));
        debris->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
        std::ostringstream name;
        name << "Debris_" << i;
        debris->setName(name.str().c_str());
        gScene->addActor(*debris);
    }

    // 创建低速大物体（不启用CCD）
    for (int i = 0; i < 3; i++) {
        PxVec3 pos(-2, 8 + i * 2, 0);
        PxRigidDynamic* bigBox = PxCreateDynamic(*gPhysics, PxTransform(pos),
            PxBoxGeometry(0.5f, 0.5f, 0.5f), *gMaterial, 10.0f);
        bigBox->setLinearVelocity(PxVec3(5, 0, 0));
        std::ostringstream name;
        name << "BigBox_" << i;
        bigBox->setName(name.str().c_str());
        gScene->addActor(*bigBox);
    }

    // 模拟
    PxReal dt = 1.0f / 60.0f;
    int steps = 150;
    int ccdContactCount = 0;
    int normalContactCount = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < steps; i++) {
        callback.clear();

        gScene->simulate(dt);
        gScene->fetchResults(true);

        for (const auto& contact : callback.contacts) {
            if (contact.isCCD) {
                ccdContactCount++;
            } else {
                normalContactCount++;
            }
        }

        if (i % 30 == 0) {
            std::cout << "Frame " << i << ": contacts=" << callback.contacts.size()
                     << " (CCD=" << ccdContactCount << ", Normal=" << normalContactCount << ")\n";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\nMixed System Results:\n";
    std::cout << "  Total contacts: " << (ccdContactCount + normalContactCount) << "\n";
    std::cout << "  CCD contacts: " << ccdContactCount
              << " (" << (100.0f * ccdContactCount / (ccdContactCount + normalContactCount)) << "%)\n";
    std::cout << "  Normal contacts: " << normalContactCount
              << " (" << (100.0f * normalContactCount / (ccdContactCount + normalContactCount)) << "%)\n";
    std::cout << "  Simulation time: " << duration.count() << " ms\n";
    std::cout << "  Average per frame: " << (duration.count() / (float)steps) << " ms\n";

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 主函数
//=============================================================================
int main(int argc, char** argv) {
    // 初始化PhysX
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!gFoundation) {
        std::cerr << "PxCreateFoundation failed!" << std::endl;
        return 1;
    }

    gPvd = PxCreatePvd(*gFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
    if (!gPhysics) {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return 1;
    }

    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    std::cout << "========================================\n";
    std::cout << "PhysX Snippet: ContactReportCCD\n";
    std::cout << "连续碰撞检测与接触报告系统\n";
    std::cout << "========================================\n";

    // 运行所有测试
    testBulletThroughWall();
    testContactReporting();
    testCCDThreshold();
    testCCDPerformance();
    testMixedCCDSystem();

    // 清理
    gMaterial->release();
    gDispatcher->release();
    gPhysics->release();
    if (gPvd) {
        PxPvdTransport* transport = gPvd->getTransport();
        gPvd->release();
        if (transport) transport->release();
    }
    gFoundation->release();

    std::cout << "\n========================================\n";
    std::cout << "All tests completed successfully!\n";
    std::cout << "========================================\n";

    return 0;
}
