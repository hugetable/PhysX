/**
 * PhysX Snippet: VehicleMultithreading
 *
 * 演示多线程车辆仿真（Multithreaded Vehicle Simulation）
 *
 * 核心功能:
 * 1. 并行车辆更新
 * 2. 线程池管理
 * 3. 性能扩展性测试
 * 4. 线程安全设计
 *
 * 物理背景:
 *
 * 多线程仿真（Multithreaded Simulation）:
 * 在大规模车辆场景中（如交通流仿真），单线程计算成为瓶颈。
 * 多线程可以充分利用多核CPU，显著提升性能。
 *
 * 并行化策略:
 *
 * 1. 数据并行（Data Parallelism）:
 *    将N辆车分配到M个线程：
 *    - Thread 0: 车辆 0, M, 2M, ...
 *    - Thread 1: 车辆 1, M+1, 2M+1, ...
 *    - ...
 *
 * 2. 任务并行（Task Parallelism）:
 *    将车辆更新分解为多个任务：
 *    - 悬挂计算
 *    - 轮胎力计算
 *    - 车身动力学
 *
 * 3. 流水线并行（Pipeline Parallelism）:
 *    不同阶段在不同线程执行：
 *    - 阶段1（Thread 0）: 输入处理
 *    - 阶段2（Thread 1）: 物理计算
 *    - 阶段3（Thread 2）: 输出处理
 *
 * Amdahl定律（Amdahl's Law）:
 *
 * 加速比理论上限:
 * Speedup = 1 / (s + p/n)
 * 其中:
 * - s: 串行部分比例
 * - p: 并行部分比例（s + p = 1）
 * - n: 线程数
 *
 * 示例:
 * 如果90%可并行（p=0.9, s=0.1）：
 * - 2线程: Speedup = 1 / (0.1 + 0.9/2) = 1.82
 * - 4线程: Speedup = 1 / (0.1 + 0.9/4) = 3.08
 * - 8线程: Speedup = 1 / (0.1 + 0.9/8) = 4.71
 * - 无穷线程: Speedup_max = 1 / 0.1 = 10
 *
 * 线程开销（Threading Overhead）:
 *
 * 1. 上下文切换（Context Switch）:
 *    T_overhead = T_switch × N_switches
 *    典型值: 1-10 μs per switch
 *
 * 2. 缓存失效（Cache Miss）:
 *    多线程访问共享数据导致缓存失效
 *    False Sharing: 不同线程写同一cache line
 *
 * 3. 同步开销（Synchronization）:
 *    锁、原子操作、内存屏障
 *
 * 线程安全设计:
 *
 * 1. 无锁设计（Lock-Free）:
 *    - 每辆车数据独立
 *    - 避免共享可写数据
 *    - 原子操作代替锁
 *
 * 2. 读写分离:
 *    - 输入阶段：只读
 *    - 计算阶段：读写独立数据
 *    - 输出阶段：只读
 *
 * 3. 数据局部性（Data Locality）:
 *    - 紧凑数组布局
 *    - 缓存行对齐
 *    - 避免false sharing
 *
 * 性能指标:
 *
 * 加速比（Speedup）:
 * S = T_single / T_multi
 *
 * 并行效率（Parallel Efficiency）:
 * E = S / n
 * 理想情况: E = 1（线性扩展）
 *
 * 吞吐量（Throughput）:
 * Throughput = N_vehicles / T_frame
 *
 * 应用场景:
 * 1. 交通流仿真（成百上千车辆）
 * 2. 大规模游戏场景
 * 3. 自动驾驶测试
 * 4. 城市规划仿真
 *
 * 注意:
 * ⚠️ 线程数不宜超过CPU核心数
 * ⚠️ 避免false sharing（填充cache line）
 * ⚠️ 测量实际加速比，不要假设线性
 */

#include <PxPhysicsAPI.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cmath>

using namespace physx;

// ============================================================================
// 简化的车辆数据结构
// ============================================================================

/**
 * 车辆状态（对齐到cache line避免false sharing）
 */
struct alignas(64) VehicleData {
    PxVec3 position;
    PxVec3 velocity;
    PxQuat orientation;
    PxVec3 angularVelocity;

    PxReal wheelAngularVel[4];
    PxReal suspensionCompression[4];

    PxReal throttle;
    PxReal brake;
    PxReal steer;

