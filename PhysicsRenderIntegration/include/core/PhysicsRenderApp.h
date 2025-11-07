// PhysicsRenderApp.h - 主应用程序类
// 继承自 OptiX_Apps Application

#ifndef PHYSICS_RENDER_APP_H
#define PHYSICS_RENDER_APP_H

#include "config.h"
#include "Application.h"  // OptiX_Apps
#include "physics/PhysicsSimulator.h"
#include "rendering/PhysicsRenderer.h"
#include "sync/SyncManager.h"

#include <memory>
#include <string>

namespace PhysicsRender {

/**
 * @brief 主应用程序类
 *
 * 集成物理模拟（PhysX/Flow/Blast）和光线追踪渲染（OptiX）
 * 继承自 OptiX_Apps 的 Application 基类
 */
class PhysicsRenderApp : public Application {
public:
    /**
     * @brief 构造函数
     * @param window GLFW 窗口句柄
     * @param options 应用程序选项
     */
    PhysicsRenderApp(GLFWwindow* window, const Options& options);

    /**
     * @brief 析构函数
     */
    ~PhysicsRenderApp() override;

    /**
     * @brief 初始化应用程序
     * @return 成功返回 true
     */
    bool initialize();

    /**
     * @brief 更新物理模拟和渲染
     * @param deltaTime 时间步长（秒）
     */
    void update(float deltaTime);

    /**
     * @brief 渲染一帧
     * @return 成功返回 true
     */
    bool render() override;

    /**
     * @brief 显示渲染结果
     */
    void display() override;

    /**
     * @brief 加载场景
     * @param sceneFile 场景描述文件路径
     * @return 成功返回 true
     */
    bool loadScene(const std::string& sceneFile);

    /**
     * @brief 重置场景
     */
    void resetScene();

    /**
     * @brief GUI 更新
     */
    void guiWindow() override;

private:
    // 物理模拟器
    std::unique_ptr<PhysicsSimulator> physicsSimulator_;

    // 渲染器（扩展自 OptiX_Apps Raytracer）
    std::unique_ptr<PhysicsRenderer> renderer_;

    // 同步管理器
    std::unique_ptr<SyncManager> syncManager_;

    // 时间管理
    float accumulatedTime_;
    float fixedTimeStep_;
    bool pausePhysics_;

    // 统计信息
    struct Statistics {
        float physicsTime;      // 物理模拟时间
        float syncTime;         // 同步时间
        float renderTime;       // 渲染时间
        int numRigidBodies;     // 刚体数量
        int numParticles;       // 粒子数量
        int numChunks;          // Blast 碎片数量
    };

    Statistics stats_;

    // 辅助函数
    void updateStatistics();
    void updateGUI();
};

} // namespace PhysicsRender

#endif // PHYSICS_RENDER_APP_H
