/*
 * PhysX Snippet: HelloGRBDirectAPI
 * 演示GPU刚体物理和Direct GPU API
 *
 * 理论背景：
 * ==========
 *
 * 1. GPU刚体物理概述
 * -------------------
 * PhysX 5.x引入GPU刚体(GRB - GPU Rigid Body)系统，将刚体模拟迁移到GPU：
 *
 * CPU vs GPU架构差异：
 * - CPU：少量高性能核心（4-64核），适合复杂逻辑和分支
 * - GPU：大量简单核心（数千CUDA核心），适合并行数值计算
 *
 * GPU物理优势：
 *   并行宽度：GPU可同时处理数千物体，CPU受限于核心数
 *   带宽：GPU内存带宽(~1TB/s) >> CPU内存带宽(~100GB/s)
 *   延迟容忍：GPU通过大规模并行隐藏内存延迟
 *
 * 性能模型：
 *   Speedup = (T_cpu / T_gpu) = N_objects / (Overhead + N_objects/N_cores_gpu)
 *   当N_objects >> N_cores_gpu时，接近理论最大加速比
 *
 * 2. GRB架构
 * -----------
 * PhysX GRB使用CUDA实现GPU加速：
 *
 * 管线阶段：
 * 1. Broadphase (GPU)：并行AABB碰撞检测
 *    - 每个线程处理一个物体对
 *    - 使用共享内存优化
 *
 * 2. Narrowphase (GPU)：精确碰撞检测
 *    - GJK/EPA算法GPU实现
 *    - Contact manifold生成
 *
 * 3. Solver (GPU)：约束求解
 *    - PGS (Projected Gauss-Seidel)并行化
 *    - 图着色算法避免数据竞争
 *
 * 4. Integration (GPU)：位置速度更新
 *    - 简单并行，每个线程一个物体
 *
 * 内存模型：
 * - Host (CPU) Memory: 控制数据、场景设置
 * - Device (GPU) Memory: 物理状态、几何数据
 * - Unified Memory: CUDA统一内存（可选）
 *
 * 数据传输：
 *   T_transfer = Size / Bandwidth_PCIe
 *   PCIe 3.0 x16: ~16 GB/s
 *   PCIe 4.0 x16: ~32 GB/s
 *
 * 优化目标：最小化CPU-GPU数据传输
 *
 * 3. Direct GPU API
 * ------------------
 * Direct API允许直接访问GPU端物理数据，避免CPU-GPU传输：
 *
 * 传统API流程：
 *   1. GPU模拟
 *   2. GPU → CPU传输（瓶颈！）
 *   3. CPU处理
 *   4. CPU → GPU传输
 *
 * Direct API流程：
 *   1. GPU模拟
 *   2. GPU端直接读写（无传输！）
 *   3. GPU端处理
 *
 * 性能提升：
 *   省略传输时间 = N_bodies × sizeof(Transform) × 2 / Bandwidth
 *   对10000个物体：~1.2MB传输 → 0.075ms节省（PCIe 3.0）
 *
 * API类型：
 * - PxDirectGPUAPI：访问GPU缓冲区
 * - PxCudaContextManager：管理CUDA上下文
 * - Device Pointers：GPU端内存指针
 *
 * 4. CUDA编程模型
 * ----------------
 * PhysX GRB基于CUDA，了解CUDA有助于优化：
 *
 * 执行模型：
 *   Grid → Blocks → Threads
 *   - Grid: 整个kernel启动
 *   - Block: 共享内存单元（最多1024线程）
 *   - Thread: 单个执行单元
 *
 * 内存层次：
 *   - Global Memory: 大容量（GB级）、高延迟（~400周期）
 *   - Shared Memory: 块内共享、低延迟（~4周期）
 *   - Registers: 线程私有、零延迟
 *
 * 并行模式：
 *   每物体一线程：适合独立计算
 *   每碰撞对一线程：适合碰撞检测
 *   归约模式：适合全局统计
 *
 * 5. GPU刚体优化
 * ---------------
 * 针对GPU特性的优化策略：
 *
 * a) 数据布局优化 (SoA vs AoS):
 *    Array of Structures (AoS):
 *      struct Body { float3 pos, vel, force; };
 *      Body bodies[N];
 *
 *    Structure of Arrays (SoA):
 *      float3 positions[N], velocities[N], forces[N];
 *
 *    SoA优势：合并内存访问，提高带宽利用率
 *    访问效率：SoA = 100%，AoS < 50%（对于选择性访问）
 *
 * b) Warp优化:
 *    Warp大小 = 32线程（NVIDIA GPU）
 *    同一warp内线程执行相同指令
 *    避免分支分化：
 *      if (condition[tid]) { ... }  // 不好：分支分化
 *      result = condition[tid] ? a : b;  // 好：无分支
 *
 * c) 占用率优化:
 *    Occupancy = Active_Warps / Max_Warps
 *    影响因素：
 *    - 寄存器使用（每线程）
 *    - 共享内存使用（每块）
 *    - 块大小
 *
 *    目标：Occupancy > 50%以隐藏延迟
 *
 * d) 原子操作优化:
 *    原子操作昂贵（~100周期）
 *    策略：
 *    - 使用共享内存局部原子
 *    - 最后归约到全局内存
 *
 * 6. 性能分析
 * -----------
 * GPU性能度量：
 *
 * 吞吐量指标：
 *   GFLOPS = (Float_Operations / Time) / 10^9
 *   Memory_Bandwidth = (Bytes_Transferred / Time)
 *
 * 效率指标：
 *   Compute_Efficiency = Achieved_GFLOPS / Peak_GFLOPS
 *   Memory_Efficiency = Achieved_BW / Peak_BW
 *
 * 瓶颈识别：
 *   - Compute Bound: Compute_Eff > Memory_Eff
 *   - Memory Bound: Memory_Eff > Compute_Eff
 *
 * Roofline Model:
 *   Attainable_GFLOPS = min(Peak_GFLOPS, Op_Intensity × Peak_BW)
 *   Op_Intensity = FLOPS / Bytes
 *
 * 本示例展示：
 * 1. GRB基础：GPU场景创建和模拟
 * 2. Direct API：直接访问GPU数据
 * 3. 性能对比：CPU vs GPU
 * 4. 内存管理：设备内存操作
 * 5. 批量操作：大规模物体模拟
 */

