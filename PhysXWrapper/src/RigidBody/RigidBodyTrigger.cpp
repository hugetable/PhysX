/**
 * @file RigidBodyTrigger.cpp
 * @brief Implementation of RigidBodyTrigger class
 */

#include "RigidBody/RigidBodyTrigger.h"

namespace PhysXWrapper {

// Forward declarations for filter shader functions
static PxFilterFlags filterShaderNativeCCD(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);

static PxFilterFlags filterShaderEmulated(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);

static PxFilterFlags filterShaderEmulatedCCD(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);

static PxFilterFlags filterShaderCallback(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);

// Helper to detect trigger using filter data
static bool isTriggerFilterData(const PxFilterData& data) {
    return data.word0 == 0xffffffff &&
           data.word1 == 0xffffffff &&
           data.word2 == 0xffffffff &&
           data.word3 == 0xffffffff;
}

/**
 * @brief Internal simulation event callback implementation
 */
class TriggerSimulationCallback : public PxSimulationEventCallback {
public:
    TriggerSimulationCallback(RigidBodyTrigger::Impl* impl) : m_impl(impl) {}

    void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {
        // Not used
    }

    void onWake(PxActor** actors, PxU32 count) override {
        // Not used
    }

    void onSleep(PxActor** actors, PxU32 count) override {
        // Not used
    }

    void onTrigger(PxTriggerPair* pairs, PxU32 count) override;

    void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override {
        // Not used
    }

    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 count) override;

private:
    RigidBodyTrigger::Impl* m_impl;
};

/**
 * @brief Internal filter callback implementation
 */
class TriggerFilterCallback : public PxSimulationFilterCallback {
public:
    TriggerFilterCallback(RigidBodyTrigger::Impl* impl) : m_impl(impl) {}

    PxFilterFlags pairFound(
        PxU64 pairID,
        PxFilterObjectAttributes attributes0, PxFilterData filterData0,
        const PxActor* a0, const PxShape* s0,
        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        const PxActor* a1, const PxShape* s1,
        PxPairFlags& pairFlags) override;

    void pairLost(
        PxU64 pairID,
        PxFilterObjectAttributes attributes0, PxFilterData filterData0,
        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        bool objectRemoved) override {
        // Not used
    }

    bool statusChange(PxU64& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags) override {
        return false;
    }

private:
    RigidBodyTrigger::Impl* m_impl;
};

/**
 * @brief Private implementation
 */
class RigidBodyTrigger::Impl {
public:
    Impl()
        : m_simulationCallback(this)
        , m_filterCallback(this)
    {
    }

    void addTriggerEvent(const TriggerEvent& event) {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.push_back(event);

        // Call user callback if set
        if (m_userCallback) {
            m_userCallback(event);
        }
    }

    std::vector<TriggerEvent> getTriggerEvents() {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        std::vector<TriggerEvent> result = std::move(m_events);
        m_events.clear();
        return result;
    }

    void clearTriggerEvents() {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_events.clear();
    }

    void setTriggerCallback(TriggerCallback callback) {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_userCallback = callback;
    }

    TriggerSimulationCallback* getSimulationCallback() {
        return &m_simulationCallback;
    }

    TriggerFilterCallback* getFilterCallback() {
        return &m_filterCallback;
    }

    TriggerConfig m_config;

private:
    std::mutex m_eventMutex;
    std::vector<TriggerEvent> m_events;
    TriggerCallback m_userCallback;
    TriggerSimulationCallback m_simulationCallback;
    TriggerFilterCallback m_filterCallback;

    friend class TriggerSimulationCallback;
    friend class TriggerFilterCallback;
};

// Implement TriggerSimulationCallback methods
void TriggerSimulationCallback::onTrigger(PxTriggerPair* pairs, PxU32 count) {
    for (PxU32 i = 0; i < count; i++) {
        const PxTriggerPair& pair = pairs[i];

        TriggerEvent event;
        event.triggerActor = pair.triggerActor;
        event.triggerShape = pair.triggerShape;
        event.otherActor = pair.otherActor;
        event.otherShape = pair.otherShape;
        event.statusFlags = pair.status;

        if (pair.status & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
            event.type = TriggerEventType::ENTER;
            m_impl->addTriggerEvent(event);
        }

        if (pair.status & PxPairFlag::eNOTIFY_TOUCH_LOST) {
            event.type = TriggerEventType::EXIT;
            m_impl->addTriggerEvent(event);
        }
    }
}

void TriggerSimulationCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 count) {
    // For emulated triggers that use contact events
    for (PxU32 i = 0; i < count; i++) {
        const PxContactPair& pair = pairs[i];

        // Check if this is a trigger pair
        bool isTrigger0 = RigidBodyTrigger::isTriggerShape(pair.shapes[0], m_impl->m_config);
        bool isTrigger1 = RigidBodyTrigger::isTriggerShape(pair.shapes[1], m_impl->m_config);

        if (isTrigger0 || isTrigger1) {
            TriggerEvent event;
            if (isTrigger0) {
                event.triggerShape = pair.shapes[0];
                event.triggerActor = pairHeader.actors[0];
                event.otherShape = pair.shapes[1];
                event.otherActor = pairHeader.actors[1];
            } else {
                event.triggerShape = pair.shapes[1];
                event.triggerActor = pairHeader.actors[1];
                event.otherShape = pair.shapes[0];
                event.otherActor = pairHeader.actors[0];
            }
            event.statusFlags = pair.events;

            if (pair.events & (PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_CCD)) {
                event.type = TriggerEventType::ENTER;
                m_impl->addTriggerEvent(event);
            }

            if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
                event.type = TriggerEventType::EXIT;
                m_impl->addTriggerEvent(event);
            }
        }
    }
}

// Implement TriggerFilterCallback methods
PxFilterFlags TriggerFilterCallback::pairFound(
    PxU64 pairID,
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    const PxActor* a0, const PxShape* s0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    const PxActor* a1, const PxShape* s1,
    PxPairFlags& pairFlags)
{
    // Check if either shape is a trigger (using userData)
    if (s0->userData || s1->userData) {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;

        if (m_impl->m_config.enableCCD) {
            pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT | PxPairFlag::eNOTIFY_TOUCH_CCD;
        }
    } else {
        pairFlags = PxPairFlag::eCONTACT_DEFAULT;
    }

    return PxFilterFlags();
}

// Filter shader implementations
static PxFilterFlags filterShaderNativeCCD(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // Just use default, but enable CCD for triggers
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;

    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;
    }

    return PxFilterFlag::eDEFAULT;
}

static PxFilterFlags filterShaderEmulated(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // Check for emulated triggers using filter data
    bool isTrigger = isTriggerFilterData(filterData0) || isTriggerFilterData(filterData1);

    if (isTrigger) {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
    } else {
        pairFlags = PxPairFlag::eCONTACT_DEFAULT;
    }

    return PxFilterFlag::eDEFAULT;
}

static PxFilterFlags filterShaderEmulatedCCD(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // Check for emulated triggers using filter data
    bool isTrigger = isTriggerFilterData(filterData0) || isTriggerFilterData(filterData1);

    if (isTrigger) {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;
    } else {
        pairFlags = PxPairFlag::eCONTACT_DEFAULT;
    }

    return PxFilterFlag::eDEFAULT;
}

static PxFilterFlags filterShaderCallback(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;
    return PxFilterFlag::eCALLBACK;
}

// RigidBodyTrigger implementation
RigidBodyTrigger::RigidBodyTrigger()
    : m_impl(std::make_unique<Impl>())
{
}

RigidBodyTrigger::~RigidBodyTrigger() = default;

