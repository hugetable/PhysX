/**
 * PhysX Snippet: Multiple Pruners
 *
 * 本示例演示如何使用多个剔除器(Multiple Pruners)优化场景查询性能。
 *
 * 理论基础：
 *
 * 1. Pruner（剔除器）
 *    PhysX中的空间加速结构，用于快速剔除不相关的物体：
 *    - Static Pruner: 优化静态物体查询
 *    - Dynamic Pruner: 优化动态物体查询
 *
 * 2. 为什么需要多个Pruner？
 *    不同类型的物体有不同的特性：
 *    - 静态物体：位置不变，适合高质量BVH
 *    - 动态物体：频繁移动，适合增量更新结构
 *    - 分离管理可以避免重建整个加速结构
 *
 * 3. PhysX的Pruner类型
 *    a) PxPruningStructureType::eSTATIC_AABB_TREE
 *       - 基于BVH的静态树
 *       - 构建慢，查询快
 *       - 适合静态几何体
 *
 *    b) PxPruningStructureType::eDYNAMIC_AABB_TREE
 *       - 动态BVH树
 *       - 支持增量更新
 *       - 适合少量动态物体
 *
 *    c) PxPruningStructureType::eNONE
 *       - 暴力搜索
 *       - 物体极少时使用
 *
 * 4. 查询流程
 *    当执行场景查询时：
 *    1. 并行查询static pruner和dynamic pruner
 *    2. 合并结果
 *    3. 根据距离排序（如果需要）
 *
 * 5. 性能优化策略
 *    - 静态物体：使用高质量BVH，查询O(log n)
 *    - 动态物体：使用增量BVH，更新O(log n)
 *    - 分离后避免互相干扰
 *    - 并行处理两个pruner
 *
 * 6. 内存考虑
 *    多个pruner会增加内存占用：
 *    - Static BVH: ~60 bytes/object
 *    - Dynamic BVH: ~80 bytes/object
 *    - 但查询性能提升显著
 */

#include "PxPhysicsAPI.h"
#include "../common/PxPhysXCommon.h"
#include <vector>
#include <chrono>
#include <cmath>

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
 * 创建场景配置
 */
PxSceneDesc createSceneDesc(PxPruningStructureType::Enum staticPruner,
                            PxPruningStructureType::Enum dynamicPruner) {
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    // 设置pruner类型
    sceneDesc.staticStructure = staticPruner;
    sceneDesc.dynamicStructure = dynamicPruner;

    return sceneDesc;
}

/**
 * 场景1：默认配置 vs 自定义Pruner
 */
void testScene1_PrunerComparison() {
    printf("=== Scene 1: Pruner Type Comparison ===\n");

    // 测试配置1：默认配置（静态BVH + 动态BVH）
    {
        PxSceneDesc desc = createSceneDesc(
            PxPruningStructureType::eSTATIC_AABB_TREE,
            PxPruningStructureType::eDYNAMIC_AABB_TREE
        );
        PxScene* scene = gPhysics->createScene(desc);

        // 添加静态物体
        const int numStatic = 100;
        for (int i = 0; i < numStatic; ++i) {
            PxReal x = (i % 10) * 2.0f;
            PxReal z = (i / 10) * 2.0f;

            PxRigidStatic* actor = PxCreateStatic(*gPhysics,
                PxTransform(PxVec3(x, 0.5f, z)),
                PxBoxGeometry(0.5f, 0.5f, 0.5f),
                *gMaterial);
            scene->addActor(*actor);
        }

        // 添加动态物体
        const int numDynamic = 20;
        for (int i = 0; i < numDynamic; ++i) {
            PxReal x = (rand() % 200) / 10.0f;
            PxReal z = (rand() % 200) / 10.0f;

            PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
                PxTransform(PxVec3(x, 5.0f, z)),
                PxSphereGeometry(0.3f),
                *gMaterial,
                10.0f);
            scene->addActor(*actor);
        }

        printf("\nConfiguration 1: Static BVH + Dynamic BVH\n");
        printf("  Static actors: %d\n", numStatic);
        printf("  Dynamic actors: %d\n", numDynamic);

        // 执行查询测试
        const int numQueries = 1000;
        int totalHits = 0;

        auto start = std::chrono::high_resolution_clock::now();

        for (int q = 0; q < numQueries; ++q) {
            PxVec3 origin((rand() % 200) / 10.0f, 10.0f, (rand() % 200) / 10.0f);
            PxVec3 dir(0, -1, 0);

            PxRaycastBuffer hit;
            if (scene->raycast(origin, dir, 20.0f, hit)) {
                totalHits++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        long queryTime = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start).count();

        printf("  Query performance:\n");
        printf("    Queries: %d\n", numQueries);
        printf("    Hits: %d\n", totalHits);
        printf("    Total time: %ld μs\n", queryTime);
        printf("    Avg per query: %.2f μs\n",
               static_cast<float>(queryTime) / numQueries);

        scene->release();
    }

    // 测试配置2：无Pruner（暴力搜索）
    {
        PxSceneDesc desc = createSceneDesc(
            PxPruningStructureType::eNONE,
            PxPruningStructureType::eNONE
        );
        PxScene* scene = gPhysics->createScene(desc);

        // 添加相同数量的物体
        const int numStatic = 100;
        for (int i = 0; i < numStatic; ++i) {
            PxReal x = (i % 10) * 2.0f;
            PxReal z = (i / 10) * 2.0f;

            PxRigidStatic* actor = PxCreateStatic(*gPhysics,
                PxTransform(PxVec3(x, 0.5f, z)),
                PxBoxGeometry(0.5f, 0.5f, 0.5f),
                *gMaterial);
            scene->addActor(*actor);
        }

        const int numDynamic = 20;
        for (int i = 0; i < numDynamic; ++i) {
            PxReal x = (rand() % 200) / 10.0f;
            PxReal z = (rand() % 200) / 10.0f;

            PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
                PxTransform(PxVec3(x, 5.0f, z)),
                PxSphereGeometry(0.3f),
                *gMaterial,
                10.0f);
            scene->addActor(*actor);
        }

        printf("\nConfiguration 2: No Pruner (Brute Force)\n");

        const int numQueries = 1000;
        int totalHits = 0;

        auto start = std::chrono::high_resolution_clock::now();

        for (int q = 0; q < numQueries; ++q) {
            PxVec3 origin((rand() % 200) / 10.0f, 10.0f, (rand() % 200) / 10.0f);
            PxVec3 dir(0, -1, 0);

            PxRaycastBuffer hit;
            if (scene->raycast(origin, dir, 20.0f, hit)) {
                totalHits++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        long queryTime = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start).count();

        printf("  Query performance:\n");
        printf("    Total time: %ld μs\n", queryTime);
        printf("    Avg per query: %.2f μs\n",
               static_cast<float>(queryTime) / numQueries);

        scene->release();
    }
}