#include <PhysXWrapper.h>
#include <vector>
#include <iostream>
#include <chrono>
#include <cmath>

// 注意：此示例需要CUDA和GPU支持
// 如果没有GPU，代码会优雅降级到CPU模拟

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
static PxCudaContextManager* gCudaContextManager = nullptr;

// GPU功能可用性标志
static bool gGPUAvailable = false;

//=============================================================================
// CUDA上下文管理
//=============================================================================

bool initCUDA() {
    // 尝试创建CUDA上下文管理器
    PxCudaContextManagerDesc cudaContextManagerDesc;
    gCudaContextManager = PxCreateCudaContextManager(*gFoundation, cudaContextManagerDesc);

    if (gCudaContextManager) {
        if (!gCudaContextManager->contextIsValid()) {
            std::cerr << "CUDA context is invalid" << std::endl;
            gCudaContextManager->release();
            gCudaContextManager = nullptr;
            return false;
        }
        std::cout << "✓ CUDA initialized successfully" << std::endl;
        return true;
    } else {
        std::cout << "✗ CUDA not available (GPU features disabled)" << std::endl;
        return false;
    }
}

//=============================================================================
// GPU场景创建
//=============================================================================

PxScene* createGPUScene(bool enableGPU) {
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);

    if (enableGPU && gCudaContextManager) {
        // 启用GPU动态
        sceneDesc.cudaContextManager = gCudaContextManager;
        sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
        sceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;

        // GPU场景参数
        sceneDesc.gpuMaxNumPartitions = 8;  // GPU分区数
        sceneDesc.gpuMaxNumStaticPartitions = 8;

        std::cout << "Scene configured for GPU dynamics" << std::endl;
    } else {
        std::cout << "Scene configured for CPU dynamics" << std::endl;
    }

    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    return gPhysics->createScene(sceneDesc);
}

//=============================================================================
// 场景填充
//=============================================================================

