// PhysicsSimulator.h - 物理模拟器统一接口

#ifndef PHYSICS_SIMULATOR_H
#define PHYSICS_SIMULATOR_H

#include "config.h"
#include "PhysX/PhysXContext.h"

#ifdef ENABLE_FLOW
#include "Flow/FlowContext.h"
#endif

#ifdef ENABLE_BLAST
#include "Blast/BlastManager.h"
#endif

#include <PxPhysicsAPI.h>
#include <vector>
#include <memory>

namespace PhysicsRender {

using namespace physx;

/**
 * @brief 可渲染对象结构
 *
 * 封装物理对象的渲染数据
 */
struct RenderableObject {
    enum Type {
        RIGID_BODY,    // 刚体
        PARTICLE,      // 粒子
        CHUNK          // Blast 碎片
    };

    Type type;

    // 变换矩阵 (4x3)
    float transform[12];

    // 几何 ID（在渲染器中）
    int geometryID;

    // 材质 ID
    int materialID;

    // 特定类型数据
    union {
        struct {
            PxRigidActor* actor;
        } rigidBody;

        struct {
            float position[3];
            float radius;
            float velocity[3];
        } particle;

        struct {
            uint32_t chunkID;
            float damageLevel;
        } chunk;
    };
};

/**
 * @brief 物理模拟器配置
 */
struct PhysicsConfig {
    // PhysX 配置
    bool enablePhysX = true;
    float physxTimeStep = 1.0f / 60.0f;
    PxVec3 gravity = PxVec3(0.0f, -9.81f, 0.0f);

    // Flow 配置
    bool enableFlow = false;
    int maxParticles = 100000;

    // Blast 配置
    bool enableBlast = false;
    int maxChunks = 5000;
};

/**
 * @brief 物理模拟器
 *
 * 统一管理 PhysX、Flow、Blast 三个物理库
 */
class PhysicsSimulator {
public:
    /**
     * @brief 构造函数
     * @param config 物理配置
     */
    explicit PhysicsSimulator(const PhysicsConfig& config = PhysicsConfig());

    /**
     * @brief 析构函数
     */
    ~PhysicsSimulator();

    /**
     * @brief 初始化物理系统
     * @return 成功返回 true
     */
    bool initialize();

    /**
     * @brief 更新物理模拟
     * @param deltaTime 时间步长（秒）
     */
    void update(float deltaTime);

    /**
     * @brief 获取所有可渲染对象
     * @return 可渲染对象列表
     */
    std::vector<RenderableObject> getRenderables() const;

    /**
     * @brief 添加刚体到场景
     * @param actor PhysX 刚体
     * @param geometryID 几何 ID
     * @param materialID 材质 ID
     */
    void addRigidBody(PxRigidActor* actor, int geometryID, int materialID);

    /**
     * @brief 重置场景
     */
    void reset();

    // Getters
    PhysXWrapper::PhysXContext* getPhysX() { return physxContext_.get(); }

#ifdef ENABLE_FLOW
    FlowWrapper::FlowContext* getFlow() { return flowContext_.get(); }
#endif

#ifdef ENABLE_BLAST
    BlastWrapper::BlastManager* getBlast() { return blastManager_.get(); }
#endif

    const PhysicsConfig& getConfig() const { return config_; }

private:
    PhysicsConfig config_;

    // PhysX
    std::unique_ptr<PhysXWrapper::PhysXContext> physxContext_;
    std::vector<RenderableObject> rigidBodies_;

#ifdef ENABLE_FLOW
    // Flow
    std::unique_ptr<FlowWrapper::FlowContext> flowContext_;
    std::vector<RenderableObject> particles_;
#endif

#ifdef ENABLE_BLAST
    // Blast
    std::unique_ptr<BlastWrapper::BlastManager> blastManager_;
    std::vector<RenderableObject> chunks_;
#endif

    // 辅助函数
    void updateRigidBodies();
    void updateParticles();
    void updateChunks();

    void convertTransform(const PxTransform& pxTransform, float* outMatrix);
};

} // namespace PhysicsRender

#endif // PHYSICS_SIMULATOR_H
