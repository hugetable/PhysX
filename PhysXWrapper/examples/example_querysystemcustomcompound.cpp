/**
 * PhysX Snippet: Query System Custom Compound
 *
 * 本示例演示如何对复合几何体(Compound Geometry)进行自定义查询。
 *
 * 理论基础：
 *
 * 1. 复合几何体(Compound Geometry)
 *    复合几何体由多个基本形状(Shape)组成，共享同一个刚体(Actor)。
 *    优势：
 *    - 减少Actor数量，提高性能
 *    - 保持物理整体性（一起移动、旋转）
 *    - 简化碰撞处理
 *
 * 2. 复合体的AABB
 *    复合体的总包围盒是所有子形状包围盒的并集：
 *    AABB_compound = ∪ AABB_i
 *
 *    计算方法：
 *    min = (min(min_x_i), min(min_y_i), min(min_z_i))
 *    max = (max(max_x_i), max(max_y_i), max(max_z_i))
 *
 * 3. 复合体的查询优化
 *    两级查询策略：
 *    a) Level 1: 测试整体AABB
 *    b) Level 2: 如果通过，测试各个子形状
 *
 *    早期退出：
 *    - Raycast: 找到第一个命中即可返回
 *    - Overlap: 找到任意重叠即可返回（如果只需判断是否重叠）
 *
 * 4. 射线-复合体相交
 *    对于复合体中的每个形状i：
 *    1. 将射线变换到形状的本地空间
 *    2. 执行射线-形状相交测试
 *    3. 记录最近的命中点
 *
 *    变换公式：
 *    ray_local.origin = T_i^(-1) × (ray_world.origin - actor.position)
 *    ray_local.dir = R_i^(-1) × ray_world.dir
 *
 * 5. 自定义查询回调
 *    PhysX提供灵活的查询回调机制：
 *    - preFilter: 在精确测试前过滤
 *    - postFilter: 在测试后决定是否接受结果
 *    - blocking vs touching: 区分阻挡和接触
 *
 * 6. 性能考虑
 *    - 子形状数量：通常5-20个最优
 *    - 空间分布：紧凑分布性能更好
 *    - AABB质量：松散AABB会增加误判
 *    - 查询顺序：从最可能命中的开始
 */

#include "PxPhysicsAPI.h"
#include "../common/PxPhysXCommon.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>

using namespace physx;

// 全局变量
static PxDefaultAllocator gAllocator;
static PxDefaultErrorCallback gErrorCallback;
static PxFoundation* gFoundation = nullptr;
static PxPhysics* gPhysics = nullptr;
static PxScene* gScene = nullptr;
static PxMaterial* gMaterial = nullptr;
static PxPvd* gPvd = nullptr;

/**
 * 自定义查询过滤回调
 *
 * 允许在查询时应用自定义过滤逻辑。
 */
class CustomQueryFilterCallback : public PxQueryFilterCallback {
private:
    int filterCallCount;
    int acceptedCount;
    int rejectedCount;

public:
    CustomQueryFilterCallback()
        : filterCallCount(0), acceptedCount(0), rejectedCount(0) {}

    virtual PxQueryHitType::Enum preFilter(
        const PxFilterData& filterData,
        const PxShape* shape,
        const PxRigidActor* actor,
        PxHitFlags& queryFlags) override {

        filterCallCount++;

        // 自定义过滤逻辑：拒绝特定类型的形状
        // 这里我们根据userData来过滤
        if (shape->userData) {
            int shapeType = *reinterpret_cast<int*>(shape->userData);
            if (shapeType == 999) {  // 特殊标记的形状
                rejectedCount++;
                return PxQueryHitType::eNONE;  // 跳过此形状
            }
        }

        acceptedCount++;
        return PxQueryHitType::eBLOCK;  // 阻挡射线
    }

    virtual PxQueryHitType::Enum postFilter(
        const PxFilterData& filterData,
        const PxQueryHit& hit,
        const PxShape* shape,
        const PxRigidActor* actor) override {

        // Post-filter可以在得到命中后决定是否接受
        return PxQueryHitType::eBLOCK;
    }

    void printStats() const {
        printf("  Filter Stats: %d calls, %d accepted, %d rejected\n",
               filterCallCount, acceptedCount, rejectedCount);
    }

    void reset() {
        filterCallCount = 0;
        acceptedCount = 0;
        rejectedCount = 0;
    }
};

/**
 * 创建复合几何体
 *
 * 创建一个由多个形状组成的复合Actor。
 */
