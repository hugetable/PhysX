/**
 * PhysX Snippet: Split Simulation
 *
 * 本示例演示如何使用分离仿真模式(Split Simulation)提高CPU利用率。
 *
 * 理论基础：
 *
 * 1. 传统仿真模式
 *    同步模式，CPU空闲等待：
 *    ```
 *    simulate(dt);           // CPU启动物理计算
 *    fetchResults(true);     // CPU阻塞等待
 *    // 现在可以读取结果
 *    ```
 *
 * 2. 分离仿真模式
 *    异步模式，CPU利用率更高：
 *    ```
 *    simulate(dt);           // 启动物理计算
 *    // CPU继续执行其他工作
 *    doGameLogic();
 *    doRendering();
 *    fetchResults(true);     // 等待完成
 *    ```
 *
 * 3. CPU利用率分析
 *    传统模式：
 *    ┌──────────┬──────────┬──────────┐
 *    │ simulate │  fetch   │   game   │
 *    │  (idle)  │ (block)  │  logic   │
 *    └──────────┴──────────┴──────────┘
 *    CPU利用率: ~33%
 *
 *    分离模式：
 *    ┌──────────┬──────────┬──────────┐
 *    │ simulate │   game   │  fetch   │
 *    │ (async)  │  logic   │ (wait)   │
 *    └──────────┴──────────┴──────────┘
 *    CPU利用率: ~67-100%
 *
 * 4. 双缓冲技术
 *    使用双缓冲避免读写冲突：
 *    - Buffer A: 当前帧读取
 *    - Buffer B: 下一帧写入
 *
 *    Frame N:
 *      simulate(B);    // 写入B
 *      work();
 *      fetchResults(); // 完成B
 *      read(B);        // 读取B结果
 *
 * 5. 时间重叠计算
 *    最优情况下的时间线：
 *
 *    Frame 0: [sim] [fetch] [game]
 *    Frame 1:       [sim] [fetch] [game]
 *    Frame 2:              [sim] [fetch] [game]
 *
 *    重叠后：
 *    Frame 0: [sim] [fetch+game]
 *    Frame 1:       [sim] [fetch+game]
 *    Frame 2:              [sim] [fetch+game]
 *
 *    理论加速：2x（如果game time ≈ sim time）
 *
 * 6. 注意事项
 *    - simulate()和fetchResults()之间不能修改场景
 *    - 不能在simulate期间添加/删除actor
 *    - 读取transform是安全的（上一帧的）
 *    - 写入force/velocity是不安全的
 *
 * 7. 最佳实践
 *    - 确保game logic时间 > physics time
 *    - 使用双缓冲管理状态
 *    - 避免在simulate期间修改场景
 *    - 监控fetchResults等待时间
 */

#include "PxPhysicsAPI.h"
#include "../common/PxPhysXCommon.h"
#include <vector>
#include <chrono>
#include <thread>
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
 * 模拟游戏逻辑工作负载
 */
void doGameLogic(int workloadMs) {
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::milliseconds(workloadMs);

    // 模拟CPU密集型工作
    volatile double result = 0.0;
    while (std::chrono::high_resolution_clock::now() < end) {
        for (int i = 0; i < 1000; ++i) {
            result += std::sin(i * 0.001) * std::cos(i * 0.002);
        }
    }
}

/**
 * 场景1：同步 vs 异步仿真对比
 */
