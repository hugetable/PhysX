// PhysicsSimulator.cpp - 物理模拟器实现

#include "physics/PhysicsSimulator.h"
#include <iostream>
#include <cstring>

namespace PhysicsRender {

PhysicsSimulator::PhysicsSimulator(const PhysicsConfig& config)
    : config_(config) {
}

PhysicsSimulator::~PhysicsSimulator() {
    // 清理工作在各个包装器的析构函数中自动完成
}

bool PhysicsSimulator::initialize() {
    std::cout << "Initializing PhysicsSimulator..." << std::endl;

    // 初始化 PhysX
    if (config_.enablePhysX) {
        std::cout << "  Initializing PhysX..." << std::endl;

        PhysXWrapper::PhysXContextConfig physxConfig;
        physxConfig.enablePVD = false;
        physxConfig.gravity = config_.gravity;

        physxContext_ = PhysXWrapper::PhysXContext::create(physxConfig);
        if (!physxContext_) {
            std::cerr << "Failed to create PhysX context!" << std::endl;
            return false;
        }

        if (!physxContext_->initialize()) {
            std::cerr << "Failed to initialize PhysX context!" << std::endl;
            return false;
        }

        std::cout << "  PhysX initialized successfully" << std::endl;
    }

#ifdef ENABLE_FLOW
    // 初始化 Flow
    if (config_.enableFlow) {
        std::cout << "  Initializing Flow..." << std::endl;

        FlowWrapper::FlowContextConfig flowConfig;
        flowConfig.enableDebug = false;
        flowConfig.memoryBudgetMB = 512;

        flowContext_ = FlowWrapper::FlowContext::create(flowConfig);
        if (!flowContext_) {
            std::cerr << "Failed to create Flow context!" << std::endl;
            return false;
        }

        if (!flowContext_->initialize()) {
            std::cerr << "Failed to initialize Flow context!" << std::endl;
            return false;
        }

        std::cout << "  Flow initialized successfully" << std::endl;
    }
#endif

#ifdef ENABLE_BLAST
    // 初始化 Blast
    if (config_.enableBlast) {
        std::cout << "  Initializing Blast..." << std::endl;

        BlastWrapper::BlastConfig blastConfig;
        blastConfig.maxActors = config_.maxChunks;

        blastManager_ = BlastWrapper::BlastManager::create(blastConfig);
        if (!blastManager_) {
            std::cerr << "Failed to create Blast manager!" << std::endl;
            return false;
        }

        if (!blastManager_->initialize()) {
            std::cerr << "Failed to initialize Blast manager!" << std::endl;
            return false;
        }

        std::cout << "  Blast initialized successfully" << std::endl;
    }
#endif

    std::cout << "PhysicsSimulator initialized successfully!" << std::endl;
    return true;
}

void PhysicsSimulator::update(float deltaTime) {
    // 更新 PhysX
    if (physxContext_ && config_.enablePhysX) {
        physxContext_->simulate(deltaTime);
        updateRigidBodies();
    }

#ifdef ENABLE_FLOW
    // 更新 Flow
    if (flowContext_ && config_.enableFlow) {
        flowContext_->update(deltaTime);
        updateParticles();
    }
#endif

#ifdef ENABLE_BLAST
    // 更新 Blast
    if (blastManager_ && config_.enableBlast) {
        blastManager_->update(deltaTime);
        updateChunks();
    }
#endif
}

std::vector<RenderableObject> PhysicsSimulator::getRenderables() const {
    std::vector<RenderableObject> renderables;

    // 添加刚体
    renderables.insert(renderables.end(), rigidBodies_.begin(), rigidBodies_.end());

#ifdef ENABLE_FLOW
    // 添加粒子
    renderables.insert(renderables.end(), particles_.begin(), particles_.end());
#endif

#ifdef ENABLE_BLAST
    // 添加碎片
    renderables.insert(renderables.end(), chunks_.begin(), chunks_.end());
#endif

    return renderables;
}

void PhysicsSimulator::addRigidBody(PxRigidActor* actor, int geometryID, int materialID) {
    if (!actor) return;

    RenderableObject obj;
    obj.type = RenderableObject::RIGID_BODY;
    obj.geometryID = geometryID;
    obj.materialID = materialID;
    obj.rigidBody.actor = actor;

    // 获取初始变换
    PxTransform transform = actor->getGlobalPose();
    convertTransform(transform, obj.transform);

    rigidBodies_.push_back(obj);
}

void PhysicsSimulator::reset() {
    std::cout << "Resetting PhysicsSimulator..." << std::endl;

    // 清空对象列表
    rigidBodies_.clear();

#ifdef ENABLE_FLOW
    particles_.clear();
#endif

#ifdef ENABLE_BLAST
    chunks_.clear();
#endif

    // 重新初始化各个库
    if (physxContext_) {
        physxContext_->shutdown();
        physxContext_->initialize();
    }

#ifdef ENABLE_FLOW
    if (flowContext_) {
        flowContext_->shutdown();
        flowContext_->initialize();
    }
#endif

#ifdef ENABLE_BLAST
    if (blastManager_) {
        blastManager_->shutdown();
        blastManager_->initialize();
    }
#endif

    std::cout << "PhysicsSimulator reset complete" << std::endl;
}

// 私有辅助函数

void PhysicsSimulator::updateRigidBodies() {
    for (auto& obj : rigidBodies_) {
        if (obj.type == RenderableObject::RIGID_BODY && obj.rigidBody.actor) {
            PxTransform transform = obj.rigidBody.actor->getGlobalPose();
            convertTransform(transform, obj.transform);
        }
    }
}

void PhysicsSimulator::updateParticles() {
#ifdef ENABLE_FLOW
    // TODO: 从 Flow 获取粒子数据并更新 particles_ 列表
    // 这需要 Flow 的具体 API
#endif
}

void PhysicsSimulator::updateChunks() {
#ifdef ENABLE_BLAST
    // TODO: 从 Blast 获取碎片数据并更新 chunks_ 列表
    // 这需要处理 Blast 的破碎事件
#endif
}

void PhysicsSimulator::convertTransform(const PxTransform& pxTransform, float* outMatrix) {
    // 将 PhysX 的 PxTransform 转换为 4x3 矩阵
    // [R R R Tx]
    // [R R R Ty]
    // [R R R Tz]

    // 获取旋转矩阵
    PxMat33 rotMatrix(pxTransform.q);

    // 填充 4x3 矩阵
    outMatrix[0] = rotMatrix[0][0];
    outMatrix[1] = rotMatrix[0][1];
    outMatrix[2] = rotMatrix[0][2];
    outMatrix[3] = pxTransform.p.x;

    outMatrix[4] = rotMatrix[1][0];
    outMatrix[5] = rotMatrix[1][1];
    outMatrix[6] = rotMatrix[1][2];
    outMatrix[7] = pxTransform.p.y;

    outMatrix[8] = rotMatrix[2][0];
    outMatrix[9] = rotMatrix[2][1];
    outMatrix[10] = rotMatrix[2][2];
    outMatrix[11] = pxTransform.p.z;
}

} // namespace PhysicsRender