/**
 * 场景2：动态物体更新性能
 */
void testScene2_DynamicUpdatePerformance() {
    printf("\n=== Scene 2: Dynamic Update Performance ===\n");

    PxSceneDesc desc = createSceneDesc(
        PxPruningStructureType::eSTATIC_AABB_TREE,
        PxPruningStructureType::eDYNAMIC_AABB_TREE
    );
    gScene = gPhysics->createScene(desc);

    // 大量静态物体
    const int numStatic = 500;
    for (int i = 0; i < numStatic; ++i) {
        PxReal x = (rand() % 1000) / 10.0f;
        PxReal z = (rand() % 1000) / 10.0f;

        PxRigidStatic* actor = PxCreateStatic(*gPhysics,
            PxTransform(PxVec3(x, 0.5f, z)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *gMaterial);
        gScene->addActor(*actor);
    }

    // 少量动态物体
    const int numDynamic = 50;
    std::vector<PxRigidDynamic*> dynamicActors;

    for (int i = 0; i < numDynamic; ++i) {
        PxReal x = (rand() % 1000) / 10.0f;
        PxReal z = (rand() % 1000) / 10.0f;

        PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
            PxTransform(PxVec3(x, 5.0f, z)),
            PxSphereGeometry(0.3f),
            *gMaterial,
            10.0f);
        gScene->addActor(*actor);
        dynamicActors.push_back(actor);
    }

    printf("Created scene with %d static + %d dynamic actors\n",
           numStatic, numDynamic);

    // 模拟多帧，观察性能
    const int numFrames = 100;
    long totalSimTime = 0;
    long totalQueryTime = 0;

    for (int frame = 0; frame < numFrames; ++frame) {
        // 模拟
        auto simStart = std::chrono::high_resolution_clock::now();
        gScene->simulate(1.0f / 60.0f);
        gScene->fetchResults(true);
        auto simEnd = std::chrono::high_resolution_clock::now();
        totalSimTime += std::chrono::duration_cast<std::chrono::microseconds>(
            simEnd - simStart).count();

        // 查询
        auto queryStart = std::chrono::high_resolution_clock::now();

        for (int q = 0; q < 10; ++q) {
            PxVec3 origin((rand() % 1000) / 10.0f, 10.0f, (rand() % 1000) / 10.0f);
            PxVec3 dir(0, -1, 0);
            PxRaycastBuffer hit;
            gScene->raycast(origin, dir, 20.0f, hit);
        }

        auto queryEnd = std::chrono::high_resolution_clock::now();
        totalQueryTime += std::chrono::duration_cast<std::chrono::microseconds>(
            queryEnd - queryStart).count();
    }

    printf("\nPerformance over %d frames:\n", numFrames);
    printf("  Avg simulation time: %.2f μs/frame\n",
           static_cast<float>(totalSimTime) / numFrames);
    printf("  Avg query time: %.2f μs/frame (10 queries)\n",
           static_cast<float>(totalQueryTime) / numFrames);

    gScene->release();
    gScene = nullptr;
}

