/**
 * @file ArticulationManager.h
 * @brief Comprehensive articulation system for PhysX reduced coordinate articulations
 *
 * This class provides utilities for creating and managing articulations:
 * - Reduced coordinate articulations (more stable than joint chains)
 * - Robot arms and manipulators
 * - Character skeletons and ragdolls
 * - Complex mechanisms (scissors, cranes, etc.)
 * - Joint drives and control
 * - Articulation chains with multiple links
 *
 * Based on SnippetArticulationRC from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Articulation link configuration
 */
struct ArticulationLinkConfig {
    PxTransform globalPose = PxTransform(PxIdentity);  ///< Global pose
    PxGeometry* geometry = nullptr;                    ///< Link geometry (sphere, box, capsule, etc.)
    PxReal density = 10.0f;                           ///< Link density (kg/m^3)
    PxReal linearDamping = 0.05f;                     ///< Linear damping
    PxReal angularDamping = 0.05f;                    ///< Angular damping
    PxReal maxLinearVelocity = 100.0f;                ///< Max linear velocity
    PxReal maxAngularVelocity = 20.0f;                ///< Max angular velocity
    std::string name;                                  ///< Optional link name
};

/**
 * @brief Articulation joint configuration
 */
struct ArticulationJointConfig {
    PxArticulationJointType::Enum type = PxArticulationJointType::eFIX;  ///< Joint type
    PxTransform parentPose = PxTransform(PxIdentity);  ///< Parent frame
    PxTransform childPose = PxTransform(PxIdentity);   ///< Child frame

    // Motion configuration for each axis
    PxArticulationMotion::Enum motionX = PxArticulationMotion::eLOCKED;
    PxArticulationMotion::Enum motionY = PxArticulationMotion::eLOCKED;
    PxArticulationMotion::Enum motionZ = PxArticulationMotion::eLOCKED;
    PxArticulationMotion::Enum motionTwist = PxArticulationMotion::eLOCKED;
    PxArticulationMotion::Enum motionSwing1 = PxArticulationMotion::eLOCKED;
    PxArticulationMotion::Enum motionSwing2 = PxArticulationMotion::eLOCKED;

    // Limits (per axis)
    bool enableLimits = false;
    PxReal limitLow = -PxPi;
    PxReal limitHigh = PxPi;

    // Drives (per axis)
    bool enableDrive = false;
    PxReal driveStiffness = 0.0f;
    PxReal driveDamping = 0.0f;
    PxReal driveMaxForce = PX_MAX_F32;
    PxReal driveTarget = 0.0f;
    PxReal driveTargetVelocity = 0.0f;

    // Friction
    PxReal friction = 0.05f;
};

/**
 * @brief Articulation configuration
 */
struct ArticulationConfig {
    PxU32 solverIterationCounts = 32;              ///< Solver iterations for stability
    PxU32 solverVelocityIterationCounts = 1;       ///< Velocity solver iterations
    PxReal sleepThreshold = 0.005f;                ///< Sleep threshold
    PxReal stabilizationThreshold = 0.01f;         ///< Stabilization threshold
    PxReal wakeCounter = 0.4f;                     ///< Wake counter
    PxReal maxDepenetrationVelocity = 10.0f;       ///< Max depenetration velocity
    bool enableSelfCollision = false;               ///< Enable self-collision
    bool fixBase = false;                           ///< Fix base link (immovable)
};

/**
 * @brief Robot arm configuration
 */
struct RobotArmConfig {
    PxU32 numJoints = 6;                           ///< Number of joints
    PxReal linkLength = 1.0f;                      ///< Length of each link
    PxReal linkRadius = 0.1f;                      ///< Radius of each link
    PxReal linkMass = 1.0f;                        ///< Mass of each link
    PxVec3 basePosition = PxVec3(0, 0, 0);         ///< Base position
    PxVec3 armDirection = PxVec3(0, 1, 0);         ///< Arm direction
    bool enableJointLimits = true;                  ///< Enable joint limits
    PxReal jointLimitLow = -PxPi / 2.0f;           ///< Default joint lower limit
    PxReal jointLimitHigh = PxPi / 2.0f;           ///< Default joint upper limit
};

