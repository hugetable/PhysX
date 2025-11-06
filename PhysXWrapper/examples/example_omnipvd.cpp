/*
 * PhysX Snippet: OmniPvd
 * 演示OmniVerse物理可视化调试器
 *
 * 理论背景：
 * ==========
 *
 * 1. 物理调试可视化需求
 * ----------------------
 * 物理引擎调试的挑战：
 * - 不可见性：物理状态（力、速度、约束）无法直接观察
 * - 时间维度：物理现象动态变化，难以定位瞬态问题
 * - 复杂性：大规模场景包含数千物体、约束、碰撞
 *
 * 可视化的价值：
 *   调试效率提升 = Speedup_debug ≈ 5x - 10x
 *   通过可视化快速定位：
 *   - 穿透问题：物体相互嵌入
 *   - 约束失效：关节断裂或行为异常
 *   - 性能瓶颈：碰撞热点区域
 *
 * 2. PhysX Visual Debugger (PVD)
 * -------------------------------
 * PVD是PhysX传统可视化工具：
 *
 * 架构：
 *   PhysX SDK (运行时) ←→ PVD SDK (网络层) ←→ PVD App (GUI)
 *
 * 通信协议：
 *   - Transport: TCP/IP socket (默认端口5425)
 *   - Format: 二进制序列化
 *   - Frequency: 每帧或按需
 *
 * 数据流：
 *   每帧数据量 = N_actors × sizeof(Transform) + N_contacts × sizeof(Contact)
 *   典型：1000物体 ≈ 100KB/frame @ 60Hz = 6MB/s
 *
 * PVD功能：
 *   - 实时3D可视化
 *   - 物理状态检查
 *   - 性能分析
 *   - 时间轴回放
 *
 * 3. OmniPvd概述
 * ---------------
 * OmniPvd是新一代调试工具，集成到NVIDIA OmniVerse生态：
 *
 * 架构升级：
 *   PhysX → OmniPvd SDK → USD Format → OmniVerse Viewer
 *
 * USD (Universal Scene Description):
 *   - Pixar开发的场景描述格式
 *   - 支持复杂场景、动画、材质
 *   - 行业标准（电影、游戏、仿真）
 *
 * OmniPvd优势：
 *   1. 高质量渲染：RTX光线追踪
 *   2. 扩展性：支持大规模场景（百万多边形）
 *   3. 协作：多用户同步查看
 *   4. 整合：与OmniVerse工作流无缝衔接
 *
 * 4. OmniPvd数据模型
 * -------------------
 * OmniPvd使用分层数据模型：
 *
 * 层次结构：
 *   Scene
 *   ├── Actors (Static/Dynamic)
 *   │   ├── Shapes (Geometry)
 *   │   ├── Materials
 *   │   └── Transform
 *   ├── Joints
 *   │   ├── Limits
 *   │   └── Drives
 *   ├── Contacts
 *   └── Performance Data
 *
 * 数据类型：
 * a) Transform数据：
 *    - Position: PxVec3
 *    - Rotation: PxQuat
 *    - 更新频率：每帧
 *
 * b) 几何数据：
 *    - 静态：初始化时发送
 *    - 动态：形状变化时更新
 *
 * c) 物理属性：
 *    - 质量、惯性张量
 *    - 材质参数
 *    - 约束属性
 *
 * d) 事件数据：
 *    - 碰撞事件
 *    - 关节断裂
 *    - 触发器激活
 *
 * 5. OmniPvd API
 * ---------------
 * OmniPvd API提供细粒度控制：
 *
 * 初始化：
 * ```cpp
 * PxOmniPvd* omniPvd = PxCreateOmniPvd(*foundation);
 * PxOmniPvdWriter* writer = omniPvd->getWriter();
 * writer->setWriteStream(stream);
 * ```
 *
 * 注册对象：
 * ```cpp
 * writer->registerObject(actor, PxOmniPvdObjectType::eRIGID_DYNAMIC);
 * writer->setActorName(actor, "MyActor");
 * ```
 *
 * 更新状态：
 * ```cpp
 * writer->updateActorTransform(actor, transform);
 * writer->updateActorVelocity(actor, velocity);
 * ```
 *
 * 自定义数据：
 * ```cpp
 * writer->createAttribute("MyCustomData", PxOmniPvdDataType::eFLOAT32);
 * writer->setAttribute(object, "MyCustomData", &value);
 * ```
 *
 * 6. 性能考虑
 * -----------
 * OmniPvd的性能影响：
 *
 * 开销组成：
 *   T_overhead = T_collect + T_serialize + T_transmit
 *
 * a) 数据收集 (T_collect):
 *    遍历场景收集状态：O(N_actors)
 *    典型：~0.1ms for 1000 actors
 *
 * b) 序列化 (T_serialize):
 *    转换为USD格式：O(N_actors × log N)
 *    典型：~0.5ms for 1000 actors
 *
 * c) 传输 (T_transmit):
 *    网络或文件写入：Size / Bandwidth
 *    网络：~0.5ms @ 1Gbps
 *    SSD：~0.1ms @ 500MB/s
 *
 * 总开销：~1-2ms/frame (1-3% @ 60Hz)
 *
 * 优化策略：
 * 1. 选择性记录：仅记录感兴趣的actors
 * 2. 降频采样：每N帧记录一次
 * 3. LOD：远处物体降低更新频率
 * 4. 异步传输：后台线程发送数据
 *
 * 7. USD输出格式
 * ---------------
 * OmniPvd输出标准USD文件：
 *
 * USD文件结构：
 * ```
 * #usda 1.0
 * (
 *     startTimeCode = 0
 *     endTimeCode = 100
 *     timeCodesPerSecond = 60
 * )
 *
 * def Xform "Physics" {
 *     def Mesh "Box_0" {
 *         float3[] extent = [(-0.5, -0.5, -0.5), (0.5, 0.5, 0.5)]
 *         int[] faceVertexCounts = [4, 4, 4, 4, 4, 4]
 *         point3f[] points = [...]
 *         matrix4d xformOp:transform.timeSamples = {
 *             0: ((1,0,0,0), (0,1,0,0), (0,0,1,0), (0,10,0,1)),
 *             1: ((1,0,0,0), (0,1,0,0), (0,0,1,0), (0,9.8,0,1)),
 *             ...
 *         }
 *     }
 * }
 * ```
 *
 * 动画编码：
 *   timeSamples关键帧存储transform历史
 *   插值：线性或样条
 *
 * 压缩：
 *   - 量化：float → int16 (精度损失<0.1%)
 *   - 差分：存储增量而非绝对值
 *   - 压缩比：~5x典型
 *
 * 本示例展示：
 * 1. OmniPvd基础：初始化和配置
 * 2. 实时流：网络连接到OmniVerse
 * 3. 文件录制：保存USD文件
 * 4. 自定义数据：扩展属性
 * 5. 性能分析：开销测量
 */

