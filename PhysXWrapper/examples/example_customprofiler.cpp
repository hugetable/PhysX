/*
 * PhysX Snippet: CustomProfiler
 * 演示自定义性能分析器实现
 *
 * 理论背景：
 * ==========
 *
 * 1. 性能分析基础
 * ----------------
 * 性能分析(Profiling)是识别程序性能瓶颈的关键技术。
 *
 * 性能指标：
 * a) 时间指标：
 *    - Frame Time: 单帧总耗时
 *    - CPU Time: CPU计算时间
 *    - GPU Time: GPU渲染时间（本例不涉及）
 *    - Wall Clock Time: 实际墙上时钟时间
 *
 * b) 吞吐量指标：
 *    - FPS (Frames Per Second): 帧率
 *      FPS = 1 / FrameTime
 *    - Objects/sec: 每秒处理物体数
 *    - Contacts/sec: 每秒处理接触数
 *
 * c) 内存指标：
 *    - Peak Memory: 峰值内存使用
 *    - Average Memory: 平均内存使用
 *    - Allocations: 分配次数
 *
 * 2. 层级分析(Hierarchical Profiling)
 * -------------------------------------
 * 使用树状结构组织性能数据，便于定位瓶颈：
 *
 * Frame (16.67ms)
 * ├── Physics (12ms)
 * │   ├── Broadphase (3ms)
 * │   ├── Narrowphase (6ms)
 * │   └── Solver (3ms)
 * ├── Rendering (3ms)
 * └── Game Logic (1.67ms)
 *
 * 树遍历算法：
 * ```
 * function enterZone(name):
 *   node = currentNode.findOrCreateChild(name)
 *   node.startTime = getCurrentTime()
 *   push(currentNode)
 *   currentNode = node
 *
 * function leaveZone():
 *   node = currentNode
 *   node.elapsedTime += getCurrentTime() - node.startTime
 *   node.callCount++
 *   currentNode = pop()
 * ```
 *
 * 3. 采样性能分析(Sampling Profiler)
 * ------------------------------------
 * 定期采样程序状态，统计热点函数：
 *
 * 采样原理：
 *   每隔Δt时间中断程序，记录当前调用栈
 *   热点函数 = 出现次数最多的栈帧
 *
 * 采样频率：
 *   f_sample = 1000 Hz (典型值)
 *   Trade-off: 高频率→高精度但高开销
 *
 * 统计置信度：
 *   采样误差 ε ≈ 1/√N
 *   其中N为采样点数
 *   例：N=10000 → ε≈1% 误差
 *
 * 4. 埋点性能分析(Instrumentation Profiler)
 * -------------------------------------------
 * 在代码中插入测量点，精确记录执行时间。
 *
 * RAII模式埋点：
 * ```cpp
 * class ScopedTimer {
 *   TimePoint start;
 *   const char* name;
 * public:
 *   ScopedTimer(const char* n) : name(n) {
 *     start = now();
 *   }
 *   ~ScopedTimer() {
 *     profiler.record(name, now() - start);
 *   }
 * };
 * ```
 *
 * 使用：
 * ```cpp
 * void simulate() {
 *   ScopedTimer timer("simulate");
 *   // ... 模拟代码
 * } // 离开作用域自动记录
 * ```
 *
 * 5. PhysX性能分析接口
 * ---------------------
 * PhysX提供内置性能分析工具：
 *
 * a) PxProfilerCallback:
 *    虚接口，接收PhysX内部事件
 * ```cpp
 * class PxProfilerCallback {
 *   virtual void* zoneStart(const char* eventName,
 *                           bool detached, uint64_t contextId) = 0;
 *   virtual void zoneEnd(void* profilerData,
 *                        const char* eventName,
 *                        bool detached, uint64_t contextId) = 0;
 * };
 * ```
 *
 * b) 事件类型：
 *    - Broadphase events
 *    - Narrowphase events
 *    - Solver events
 *    - Scene query events
 *
 * c) 性能统计：
 * ```cpp
 * PxSimulationStatistics stats;
 * scene->getSimulationStatistics(stats);
 * // stats包含：
 * // - nbActiveConstraints: 活动约束数
 * // - nbActiveDynamicBodies: 活动动态物体数
 * // - nbNewPairs: 新碰撞对数
 * // - nbLostPairs: 丢失碰撞对数
 * ```
 *
 * 6. 统计分析
 * -----------
 * 对性能数据进行统计分析：
 *
 * a) 均值 (Mean):
 *    μ = (1/N) Σxᵢ
 *
 * b) 方差 (Variance):
 *    σ² = (1/N) Σ(xᵢ - μ)²
 *
 * c) 标准差 (Standard Deviation):
 *    σ = √(σ²)
 *
 * d) 百分位数 (Percentile):
 *    P₉₅：95%的帧时间 ≤ P₉₅
 *    用于识别偶发性能尖峰
 *
 * e) 移动平均 (Moving Average):
 *    MA(t) = (1/k) Σ(i=0..k-1) x(t-i)
 *    平滑噪声，显示趋势
 *
 * 7. 性能优化策略
 * ----------------
 * 基于分析结果的优化方法：
 *
 * a) 阿姆达尔定律 (Amdahl's Law):
 *    加速比 S = 1 / ((1-P) + P/N)
 *    其中：
 *    - P: 可并行部分占比
 *    - N: 处理器数量
 *
 *    示例：若Broadphase占50%且完全并行化到4核
 *    S = 1 / (0.5 + 0.5/4) = 1.6x
 *
 * b) 热点优化 (Hotspot Optimization):
 *    优先优化占比最高的模块
 *    80/20法则：80%时间花在20%代码上
 *
 * c) 缓存优化 (Cache Optimization):
 *    - 数据局部性：连续访问提高缓存命中率
 *    - 对齐：按缓存行大小对齐（64字节）
 *    - 预取：提前加载数据到缓存
 *
 * d) 算法复杂度优化：
 *    - O(n²) → O(n log n): 排序算法
 *    - O(n) → O(log n): 空间分区
 *    - O(n) → O(1): 哈希表查找
 *
 * 8. 性能预算(Performance Budget)
 * --------------------------------
 * 为各模块分配时间预算：
 *
 * 60 FPS目标 = 16.67ms/frame
 * - Physics: 8ms (48%)
 * - Rendering: 6ms (36%)
 * - Game Logic: 2ms (12%)
 * - Misc: 0.67ms (4%)
 *
 * 预警机制：
 *   if (actualTime > budget * 1.2) {
 *     WARNING("Budget exceeded!");
 *   }
 *
 * 本示例展示：
 * 1. 基础Profiler：层级时间测量
 * 2. 统计分析：均值、方差、百分位数
 * 3. PhysX集成：捕获内部事件
 * 4. 性能预算：超预算告警
 * 5. 对比分析：优化前后对比
 */