PxShape* RigidBodyTrigger::createTriggerShape(
    PxPhysics* physics,
    const PxGeometry& geometry,
    PxMaterial* material,
    const TriggerConfig& config,
    bool isExclusive)
{
    m_currentConfig = config;
    m_impl->m_config = config;

    PxShape* shape = nullptr;

    if (config.implementation == TriggerImplementation::NATIVE) {
        // Use built-in triggers
        PxShapeFlags flags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eTRIGGER_SHAPE;
        shape = physics->createShape(geometry, *material, isExclusive, flags);
    }
    else if (config.implementation == TriggerImplementation::FILTER_SHADER) {
        // Emulate using filter data
        PxShapeFlags flags = PxShapeFlag::eVISUALIZATION | PxShapeFlag::eSIMULATION_SHAPE;
        shape = physics->createShape(geometry, *material, isExclusive, flags);

        // Mark as trigger using special filter data
        const PxFilterData triggerFilterData(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
        shape->setSimulationFilterData(triggerFilterData);
    }
    else if (config.implementation == TriggerImplementation::FILTER_CALLBACK) {
        // Emulate using shape userData
        shape = physics->createShape(geometry, *material, isExclusive);
        shape->userData = shape;  // Mark as trigger
    }

    return shape;
}

PxRigidStatic* RigidBodyTrigger::createBoxTrigger(
    PxPhysics* physics,
    PxScene* scene,
    PxMaterial* material,
    const PxVec3& position,
    const PxVec3& halfExtents,
    const TriggerConfig& config)
{
    PxShape* shape = createTriggerShape(physics, PxBoxGeometry(halfExtents), material, config, false);
    if (!shape) return nullptr;

    PxRigidStatic* actor = physics->createRigidStatic(PxTransform(position));
    actor->attachShape(*shape);
    scene->addActor(*actor);
    shape->release();

    return actor;
}

PxRigidStatic* RigidBodyTrigger::createSphereTrigger(
    PxPhysics* physics,
    PxScene* scene,
    PxMaterial* material,
    const PxVec3& position,
    PxReal radius,
    const TriggerConfig& config)
{
    PxShape* shape = createTriggerShape(physics, PxSphereGeometry(radius), material, config, false);
    if (!shape) return nullptr;

    PxRigidStatic* actor = physics->createRigidStatic(PxTransform(position));
    actor->attachShape(*shape);
    scene->addActor(*actor);
    shape->release();

    return actor;
}

PxRigidStatic* RigidBodyTrigger::createCapsuleTrigger(
    PxPhysics* physics,
    PxScene* scene,
    PxMaterial* material,
    const PxVec3& position,
    PxReal radius,
    PxReal halfHeight,
    const TriggerConfig& config)
{
    PxShape* shape = createTriggerShape(physics, PxCapsuleGeometry(radius, halfHeight), material, config, false);
    if (!shape) return nullptr;

    PxRigidStatic* actor = physics->createRigidStatic(PxTransform(position));
    actor->attachShape(*shape);
    scene->addActor(*actor);
    shape->release();

    return actor;
}

PxSceneDesc RigidBodyTrigger::createSceneDesc(
    const PxTolerancesScale& scale,
    const TriggerConfig& config,
    const PxVec3& gravity)
{
    m_currentConfig = config;
    m_impl->m_config = config;

    PxSceneDesc sceneDesc(scale);
    sceneDesc.gravity = gravity;

    if (config.autoAttachCallback) {
        sceneDesc.simulationEventCallback = getSimulationEventCallback();
    }

    // Select filter shader based on implementation
    if (config.implementation == TriggerImplementation::NATIVE) {
        if (config.enableCCD) {
            sceneDesc.filterShader = filterShaderNativeCCD;
            sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        } else {
            sceneDesc.filterShader = PxDefaultSimulationFilterShader;
        }
    }
    else if (config.implementation == TriggerImplementation::FILTER_SHADER) {
        if (config.enableCCD) {
            sceneDesc.filterShader = filterShaderEmulatedCCD;
            sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        } else {
            sceneDesc.filterShader = filterShaderEmulated;
        }
    }
    else if (config.implementation == TriggerImplementation::FILTER_CALLBACK) {
        sceneDesc.filterShader = filterShaderCallback;
        sceneDesc.filterCallback = getFilterCallback();
        if (config.enableCCD) {
            sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        }
    }

    return sceneDesc;
}

void RigidBodyTrigger::setTriggerCallback(TriggerCallback callback) {
    m_impl->setTriggerCallback(callback);
}

std::vector<TriggerEvent> RigidBodyTrigger::getTriggerEvents() {
    return m_impl->getTriggerEvents();
}

void RigidBodyTrigger::clearTriggerEvents() {
    m_impl->clearTriggerEvents();
}

bool RigidBodyTrigger::isTriggerShape(PxShape* shape, const TriggerConfig& config) {
    if (!shape) return false;

    if (config.implementation == TriggerImplementation::NATIVE) {
        return (shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE) != 0;
    }
    else if (config.implementation == TriggerImplementation::FILTER_SHADER) {
        return isTriggerFilterData(shape->getSimulationFilterData());
    }
    else if (config.implementation == TriggerImplementation::FILTER_CALLBACK) {
        return shape->userData != nullptr;
    }

    return false;
}

PxSimulationEventCallback* RigidBodyTrigger::getSimulationEventCallback() {
    return m_impl->getSimulationCallback();
}

PxSimulationFilterCallback* RigidBodyTrigger::getFilterCallback() {
    return m_impl->getFilterCallback();
}

PxSimulationFilterShader RigidBodyTrigger::getFilterShader(const TriggerConfig& config) {
    if (config.implementation == TriggerImplementation::NATIVE) {
        if (config.enableCCD) {
            return filterShaderNativeCCD;
        } else {
            return PxDefaultSimulationFilterShader;
        }
    }
    else if (config.implementation == TriggerImplementation::FILTER_SHADER) {
        if (config.enableCCD) {
            return filterShaderEmulatedCCD;
        } else {
            return filterShaderEmulated;
        }
    }
    else {
        return filterShaderCallback;
    }
}

} // namespace PhysXWrapper