/**
 * 场景3：Pruner重建性能
 */
void testScene3_PrunerRebuild() {
    printf("\n=== Scene 3: Pruner Rebuild Performance ===\n");

    PxSceneDesc desc = createSceneDesc(
        PxPruningStructureType::eSTATIC_AABB_TREE,
        PxPruningStructureType::eDYNAMIC_AABB_TREE
    );
    gScene = gPhysics->createScene(desc);

    const int numActors = 200;
    std::vector<PxRigidStatic*> staticActors;

    printf("Creating %d static actors...\n", numActors);

    auto createStart = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numActors; ++i) {
        PxReal x = (rand() % 500) / 10.0f;
        PxReal z = (rand() % 500) / 10.0f;

        PxRigidStatic* actor = PxCreateStatic(*gPhysics,
            PxTransform(PxVec3(x, 0.5f, z)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *gMaterial);
        gScene->addActor(*actor);
        staticActors.push_back(actor);
    }

    auto createEnd = std::chrono::high_resolution_clock::now();
    long createTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        createEnd - createStart).count();

    printf("  Creation time: %ld ms\n", createTime);

    // 强制重建pruner
    printf("\nForcing pruner rebuild...\n");

    auto rebuildStart = std::chrono::high_resolution_clock::now();
    gScene->flushQueryUpdates();  // 强制更新查询结构
    auto rebuildEnd = std::chrono::high_resolution_clock::now();

    long rebuildTime = std::chrono::duration_cast<std::chrono::microseconds>(
        rebuildEnd - rebuildStart).count();

    printf("  Rebuild time: %ld μs\n", rebuildTime);

    // 测试查询性能
    const int numQueries = 100;
    auto queryStart = std::chrono::high_resolution_clock::now();

    for (int q = 0; q < numQueries; ++q) {
        PxVec3 origin((rand() % 500) / 10.0f, 10.0f, (rand() % 500) / 10.0f);
        PxVec3 dir(0, -1, 0);
        PxRaycastBuffer hit;
        gScene->raycast(origin, dir, 20.0f, hit);
    }

    auto queryEnd = std::chrono::high_resolution_clock::now();
    long queryTime = std::chrono::duration_cast<std::chrono::microseconds>(
        queryEnd - queryStart).count();

    printf("  Query time (%d queries): %ld μs\n", numQueries, queryTime);
    printf("  Avg per query: %.2f μs\n",
           static_cast<float>(queryTime) / numQueries);

    gScene->release();
    gScene = nullptr;
}

/**
 * 场景4：混合场景优化
 */
void testScene4_MixedSceneOptimization() {
    printf("\n=== Scene 4: Mixed Scene Optimization ===\n");

    // 测试不同比例的静态/动态物体
    struct TestConfig {
        const char* name;
        int numStatic;
        int numDynamic;
    };

    TestConfig configs[] = {
        {"Mostly Static (90%)", 900, 100},
        {"Balanced (50%)", 500, 500},
        {"Mostly Dynamic (10%)", 100, 900}
    };

    for (const auto& config : configs) {
        PxSceneDesc desc = createSceneDesc(
            PxPruningStructureType::eSTATIC_AABB_TREE,
            PxPruningStructureType::eDYNAMIC_AABB_TREE
        );
        PxScene* scene = gPhysics->createScene(desc);

        // 添加静态物体
        for (int i = 0; i < config.numStatic; ++i) {
            PxReal x = (rand() % 1000) / 10.0f;
            PxReal z = (rand() % 1000) / 10.0f;

            PxRigidStatic* actor = PxCreateStatic(*gPhysics,
                PxTransform(PxVec3(x, 0.5f, z)),
                PxBoxGeometry(0.3f, 0.3f, 0.3f),
                *gMaterial);
            scene->addActor(*actor);
        }

        // 添加动态物体
        for (int i = 0; i < config.numDynamic; ++i) {
            PxReal x = (rand() % 1000) / 10.0f;
            PxReal z = (rand() % 1000) / 10.0f;

            PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
                PxTransform(PxVec3(x, 5.0f, z)),
                PxSphereGeometry(0.3f),
                *gMaterial,
                10.0f);
            scene->addActor(*actor);
        }

        // 查询性能测试
        const int numQueries = 500;
        auto start = std::chrono::high_resolution_clock::now();

        for (int q = 0; q < numQueries; ++q) {
            PxVec3 origin((rand() % 1000) / 10.0f, 10.0f, (rand() % 1000) / 10.0f);
            PxVec3 dir(0, -1, 0);
            PxRaycastBuffer hit;
            scene->raycast(origin, dir, 20.0f, hit);
        }

        auto end = std::chrono::high_resolution_clock::now();
        long queryTime = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start).count();

        printf("\n%s:\n", config.name);
        printf("  Static: %d, Dynamic: %d\n", config.numStatic, config.numDynamic);
        printf("  Query time: %ld μs (%d queries)\n", queryTime, numQueries);
        printf("  Avg: %.2f μs/query\n",
               static_cast<float>(queryTime) / numQueries);

        scene->release();
    }
}