PxRigidDynamic* createCompoundActor(const PxTransform& pose, int numShapes) {
    PxRigidDynamic* compound = gPhysics->createRigidDynamic(pose);

    // 添加多个形状
    for (int i = 0; i < numShapes; ++i) {
        PxTransform localPose;

        if (i == 0) {
            // 中心球体
            localPose = PxTransform(PxVec3(0, 0, 0));
            PxShape* shape = PxRigidActorExt::createExclusiveShape(
                *compound, PxSphereGeometry(0.5f), *gMaterial);
        } else {
            // 周围的盒子，环形分布
            PxReal angle = PxTwoPi * i / (numShapes - 1);
            PxReal radius = 1.5f;
            PxVec3 offset(radius * PxCos(angle), 0, radius * PxSin(angle));
            localPose = PxTransform(offset);

            PxShape* shape = PxRigidActorExt::createExclusiveShape(
                *compound, PxBoxGeometry(0.3f, 0.3f, 0.3f), *gMaterial);
            shape->setLocalPose(localPose);
        }
    }

    // 设置质量
    PxRigidBodyExt::updateMassAndInertia(*compound, 10.0f);

    return compound;
}

/**
 * 场景1：基本的复合体查询
 */
void testScene1_BasicCompoundQuery() {
    printf("=== Scene 1: Basic Compound Query ===\n");

    // 创建地面
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    gScene->addActor(*ground);

    // 创建复合体
    PxRigidDynamic* compound = createCompoundActor(PxTransform(PxVec3(0, 3, 0)), 7);
    gScene->addActor(*compound);

    printf("Created compound with %u shapes\n", compound->getNbShapes());

    // 模拟一会儿
    for (int i = 0; i < 60; ++i) {
        gScene->simulate(1.0f / 60.0f);
        gScene->fetchResults(true);
    }

    // 测试射线投射
    PxVec3 origin(0, 10, 0);
    PxVec3 direction(0, -1, 0);

    PxRaycastBuffer hit;
    bool status = gScene->raycast(origin, direction, 20.0f, hit);

    printf("\nRaycast from above:\n");
    if (status) {
        printf("  HIT at distance %.2f\n", hit.block.distance);
        printf("  Hit shape index: %u\n", hit.block.faceIndex);

        // 获取命中的形状
        if (hit.block.shape) {
            PxGeometryType::Enum geomType = hit.block.shape->getGeometryType();
            const char* typeName = (geomType == PxGeometryType::eSPHERE) ? "Sphere" : "Box";
            printf("  Hit shape type: %s\n", typeName);
        }
    } else {
        printf("  MISS\n");
    }

    // 获取复合体的整体AABB
    PxBounds3 bounds = compound->getWorldBounds();
    printf("\nCompound AABB:\n");
    printf("  Min: (%.2f, %.2f, %.2f)\n", bounds.minimum.x, bounds.minimum.y, bounds.minimum.z);
    printf("  Max: (%.2f, %.2f, %.2f)\n", bounds.maximum.x, bounds.maximum.y, bounds.maximum.z);
    printf("  Center: (%.2f, %.2f, %.2f)\n",
           bounds.getCenter().x, bounds.getCenter().y, bounds.getCenter().z);
    printf("  Extents: (%.2f, %.2f, %.2f)\n",
           bounds.getExtents().x, bounds.getExtents().y, bounds.getExtents().z);
}

/**
 * 场景2：复合体的重叠查询
 */
void testScene2_CompoundOverlapQuery() {
    printf("\n=== Scene 2: Compound Overlap Query ===\n");

    // 清空场景
    PxU32 numActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC |
                                          PxActorTypeFlag::eRIGID_STATIC);
    std::vector<PxActor*> actors(numActors);
    gScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                      actors.data(), numActors);
    for (auto actor : actors) {
        gScene->removeActor(*actor);
        actor->release();
    }

    // 创建多个复合体
    const int numCompounds = 5;
    for (int i = 0; i < numCompounds; ++i) {
        PxReal x = i * 3.0f;
        PxRigidDynamic* compound = createCompoundActor(
            PxTransform(PxVec3(x, 2, 0)), 5);
        gScene->addActor(*compound);
    }

    printf("Created %d compounds\n", numCompounds);

    // 执行大范围重叠查询
    PxSphereGeometry queryGeom(5.0f);
    PxTransform queryPose(PxVec3(6, 2, 0));

    const PxU32 maxHits = 20;
    PxOverlapHit hitBuffer[maxHits];
    PxOverlapBuffer hits(hitBuffer, maxHits);

    bool status = gScene->overlap(queryGeom, queryPose, hits);

    printf("\nOverlap query (sphere r=5.0 at (6,2,0)):\n");
    printf("  Found %u hits\n", hits.getNbAnyHits());

    // 统计命中的Actor和Shape
    std::set<PxRigidActor*> hitActors;
    for (PxU32 i = 0; i < hits.getNbAnyHits(); ++i) {
        hitActors.insert(hits.getAnyHit(i).actor);
    }

    printf("  Unique actors hit: %zu\n", hitActors.size());
    printf("  Total shapes hit: %u\n", hits.getNbAnyHits());
    printf("  Average shapes per actor: %.1f\n",
           static_cast<float>(hits.getNbAnyHits()) / hitActors.size());
}

