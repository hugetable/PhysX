/**
 * PhysX Snippet: Split Fetch Results
 *
 * 本示例演示如何使用分离获取结果(Split Fetch Results)实现更灵活的异步控制。
 *
 * 理论基础：
 *
 * 1. 标准fetchResults()
 *    一次调用，阻塞或非阻塞：
 *    ```
 *    simulate(dt);
 *    fetchResults(block=true);   // 阻塞直到完成
 *    ```
 *
 * 2. 非阻塞fetchResults()
 *    允许轮询检查：
 *    ```
 *    simulate(dt);
 *    if (fetchResults(block=false)) {
 *        // 物理已完成
 *    } else {
 *        // 物理还在运行，继续其他工作
 *    }
 *    ```
 *
 * 3. 时间线对比
 *    阻塞模式：
 *    ┌─────────┬─────────┬─────────┐
 *    │simulate │  wait   │  game   │
 *    └─────────┴─────────┴─────────┘
 *
 *    非阻塞轮询：
 *    ┌─────────┬─────────┬─────────┐
 *    │simulate │  game   │  check  │
 *    │         │  work   │ & fetch │
 *    └─────────┴─────────┴─────────┘
 *
 * 4. 轮询策略
 *    a) 单次检查：
 *       simulate(); work(); fetchResults(false);
 *
 *    b) 轮询循环：
 *       simulate();
 *       while (!fetchResults(false)) {
 *           doSmallWork();
 *       }
 *
 *    c) 超时轮询：
 *       simulate();
 *       start = now();
 *       while (!fetchResults(false) && (now() - start < timeout)) {
 *           doWork();
 *       }
 *       fetchResults(true);  // 最后强制完成
 *
 * 5. 适用场景
 *    非阻塞fetchResults适合：
 *    - 可变长度的游戏逻辑
 *    - 需要保证帧率上限
 *    - 复杂的任务调度
 *    - 多物理场景并行
 *
 * 6. 性能考虑
 *    - 轮询本身有开销（~1-5μs）
 *    - 不要过于频繁轮询
 *    - 轮询间隔建议：100-500μs
 *    - 避免busy wait（空循环）
 *
 * 7. 错误处理
 *    - 不能在simulate和fetchResults之间修改场景
 *    - fetchResults(false)返回false时，结果不可用
 *    - 必须最终调用成功的fetchResults
 *    - 下一次simulate前必须完成上次fetchResults
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
 * 模拟游戏工作（可变长度）
 */
void doVariableWork(int minMs, int maxMs) {
    int workMs = minMs + (rand() % (maxMs - minMs + 1));
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::milliseconds(workMs);

    volatile double result = 0.0;
    while (std::chrono::high_resolution_clock::now() < end) {
        for (int i = 0; i < 1000; ++i) {
            result += std::sin(i * 0.001) * std::cos(i * 0.002);
        }
    }
}

/**
 * 场景1：阻塞 vs 非阻塞fetchResults
 */
void testScene1_BlockingVsNonBlocking() {
    printf("=== Scene 1: Blocking vs Non-Blocking Fetch ===\n");

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

    const int numFrames = 100;

    // 测试1：阻塞模式
    {
        printf("\nTest 1: Blocking fetch\n");

        long totalWaitTime = 0;

        auto totalStart = std::chrono::high_resolution_clock::now();

        for (int frame = 0; frame < numFrames; ++frame) {
            gScene->simulate(1.0f / 60.0f);

            // 阻塞等待
            auto waitStart = std::chrono::high_resolution_clock::now();
            gScene->fetchResults(true);  // block=true
            auto waitEnd = std::chrono::high_resolution_clock::now();

            totalWaitTime += std::chrono::duration_cast<std::chrono::microseconds>(
                waitEnd - waitStart).count();
        }

        auto totalEnd = std::chrono::high_resolution_clock::now();
        long totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            totalEnd - totalStart).count();

        printf("  Total time: %ld ms\n", totalTime);
        printf("  Avg wait time: %.2f μs\n",
               static_cast<float>(totalWaitTime) / numFrames);
        printf("  FPS: %.1f\n", (numFrames * 1000.0f) / totalTime);
    }

    // 测试2：非阻塞轮询
    {
        printf("\nTest 2: Non-blocking fetch with polling\n");

        long totalPollCount = 0;
        long totalPollTime = 0;

        auto totalStart = std::chrono::high_resolution_clock::now();

        for (int frame = 0; frame < numFrames; ++frame) {
            gScene->simulate(1.0f / 60.0f);

            // 轮询检查
            int pollCount = 0;
            auto pollStart = std::chrono::high_resolution_clock::now();

            while (!gScene->fetchResults(false)) {  // block=false
                pollCount++;
                // 小等待避免busy wait
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            auto pollEnd = std::chrono::high_resolution_clock::now();
            totalPollCount += pollCount;
            totalPollTime += std::chrono::duration_cast<std::chrono::microseconds>(
                pollEnd - pollStart).count();
        }

        auto totalEnd = std::chrono::high_resolution_clock::now();
        long totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            totalEnd - totalStart).count();

        printf("  Total time: %ld ms\n", totalTime);
        printf("  Avg polls per frame: %.1f\n",
               static_cast<float>(totalPollCount) / numFrames);
        printf("  Avg poll time: %.2f μs\n",
               static_cast<float>(totalPollTime) / numFrames);
        printf("  FPS: %.1f\n", (numFrames * 1000.0f) / totalTime);
    }
}