#include <PhysXWrapper.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <stack>

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

// 高精度时钟
using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Duration = std::chrono::duration<double, std::milli>;

//=============================================================================
// 性能分析器实现
//=============================================================================

// 性能统计数据
struct ProfileStats {
    double mean = 0.0;
    double variance = 0.0;
    double stddev = 0.0;
    double min = 0.0;
    double max = 0.0;
    double median = 0.0;
    double p95 = 0.0;  // 95百分位数
    double p99 = 0.0;  // 99百分位数
    int count = 0;

    void compute(std::vector<double>& samples) {
        if (samples.empty()) return;

        count = samples.size();

        // 排序用于百分位数计算
        std::sort(samples.begin(), samples.end());

        // 最小值和最大值
        min = samples.front();
        max = samples.back();

        // 均值
        double sum = 0.0;
        for (double v : samples) sum += v;
        mean = sum / count;

        // 方差和标准差
        double sumSq = 0.0;
        for (double v : samples) {
            double diff = v - mean;
            sumSq += diff * diff;
        }
        variance = sumSq / count;
        stddev = std::sqrt(variance);

        // 百分位数
        median = samples[count / 2];
        p95 = samples[static_cast<int>(count * 0.95)];
        p99 = samples[static_cast<int>(count * 0.99)];
    }