    PxReal mass;
    PxVec3 inertia;

    // 填充到64字节（cache line大小）
    char padding[64 - sizeof(PxVec3)*2 - sizeof(PxQuat) - sizeof(PxVec3) -
                 sizeof(PxReal)*4 - sizeof(PxReal)*4 - sizeof(PxReal)*3 -
                 sizeof(PxReal) - sizeof(PxVec3)];
};

// ============================================================================
// 线程池实现
// ============================================================================

/**
 * 简单的任务
 */
struct Task {
    int startIndex;
    int endIndex;
    std::vector<VehicleData>* vehicles;
    PxReal dt;
};

/**
 * 简单的线程池
 */
class SimpleThreadPool {
public:
    SimpleThreadPool(int numThreads) : stopFlag(false) {
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back([this]() {
                this->workerThread();
            });
        }
    }

    ~SimpleThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stopFlag = true;
        }
        condition.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void enqueueTask(const Task& task) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            tasks.push_back(task);
        }
        condition.notify_one();
    }

    void waitAll() {
        std::unique_lock<std::mutex> lock(queueMutex);
        doneCondition.wait(lock, [this]() {
            return tasks.empty() && activeTasks == 0;
        });
    }

private:
    void workerThread() {
        while (true) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, [this]() {
                    return stopFlag || !tasks.empty();
                });

                if (stopFlag && tasks.empty()) {
                    return;
                }

                task = tasks.front();
                tasks.erase(tasks.begin());
                activeTasks++;
            }

            // 执行任务
            processVehicleRange(task);

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                activeTasks--;
            }
            doneCondition.notify_all();
        }
    }

    void processVehicleRange(const Task& task) {
        for (int i = task.startIndex; i < task.endIndex; ++i) {
            updateVehicle((*task.vehicles)[i], task.dt);
        }
    }

    static void updateVehicle(VehicleData& vehicle, PxReal dt) {
        // 简化的车辆更新（模拟计算负载）

        // 1. 应用输入
        PxVec3 forwardDir = vehicle.orientation.rotate(PxVec3(1, 0, 0));

        // 2. 计算驱动力
        PxReal driveForce = vehicle.throttle * 5000.0f;
        PxVec3 totalForce = forwardDir * driveForce;

        // 3. 计算制动力
        PxReal brakeForce = vehicle.brake * 8000.0f;
        PxReal currentSpeed = vehicle.velocity.magnitude();
        if (currentSpeed > 0.1f) {
            totalForce -= vehicle.velocity.getNormalized() * brakeForce;
        }

        // 4. 空气阻力
        PxReal dragCoeff = 0.3f;
        totalForce -= vehicle.velocity * dragCoeff;

        // 5. 更新速度和位置
        PxVec3 acceleration = totalForce / vehicle.mass;
        vehicle.velocity += acceleration * dt;
        vehicle.position += vehicle.velocity * dt;

        // 6. 转向（简化）
        PxReal steerRate = vehicle.steer * 2.0f;  // rad/s
        PxQuat steerRotation(steerRate * dt, PxVec3(0, 0, 1));
        vehicle.orientation = steerRotation * vehicle.orientation;
        vehicle.orientation.normalize();

        // 7. 模拟计算负载（悬挂、轮胎等）
        for (int w = 0; w < 4; ++w) {
            // 简化的悬挂计算
            vehicle.suspensionCompression[w] = 0.1f + 0.05f * PxSin(vehicle.position.x * 0.1f + w);

            // 简化的车轮角速度
            PxReal wheelRadius = 0.35f;
            vehicle.wheelAngularVel[w] = vehicle.velocity.magnitude() / wheelRadius;
        }
    }

    std::vector<std::thread> workers;
    std::vector<Task> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable doneCondition;
    bool stopFlag;
    std::atomic<int> activeTasks{0};
};

// ============================================================================
// 性能测试
// ============================================================================

/**
 * 单线程更新
 */