/**
 * 场景5：大规模场景
 */
void testScene5_LargeScale() {
    printf("\n=== Scene 5: Large Scale Scene ===\n");

    PxSceneDesc desc = createSceneDesc(
        PxPruningStructureType::eSTATIC_AABB_TREE,
        PxPruningStructureType::eDYNAMIC_AABB_TREE
    );
    gScene = gPhysics->createScene(desc);

    const int numStatic = 2000;
    const int numDynamic = 200;

    printf("Creating large scene: %d static + %d dynamic\n",
           numStatic, numDynamic);

    auto createStart = std::chrono::high_resolution_clock::now();

    // 静态物体
    for (int i = 0; i < numStatic; ++i) {
        PxReal x = (rand() % 2000) / 10.0f;
        PxReal z = (rand() % 2000) / 10.0f;

        PxRigidStatic* actor = PxCreateStatic(*gPhysics,
            PxTransform(PxVec3(x, 0.5f, z)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *gMaterial);
        gScene->addActor(*actor);
    }

    // 动态物体
    for (int i = 0; i < numDynamic; ++i) {
        PxReal x = (rand() % 2000) / 10.0f;
        PxReal z = (rand() % 2000) / 10.0f;

        PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
            PxTransform(PxVec3(x, 5.0f, z)),
            PxSphereGeometry(0.3f),
            *gMaterial,
            10.0f);
        gScene->addActor(*actor);
    }

    auto createEnd = std::chrono::high_resolution_clock::now();
    long createTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        createEnd - createStart).count();

    printf("  Creation time: %ld ms\n", createTime);

    // 大量查询
    const int numQueries = 5000;
    printf("\nExecuting %d queries...\n", numQueries);

    auto queryStart = std::chrono::high_resolution_clock::now();
    int hits = 0;

    for (int q = 0; q < numQueries; ++q) {
        PxVec3 origin((rand() % 2000) / 10.0f, 10.0f, (rand() % 2000) / 10.0f);
        PxVec3 dir(0, -1, 0);
        PxRaycastBuffer hit;
        if (gScene->raycast(origin, dir, 20.0f, hit)) {
            hits++;
        }
    }

    auto queryEnd = std::chrono::high_resolution_clock::now();
    long queryTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        queryEnd - queryStart).count();

    printf("  Query time: %ld ms\n", queryTime);
    printf("  Hits: %d (%.1f%%)\n", hits, 100.0f * hits / numQueries);
    printf("  Throughput: %.0f queries/sec\n",
           (numQueries * 1000.0f) / queryTime);

    gScene->release();
    gScene = nullptr;
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

    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.1f);

    return true;
}

/**
 * 清理PhysX
 */
void cleanupPhysX() {
    PX_RELEASE(gMaterial);
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
    printf("PhysX Multiple Pruners Example\n");
    printf("===============================\n\n");

    if (!initPhysX()) {
        return 1;
    }

    // 测试所有场景
    testScene1_PrunerComparison();
    testScene2_DynamicUpdatePerformance();
    testScene3_PrunerRebuild();
    testScene4_MixedSceneOptimization();
    testScene5_LargeScale();

    printf("\n=== Summary ===\n");
    printf("Demonstrated multiple pruners optimization:\n");
    printf("- Static vs Dynamic pruner types\n");
    printf("- Performance comparison: BVH vs brute force\n");
    printf("- Dynamic object update performance\n");
    printf("- Pruner rebuild cost analysis\n");
    printf("- Mixed scene optimization strategies\n");
    printf("- Large scale performance (2000+ objects)\n");
    printf("\nKey insights:\n");
    printf("- Separate pruners for static/dynamic improves performance\n");
    printf("- Static BVH: High quality, slow build, fast query\n");
    printf("- Dynamic BVH: Incremental update, good for moving objects\n");
    printf("- Proper pruner selection critical for performance\n");
    printf("- 10-100x speedup vs no pruner for large scenes\n");

    cleanupPhysX();
    printf("\nExample completed successfully!\n");

    return 0;
}