void testScene1_SyncVsAsync() {
    printf("=== Scene 1: Synchronous vs Asynchronous Simulation ===\n");

    // 创建测试场景
    const int numObjects = 200;
    std::vector<PxRigidDynamic*> actors;

    for (int i = 0; i < numObjects; ++i) {
        PxReal x = (rand() % 200) / 10.0f - 10.0f;
        PxReal y = 5.0f + (rand() % 100) / 10.0f;
        PxReal z = (rand() % 200) / 10.0f - 10.0f;

        PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
            PxTransform(PxVec3(x, y, z)),
            PxSphereGeometry(0.3f),
            *gMaterial,
            10.0f);
        gScene->addActor(*actor);
        actors.push_back(actor);
    }

    printf("Created scene with %d dynamic objects\n", numObjects);

    const int numFrames = 100;
    const int gameWorkMs = 5;  // 模拟5ms游戏逻辑

    // 测试1：同步模式
    {
        printf("\nTest 1: Synchronous Mode\n");

        auto totalStart = std::chrono::high_resolution_clock::now();
        long totalSimTime = 0;
        long totalFetchTime = 0;
        long totalGameTime = 0;

        for (int frame = 0; frame < numFrames; ++frame) {
            // 物理仿真
            auto simStart = std::chrono::high_resolution_clock::now();
            gScene->simulate(1.0f / 60.0f);
            auto simEnd = std::chrono::high_resolution_clock::now();
            totalSimTime += std::chrono::duration_cast<std::chrono::microseconds>(
                simEnd - simStart).count();

            // 等待结果（同步）
            auto fetchStart = std::chrono::high_resolution_clock::now();
            gScene->fetchResults(true);
            auto fetchEnd = std::chrono::high_resolution_clock::now();
            totalFetchTime += std::chrono::duration_cast<std::chrono::microseconds>(
                fetchEnd - fetchStart).count();

            // 游戏逻辑（在fetch之后）
            auto gameStart = std::chrono::high_resolution_clock::now();
            doGameLogic(gameWorkMs);
            auto gameEnd = std::chrono::high_resolution_clock::now();
            totalGameTime += std::chrono::duration_cast<std::chrono::microseconds>(
                gameEnd - gameStart).count();
        }

        auto totalEnd = std::chrono::high_resolution_clock::now();
        long totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            totalEnd - totalStart).count();

        printf("  Total time: %ld ms\n", totalTime);
        printf("  Avg simulate: %.2f μs\n", static_cast<float>(totalSimTime) / numFrames);
        printf("  Avg fetch: %.2f μs\n", static_cast<float>(totalFetchTime) / numFrames);
        printf("  Avg game: %.2f μs\n", static_cast<float>(totalGameTime) / numFrames);
        printf("  FPS: %.1f\n", (numFrames * 1000.0f) / totalTime);
    }

    // 重置场景
    for (auto actor : actors) {
        PxReal x = (rand() % 200) / 10.0f - 10.0f;
        PxReal y = 5.0f + (rand() % 100) / 10.0f;
        PxReal z = (rand() % 200) / 10.0f - 10.0f;
        actor->setGlobalPose(PxTransform(PxVec3(x, y, z)));
        actor->setLinearVelocity(PxVec3(0));
        actor->setAngularVelocity(PxVec3(0));
    }

    // 测试2：异步模式（分离仿真）
    {
        printf("\nTest 2: Asynchronous Mode (Split Simulation)\n");

        auto totalStart = std::chrono::high_resolution_clock::now();
        long totalSimTime = 0;
        long totalFetchTime = 0;
        long totalGameTime = 0;

        for (int frame = 0; frame < numFrames; ++frame) {
            // 启动物理仿真（不等待）
            auto simStart = std::chrono::high_resolution_clock::now();
            gScene->simulate(1.0f / 60.0f);
            auto simEnd = std::chrono::high_resolution_clock::now();
            totalSimTime += std::chrono::duration_cast<std::chrono::microseconds>(
                simEnd - simStart).count();

            // 游戏逻辑（与物理并行）
            auto gameStart = std::chrono::high_resolution_clock::now();
            doGameLogic(gameWorkMs);
            auto gameEnd = std::chrono::high_resolution_clock::now();
            totalGameTime += std::chrono::duration_cast<std::chrono::microseconds>(
                gameEnd - gameStart).count();

            // 等待物理完成
            auto fetchStart = std::chrono::high_resolution_clock::now();
            gScene->fetchResults(true);
            auto fetchEnd = std::chrono::high_resolution_clock::now();
            totalFetchTime += std::chrono::duration_cast<std::chrono::microseconds>(
                fetchEnd - fetchStart).count();
        }

        auto totalEnd = std::chrono::high_resolution_clock::now();
        long totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            totalEnd - totalStart).count();

        printf("  Total time: %ld ms\n", totalTime);
        printf("  Avg simulate: %.2f μs\n", static_cast<float>(totalSimTime) / numFrames);
        printf("  Avg fetch: %.2f μs\n", static_cast<float>(totalFetchTime) / numFrames);
        printf("  Avg game: %.2f μs\n", static_cast<float>(totalGameTime) / numFrames);
        printf("  FPS: %.1f\n", (numFrames * 1000.0f) / totalTime);
    }
}

