// example_physx_basic.cpp - PhysX 基础集成示例
// 演示: 简单的 PhysX 场景（堆叠箱子）+ OptiX 光线追踪渲染

#include "core/PhysicsRenderApp.h"
#include "PhysX/PhysXContext.h"
#include "Utility/SceneBuilder.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

using namespace PhysicsRender;
using namespace PhysXWrapper;

/**
 * @brief 创建 PhysX 测试场景
 *
 * 场景内容:
 * - 地面平面
 * - 5x5 堆叠的动态箱子
 */
void createPhysXScene(PhysicsSimulator* physics) {
    auto* physx = physics->getPhysX();
    if (!physx) {
        std::cerr << "PhysX context not available!" << std::endl;
        return;
    }

    PxPhysics* pxPhysics = physx->getPhysics();
    PxScene* scene = physx->getScene();

    // 使用 SceneBuilder
    SceneBuilder builder(pxPhysics, scene);

    // 1. 创建地面
    auto* ground = builder.createGround(
        PxVec3(0, 0, 0),    // 位置
        PxVec3(50, 1, 50),  // 尺寸
        builder.createMaterial(MaterialPreset::Default())
    );

    // 2. 创建堆叠的箱子
    auto boxes = builder.createBoxStack(
        5,                              // 堆叠高度
        PxVec3(0, 5, 0),               // 起始位置
        PxVec3(1.0f, 1.0f, 1.0f),      // 箱子尺寸
        10.0f,                          // 密度
        builder.createMaterial(MaterialPreset::Wood())
    );

    std::cout << "Created PhysX scene with:" << std::endl;
    std::cout << "  - Ground plane" << std::endl;
    std::cout << "  - " << boxes.size() << " dynamic boxes" << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "PhysicsRenderIntegration - PhysX Basic Example" << std::endl;
    std::cout << "================================================" << std::endl;

    // 1. 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // 2. 创建窗口
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1920, 1080,
        "PhysX + OptiX Integration - Basic Example",
        nullptr, nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // 3. 创建应用程序选项
    Options options;
    options.width = 1920;
    options.height = 1080;
    options.samples = 4;

    // 4. 创建应用程序
    std::unique_ptr<PhysicsRenderApp> app;
    try {
        app = std::make_unique<PhysicsRenderApp>(window, options);

        if (!app->initialize()) {
            std::cerr << "Failed to initialize application!" << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            return -1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "Application initialized successfully!" << std::endl;

    // 5. 创建场景
    // TODO: 在 PhysicsRenderApp 完全实现后启用
    // createPhysXScene(app->getPhysicsSimulator());

    std::cout << "\nControls:" << std::endl;
    std::cout << "  - Mouse: Orbit/Pan/Zoom camera" << std::endl;
    std::cout << "  - SPACE: Toggle GUI" << std::endl;
    std::cout << "  - P: Pause/Resume physics" << std::endl;
    std::cout << "  - R: Reset scene" << std::endl;
    std::cout << "  - ESC: Exit" << std::endl;
    std::cout << std::endl;

    // 6. 主循环
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        // 计算时间步长
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        // 限制 deltaTime
        deltaTime = std::min(deltaTime, 0.1f);  // 最大 100ms

        // 处理事件
        glfwPollEvents();

        // 更新应用程序
        app->update(deltaTime);

        // 渲染
        if (app->render()) {
            app->display();
        }

        // 交换缓冲区
        glfwSwapBuffers(window);
    }

    // 7. 清理
    std::cout << "\nShutting down..." << std::endl;
    app.reset();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Done!" << std::endl;
    return 0;
}
