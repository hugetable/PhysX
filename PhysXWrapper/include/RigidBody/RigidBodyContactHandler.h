/**
 * @file RigidBodyContactHandler.h
 * @brief Contact report handler for rigid body collisions
 *
 * This class provides a simplified interface for handling contact events
 * between rigid bodies. Based on SnippetContactReport from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <functional>
#include <vector>
#include <memory>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Contact point information
 */
struct ContactPoint {
    /** Contact position in world space */
    PxVec3 position;

    /** Contact normal (points from actor1 to actor0) */
    PxVec3 normal;

    /** Contact impulse */
    PxVec3 impulse;

    /** Separation distance (negative for penetration) */
    PxReal separation;

    /** First actor in the contact pair */
    PxRigidActor* actor0;

    /** Second actor in the contact pair */
    PxRigidActor* actor1;

    /** First shape in the contact pair */
    PxShape* shape0;

    /** Second shape in the contact pair */
    PxShape* shape1;
};

/**
 * @brief Contact event information
 */
struct ContactEvent {
    /** Type of contact event */
    enum class Type {
        TOUCH_FOUND,      // First contact
        TOUCH_PERSISTS,   // Continuing contact
        TOUCH_LOST        // Contact ended
    };

    /** Event type */
    Type type;

    /** List of contact points */
    std::vector<ContactPoint> contactPoints;

    /** First actor */
    PxRigidActor* actor0;

    /** Second actor */
    PxRigidActor* actor1;
};

/**
 * @brief Callback function type for contact events
 *
 * @param event Contact event information
 */
using ContactCallback = std::function<void(const ContactEvent& event)>;

/**
 * @brief Contact report handler class
 *
 * This class simplifies the handling of contact events between rigid bodies.
 * It provides:
 * - Easy registration of contact callbacks
 * - Automatic contact data collection
 * - Filter shader configuration
 * - Thread-safe callback invocation
 *
 * @example
 * @code
 * // Create handler
 * RigidBodyContactHandler contactHandler;
 *
 * // Register callback
 * contactHandler.setContactCallback([](const ContactEvent& event) {
 *     if (event.type == ContactEvent::Type::TOUCH_FOUND) {
 *         std::cout << "New contact: " << event.contactPoints.size() << " points\n";
 *         for (const auto& cp : event.contactPoints) {
 *             std::cout << "  Position: " << cp.position.x << ", "
 *                       << cp.position.y << ", " << cp.position.z << "\n";
 *             std::cout << "  Impulse: " << cp.impulse.magnitude() << "\n";
 *         }
 *     }
 * });
 *
 * // Create scene with contact reporting
 * PxSceneDesc sceneDesc = contactHandler.createSceneDesc(physics->getTolerancesScale());
 * sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
 * PxScene* scene = physics->createScene(sceneDesc);
 *
 * // ... add actors ...
 *
 * // Simulate
 * scene->simulate(1.0f / 60.0f);
 * scene->fetchResults(true);
 * // Callbacks are invoked during fetchResults
 * @endcode
 */
class RigidBodyContactHandler {
public:
    /**
     * @brief Constructor
     */
    RigidBodyContactHandler();

    /**
     * @brief Destructor
     */
    ~RigidBodyContactHandler();

    // Disable copy
    RigidBodyContactHandler(const RigidBodyContactHandler&) = delete;
    RigidBodyContactHandler& operator=(const RigidBodyContactHandler&) = delete;

    /**
     * @brief Set contact event callback
     * @param callback Function to call when contact events occur
     */
    void setContactCallback(ContactCallback callback);

    /**
     * @brief Create a scene descriptor with contact reporting enabled
     * @param scale Tolerance scale
     * @return Configured scene descriptor
     */
    PxSceneDesc createSceneDesc(const PxTolerancesScale& scale);

    /**
     * @brief Get the simulation event callback (for manual scene setup)
     * @return Pointer to the internal event callback
     */
    PxSimulationEventCallback* getEventCallback();

    /**
     * @brief Get the default filter shader for contact reporting
     * @return Filter shader function pointer
     */
    static PxSimulationFilterShader getDefaultFilterShader();

    /**
     * @brief Enable/disable detailed contact point reporting
     * @param enable True to enable detailed reporting
     */
    void setDetailedReporting(bool enable);

    /**
     * @brief Enable/disable persistent contact reporting
     * @param enable True to report continuing contacts
     */
    void setPersistentReporting(bool enable);

    /**
     * @brief Get all contact points from the last simulation step
     * @return Vector of contact points
     */
    const std::vector<ContactPoint>& getContactPoints() const;

    /**
     * @brief Clear stored contact points
     */
    void clearContactPoints();

    /**
     * @brief Get number of contact events in last simulation step
     * @return Contact event count
     */
    size_t getContactEventCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Default filter shader for contact reporting
 *
 * This filter shader requests contact reports for all pairs.
 * It enables:
 * - Contact solving
 * - Discrete collision detection
 * - Touch found notifications
 * - Touch persists notifications (optional)
 * - Contact point data
 */
PxFilterFlags DefaultContactReportFilter(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize
);

} // namespace PhysXWrapper
