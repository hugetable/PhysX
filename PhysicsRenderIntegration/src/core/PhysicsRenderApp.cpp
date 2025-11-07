// PhysicsRenderApp.cpp - 主应用程序实现

#include "core/PhysicsRenderApp.h"
#include <iostream>
#include <sstream>

namespace PhysicsRender {

PhysicsRenderApp::PhysicsRenderApp(GLFWwindow* window, const Options& options)
    : Application(window, options)  // 调用基类构造函数
    , accumulatedTime_(0.0f)
    , fixedTimeStep_(1.0f / 60.0f)
    , pausePhysics_(false) {

    // 清空统计信息
    std::memset(&stats_, 0, sizeof(stats_));
}

PhysicsRenderApp::~PhysicsRenderApp() {
    std::cout << "Shutting down PhysicsRenderApp..." << std::endl;

    // 清理资源（智能指针会自动释放）
    syncManager_.reset();
    renderer_.reset();
    physicsSimulator_.reset();

    std::cout << "PhysicsRenderApp shutdown complete" << std::endl;
}

bool PhysicsRenderApp::initialize() {
    std::cout << "Initializing PhysicsRenderApp..." << std::endl;

    // 1. 创建物理模拟器
    PhysicsConfig physicsConfig;
    physicsConfig.enablePhysX = true;
    physicsConfig.physxTimeStep = fixedTimeStep_;
    physicsConfig.gravity = PxVec3(0.0f, -9.81f, 0.0f);

#ifdef ENABLE_FLOW
    physicsConfig.enableFlow = false;  // 默认关闭
#endif

#ifdef ENABLE_BLAST
    physicsConfig.enableBlast = false;  // 默认关闭
#endif

    physicsSimulator_ = std::make_unique<PhysicsSimulator>(physicsConfig);
    if (!physicsSimulator_->initialize()) {
        std::cerr << "Failed to initialize PhysicsSimulator!" << std::endl;
        return false;
    }

    // 2. 创建渲染器
    PhysicsRendererConfig rendererConfig;
    rendererConfig.resolution = make_int2(m_width, m_height);
    rendererConfig.samplesPerPixel = 4;
    rendererConfig.maxPathLength = 8;

    renderer_ = std::make_unique<PhysicsRenderer>(rendererConfig);
    if (!renderer_->initialize()) {
        std::cerr << "Failed to initialize PhysicsRenderer!" << std::endl;
        return false;
    }

    // 3. 创建同步管理器
    SyncConfig syncConfig;
    syncConfig.strategy = SyncStrategy::BUFFERED;
    syncConfig.incrementalUpdate = true;

    syncManager_ = std::make_unique<SyncManager>(syncConfig);
    if (!syncManager_->initialize()) {
        std::cerr << "Failed to initialize SyncManager!" << std::endl;
        return false;
    }

    std::cout << "PhysicsRenderApp initialized successfully!" << std::endl;
    return true;
}

void PhysicsRenderApp::update(float deltaTime) {
    if (pausePhysics_) {
        return;
    }

    // 固定时间步长更新
    accumulatedTime_ += deltaTime;

    auto physicsStart = std::chrono::high_resolution_clock::now();

    while (accumulatedTime_ >= fixedTimeStep_) {
        physicsSimulator_->update(fixedTimeStep_);
        accumulatedTime_ -= fixedTimeStep_;
    }

    auto physicsEnd = std::chrono::high_resolution_clock::now();
    stats_.physicsTime = std::chrono::duration<float, std::milli>(
        physicsEnd - physicsStart
    ).count();

    // 同步物理到渲染
    auto syncStart = std::chrono::high_resolution_clock::now();

    syncManager_->sync(*physicsSimulator_, *renderer_);

    auto syncEnd = std::chrono::high_resolution_clock::now();
    stats_.syncTime = std::chrono::duration<float, std::milli>(
        syncEnd - syncStart
    ).count();

    // 更新统计信息
    updateStatistics();
}

bool PhysicsRenderApp::render() {
    auto renderStart = std::chrono::high_resolution_clock::now();

    // 调用渲染器
    unsigned int renderTime = renderer_->render();

    auto renderEnd = std::chrono::high_resolution_clock::now();
    stats_.renderTime = std::chrono::duration<float, std::milli>(
        renderEnd - renderStart
    ).count();

    (void)renderTime;  // 避免未使用警告

    return true;
}

void PhysicsRenderApp::display() {
    // 显示渲染结果
    // 由基类 Application 处理 OpenGL 显示
    Application::display();
}

bool PhysicsRenderApp::loadScene(const std::string& sceneFile) {
    std::cout << "Loading scene: " << sceneFile << std::endl;

    // TODO: 实现场景加载
    // 1. 解析场景文件
    // 2. 创建物理对象
    // 3. 创建渲染几何
    // 4. 设置相机和光源

    std::cout << "Scene loaded (placeholder)" << std::endl;
    return true;
}

void PhysicsRenderApp::resetScene() {
    std::cout << "Resetting scene..." << std::endl;

    // 重置物理模拟器
    if (physicsSimulator_) {
        physicsSimulator_->reset();
    }

    // 重置渲染器
    // TODO: 实现渲染器重置

    // 重置时间
    accumulatedTime_ = 0.0f;

    std::cout << "Scene reset complete" << std::endl;
}

void PhysicsRenderApp::guiWindow() {
    // 扩展基类的 GUI
    Application::guiWindow();

    // 添加物理渲染特定的 GUI
    ImGui::Begin("Physics Render Info");

    // 统计信息
    ImGui::Text("Performance:");
    ImGui::Text("  Physics: %.2f ms", stats_.physicsTime);
    ImGui::Text("  Sync: %.2f ms", stats_.syncTime);
    ImGui::Text("  Render: %.2f ms", stats_.renderTime);

    float totalTime = stats_.physicsTime + stats_.syncTime + stats_.renderTime;
    float fps = (totalTime > 0) ? (1000.0f / totalTime) : 0.0f;
    ImGui::Text("  Total: %.2f ms (%.1f FPS)", totalTime, fps);

    ImGui::Separator();

    // 对象数量
    ImGui::Text("Scene Objects:");
    ImGui::Text("  Rigid Bodies: %d", stats_.numRigidBodies);
    ImGui::Text("  Particles: %d", stats_.numParticles);
    ImGui::Text("  Blast Chunks: %d", stats_.numChunks);

    ImGui::Separator();

    // 控制按钮
    if (ImGui::Button(pausePhysics_ ? "Resume Physics" : "Pause Physics")) {
        pausePhysics_ = !pausePhysics_;
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset Scene")) {
        resetScene();
    }

    ImGui::End();
}

// 私有辅助函数

void PhysicsRenderApp::updateStatistics() {
    // 更新对象计数
    auto renderables = physicsSimulator_->getRenderables();

    stats_.numRigidBodies = 0;
    stats_.numParticles = 0;
    stats_.numChunks = 0;

    for (const auto& obj : renderables) {
        switch (obj.type) {
            case RenderableObject::RIGID_BODY:
                stats_.numRigidBodies++;
                break;
            case RenderableObject::PARTICLE:
                stats_.numParticles++;
                break;
            case RenderableObject::CHUNK:
                stats_.numChunks++;
                break;
        }
    }
}

void PhysicsRenderApp::updateGUI() {
    // GUI 更新逻辑
    guiWindow();
}

} // namespace PhysicsRender
