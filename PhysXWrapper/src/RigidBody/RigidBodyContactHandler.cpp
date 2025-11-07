/**
 * @file RigidBodyContactHandler.cpp
 * @brief Implementation of RigidBodyContactHandler class
 */

#include "RigidBody/RigidBodyContactHandler.h"
#include <iostream>
#include <mutex>

namespace PhysXWrapper {

// Forward declaration
class ContactEventCallback;

/**
 * @brief Internal implementation of contact event callback
 */
class ContactEventCallback : public PxSimulationEventCallback {
public:
    ContactEventCallback(RigidBodyContactHandler::Impl* handler)
        : m_handler(handler)
    {
    }

    // Unused callbacks
    void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {
        PX_UNUSED(constraints);
        PX_UNUSED(count);
    }

    void onWake(PxActor** actors, PxU32 count) override {
        PX_UNUSED(actors);
        PX_UNUSED(count);
    }

    void onSleep(PxActor** actors, PxU32 count) override {
        PX_UNUSED(actors);
        PX_UNUSED(count);
    }

    void onTrigger(PxTriggerPair* pairs, PxU32 count) override {
        PX_UNUSED(pairs);
        PX_UNUSED(count);
    }

    void onAdvance(const PxRigidBody*const*, const PxTransform*, const PxU32) override {
    }

    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;

private:
    RigidBodyContactHandler::Impl* m_handler;
};

/**
 * @brief Private implementation class
 */
class RigidBodyContactHandler::Impl {
    friend class ContactEventCallback;
public:
    Impl()
        : m_eventCallback(this)
        , m_detailedReporting(true)
        , m_persistentReporting(true)
        , m_eventCount(0)
    {
    }

    ~Impl() = default;

    void processContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (PxU32 i = 0; i < nbPairs; i++) {
            const PxContactPair& cp = pairs[i];

            // Determine event type
            ContactEvent::Type eventType;
            if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
                eventType = ContactEvent::Type::TOUCH_FOUND;
            } else if (cp.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) {
                eventType = ContactEvent::Type::TOUCH_PERSISTS;
            } else if (cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
                eventType = ContactEvent::Type::TOUCH_LOST;
            } else {
                continue; // Unknown event type
            }

            // Skip persisting contacts if not enabled
            if (!m_persistentReporting && eventType == ContactEvent::Type::TOUCH_PERSISTS) {
                continue;
            }

            // Create contact event
            ContactEvent event;
            event.type = eventType;
            // PhysX 5.x: PxContactPairHeader uses PxActor*, cast to PxRigidActor*
            event.actor0 = static_cast<PxRigidActor*>(pairHeader.actors[0]);
            event.actor1 = static_cast<PxRigidActor*>(pairHeader.actors[1]);

            // Extract contact points if detailed reporting is enabled
            if (m_detailedReporting && cp.contactCount > 0) {
                std::vector<PxContactPairPoint> contactPoints(cp.contactCount);
                cp.extractContacts(&contactPoints[0], cp.contactCount);

                for (PxU32 j = 0; j < cp.contactCount; j++) {
                    ContactPoint point;
                    point.position = contactPoints[j].position;
                    point.normal = contactPoints[j].normal;
                    point.impulse = contactPoints[j].impulse;
                    point.separation = contactPoints[j].separation;
                    // PhysX 5.x: cast PxActor* to PxRigidActor*
                    point.actor0 = static_cast<PxRigidActor*>(pairHeader.actors[0]);
                    point.actor1 = static_cast<PxRigidActor*>(pairHeader.actors[1]);
                    point.shape0 = cp.shapes[0];
                    point.shape1 = cp.shapes[1];

                    event.contactPoints.push_back(point);
                    m_allContactPoints.push_back(point);
                }
            }

            // Invoke user callback
            if (m_callback) {
                m_callback(event);
            }

            m_eventCount++;
        }
    }

    void clearContactPoints() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_allContactPoints.clear();
        m_eventCount = 0;
    }

    ContactCallback m_callback;
    ContactEventCallback m_eventCallback;
    std::vector<ContactPoint> m_allContactPoints;
    bool m_detailedReporting;
    bool m_persistentReporting;
    size_t m_eventCount;
    mutable std::mutex m_mutex;
};

// ContactEventCallback::onContact implementation
void ContactEventCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) {
    m_handler->processContact(pairHeader, pairs, nbPairs);
}

// Default filter shader implementation
PxFilterFlags DefaultContactReportFilter(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    PX_UNUSED(attributes0);
    PX_UNUSED(attributes1);
    PX_UNUSED(filterData0);
    PX_UNUSED(filterData1);
    PX_UNUSED(constantBlock);
    PX_UNUSED(constantBlockSize);

    // Enable contact solving and discrete collision detection
    pairFlags = PxPairFlag::eSOLVE_CONTACT | PxPairFlag::eDETECT_DISCRETE_CONTACT;

    // Request contact notifications
    pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
    pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
    pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;

    // Request contact point data
    pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;

    return PxFilterFlag::eDEFAULT;
}

// RigidBodyContactHandler implementation

RigidBodyContactHandler::RigidBodyContactHandler()
    : m_impl(std::make_unique<Impl>())
{
}

RigidBodyContactHandler::~RigidBodyContactHandler() = default;

void RigidBodyContactHandler::setContactCallback(ContactCallback callback) {
    m_impl->m_callback = callback;
}

PxSceneDesc RigidBodyContactHandler::createSceneDesc(const PxTolerancesScale& scale) {
    PxSceneDesc sceneDesc(scale);
    sceneDesc.filterShader = DefaultContactReportFilter;
    sceneDesc.simulationEventCallback = &m_impl->m_eventCallback;
    return sceneDesc;
}

PxSimulationEventCallback* RigidBodyContactHandler::getEventCallback() {
    return &m_impl->m_eventCallback;
}

PxSimulationFilterShader RigidBodyContactHandler::getDefaultFilterShader() {
    return DefaultContactReportFilter;
}

void RigidBodyContactHandler::setDetailedReporting(bool enable) {
    m_impl->m_detailedReporting = enable;
}

void RigidBodyContactHandler::setPersistentReporting(bool enable) {
    m_impl->m_persistentReporting = enable;
}

const std::vector<ContactPoint>& RigidBodyContactHandler::getContactPoints() const {
    return m_impl->m_allContactPoints;
}

void RigidBodyContactHandler::clearContactPoints() {
    m_impl->clearContactPoints();
}

size_t RigidBodyContactHandler::getContactEventCount() const {
    return m_impl->m_eventCount;
}

} // namespace PhysXWrapper