/**
 * @brief Articulation manager class
 *
 * This class provides comprehensive articulation management:
 * - Create and manage reduced coordinate articulations
 * - Build robot arms, skeletons, and complex mechanisms
 * - Configure joints with limits, drives, and friction
 * - Control articulation motion through drive targets
 * - Query articulation state
 *
 * Articulations provide more stable simulation than regular joint chains
 * for connected multi-body systems.
 *
 * @example
 * @code
 * // Create articulation manager
 * ArticulationManager artMgr;
 * artMgr.initialize(physics);
 *
 * // Create articulation
 * ArticulationConfig config;
 * config.solverIterationCounts = 32;
 * PxArticulationReducedCoordinate* art = artMgr.createArticulation(scene, config);
 *
 * // Create base link
 * ArticulationLinkConfig linkConfig;
 * linkConfig.geometry = new PxBoxGeometry(0.5f, 0.5f, 0.5f);
 * PxArticulationLink* base = artMgr.createLink(art, nullptr, linkConfig);
 *
 * // Create child link with revolute joint
 * ArticulationJointConfig jointConfig;
 * jointConfig.type = PxArticulationJointType::eREVOLUTE;
 * jointConfig.motionTwist = PxArticulationMotion::eLIMITED;
 * jointConfig.enableLimits = true;
 * jointConfig.limitLow = -PxPi / 2.0f;
 * jointConfig.limitHigh = PxPi / 2.0f;
 *
 * linkConfig.globalPose = PxTransform(PxVec3(0, 1, 0));
 * PxArticulationLink* link = artMgr.createLink(art, base, linkConfig, jointConfig);
 *
 * // Set drive target
 * artMgr.setDriveTarget(link, PxArticulationAxis::eTWIST, PxPi / 4.0f);
 * @endcode
 */
class ArticulationManager {
public:
    /**
     * @brief Constructor
     */
    ArticulationManager();

    /**
     * @brief Destructor
     */
    ~ArticulationManager();

    // No copy
    ArticulationManager(const ArticulationManager&) = delete;
    ArticulationManager& operator=(const ArticulationManager&) = delete;

    // Move allowed
    ArticulationManager(ArticulationManager&&) noexcept;
    ArticulationManager& operator=(ArticulationManager&&) noexcept;

    /**
     * @brief Initialize articulation manager
     * @param physics PhysX physics instance
     * @return True if successful
     */
    bool initialize(PxPhysics* physics);

    /**
     * @brief Cleanup and release resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const;

    // ========================================================================
    // Articulation Creation
    // ========================================================================

    /**
     * @brief Create articulation
     * @param scene Scene to add articulation to
     * @param config Articulation configuration
     * @return Created articulation or nullptr on failure
     */
    PxArticulationReducedCoordinate* createArticulation(
        PxScene* scene,
        const ArticulationConfig& config = ArticulationConfig()
    );

    /**
     * @brief Release articulation
     * @param articulation Articulation to release
     */
    void releaseArticulation(PxArticulationReducedCoordinate* articulation);

    // ========================================================================
    // Link Creation
    // ========================================================================

    /**
     * @brief Create articulation link
     * @param articulation Parent articulation
     * @param parent Parent link (nullptr for root link)
     * @param linkConfig Link configuration
     * @param jointConfig Joint configuration (ignored for root link)
     * @param material Material for shapes (uses default if nullptr)
     * @return Created link or nullptr on failure
     */
    PxArticulationLink* createLink(
        PxArticulationReducedCoordinate* articulation,
        PxArticulationLink* parent,
        const ArticulationLinkConfig& linkConfig,
        const ArticulationJointConfig& jointConfig = ArticulationJointConfig(),
        PxMaterial* material = nullptr
    );

    // ========================================================================
    // Joint Configuration
    // ========================================================================

    /**
     * @brief Configure joint after creation
     * @param link Link whose inbound joint to configure
     * @param jointConfig Joint configuration
     * @return True if successful
     */
    bool configureJoint(
        PxArticulationLink* link,
        const ArticulationJointConfig& jointConfig
    );

    /**
     * @brief Set joint motion type for an axis
     * @param link Link whose inbound joint to configure
     * @param axis Joint axis
     * @param motion Motion type (FREE, LIMITED, LOCKED)
     * @return True if successful
     */
    bool setJointMotion(
        PxArticulationLink* link,
        PxArticulationAxis::Enum axis,
        PxArticulationMotion::Enum motion
    );

    /**
     * @brief Set joint limits for an axis
     * @param link Link whose inbound joint to configure
     * @param axis Joint axis
     * @param low Lower limit
     * @param high Upper limit
     * @return True if successful
     */
    bool setJointLimits(
        PxArticulationLink* link,
        PxArticulationAxis::Enum axis,
        PxReal low,
        PxReal high
    );