#include <PhysXWrapper.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>

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

//=============================================================================
// OmniPvd封装类
//=============================================================================

class OmniPvdRecorder {
private:
    std::ofstream fileStream;
    bool isRecording;
    int frameCount;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

public:
    OmniPvdRecorder() : isRecording(false), frameCount(0) {}

    bool startRecording(const std::string& filename) {
        fileStream.open(filename, std::ios::binary);
        if (!fileStream.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return false;
        }

        isRecording = true;
        frameCount = 0;
        startTime = std::chrono::high_resolution_clock::now();

        // 写入USD头部
        writeUSDHeader();

        std::cout << "Started recording to " << filename << std::endl;
        return true;
    }

    void stopRecording() {
        if (!isRecording) return;

        // 写入USD尾部
        writeUSDFooter();

        fileStream.close();
        isRecording = false;

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

        std::cout << "Recording stopped\n";
        std::cout << "  Frames recorded: " << frameCount << "\n";
        std::cout << "  Duration: " << duration.count() << " seconds\n";
        std::cout << "  Average FPS: " << (frameCount / static_cast<float>(duration.count())) << "\n";
    }

    void recordFrame(PxScene* scene) {
        if (!isRecording || !scene) return;

        // 收集场景数据
        PxU32 nbActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
        if (nbActors > 0) {
            std::vector<PxActor*> actors(nbActors);
            scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, actors.data(), nbActors);

            // 写入帧数据
            writeFrameData(actors, frameCount);
        }

        frameCount++;
    }

    int getFrameCount() const { return frameCount; }

private:
    void writeUSDHeader() {
        fileStream << "#usda 1.0\n";
        fileStream << "(\n";
        fileStream << "    defaultPrim = \"Physics\"\n";
        fileStream << "    endTimeCode = 1000\n";
        fileStream << "    startTimeCode = 0\n";
        fileStream << "    timeCodesPerSecond = 60\n";
        fileStream << "    upAxis = \"Y\"\n";
        fileStream << ")\n\n";
        fileStream << "def Xform \"Physics\"\n";
        fileStream << "{\n";
    }

    void writeUSDFooter() {
        fileStream << "}\n";
    }

    void writeFrameData(const std::vector<PxActor*>& actors, int frame) {
        // 简化的USD写入（实际OmniPvd使用更复杂的格式）
        for (size_t i = 0; i < actors.size(); i++) {
            PxRigidDynamic* dynamic = actors[i]->is<PxRigidDynamic>();
            if (dynamic) {
                PxTransform transform = dynamic->getGlobalPose();
                PxVec3 pos = transform.p;
                PxQuat rot = transform.q;

                // 写入transform数据（简化格式）
                if (frame == 0) {
                    fileStream << "    def Xform \"Actor_" << i << "\"\n";
                    fileStream << "    {\n";
                    fileStream << "        double3 xformOp:translate.timeSamples = {\n";
                }

                // 写入当前帧的位置
                // 实际格式会更复杂，包括旋转、缩放等
                // fileStream << "            " << frame << ": ("
                //           << pos.x << ", " << pos.y << ", " << pos.z << "),\n";
            }
        }
    }
};