    void print(const std::string& name) const {
        std::cout << name << " Statistics:\n";
        std::cout << "  Count: " << count << "\n";
        std::cout << "  Mean: " << mean << " ms\n";
        std::cout << "  Stddev: " << stddev << " ms\n";
        std::cout << "  Min: " << min << " ms\n";
        std::cout << "  Max: " << max << " ms\n";
        std::cout << "  Median: " << median << " ms\n";
        std::cout << "  P95: " << p95 << " ms\n";
        std::cout << "  P99: " << p99 << " ms\n";
    }
};

// 性能区域节点
struct ProfileZone {
    std::string name;
    TimePoint startTime;
    double totalTime = 0.0;
    int callCount = 0;
    std::vector<double> samples;
    std::unordered_map<std::string, ProfileZone*> children;
    ProfileZone* parent = nullptr;

    ~ProfileZone() {
        for (auto& pair : children) {
            delete pair.second;
        }
    }

    ProfileZone* getOrCreateChild(const std::string& childName) {
        auto it = children.find(childName);
        if (it != children.end()) {
            return it->second;
        }
        ProfileZone* child = new ProfileZone();
        child->name = childName;
        child->parent = this;
        children[childName] = child;
        return child;
    }

    void printHierarchy(int depth = 0) const {
        std::string indent(depth * 2, ' ');
        double avgTime = callCount > 0 ? totalTime / callCount : 0.0;
        std::cout << indent << name << ": "
                 << std::fixed << std::setprecision(3)
                 << "total=" << totalTime << "ms, "
                 << "calls=" << callCount << ", "
                 << "avg=" << avgTime << "ms\n";

        for (const auto& pair : children) {
            pair.second->printHierarchy(depth + 1);
        }
    }

    ProfileStats getStats() {
        ProfileStats stats;
        std::vector<double> samplesCopy = samples;
        stats.compute(samplesCopy);
        return stats;
    }
};

// 层级性能分析器
class HierarchicalProfiler {
private:
    ProfileZone root;
    ProfileZone* current = nullptr;
    std::stack<ProfileZone*> stack;

public:
    HierarchicalProfiler() {
        root.name = "Root";
        current = &root;
    }

    void enterZone(const std::string& name) {
        ProfileZone* zone = current->getOrCreateChild(name);
        zone->startTime = Clock::now();
        stack.push(current);
        current = zone;
    }

    void leaveZone() {
        if (stack.empty()) return;

        TimePoint endTime = Clock::now();
        double elapsed = Duration(endTime - current->startTime).count();

        current->totalTime += elapsed;
        current->callCount++;
        current->samples.push_back(elapsed);

        current = stack.top();
        stack.pop();
    }

    void reset() {
        root.totalTime = 0.0;
        root.callCount = 0;
        root.samples.clear();
        for (auto& pair : root.children) {
            delete pair.second;
        }
        root.children.clear();
        current = &root;
        while (!stack.empty()) stack.pop();
    }

    void printReport() const {
        std::cout << "\n=== Hierarchical Profile Report ===\n";
        root.printHierarchy();
    }

    ProfileZone* getRoot() { return &root; }
};

// RAII作用域计时器
class ScopedTimer {
private:
    HierarchicalProfiler& profiler;
    std::string name;

public:
    ScopedTimer(HierarchicalProfiler& p, const std::string& n)
        : profiler(p), name(n) {
        profiler.enterZone(name);
    }

    ~ScopedTimer() {
        profiler.leaveZone();
    }
};

// PhysX性能回调
class CustomProfilerCallback : public PxProfilerCallback {
private:
    HierarchicalProfiler& profiler;
    std::unordered_map<const void*, std::string> activeZones;

public:
    CustomProfilerCallback(HierarchicalProfiler& p) : profiler(p) {}