double updateVehiclesSingleThreaded(std::vector<VehicleData>& vehicles, PxReal dt) {
    auto start = std::chrono::high_resolution_clock::now();

    for (auto& vehicle : vehicles) {
        SimpleThreadPool::updateVehicle(vehicle, dt);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    return duration.count();
}

/**
 * 多线程更新
 */
double updateVehiclesMultiThreaded(std::vector<VehicleData>& vehicles, PxReal dt, int numThreads) {
    SimpleThreadPool pool(numThreads);

    auto start = std::chrono::high_resolution_clock::now();

    // 分配任务
    int vehiclesPerThread = (vehicles.size() + numThreads - 1) / numThreads;

    for (int t = 0; t < numThreads; ++t) {
        Task task;
        task.startIndex = t * vehiclesPerThread;
        task.endIndex = PxMin((t + 1) * vehiclesPerThread, static_cast<int>(vehicles.size()));
        task.vehicles = &vehicles;
        task.dt = dt;

        if (task.startIndex < task.endIndex) {
            pool.enqueueTask(task);
        }
    }

    pool.waitAll();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    return duration.count();
}

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 线程数扩展性测试
 */
void demonstrateThreadScaling() {
    std::cout << "\n=== 场景1: 线程数扩展性测试 ===" << std::endl;
    std::cout << "测试不同线程数对1000辆车更新的性能影响" << std::endl;

    const int numVehicles = 1000;
    const PxReal dt = 1.0f / 60.0f;

    // 初始化车辆
    std::vector<VehicleData> vehicles(numVehicles);
    for (int i = 0; i < numVehicles; ++i) {
        vehicles[i].position = PxVec3(i % 50, i / 50, 0);
        vehicles[i].velocity = PxVec3(10, 0, 0);
        vehicles[i].orientation = PxQuat(PxIdentity);
        vehicles[i].mass = 1500.0f;
        vehicles[i].throttle = 0.5f;
    }

    std::cout << "\n线程数\t时间(ms)\t加速比\t\t并行效率" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;

    // 单线程基准
    double singleThreadTime = updateVehiclesSingleThreaded(vehicles, dt);
    std::cout << "1\t" << singleThreadTime << "\t1.00\t\t100.0%" << std::endl;

    // 测试不同线程数
    int threadCounts[] = {2, 4, 8, 16};
    for (int numThreads : threadCounts) {
        double multiThreadTime = updateVehiclesMultiThreaded(vehicles, dt, numThreads);
        double speedup = singleThreadTime / multiThreadTime;
        double efficiency = (speedup / numThreads) * 100.0;

        std::cout << numThreads << "\t"
                  << multiThreadTime << "\t\t"
                  << speedup << "\t\t"
                  << efficiency << "%" << std::endl;
    }

    std::cout << "\n观察:" << std::endl;
    std::cout << "- 加速比随线程数增加，但不是线性的" << std::endl;
    std::cout << "- 线程数超过CPU核心数后，效率下降" << std::endl;
    std::cout << "- 开销（同步、缓存）限制了最大加速比" << std::endl;
}

/**
 * 场景2: 车辆数量扩展性测试
 */
void demonstrateVehicleScaling() {
    std::cout << "\n=== 场景2: 车辆数量扩展性测试 ===" << std::endl;
    std::cout << "测试不同车辆数量的性能（固定4线程）" << std::endl;

    const int numThreads = 4;
    const PxReal dt = 1.0f / 60.0f;

    int vehicleCounts[] = {100, 500, 1000, 2000, 5000};

    std::cout << "\n车辆数\t单线程(ms)\t多线程(ms)\t加速比" << std::endl;
    std::cout << "-----------------------------------------------------------" << std::endl;

    for (int numVehicles : vehicleCounts) {
        std::vector<VehicleData> vehicles(numVehicles);
        for (int i = 0; i < numVehicles; ++i) {
            vehicles[i].position = PxVec3(i % 100, i / 100, 0);
            vehicles[i].velocity = PxVec3(10, 0, 0);
            vehicles[i].orientation = PxQuat(PxIdentity);
            vehicles[i].mass = 1500.0f;
            vehicles[i].throttle = 0.5f;
        }

        double singleTime = updateVehiclesSingleThreaded(vehicles, dt);
        double multiTime = updateVehiclesMultiThreaded(vehicles, dt, numThreads);
        double speedup = singleTime / multiTime;

        std::cout << numVehicles << "\t"
                  << singleTime << "\t\t"
                  << multiTime << "\t\t"
                  << speedup << std::endl;
    }

    std::cout << "\n观察:" << std::endl;
    std::cout << "- 车辆数越多，多线程优势越明显" << std::endl;
    std::cout << "- 少量车辆时，线程开销可能大于收益" << std::endl;
    std::cout << "- 吞吐量（车辆/秒）随车辆数线性增加" << std::endl;
}

/**
 * 场景3: Amdahl定律验证
 */
void demonstrateAmdahlLaw() {
    std::cout << "\n=== 场景3: Amdahl定律验证 ===" << std::endl;
    std::cout << "理论加速比 vs 实际测量" << std::endl;

    const int numVehicles = 2000;
    const PxReal dt = 1.0f / 60.0f;

    std::vector<VehicleData> vehicles(numVehicles);
    for (int i = 0; i < numVehicles; ++i) {
        vehicles[i].position = PxVec3(i % 100, i / 100, 0);
        vehicles[i].velocity = PxVec3(10, 0, 0);
        vehicles[i].orientation = PxQuat(PxIdentity);
        vehicles[i].mass = 1500.0f;
        vehicles[i].throttle = 0.5f;
    }

    double singleTime = updateVehiclesSingleThreaded(vehicles, dt);

    std::cout << "\n线程数\t实际加速比\tAmdahl理论(s=0.05)\tAmdahl理论(s=0.1)" << std::endl;
    std::cout << "-----------------------------------------------------------------------" << std::endl;

    int threadCounts[] = {1, 2, 4, 8, 16};
    for (int n : threadCounts) {
        double actualSpeedup = 1.0;
        if (n > 1) {
            double multiTime = updateVehiclesMultiThreaded(vehicles, dt, n);
            actualSpeedup = singleTime / multiTime;
        }

        // Amdahl定律：Speedup = 1 / (s + p/n)
        double s1 = 0.05;  // 5%串行
        double s2 = 0.1;   // 10%串行
        double amdahl1 = 1.0 / (s1 + (1.0 - s1) / n);
        double amdahl2 = 1.0 / (s2 + (1.0 - s2) / n);

        std::cout << n << "\t"
                  << actualSpeedup << "\t\t"
                  << amdahl1 << "\t\t\t"
                  << amdahl2 << std::endl;
    }

    std::cout << "\n结论:" << std::endl;
    std::cout << "- 实际加速比接近理论值（串行部分约5-10%）" << std::endl;
    std::cout << "- 即使少量串行代码也会显著限制加速比" << std::endl;
    std::cout << "- 优化串行部分比增加线程数更重要" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: VehicleMultithreading" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "\n演示多线程车辆仿真" << std::endl;

    std::cout << "\n系统信息:" << std::endl;
    std::cout << "CPU核心数: " << std::thread::hardware_concurrency() << std::endl;

    // 运行3个场景
    demonstrateThreadScaling();
    demonstrateVehicleScaling();
    demonstrateAmdahlLaw();

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\n多线程优化关键点:" << std::endl;
    std::cout << "1. 数据独立性 - 每辆车数据独立，无共享写" << std::endl;
    std::cout << "2. 缓存友好 - 对齐到cache line，避免false sharing" << std::endl;
    std::cout << "3. 负载均衡 - 均匀分配车辆到各线程" << std::endl;
    std::cout << "4. 最小化同步 - 只在开始和结束同步" << std::endl;

    std::cout << "\n理论公式:" << std::endl;
    std::cout << "Amdahl定律: Speedup = 1 / (s + p/n)" << std::endl;
    std::cout << "并行效率: E = Speedup / n" << std::endl;
    std::cout << "吞吐量: Throughput = N_vehicles / T_frame" << std::endl;

    std::cout << "\n实践建议:" << std::endl;
    std::cout << "1. 线程数 ≈ CPU核心数（避免过度订阅）" << std::endl;
    std::cout << "2. 车辆数 > 线程数 × 10（充分利用并行）" << std::endl;
    std::cout << "3. 测量实际性能（不要假设线性扩展）" << std::endl;
    std::cout << "4. Profile找瓶颈（可能在串行部分）" << std::endl;

    std::cout << "\n应用场景:" << std::endl;
    std::cout << "- 交通流仿真：成百上千车辆" << std::endl;
    std::cout << "- 大型多人游戏：NPC车辆群" << std::endl;
    std::cout << "- 自动驾驶测试：多场景并行" << std::endl;
    std::cout << "- 城市规划：长时间仿真" << std::endl;

    return 0;
}