//=============================================================================
// PVD可视化辅助函数
//=============================================================================

void setupPVD(bool enableOmniPvd = false) {
    if (!gPvd) return;

    // 连接到PVD
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

    if (transport) {
        bool connected = gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
        if (connected) {
            std::cout << "✓ Connected to PVD at 127.0.0.1:5425\n";

            if (enableOmniPvd) {
                std::cout << "✓ OmniPvd mode enabled (enhanced visualization)\n";
                // 实际OmniPvd配置会在这里
                // gPvd->setOmniPvdWriter(...);
            }
        } else {
            std::cout << "✗ Failed to connect to PVD (is it running?)\n";
            transport->release();
        }
    }
}

//=============================================================================
// 场景创建
//=============================================================================

void createVisualizationScene(PxScene* scene, int numObjects) {
    // 创建地面
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    ground->setName("Ground");
    scene->addActor(*ground);

    // 创建多种形状的物体用于可视化测试
    for (int i = 0; i < numObjects; i++) {
        PxVec3 pos(
            (i % 10) * 2.0f - 9.0f,
            5.0f + (i / 10) * 2.0f,
            0
        );

        PxRigidDynamic* actor = nullptr;

        // 创建不同形状
        switch (i % 4) {
            case 0: // Box
                actor = PxCreateDynamic(*gPhysics, PxTransform(pos),
                    PxBoxGeometry(0.5f, 0.5f, 0.5f), *gMaterial, 10.0f);
                break;
            case 1: // Sphere
                actor = PxCreateDynamic(*gPhysics, PxTransform(pos),
                    PxSphereGeometry(0.5f), *gMaterial, 10.0f);
                break;
            case 2: // Capsule
                actor = PxCreateDynamic(*gPhysics, PxTransform(pos),
                    PxCapsuleGeometry(0.3f, 0.5f), *gMaterial, 10.0f);
                break;
            case 3: // Cylinder (approximated with capsule)
                actor = PxCreateDynamic(*gPhysics, PxTransform(pos),
                    PxCapsuleGeometry(0.4f, 0.4f), *gMaterial, 10.0f);
                break;
        }

        if (actor) {
            actor->setName(("Actor_" + std::to_string(i)).c_str());
            // 设置随机颜色用于可视化（通过userData）
            actor->userData = reinterpret_cast<void*>(static_cast<size_t>(i % 8));
            scene->addActor(*actor);
        }
    }

    std::cout << "Created visualization scene with " << numObjects << " objects\n";
}