    virtual void* zoneStart(const char* eventName, bool detached, uint64_t contextId) override {
        profiler.enterZone(eventName);
        const void* handle = reinterpret_cast<const void*>(contextId);
        activeZones[handle] = eventName;
        return const_cast<void*>(handle);
    }

    virtual void zoneEnd(void* profilerData, const char* eventName,
                        bool detached, uint64_t contextId) override {
        profiler.leaveZone();
        const void* handle = reinterpret_cast<const void*>(contextId);
        activeZones.erase(handle);
    }
};

// 性能预算管理器
class PerformanceBudget {
private:
    struct Budget {
        std::string name;
        double allocated;  // ms
        double actual;     // ms
        double threshold;  // 超标阈值（倍数）

        bool isExceeded() const {
            return actual > allocated * threshold;
        }

        double getUsagePercent() const {
            return (actual / allocated) * 100.0;
        }
    };

    std::unordered_map<std::string, Budget> budgets;

public:
    void addBudget(const std::string& name, double allocatedMs, double threshold = 1.2) {
        Budget b;
        b.name = name;
        b.allocated = allocatedMs;
        b.actual = 0.0;
        b.threshold = threshold;
        budgets[name] = b;
    }

    void recordActual(const std::string& name, double actualMs) {
        auto it = budgets.find(name);
        if (it != budgets.end()) {
            it->second.actual = actualMs;
        }
    }

    void printReport() const {
        std::cout << "\n=== Performance Budget Report ===\n";
        std::cout << std::fixed << std::setprecision(2);

        for (const auto& pair : budgets) {
            const Budget& b = pair.second;
            std::cout << b.name << ":\n";
            std::cout << "  Allocated: " << b.allocated << " ms\n";
            std::cout << "  Actual: " << b.actual << " ms\n";
            std::cout << "  Usage: " << b.getUsagePercent() << "%\n";

            if (b.isExceeded()) {
                std::cout << "  ⚠️ WARNING: Budget exceeded by "
                         << (b.actual - b.allocated) << " ms!\n";
            } else {
                std::cout << "  ✓ Within budget\n";
            }
        }
    }
};

//=============================================================================
// 场景创建辅助函数
//=============================================================================

void createTestScene(PxScene* scene, int complexity) {
    // 创建地面
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    scene->addActor(*ground);

    // 创建动态物体
    for (int i = 0; i < complexity; i++) {
        for (int j = 0; j < complexity; j++) {
            PxVec3 pos(i * 2.0f, 10.0f + j * 2.0f, 0);
            PxRigidDynamic* box = PxCreateDynamic(*gPhysics, PxTransform(pos),
                PxBoxGeometry(0.5f, 0.5f, 0.5f), *gMaterial, 10.0f);
            scene->addActor(*box);
        }
    }
}