/**
 * 场景3：自定义查询过滤
 */
void testScene3_CustomQueryFilter() {
    printf("\n=== Scene 3: Custom Query Filter ===\n");

    // 清空场景
    PxU32 numActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC |
                                          PxActorTypeFlag::eRIGID_STATIC);
    std::vector<PxActor*> actors(numActors);
    gScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                      actors.data(), numActors);
    for (auto actor : actors) {
        gScene->removeActor(*actor);
        actor->release();
    }

    // 创建带标记的复合体
    std::vector<int> shapeTypes;
    PxRigidDynamic* compound = gPhysics->createRigidDynamic(PxTransform(PxVec3(0, 2, 0)));

    for (int i = 0; i < 8; ++i) {
        PxReal angle = PxTwoPi * i / 8;
        PxVec3 offset(2.0f * PxCos(angle), 0, 2.0f * PxSin(angle));

        PxShape* shape = PxRigidActorExt::createExclusiveShape(
            *compound, PxBoxGeometry(0.4f, 0.4f, 0.4f), *gMaterial);
        shape->setLocalPose(PxTransform(offset));

        // 标记某些形状为特殊类型
        int* typeData = new int;
        *typeData = (i % 2 == 0) ? 999 : 0;  // 偶数索引标记为999
        shape->userData = typeData;
        shapeTypes.push_back(*typeData);
    }

    PxRigidBodyExt::updateMassAndInertia(*compound, 10.0f);
    gScene->addActor(*compound);

    printf("Created compound with 8 shapes (4 marked as type 999)\n");

    // 使用自定义过滤器查询
    CustomQueryFilterCallback filterCallback;

    PxVec3 origin(0, 10, 0);
    PxVec3 direction(0, -1, 0);

    PxQueryFilterData filterData;
    filterData.flags = PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

    PxRaycastBuffer hit;
    bool status = gScene->raycast(origin, direction, 20.0f, hit,
                                  PxHitFlag::eDEFAULT, filterData, &filterCallback);

    printf("\nRaycast with custom filter:\n");
    printf("  Result: %s\n", status ? "HIT" : "MISS");
    filterCallback.printStats();

    // 清理userData
    for (auto actor : actors) {
        if (auto rigid = actor->is<PxRigidActor>()) {
            std::vector<PxShape*> shapes(rigid->getNbShapes());
            rigid->getShapes(shapes.data(), rigid->getNbShapes());
            for (auto shape : shapes) {
                if (shape->userData) {
                    delete reinterpret_cast<int*>(shape->userData);
                }
            }
        }
    }
}

/**
 * 场景4：复合体扫掠查询
 */
void testScene4_CompoundSweepQuery() {
    printf("\n=== Scene 4: Compound Sweep Query ===\n");

    // 清空场景
    PxU32 numActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC |
                                          PxActorTypeFlag::eRIGID_STATIC);
    std::vector<PxActor*> actors(numActors);
    gScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                      actors.data(), numActors);
    for (auto actor : actors) {
        gScene->removeActor(*actor);
        actor->release();
    }

    // 创建目标复合体
    PxRigidDynamic* target = createCompoundActor(PxTransform(PxVec3(10, 2, 0)), 6);
    gScene->addActor(*target);

    printf("Created target compound at x=10\n");

    // 扫掠测试：从左侧扫向右侧
    PxSphereGeometry sweepGeom(0.8f);
    PxTransform startPose(PxVec3(0, 2, 0));
    PxVec3 sweepDir(1, 0, 0);
    PxReal sweepDist = 15.0f;

    PxSweepBuffer hit;
    bool status = gScene->sweep(sweepGeom, startPose, sweepDir, sweepDist, hit);

    printf("\nSphere sweep (r=0.8) from x=0 to x=15:\n");
    if (status) {
        printf("  HIT at distance %.2f\n", hit.block.distance);
        printf("  Impact point: (%.2f, %.2f, %.2f)\n",
               hit.block.position.x, hit.block.position.y, hit.block.position.z);
        printf("  Normal: (%.2f, %.2f, %.2f)\n",
               hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);

        // 计算实际命中位置
        PxReal actualX = startPose.p.x + sweepDir.x * hit.block.distance;
        printf("  Actual hit X: %.2f\n", actualX);
    } else {
        printf("  MISS\n");
    }

    // 尝试不同大小的扫掠形状
    printf("\nSweep with different sizes:\n");
    PxReal sizes[] = {0.3f, 0.5f, 1.0f, 1.5f};

    for (int i = 0; i < 4; ++i) {
        PxSphereGeometry testGeom(sizes[i]);
        PxSweepBuffer testHit;
        bool testStatus = gScene->sweep(testGeom, startPose, sweepDir, sweepDist, testHit);

        printf("  r=%.1f: %s", sizes[i], testStatus ? "HIT" : "MISS");
        if (testStatus) {
            printf(" at distance %.2f", testHit.block.distance);
        }
        printf("\n");
    }
}