    /**
     * @brief Set joint drive parameters
     * @param link Link whose inbound joint to configure
     * @param axis Joint axis
     * @param stiffness Drive stiffness
     * @param damping Drive damping
     * @param maxForce Maximum drive force
     * @param isAcceleration True if drive is acceleration-based
     * @return True if successful
     */
    bool setJointDrive(
        PxArticulationLink* link,
        PxArticulationAxis::Enum axis,
        PxReal stiffness,
        PxReal damping,
        PxReal maxForce = PX_MAX_F32,
        bool isAcceleration = true
    );

    /**
     * @brief Set joint drive target position
     * @param link Link whose inbound joint to configure
     * @param axis Joint axis
     * @param target Target position
     * @return True if successful
     */
    bool setDriveTarget(
        PxArticulationLink* link,
        PxArticulationAxis::Enum axis,
        PxReal target
    );

    /**
     * @brief Set joint drive target velocity
     * @param link Link whose inbound joint to configure
     * @param axis Joint axis
     * @param targetVelocity Target velocity
     * @return True if successful
     */
    bool setDriveVelocity(
        PxArticulationLink* link,
        PxArticulationAxis::Enum axis,
        PxReal targetVelocity
    );

    // ========================================================================
    // Helper Builders
    // ========================================================================

    /**
     * @brief Create robot arm
     * @param scene Scene to add articulation to
     * @param config Robot arm configuration
     * @param material Material for shapes
     * @return Created articulation or nullptr on failure
     */
    PxArticulationReducedCoordinate* createRobotArm(
        PxScene* scene,
        const RobotArmConfig& config,
        PxMaterial* material = nullptr
    );

    /**
     * @brief Create simple chain (like rope or snake)
     * @param scene Scene to add articulation to
     * @param basePosition Starting position
     * @param direction Chain direction
     * @param numLinks Number of links
     * @param linkLength Length of each link
     * @param linkRadius Radius of each link
     * @param material Material for shapes
     * @return Created articulation or nullptr on failure
     */
    PxArticulationReducedCoordinate* createChain(
        PxScene* scene,
        const PxVec3& basePosition,
        const PxVec3& direction,
        PxU32 numLinks,
        PxReal linkLength,
        PxReal linkRadius,
        PxMaterial* material = nullptr
    );

    // ========================================================================
    // Query and Control
    // ========================================================================

    /**
     * @brief Get all links in articulation
     * @param articulation Articulation
     * @return Vector of links
     */
    std::vector<PxArticulationLink*> getLinks(
        PxArticulationReducedCoordinate* articulation
    ) const;

    /**
     * @brief Get link count
     * @param articulation Articulation
     * @return Number of links
     */
    PxU32 getLinkCount(PxArticulationReducedCoordinate* articulation) const;

    /**
     * @brief Get link by index
     * @param articulation Articulation
     * @param index Link index
     * @return Link or nullptr if invalid index
     */
    PxArticulationLink* getLink(
        PxArticulationReducedCoordinate* articulation,
        PxU32 index
    ) const;

    /**
     * @brief Get current joint position
     * @param link Link
     * @param axis Joint axis
     * @return Current joint position
     */
    PxReal getJointPosition(
        PxArticulationLink* link,
        PxArticulationAxis::Enum axis
    ) const;

    /**
     * @brief Get current joint velocity
     * @param link Link
     * @param axis Joint axis
     * @return Current joint velocity
     */
    PxReal getJointVelocity(
        PxArticulationLink* link,
        PxArticulationAxis::Enum axis
    ) const;

    /**
     * @brief Wake articulation
     * @param articulation Articulation to wake
     */
    void wakeUp(PxArticulationReducedCoordinate* articulation);

    /**
     * @brief Put articulation to sleep
     * @param articulation Articulation to sleep
     */
    void putToSleep(PxArticulationReducedCoordinate* articulation);

    /**
     * @brief Check if articulation is sleeping
     * @param articulation Articulation
     * @return True if sleeping
     */
    bool isSleeping(PxArticulationReducedCoordinate* articulation) const;

    /**
     * @brief Get managed articulations
     * @return Vector of articulations
     */
    std::vector<PxArticulationReducedCoordinate*> getArticulations() const;

    /**
     * @brief Get articulation count
     * @return Number of managed articulations
     */
    PxU32 getArticulationCount() const;

    /**
     * @brief Get last error message
     * @return Error message or empty string
     */
    std::string getLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    void applyArticulationConfig(
        PxArticulationReducedCoordinate* articulation,
        const ArticulationConfig& config
    );

    void applyJointConfig(
        PxArticulationJointReducedCoordinate* joint,
        const ArticulationJointConfig& config
    );
};

} // namespace PhysXWrapper