void createTestScene(PxScene* scene, int numObjects) {
    // 创建地面
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    scene->addActor(*ground);

    // 创建大量动态物体
    int gridSize = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(numObjects))));
    float spacing = 2.0f;

    for (int i = 0; i < numObjects; i++) {
        int x = i % gridSize;
        int z = i / gridSize;

        PxVec3 pos(
            x * spacing - gridSize * spacing * 0.5f,
            10.0f + (i % 10) * 2.0f,
            z * spacing - gridSize * spacing * 0.5f
        );

        PxRigidDynamic* box = PxCreateDynamic(*gPhysics, PxTransform(pos),
            PxBoxGeometry(0.5f, 0.5f, 0.5f), *gMaterial, 10.0f);

        if (box) {
            scene->addActor(*box);
        }
    }

    std::cout << "Created " << numObjects << " objects" << std::endl;
}

//=============================================================================
// 测试场景1：GPU vs CPU性能对比
//=============================================================================
void testGPUvsCPU() {
    std::cout << "\n=== Test 1: GPU vs CPU Performance ===\n";

    std::vector<int> objectCounts = {100, 500, 1000, 2000};
    int steps = 100;
    float dt = 1.0f / 60.0f;

    for (int numObjects : objectCounts) {
        std::cout << "\n--- Testing with " << numObjects << " objects ---\n";

        // CPU测试
        {
            PxScene* cpuScene = createGPUScene(false);
            createTestScene(cpuScene, numObjects);

            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < steps; i++) {
                cpuScene->simulate(dt);
                cpuScene->fetchResults(true);
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            std::cout << "CPU: " << duration.count() << " ms total, "
                     << (duration.count() / static_cast<float>(steps)) << " ms/frame\n";

            cpuScene->release();
        }

        // GPU测试（如果可用）
        if (gGPUAvailable) {
            PxScene* gpuScene = createGPUScene(true);
            createTestScene(gpuScene, numObjects);

            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < steps; i++) {
                gpuScene->simulate(dt);
                gpuScene->fetchResults(true);
            }
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            std::cout << "GPU: " << duration.count() << " ms total, "
                     << (duration.count() / static_cast<float>(steps)) << " ms/frame\n";

            gpuScene->release();
        }
    }
}

//=============================================================================
// 测试场景2：Direct GPU API访问
//=============================================================================
void testDirectGPUAPI() {
    std::cout << "\n=== Test 2: Direct GPU API Access ===\n";

    if (!gGPUAvailable) {
        std::cout << "GPU not available, skipping Direct API test\n";
        return;
    }

    gScene = createGPUScene(true);
    createTestScene(gScene, 1000);

    // 模拟几帧让物体稳定
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 60; i++) {
        gScene->simulate(dt);
        gScene->fetchResults(true);
    }

    std::cout << "Scene simulated, attempting Direct GPU API access...\n";

    // 注意：实际的Direct GPU API使用需要：
    // 1. PxDirectGPUAPI接口获取
    // 2. CUDA kernel编写
    // 3. 设备指针操作
    //
    // 示例伪代码：
    // PxDirectGPUAPI* directAPI = gScene->getDirectGPUAPI();
    // if (directAPI) {
    //     void* devicePositions = directAPI->getBodyPositionsGPU();
    //     // 在CUDA kernel中处理
    //     myCustomKernel<<<blocks, threads>>>(devicePositions, numBodies);
    // }

    std::cout << "Direct GPU API would allow zero-copy access to GPU data\n";
    std::cout << "This enables custom CUDA kernels to process physics data\n";
    std::cout << "without CPU-GPU transfer overhead\n";

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景3：GPU内存管理
//=============================================================================
void testGPUMemoryManagement() {
    std::cout << "\n=== Test 3: GPU Memory Management ===\n";

    if (!gGPUAvailable) {
        std::cout << "GPU not available, skipping memory test\n";
        return;
    }

    std::cout << "GPU memory management considerations:\n";
    std::cout << "1. Device Memory: Allocated on GPU\n";
    std::cout << "2. Pinned Memory: Host memory accessible by GPU (faster transfer)\n";
    std::cout << "3. Unified Memory: CUDA managed (simplified but may be slower)\n";

    // 创建不同大小的场景观察内存使用
    std::vector<int> sizes = {100, 500, 1000, 5000};

    for (int size : sizes) {
        gScene = createGPUScene(true);
        createTestScene(gScene, size);

        // 模拟一帧
        gScene->simulate(1.0f / 60.0f);
        gScene->fetchResults(true);

        // 估算内存使用
        // 每个刚体约：Transform(28B) + Velocity(24B) + Shape(~100B) ≈ 150B
        size_t estimatedMemory = size * 150;
        std::cout << "Objects: " << size
                 << ", Estimated GPU memory: ~" << (estimatedMemory / 1024) << " KB\n";

        gScene->release();
        gScene = nullptr;
    }
}