/**
 * 场景2：在等待期间执行工作
 */
void testScene2_WorkDuringWait() {
    printf("\n=== Scene 2: Work During Physics Wait ===\n");

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

    const int numFrames = 50;

    printf("\nExecuting variable-length tasks during physics:\n");

    long totalWorkTime = 0;
    long totalPhysicsTime = 0;
    int totalTasks = 0;

    auto totalStart = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < numFrames; ++frame) {
        // 启动物理
        auto physicsStart = std::chrono::high_resolution_clock::now();
        gScene->simulate(1.0f / 60.0f);

        // 执行可变长度任务直到物理完成
        int tasksCompleted = 0;
        auto workStart = std::chrono::high_resolution_clock::now();

        while (!gScene->fetchResults(false)) {
            // 执行小任务（1-3ms）
            doVariableWork(1, 3);
            tasksCompleted++;
        }

        auto workEnd = std::chrono::high_resolution_clock::now();
        auto physicsEnd = workEnd;

        totalWorkTime += std::chrono::duration_cast<std::chrono::microseconds>(
            workEnd - workStart).count();
        totalPhysicsTime += std::chrono::duration_cast<std::chrono::microseconds>(
            physicsEnd - physicsStart).count();
        totalTasks += tasksCompleted;
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    long totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        totalEnd - totalStart).count();

    printf("  Total time: %ld ms\n", totalTime);
    printf("  Avg tasks per frame: %.1f\n",
           static_cast<float>(totalTasks) / numFrames);
    printf("  Avg work time: %.2f ms\n",
           static_cast<float>(totalWorkTime) / numFrames / 1000.0f);
    printf("  Avg physics time: %.2f ms\n",
           static_cast<float>(totalPhysicsTime) / numFrames / 1000.0f);
    printf("  Work overlap: %.1f%%\n",
           (static_cast<float>(totalWorkTime) / totalPhysicsTime) * 100.0f);
}

/**
 * 场景3：帧率限制
 */
void testScene3_FrameRateLimiting() {
    printf("\n=== Scene 3: Frame Rate Limiting ===\n");

    // 创建场景
    const int numObjects = 100;
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

    const int targetFPS = 60;
    const long targetFrameTimeUs = 1000000 / targetFPS;
    const int numFrames = 100;

    printf("\nTarget FPS: %d (%.2f ms/frame)\n",
           targetFPS, targetFrameTimeUs / 1000.0f);

    int framesOnTime = 0;
    int framesLate = 0;
    long totalFrameTime = 0;

    for (int frame = 0; frame < numFrames; ++frame) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // 启动物理
        gScene->simulate(1.0f / 60.0f);

        // 执行游戏逻辑
        doVariableWork(3, 8);

        // 智能等待：轮询直到物理完成或超时
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            now - frameStart).count();

        while (elapsed < targetFrameTimeUs && !gScene->fetchResults(false)) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            now = std::chrono::high_resolution_clock::now();
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                now - frameStart).count();
        }

        // 如果还没完成，强制完成
        if (!gScene->checkResults()) {
            gScene->fetchResults(true);
        }

        auto frameEnd = std::chrono::high_resolution_clock::now();
        long frameTime = std::chrono::duration_cast<std::chrono::microseconds>(
            frameEnd - frameStart).count();

        totalFrameTime += frameTime;

        if (frameTime <= targetFrameTimeUs) {
            framesOnTime++;
        } else {
            framesLate++;
        }
    }

    printf("\nResults:\n");
    printf("  Frames on time: %d (%.1f%%)\n",
           framesOnTime, 100.0f * framesOnTime / numFrames);
    printf("  Frames late: %d (%.1f%%)\n",
           framesLate, 100.0f * framesLate / numFrames);
    printf("  Avg frame time: %.2f ms\n",
           static_cast<float>(totalFrameTime) / numFrames / 1000.0f);
    printf("  Achieved FPS: %.1f\n",
           (numFrames * 1000000.0f) / totalFrameTime);
}

/**
 * 场景4：多任务调度
 */