/**
 * 场景2：不同工作负载的影响
 */
void testScene2_WorkloadImpact() {
    printf("\n=== Scene 2: Impact of Game Logic Workload ===\n");

    // 创建场景
    const int numObjects = 150;
    for (int i = 0; i < numObjects; ++i) {
        PxReal x = (rand() % 200) / 10.0f - 10.0f;
        PxReal y = 5.0f + (rand() % 100) / 10.0f;
        PxReal z = (rand() % 200) / 10.0f - 10.0f;

        PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
            PxTransform(PxVec3(x, y, z)),
            PxSphereGeometry(0.3f),
            *gMaterial,
            10.0f);
        gScene->addActor(*actor);
    }

    const int numFrames = 50;
    int workloads[] = {0, 2, 5, 10, 15};

    printf("\n%-15s %-15s %-15s %-15s\n",
           "Workload(ms)", "Sync FPS", "Async FPS", "Speedup");
    printf("---------------------------------------------------------------\n");

    for (int workloadMs : workloads) {
        // 同步模式
        auto syncStart = std::chrono::high_resolution_clock::now();
        for (int frame = 0; frame < numFrames; ++frame) {
            gScene->simulate(1.0f / 60.0f);
            gScene->fetchResults(true);
            doGameLogic(workloadMs);
        }
        auto syncEnd = std::chrono::high_resolution_clock::now();
        long syncTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            syncEnd - syncStart).count();
        float syncFps = (numFrames * 1000.0f) / syncTime;

        // 异步模式
        auto asyncStart = std::chrono::high_resolution_clock::now();
        for (int frame = 0; frame < numFrames; ++frame) {
            gScene->simulate(1.0f / 60.0f);
            doGameLogic(workloadMs);
            gScene->fetchResults(true);
        }
        auto asyncEnd = std::chrono::high_resolution_clock::now();
        long asyncTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            asyncEnd - asyncStart).count();
        float asyncFps = (numFrames * 1000.0f) / asyncTime;

        float speedup = asyncFps / syncFps;

        printf("%-15d %-15.1f %-15.1f %.2fx\n",
               workloadMs, syncFps, asyncFps, speedup);
    }
}

/**
 * 场景3：CPU利用率分析
 */