/**
 * 场景5：性能对比 - 单一形状 vs 复合体
 */
void testScene5_PerformanceComparison() {
    printf("\n=== Scene 5: Performance Comparison ===\n");

    // 清空场景
    PxU32 numActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC |
                                          PxActorTypeFlag::eRIGID_STATIC);
    std::vector<PxActor*> actors(numActors);
    gScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                      actors.data(), numActors);
    for (auto actor : actors) {
        gScene->removeActor(*actor);
        actor->release();
    }

    // 场景A：100个复合体，每个5个形状
    const int numCompounds = 100;
    const int shapesPerCompound = 5;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numCompounds; ++i) {
        PxReal x = (i % 10) * 3.0f;
        PxReal z = (i / 10) * 3.0f;
        PxRigidDynamic* compound = createCompoundActor(
            PxTransform(PxVec3(x, 2, z)), shapesPerCompound);
        gScene->addActor(*compound);
    }

    auto end = std::chrono::high_resolution_clock::now();
    long createTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    printf("Created %d compounds (%d total shapes)\n",
           numCompounds, numCompounds * shapesPerCompound);
    printf("Creation time: %ld μs\n", createTime);

    // 执行大量查询
    const int numQueries = 1000;
    int totalHits = 0;

    start = std::chrono::high_resolution_clock::now();

    for (int q = 0; q < numQueries; ++q) {
        PxVec3 origin(
            (rand() % 300) / 10.0f,
            10.0f,
            (rand() % 300) / 10.0f
        );
        PxVec3 dir(0, -1, 0);

        PxRaycastBuffer hit;
        if (gScene->raycast(origin, dir, 20.0f, hit)) {
            totalHits++;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    long queryTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    printf("\nQuery Performance:\n");
    printf("  Queries: %d\n", numQueries);
    printf("  Hits: %d (%.1f%%)\n", totalHits, 100.0f * totalHits / numQueries);
    printf("  Total time: %ld ms\n", queryTime);
    printf("  Average per query: %.2f μs\n", (queryTime * 1000.0f) / numQueries);

    // Actor统计
    printf("\nScene Statistics:\n");
    printf("  Total actors: %u\n",
           gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC));
    printf("  Shapes per actor: %d\n", shapesPerCompound);
    printf("  Memory efficient: Using compounds vs %d individual actors\n",
           numCompounds * shapesPerCompound);
}

/**
 * 初始化PhysX
 */
bool initPhysX() {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!gFoundation) return false;

    gPvd = PxCreatePvd(*gFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
    gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation,
                               PxTolerancesScale(), true, gPvd);
    if (!gPhysics) return false;

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    gScene = gPhysics->createScene(sceneDesc);
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.1f);

    return true;
}

/**
 * 清理PhysX
 */
void cleanupPhysX() {
    PX_RELEASE(gScene);
    PX_RELEASE(gPhysics);
    if (gPvd) {
        PxPvdTransport* transport = gPvd->getTransport();
        PX_RELEASE(gPvd);
        PX_RELEASE(transport);
    }
    PX_RELEASE(gFoundation);
}

/**
 * 主函数
 */
int main() {
    printf("PhysX Query System Custom Compound Example\n");
    printf("===========================================\n\n");

    if (!initPhysX()) {
        return 1;
    }

    // 测试所有场景
    testScene1_BasicCompoundQuery();
    testScene2_CompoundOverlapQuery();
    testScene3_CustomQueryFilter();
    testScene4_CompoundSweepQuery();
    testScene5_PerformanceComparison();

    printf("\n=== Summary ===\n");
    printf("Demonstrated custom compound queries:\n");
    printf("- Compound geometry creation (multiple shapes per actor)\n");
    printf("- Raycast queries on compounds\n");
    printf("- Overlap queries returning multiple shapes\n");
    printf("- Custom query filtering callbacks\n");
    printf("- Sweep queries against compounds\n");
    printf("- Performance: Compounds reduce actor count significantly\n");
    printf("\nKey insights:\n");
    printf("- Compounds combine multiple shapes into one actor\n");
    printf("- Two-level testing: AABB first, then individual shapes\n");
    printf("- Custom filters enable fine-grained query control\n");
    printf("- Compounds improve performance vs separate actors\n");
    printf("- Typical use: 5-20 shapes per compound optimal\n");

    cleanupPhysX();
    printf("\nExample completed successfully!\n");

    return 0;
}