void testScene4_TaskScheduling() {
    printf("\n=== Scene 4: Task Scheduling ===\n");

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

    printf("\nDemonstrating task prioritization:\n\n");

    const int numFrames = 20;

    for (int frame = 0; frame < numFrames; ++frame) {
        // 启动物理
        gScene->simulate(1.0f / 60.0f);

        // 任务优先级队列
        struct Task {
            const char* name;
            int durationMs;
            int priority;  // 高优先级=小数字
        };

        std::vector<Task> tasks = {
            {"Critical AI", 2, 1},
            {"Render prep", 1, 1},
            {"Audio mix", 3, 2},
            {"Network sync", 2, 2},
            {"LOD calc", 1, 3},
            {"Texture stream", 2, 3}
        };

        // 按优先级排序
        std::sort(tasks.begin(), tasks.end(),
                 [](const Task& a, const Task& b) {
                     return a.priority < b.priority;
                 });

        int tasksCompleted = 0;
        printf("Frame %d: ", frame);

        // 执行任务直到物理完成
        for (const auto& task : tasks) {
            if (gScene->fetchResults(false)) {
                printf("(Physics done) ");
                break;
            }

            printf("%s ", task.name);
            doVariableWork(task.durationMs, task.durationMs);
            tasksCompleted++;
        }

        // 确保物理完成
        if (!gScene->checkResults()) {
            gScene->fetchResults(true);
        }

        printf("- %d tasks\n", tasksCompleted);
    }
}

/**
 * 场景5：性能统计
 */
void testScene5_PerformanceStats() {
    printf("\n=== Scene 5: Performance Statistics ===\n");

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

    // 统计数据
    long minFetchTime = LONG_MAX;
    long maxFetchTime = 0;
    long totalFetchTime = 0;
    long totalPollCount = 0;

    std::vector<long> fetchTimes;

    for (int frame = 0; frame < numFrames; ++frame) {
        gScene->simulate(1.0f / 60.0f);

        // 执行工作
        doVariableWork(3, 7);

        // 测量fetch时间
        int pollCount = 0;
        auto fetchStart = std::chrono::high_resolution_clock::now();

        while (!gScene->fetchResults(false)) {
            pollCount++;
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }

        auto fetchEnd = std::chrono::high_resolution_clock::now();
        long fetchTime = std::chrono::duration_cast<std::chrono::microseconds>(
            fetchEnd - fetchStart).count();

        fetchTimes.push_back(fetchTime);
        minFetchTime = PxMin(minFetchTime, fetchTime);
        maxFetchTime = PxMax(maxFetchTime, fetchTime);
        totalFetchTime += fetchTime;
        totalPollCount += pollCount;
    }

    // 计算百分位数
    std::sort(fetchTimes.begin(), fetchTimes.end());
    long p50 = fetchTimes[numFrames / 2];
    long p95 = fetchTimes[(numFrames * 95) / 100];
    long p99 = fetchTimes[(numFrames * 99) / 100];

    printf("\nFetch time statistics (%d frames):\n", numFrames);
    printf("  Min: %ld μs\n", minFetchTime);
    printf("  Max: %ld μs\n", maxFetchTime);
    printf("  Avg: %.2f μs\n", static_cast<float>(totalFetchTime) / numFrames);
    printf("  P50: %ld μs\n", p50);
    printf("  P95: %ld μs\n", p95);
    printf("  P99: %ld μs\n", p99);
    printf("\nPolling statistics:\n");
    printf("  Avg polls per frame: %.1f\n",
           static_cast<float>(totalPollCount) / numFrames);
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
    printf("PhysX Split Fetch Results Example\n");
    printf("==================================\n\n");

    if (!initPhysX()) {
        return 1;
    }

    // 测试所有场景
    testScene1_BlockingVsNonBlocking();
    testScene2_WorkDuringWait();
    testScene3_FrameRateLimiting();
    testScene4_TaskScheduling();
    testScene5_PerformanceStats();

    printf("\n=== Summary ===\n");
    printf("Demonstrated split fetch results technique:\n");
    printf("- Blocking vs non-blocking fetch comparison\n");
    printf("- Executing work during physics computation\n");
    printf("- Frame rate limiting with smart polling\n");
    printf("- Task scheduling and prioritization\n");
    printf("- Performance statistics and analysis\n");
    printf("\nKey insights:\n");
    printf("- fetchResults(false) enables polling\n");
    printf("- Allows flexible task scheduling\n");
    printf("- Can improve CPU utilization\n");
    printf("- Polling has small overhead (~1-5μs)\n");
    printf("- Best for variable-length tasks\n");
    printf("\nBest practices:\n");
    printf("- Use non-blocking for variable workloads\n");
    printf("- Don't poll too frequently (<100μs interval)\n");
    printf("- Always eventually call blocking fetch\n");
    printf("- Prioritize tasks by importance\n");
    printf("- Monitor fetch wait times\n");

    cleanupPhysX();
    printf("\nExample completed successfully!\n");

    return 0;
}