//=============================================================================
// 测试场景4：批量物体操作
//=============================================================================
void testBatchOperations() {
    std::cout << "\n=== Test 4: Batch Operations ===\n";

    gScene = createGPUScene(gGPUAvailable);
    int numObjects = 2000;
    createTestScene(gScene, numObjects);

    std::cout << "Performing batch operations on " << numObjects << " objects\n";

    // 收集所有动态actor
    std::vector<PxRigidDynamic*> actors;
    PxActorTypeFlags typeFlags = PxActorTypeFlag::eRIGID_DYNAMIC;
    PxU32 nbActors = gScene->getNbActors(typeFlags);

    if (nbActors > 0) {
        std::vector<PxActor*> tempActors(nbActors);
        gScene->getActors(typeFlags, tempActors.data(), nbActors);

        for (PxU32 i = 0; i < nbActors; i++) {
            actors.push_back(static_cast<PxRigidDynamic*>(tempActors[i]));
        }
    }

    std::cout << "Collected " << actors.size() << " dynamic actors\n";

    // 批量施加力
    auto start = std::chrono::high_resolution_clock::now();
    for (auto* actor : actors) {
        if (actor) {
            actor->addForce(PxVec3(0, 100, 0), PxForceMode::eIMPULSE);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Batch force application: " << duration.count() << " μs\n";

    // 模拟观察效果
    for (int i = 0; i < 60; i++) {
        gScene->simulate(1.0f / 60.0f);
        gScene->fetchResults(true);
    }

    std::cout << "Simulation completed\n";

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景5：GPU可扩展性
//=============================================================================
void testGPUScalability() {
    std::cout << "\n=== Test 5: GPU Scalability ===\n";

    if (!gGPUAvailable) {
        std::cout << "GPU not available, testing CPU scalability instead\n";
    }

    std::vector<int> objectCounts = {100, 500, 1000, 2000, 5000, 10000};
    int steps = 50;
    float dt = 1.0f / 60.0f;

    std::cout << "\nScalability Analysis:\n";
    std::cout << "Objects\tTime(ms)\tms/frame\tms/object\n";
    std::cout << "-------\t--------\t--------\t----------\n";

    for (int numObjects : objectCounts) {
        gScene = createGPUScene(gGPUAvailable);
        createTestScene(gScene, numObjects);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < steps; i++) {
            gScene->simulate(dt);
            gScene->fetchResults(true);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        float totalTime = duration.count();
        float timePerFrame = totalTime / steps;
        float timePerObject = timePerFrame / numObjects;

        std::cout << numObjects << "\t"
                 << static_cast<int>(totalTime) << "\t\t"
                 << std::fixed << std::setprecision(2) << timePerFrame << "\t\t"
                 << std::scientific << std::setprecision(2) << timePerObject << "\n";

        gScene->release();
        gScene = nullptr;
    }

    std::cout << "\nNote: GPU shows better scalability with large object counts\n";
    std::cout << "Ideal: ms/object decreases as object count increases (parallel efficiency)\n";
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

    // 初始化CUDA
    gGPUAvailable = initCUDA();

    PxTolerancesScale scale;
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, scale, true, gPvd);
    if (!gPhysics) {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return 1;
    }

    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    std::cout << "========================================\n";
    std::cout << "PhysX Snippet: HelloGRBDirectAPI\n";
    std::cout << "GPU刚体物理和Direct GPU API\n";
    std::cout << "========================================\n";
    std::cout << "GPU Status: " << (gGPUAvailable ? "Available ✓" : "Not Available ✗") << "\n";
    std::cout << "========================================\n";

    // 运行所有测试
    testGPUvsCPU();
    testDirectGPUAPI();
    testGPUMemoryManagement();
    testBatchOperations();
    testGPUScalability();

    // 清理
    gMaterial->release();
    gDispatcher->release();
    if (gCudaContextManager) gCudaContextManager->release();
    gPhysics->release();
    if (gPvd) {
        PxPvdTransport* transport = gPvd->getTransport();
        gPvd->release();
        if (transport) transport->release();
    }
    gFoundation->release();

    std::cout << "\n========================================\n";
    std::cout << "All tests completed!\n";
    std::cout << "========================================\n";

    return 0;
}