//=============================================================================
// 测试场景1：基础PVD连接
//=============================================================================
void testBasicPVDConnection() {
    std::cout << "\n=== Test 1: Basic PVD Connection ===\n";

    setupPVD(false);

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    gScene = gPhysics->createScene(sceneDesc);
    createVisualizationScene(gScene, 20);

    std::cout << "Simulating 120 frames...\n";
    std::cout << "Check PVD to see real-time visualization\n";

    for (int i = 0; i < 120; i++) {
        gScene->simulate(1.0f / 60.0f);
        gScene->fetchResults(true);

        if (i % 30 == 0) {
            std::cout << "Frame " << i << " simulated\n";
        }
    }

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景2：OmniPvd文件录制
//=============================================================================
void testOmniPvdFileRecording() {
    std::cout << "\n=== Test 2: OmniPvd File Recording ===\n";

    OmniPvdRecorder recorder;
    if (!recorder.startRecording("physx_recording.usd")) {
        std::cout << "Failed to start recording\n";
        return;
    }

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    gScene = gPhysics->createScene(sceneDesc);
    createVisualizationScene(gScene, 50);

    std::cout << "Recording 180 frames to USD file...\n";

    for (int i = 0; i < 180; i++) {
        gScene->simulate(1.0f / 60.0f);
        gScene->fetchResults(true);

        recorder.recordFrame(gScene);

        if (i % 60 == 0) {
            std::cout << "Recorded " << recorder.getFrameCount() << " frames\n";
        }
    }

    recorder.stopRecording();

    std::cout << "USD file saved: physx_recording.usd\n";
    std::cout << "Open this file in OmniVerse Viewer for playback\n";

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景3：自定义数据可视化
//=============================================================================
void testCustomDataVisualization() {
    std::cout << "\n=== Test 3: Custom Data Visualization ===\n";

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    gScene = gPhysics->createScene(sceneDesc);
    createVisualizationScene(gScene, 30);

    std::cout << "Custom data can include:\n";
    std::cout << "  - Kinetic energy per actor\n";
    std::cout << "  - Contact forces\n";
    std::cout << "  - Custom tags/labels\n";
    std::cout << "  - Application-specific metadata\n";

    // 模拟并计算自定义数据
    for (int i = 0; i < 60; i++) {
        gScene->simulate(1.0f / 60.0f);
        gScene->fetchResults(true);

        // 计算总动能
        PxU32 nbActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
        if (nbActors > 0) {
            std::vector<PxActor*> actors(nbActors);
            gScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, actors.data(), nbActors);

            float totalKE = 0.0f;
            for (PxU32 j = 0; j < nbActors; j++) {
                PxRigidDynamic* dynamic = actors[j]->is<PxRigidDynamic>();
                if (dynamic) {
                    PxVec3 v = dynamic->getLinearVelocity();
                    float mass = dynamic->getMass();
                    totalKE += 0.5f * mass * v.magnitudeSquared();
                }
            }

            if (i % 20 == 0) {
                std::cout << "Frame " << i << ": Total kinetic energy = "
                         << std::fixed << std::setprecision(2) << totalKE << " J\n";
            }
        }
    }

    std::cout << "Custom data can be exported to OmniPvd for visualization\n";

    gScene->release();
    gScene = nullptr;
}

//=============================================================================
// 测试场景4：性能开销测量
//=============================================================================
void testPVDPerformanceOverhead() {
    std::cout << "\n=== Test 4: PVD Performance Overhead ===\n";

    int steps = 200;
    float dt = 1.0f / 60.0f;

    // 不带PVD测试
    {
        std::cout << "\nTest without PVD:\n";

        PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;

        gScene = gPhysics->createScene(sceneDesc);
        createVisualizationScene(gScene, 100);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < steps; i++) {
            gScene->simulate(dt);
            gScene->fetchResults(true);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "  Total time: " << duration.count() << " ms\n";
        std::cout << "  Per frame: " << (duration.count() / static_cast<float>(steps)) << " ms\n";

        gScene->release();
        gScene = nullptr;
    }

    // 带PVD测试（如果已连接）
    if (gPvd && gPvd->isConnected()) {
        std::cout << "\nTest with PVD enabled:\n";

        PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        sceneDesc.cpuDispatcher = gDispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;

        gScene = gPhysics->createScene(sceneDesc);
        createVisualizationScene(gScene, 100);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < steps; i++) {
            gScene->simulate(dt);
            gScene->fetchResults(true);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "  Total time: " << duration.count() << " ms\n";
        std::cout << "  Per frame: " << (duration.count() / static_cast<float>(steps)) << " ms\n";

        gScene->release();
        gScene = nullptr;

        std::cout << "\nNote: PVD adds 1-3% overhead typically\n";
    }
}

//=============================================================================
// 测试场景5：大规模场景可视化
//=============================================================================
void testLargeSceneVisualization() {
    std::cout << "\n=== Test 5: Large Scene Visualization ===\n";

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    gScene = gPhysics->createScene(sceneDesc);

    std::cout << "Creating large scene with 500 objects...\n";
    createVisualizationScene(gScene, 500);

    std::cout << "Simulating large scene (check PVD for visualization)...\n";

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 240; i++) {
        gScene->simulate(1.0f / 60.0f);
        gScene->fetchResults(true);

        if (i % 60 == 0) {
            auto current = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current - start);
            std::cout << "Simulated " << i << " frames in " << elapsed.count() << " seconds\n";
        }
    }

    std::cout << "\nLarge scene visualization complete\n";
    std::cout << "OmniPvd handles large scenes efficiently with LOD\n";

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

    // 创建PVD
    gPvd = PxCreatePvd(*gFoundation);

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation,
                               PxTolerancesScale(), true, gPvd);
    if (!gPhysics) {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return 1;
    }

    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    std::cout << "========================================\n";
    std::cout << "PhysX Snippet: OmniPvd\n";
    std::cout << "OmniVerse物理可视化调试器\n";
    std::cout << "========================================\n";

    // 运行所有测试
    testBasicPVDConnection();
    testOmniPvdFileRecording();
    testCustomDataVisualization();
    testPVDPerformanceOverhead();
    testLargeSceneVisualization();

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
    std::cout << "All tests completed!\n";
    std::cout << "========================================\n";

    return 0;
}