void testScene3_CPUUtilization() {
    printf("\n=== Scene 3: CPU Utilization Analysis ===\n");

    // 创建场景
    const int numObjects = 200;
    for (int i = 0; i < numObjects; ++i) {
        PxReal x = (rand() % 200) / 10.0f - 10.0f;
        PxReal y = 5.0f + (rand() % 100) / 10.0f;
        PxReal z = (rand() % 200) / 10.0f - 10.0f;

        PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
            PxTransform(PxVec3(x, y, z)),
            PxSphereGeometry(0.3f),
            *gMaterial,
            10.0f);
        gScene->addActor(*actor);
    }

    const int numFrames = 100;
    const int gameWorkMs = 8;

    printf("\nMeasuring frame time breakdown:\n\n");

    // 异步模式详细分析
    long totalSimStartTime = 0;
    long totalGameTime = 0;
    long totalFetchWaitTime = 0;
    long totalFrameTime = 0;

    for (int frame = 0; frame < numFrames; ++frame) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // 启动物理
        auto simStart = std::chrono::high_resolution_clock::now();
        gScene->simulate(1.0f / 60.0f);
        auto simEnd = std::chrono::high_resolution_clock::now();
        totalSimStartTime += std::chrono::duration_cast<std::chrono::microseconds>(
            simEnd - simStart).count();

        // 游戏逻辑（与物理并行）
        auto gameStart = std::chrono::high_resolution_clock::now();
        doGameLogic(gameWorkMs);
        auto gameEnd = std::chrono::high_resolution_clock::now();
        totalGameTime += std::chrono::duration_cast<std::chrono::microseconds>(
            gameEnd - gameStart).count();

        // 等待物理完成
        auto fetchStart = std::chrono::high_resolution_clock::now();
        gScene->fetchResults(true);
        auto fetchEnd = std::chrono::high_resolution_clock::now();
        totalFetchWaitTime += std::chrono::duration_cast<std::chrono::microseconds>(
            fetchEnd - fetchStart).count();

        auto frameEnd = std::chrono::high_resolution_clock::now();
        totalFrameTime += std::chrono::duration_cast<std::chrono::microseconds>(
            frameEnd - frameStart).count();
    }

    float avgSimStart = static_cast<float>(totalSimStartTime) / numFrames / 1000.0f;
    float avgGame = static_cast<float>(totalGameTime) / numFrames / 1000.0f;
    float avgFetchWait = static_cast<float>(totalFetchWaitTime) / numFrames / 1000.0f;
    float avgFrame = static_cast<float>(totalFrameTime) / numFrames / 1000.0f;

    printf("Average timings per frame:\n");
    printf("  Simulate start: %.2f ms\n", avgSimStart);
    printf("  Game logic: %.2f ms\n", avgGame);
    printf("  Fetch wait: %.2f ms\n", avgFetchWait);
    printf("  Total frame: %.2f ms\n", avgFrame);

    float physicsTime = avgSimStart + avgFetchWait;
    float overlap = avgGame;
    float savedTime = PxMin(physicsTime, overlap);

    printf("\nCPU utilization:\n");
    printf("  Physics time: %.2f ms\n", physicsTime);
    printf("  Overlapped work: %.2f ms\n", overlap);
    printf("  Time saved: %.2f ms\n", savedTime);
    printf("  Efficiency: %.1f%%\n", (savedTime / physicsTime) * 100.0f);
}

/**
 * 场景4：最佳实践演示
 */
void testScene4_BestPractices() {
    printf("\n=== Scene 4: Best Practices ===\n");

    // 创建场景
    const int numObjects = 100;
    std::vector<PxRigidDynamic*> actors;

    for (int i = 0; i < numObjects; ++i) {
        PxReal x = (rand() % 200) / 10.0f - 10.0f;
        PxReal y = 5.0f + (rand() % 100) / 10.0f;
        PxReal z = (rand() % 200) / 10.0f - 10.0f;

        PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
            PxTransform(PxVec3(x, y, z)),
            PxSphereGeometry(0.3f),
            *gMaterial,
            10.0f);
        gScene->addActor(*actor);
        actors.push_back(actor);
    }

    printf("\nDemonstrating proper split simulation pattern:\n\n");

    const int numFrames = 10;

    for (int frame = 0; frame < numFrames; ++frame) {
        // 1. 启动物理仿真
        gScene->simulate(1.0f / 60.0f);

        // 2. 执行不修改场景的游戏逻辑
        //    - 渲染准备
        //    - AI计算
        //    - 音频处理
        //    - 网络通信
        printf("Frame %d: Physics started, doing game work...\n", frame);
        doGameLogic(3);

        // 3. 等待物理完成
        gScene->fetchResults(true);

        // 4. 读取物理结果
        int movingCount = 0;
        for (auto actor : actors) {
            PxVec3 vel = actor->getLinearVelocity();
            if (vel.magnitude() > 0.1f) {
                movingCount++;
            }
        }

        printf("         Physics done, %d objects moving\n", movingCount);

        // 5. 应用游戏逻辑到物理（下一帧的输入）
        // 这里可以安全地修改场景了
    }

    printf("\nBest practices summary:\n");
    printf("  1. Call simulate() early in frame\n");
    printf("  2. Do non-physics work while physics runs\n");
    printf("  3. Call fetchResults() when needed\n");
    printf("  4. Only modify scene after fetchResults()\n");
    printf("  5. Reading transforms is safe during simulation\n");
}