//=============================================================================
// 测试场景1：基础层级Profiler
//=============================================================================
void testBasicHierarchicalProfiler() {
    std::cout << "\n=== Test 1: Basic Hierarchical Profiler ===\n";

    HierarchicalProfiler profiler;

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(sceneDesc);

    createTestScene(gScene, 10);  // 100个物体

    PxReal dt = 1.0f / 60.0f;

    for (int i = 0; i < 100; i++) {
        {
            ScopedTimer frameTimer(profiler, "Frame");
            {
                ScopedTimer physicsTimer(profiler, "Physics");
                {
                    ScopedTimer simTimer(profiler, "Simulate");
                    gScene->simulate(dt);
                }
                {
                    ScopedTimer fetchTimer(profiler, "FetchResults");
                    gScene->fetchResults(true);
                }
            }
            {
                ScopedTimer renderTimer(profiler, "Rendering");
                // 模拟渲染工作
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        }
    }

    profiler.printReport();

    // 获取Frame统计
    ProfileZone* frameZone = profiler.getRoot()->children["Frame"];
    if (frameZone) {
        ProfileStats stats = frameZone->getStats();
        stats.print("Frame");
    }

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景2：统计分析
//=============================================================================
void testStatisticalAnalysis() {
    std::cout << "\n=== Test 2: Statistical Analysis ===\n";

    HierarchicalProfiler profiler;

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(sceneDesc);

    createTestScene(gScene, 15);  // 225个物体

    PxReal dt = 1.0f / 60.0f;

    for (int i = 0; i < 200; i++) {
        profiler.enterZone("Simulate");
        gScene->simulate(dt);
        profiler.leaveZone();

        profiler.enterZone("FetchResults");
        gScene->fetchResults(true);
        profiler.leaveZone();

        // 模拟偶发性能尖峰
        if (i % 50 == 0) {
            profiler.enterZone("Spike");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            profiler.leaveZone();
        }
    }

    // 打印各模块统计
    ProfileZone* root = profiler.getRoot();
    for (const auto& pair : root->children) {
        ProfileStats stats = pair.second->getStats();
        stats.print(pair.first);
    }

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景3：PhysX内部事件捕获
//=============================================================================
void testPhysXProfilerIntegration() {
    std::cout << "\n=== Test 3: PhysX Profiler Integration ===\n";

    HierarchicalProfiler profiler;
    CustomProfilerCallback callback(profiler);

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(sceneDesc);

    // 设置profiler回调
    gPhysics->setProfilerCallback(&callback);

    createTestScene(gScene, 12);  // 144个物体

    PxReal dt = 1.0f / 60.0f;

    for (int i = 0; i < 60; i++) {
        gScene->simulate(dt);
        gScene->fetchResults(true);
    }

    profiler.printReport();

    gPhysics->setProfilerCallback(nullptr);
    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景4：性能预算管理
//=============================================================================
void testPerformanceBudget() {
    std::cout << "\n=== Test 4: Performance Budget Management ===\n";

    HierarchicalProfiler profiler;
    PerformanceBudget budget;

    // 设置预算（目标60 FPS = 16.67ms）
    budget.addBudget("Physics", 10.0);
    budget.addBudget("Rendering", 5.0);
    budget.addBudget("GameLogic", 1.5);

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(sceneDesc);

    createTestScene(gScene, 20);  // 400个物体（可能超预算）

    PxReal dt = 1.0f / 60.0f;
    std::vector<double> physicsTimes;
    std::vector<double> renderTimes;
    std::vector<double> logicTimes;

    for (int i = 0; i < 100; i++) {
        // Physics
        {
            profiler.enterZone("Physics");
            gScene->simulate(dt);
            gScene->fetchResults(true);
            profiler.leaveZone();
        }

        // Rendering (模拟)
        {
            profiler.enterZone("Rendering");
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
            profiler.leaveZone();
        }

        // Game Logic (模拟)
        {
            profiler.enterZone("GameLogic");
            std::this_thread::sleep_for(std::chrono::microseconds(800));
            profiler.leaveZone();
        }
    }

    // 记录实际时间
    ProfileZone* root = profiler.getRoot();
    if (root->children.count("Physics")) {
        double avgPhysics = root->children["Physics"]->totalTime / root->children["Physics"]->callCount;
        budget.recordActual("Physics", avgPhysics);
    }
    if (root->children.count("Rendering")) {
        double avgRender = root->children["Rendering"]->totalTime / root->children["Rendering"]->callCount;
        budget.recordActual("Rendering", avgRender);
    }
    if (root->children.count("GameLogic")) {
        double avgLogic = root->children["GameLogic"]->totalTime / root->children["GameLogic"]->callCount;
        budget.recordActual("GameLogic", avgLogic);
    }

    budget.printReport();

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景5：优化前后对比
//=============================================================================
void testBeforeAfterComparison() {
    std::cout << "\n=== Test 5: Before/After Optimization Comparison ===\n";

    HierarchicalProfiler profilerBefore;
    HierarchicalProfiler profilerAfter;

    // 优化前：大量小物体，无sleeping
    {
        std::cout << "\n--- Before Optimization ---\n";

        PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
        gScene = gPhysics->createScene(sceneDesc);

        // 创建很多物体
        for (int i = 0; i < 30; i++) {
            for (int j = 0; j < 30; j++) {
                PxVec3 pos(i * 1.5f, 5.0f, j * 1.5f);
                PxRigidDynamic* box = PxCreateDynamic(*gPhysics, PxTransform(pos),
                    PxBoxGeometry(0.5f, 0.5f, 0.5f), *gMaterial, 10.0f);
                // 禁用sleeping
                box->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, false);
                box->setSleepThreshold(0.0f);
                gScene->addActor(*box);
            }
        }

        PxReal dt = 1.0f / 60.0f;
        for (int i = 0; i < 100; i++) {
            profilerBefore.enterZone("Simulate");
            gScene->simulate(dt);
            profilerBefore.leaveZone();

            profilerBefore.enterZone("FetchResults");
            gScene->fetchResults(true);
            profilerBefore.leaveZone();
        }

        ProfileZone* simZone = profilerBefore.getRoot()->children["Simulate"];
        if (simZone) {
            ProfileStats stats = simZone->getStats();
            stats.print("Before - Simulate");
        }

        gScene->release();
        gScene = nullptr;
    }

    // 优化后：启用sleeping，减少活动物体
    {
        std::cout << "\n--- After Optimization ---\n";

        PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
        gScene = gPhysics->createScene(sceneDesc);

        // 同样的物体，但启用sleeping
        for (int i = 0; i < 30; i++) {
            for (int j = 0; j < 30; j++) {
                PxVec3 pos(i * 1.5f, 5.0f, j * 1.5f);
                PxRigidDynamic* box = PxCreateDynamic(*gPhysics, PxTransform(pos),
                    PxBoxGeometry(0.5f, 0.5f, 0.5f), *gMaterial, 10.0f);
                // 启用sleeping（默认阈值）
                gScene->addActor(*box);
            }
        }

        PxReal dt = 1.0f / 60.0f;
        for (int i = 0; i < 100; i++) {
            profilerAfter.enterZone("Simulate");
            gScene->simulate(dt);
            profilerAfter.leaveZone();

            profilerAfter.enterZone("FetchResults");
            gScene->fetchResults(true);
            profilerAfter.leaveZone();
        }

        ProfileZone* simZone = profilerAfter.getRoot()->children["Simulate"];
        if (simZone) {
            ProfileStats stats = simZone->getStats();
            stats.print("After - Simulate");
        }

        gScene->release();
        gScene = nullptr;
    }

    // 对比结果
    std::cout << "\n--- Comparison ---\n";
    ProfileZone* beforeSim = profilerBefore.getRoot()->children["Simulate"];
    ProfileZone* afterSim = profilerAfter.getRoot()->children["Simulate"];

    if (beforeSim && afterSim) {
        double avgBefore = beforeSim->totalTime / beforeSim->callCount;
        double avgAfter = afterSim->totalTime / afterSim->callCount;
        double speedup = avgBefore / avgAfter;

        std::cout << "Average Simulate time:\n";
        std::cout << "  Before: " << avgBefore << " ms\n";
        std::cout << "  After: " << avgAfter << " ms\n";
        std::cout << "  Speedup: " << speedup << "x\n";
        std::cout << "  Improvement: " << ((speedup - 1.0) * 100.0) << "%\n";
    }
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
    std::cout << "PhysX Snippet: CustomProfiler\n";
    std::cout << "自定义性能分析器实现\n";
    std::cout << "========================================\n";

    // 运行所有测试
    testBasicHierarchicalProfiler();
    testStatisticalAnalysis();
    testPhysXProfilerIntegration();
    testPerformanceBudget();
    testBeforeAfterComparison();

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
