/**
 * @file RigidBodyTrigger.h
 * @brief Simplified trigger volume implementation
 *
 * This class provides a simplified interface for creating trigger volumes that
 * detect when objects enter or exit them. Based on SnippetTriggers from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Trigger event type
 */
enum class TriggerEventType {
    ENTER,  ///< Object entered trigger volume
    EXIT    ///< Object exited trigger volume
};

/**
 * @brief Trigger event data
 */
struct TriggerEvent {
    TriggerEventType type;          ///< Event type (enter/exit)
    PxRigidActor* triggerActor;     ///< The trigger actor
    PxShape* triggerShape;          ///< The trigger shape
    PxRigidActor* otherActor;       ///< The other actor (that entered/exited)
    PxShape* otherShape;            ///< The other shape
    PxPairFlags statusFlags;        ///< Status flags from PhysX
};

/**
 * @brief Callback function for trigger events
 * @param event The trigger event data
 */
using TriggerCallback = std::function<void(const TriggerEvent& event)>;

/**
 * @brief Trigger implementation method
 */
enum class TriggerImplementation {
    NATIVE,         ///< Use PxShapeFlag::eTRIGGER_SHAPE (default, fastest)
    FILTER_SHADER,  ///< Emulate with filter shader (allows CCD + trigger-trigger)
    FILTER_CALLBACK ///< Emulate with filter callback (most flexible)
};

/**
 * @brief Configuration for trigger volumes
 */
struct TriggerConfig {
    /** Implementation method */
    TriggerImplementation implementation = TriggerImplementation::NATIVE;

    /** Enable CCD for trigger detection */
    bool enableCCD = false;

    /** Enable trigger-trigger overlap detection */
    bool detectTriggerTrigger = false;

    /** Auto-attach callback to scene */
    bool autoAttachCallback = true;
};

/**
 * @brief Rigid body trigger manager class
 *
 * This class simplifies working with trigger volumes in PhysX:
 * - Easy trigger shape creation
 * - Event callbacks for enter/exit
 * - Support for CCD and trigger-trigger detection
 * - Thread-safe event collection
 * - Multiple implementation methods
 *
 * @example
 * @code
 * // Create trigger manager
 * RigidBodyTrigger triggerManager;
 *
 * // Configure scene for triggers
 * TriggerConfig config;
 * config.enableCCD = true;
 * PxSceneDesc sceneDesc = triggerManager.createSceneDesc(scale, config);
 * scene = physics->createScene(sceneDesc);
 *
 * // Set callback
 * triggerManager.setTriggerCallback([](const TriggerEvent& event) {
 *     if (event.type == TriggerEventType::ENTER) {
 *         std::cout << "Object entered trigger!" << std::endl;
 *     } else {
 *         std::cout << "Object exited trigger!" << std::endl;
 *     }
 * });
 *
 * // Create trigger shape
 * PxShape* triggerShape = triggerManager.createTriggerShape(
 *     physics, PxBoxGeometry(10, 1, 10), material, config
 * );
 *
 * // Attach to actor
 * PxRigidStatic* actor = physics->createRigidStatic(PxTransform(0, 10, 0));
 * actor->attachShape(*triggerShape);
 * scene->addActor(*actor);
 * triggerShape->release();
 * @endcode
 */
class RigidBodyTrigger {
public:
    /**
     * @brief Constructor
     */
    RigidBodyTrigger();

    /**
     * @brief Destructor
     */
    ~RigidBodyTrigger();

    // Disable copy
    RigidBodyTrigger(const RigidBodyTrigger&) = delete;
    RigidBodyTrigger& operator=(const RigidBodyTrigger&) = delete;

    /**
     * @brief Create a trigger shape
     * @param physics PxPhysics instance
     * @param geometry Geometry for the trigger
     * @param material Material for the shape
     * @param config Trigger configuration
     * @param isExclusive Whether shape is exclusive to one actor
     * @return Created trigger shape (caller must release)
     */
    PxShape* createTriggerShape(
        PxPhysics* physics,
        const PxGeometry& geometry,
        PxMaterial* material,
        const TriggerConfig& config = TriggerConfig(),
        bool isExclusive = false
    );

    /**
     * @brief Create a box trigger actor (static)
     * @param physics PxPhysics instance
     * @param scene PxScene instance
     * @param material Material for the shape
     * @param position Position of the trigger
     * @param halfExtents Half extents of the box
     * @param config Trigger configuration
     * @return Created trigger actor (owned by scene)
     */
    PxRigidStatic* createBoxTrigger(
        PxPhysics* physics,
        PxScene* scene,
        PxMaterial* material,
        const PxVec3& position,
        const PxVec3& halfExtents,
        const TriggerConfig& config = TriggerConfig()
    );

    /**
     * @brief Create a sphere trigger actor (static)
     * @param physics PxPhysics instance
     * @param scene PxScene instance
     * @param material Material for the shape
     * @param position Position of the trigger
     * @param radius Radius of the sphere
     * @param config Trigger configuration
     * @return Created trigger actor (owned by scene)
     */
    PxRigidStatic* createSphereTrigger(
        PxPhysics* physics,
        PxScene* scene,
        PxMaterial* material,
        const PxVec3& position,
        PxReal radius,
        const TriggerConfig& config = TriggerConfig()
    );

    /**
     * @brief Create a capsule trigger actor (static)
     * @param physics PxPhysics instance
     * @param scene PxScene instance
     * @param material Material for the shape
     * @param position Position of the trigger
     * @param radius Radius of the capsule
     * @param halfHeight Half height of the capsule
     * @param config Trigger configuration
     * @return Created trigger actor (owned by scene)
     */
    PxRigidStatic* createCapsuleTrigger(
        PxPhysics* physics,
        PxScene* scene,
        PxMaterial* material,
        const PxVec3& position,
        PxReal radius,
        PxReal halfHeight,
        const TriggerConfig& config = TriggerConfig()
    );

    /**
     * @brief Create scene descriptor configured for triggers
     * @param scale Tolerance scale
     * @param config Trigger configuration
     * @param gravity Gravity vector
     * @return Configured scene descriptor
     */
    PxSceneDesc createSceneDesc(
        const PxTolerancesScale& scale,
        const TriggerConfig& config,
        const PxVec3& gravity = PxVec3(0, -9.81f, 0)
    );

    /**
     * @brief Set trigger event callback
     * @param callback Callback function to receive events
     */
    void setTriggerCallback(TriggerCallback callback);

    /**
     * @brief Get collected trigger events (thread-safe)
     * @return Vector of trigger events since last call
     */
    std::vector<TriggerEvent> getTriggerEvents();

    /**
     * @brief Clear collected trigger events
     */
    void clearTriggerEvents();

    /**
     * @brief Check if a shape is a trigger
     * @param shape Shape to check
     * @param config Configuration to use for checking
     * @return True if shape is a trigger
     */
    static bool isTriggerShape(PxShape* shape, const TriggerConfig& config);

    /**
     * @brief Get the simulation event callback (for attaching to scene)
     * @return Pointer to simulation event callback
     */
    PxSimulationEventCallback* getSimulationEventCallback();

    /**
     * @brief Get the filter callback (for attaching to scene if using FILTER_CALLBACK)
     * @return Pointer to filter callback
     */
    PxSimulationFilterCallback* getFilterCallback();

    /**
     * @brief Get filter shader function for FILTER_SHADER implementation
     * @param config Configuration containing CCD settings
     * @return Filter shader function pointer
     */
    static PxSimulationFilterShader getFilterShader(const TriggerConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    // Store current config for internal use
    TriggerConfig m_currentConfig;
};

} // namespace PhysXWrapper