/**
 * 场景5：性能缩放测试
 */
void testScene5_PerformanceScaling() {
    printf("\n=== Scene 5: Performance Scaling ===\n");

    int objectCounts[] = {50, 100, 200, 500};

    printf("\n%-15s %-15s %-15s %-15s\n",
           "Objects", "Sync(ms)", "Async(ms)", "Speedup");
    printf("---------------------------------------------------------------\n");

    for (int numObjects : objectCounts) {
        // 清空场景
        PxU32 numActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
        if (numActors > 0) {
            std::vector<PxActor*> actors(numActors);
            gScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC,
                             actors.data(), numActors);
            for (auto actor : actors) {
                gScene->removeActor(*actor);
                actor->release();
            }
        }

        // 创建新场景
        for (int i = 0; i < numObjects; ++i) {
            PxReal x = (rand() % 200) / 10.0f - 10.0f;
            PxReal y = 5.0f + (rand() % 100) / 10.0f;
            PxReal z = (rand() % 200) / 10.0f - 10.0f;

            PxRigidDynamic* actor = PxCreateDynamic(*gPhysics,
                PxTransform(PxVec3(x, y, z)),
                PxSphereGeometry(0.3f),
                *gMaterial,
                10.0f);
            gScene->addActor(*actor);
        }

        const int numFrames = 50;

        // 同步
        auto syncStart = std::chrono::high_resolution_clock::now();
        for (int frame = 0; frame < numFrames; ++frame) {
            gScene->simulate(1.0f / 60.0f);
            gScene->fetchResults(true);
            doGameLogic(5);
        }
        auto syncEnd = std::chrono::high_resolution_clock::now();
        long syncTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            syncEnd - syncStart).count();

        // 异步
        auto asyncStart = std::chrono::high_resolution_clock::now();
        for (int frame = 0; frame < numFrames; ++frame) {
            gScene->simulate(1.0f / 60.0f);
            doGameLogic(5);
            gScene->fetchResults(true);
        }
        auto asyncEnd = std::chrono::high_resolution_clock::now();
        long asyncTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            asyncEnd - asyncStart).count();

        float speedup = static_cast<float>(syncTime) / asyncTime;

        printf("%-15d %-15ld %-15ld %.2fx\n",
               numObjects, syncTime, asyncTime, speedup);
    }
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
    printf("PhysX Split Simulation Example\n");
    printf("===============================\n\n");

    if (!initPhysX()) {
        return 1;
    }

    // 测试所有场景
    testScene1_SyncVsAsync();
    testScene2_WorkloadImpact();
    testScene3_CPUUtilization();
    testScene4_BestPractices();
    testScene5_PerformanceScaling();

    printf("\n=== Summary ===\n");
    printf("Demonstrated split simulation technique:\n");
    printf("- Synchronous vs asynchronous simulation\n");
    printf("- Impact of game logic workload\n");
    printf("- CPU utilization analysis\n");
    printf("- Best practices for split simulation\n");
    printf("- Performance scaling with object count\n");
    printf("\nKey insights:\n");
    printf("- Split simulation improves CPU utilization\n");
    printf("- Speedup depends on game logic duration\n");
    printf("- Typical speedup: 1.2x - 2.0x\n");
    printf("- Best when game work ≈ physics time\n");
    printf("- Critical: Don't modify scene during simulation\n");
    printf("\nBest practices:\n");
    printf("- Call simulate() early\n");
    printf("- Do non-physics work while physics runs\n");
    printf("- fetchResults() only when you need results\n");
    printf("- Modify scene only after fetchResults()\n");

    cleanupPhysX();
    printf("\nExample completed successfully!\n");

    return 0;
}
