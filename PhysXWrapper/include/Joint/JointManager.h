/**
 * @file JointManager.h
 * @brief Comprehensive joint system for PhysX rigid bodies
 *
 * This class provides utilities for creating and managing joints between rigid bodies:
 * - Spherical (ball-and-socket) joints
 * - Fixed (welded) joints
 * - Revolute (hinge) joints
 * - Prismatic (slider) joints
 * - Distance joints
 * - D6 (six degree-of-freedom) joints
 * - Joint limits and drives
 * - Breakable joints
 * - Joint chains and ragdolls
 *
 * Based on SnippetJoint from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>
#include <memory>
#include <functional>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Joint break event data
 */
struct JointBreakEvent {
    PxJoint* joint;                  ///< Joint that broke
    PxReal forceApplied;             ///< Force that caused break
    PxReal torqueApplied;            ///< Torque that caused break
    void* userData;                  ///< User data from joint
};

/**
 * @brief Joint break callback function
 */
using JointBreakCallback = std::function<void(const JointBreakEvent& event)>;

/**
 * @brief Spherical joint configuration
 */
struct SphericalJointConfig {
    bool enableLimit = false;                    ///< Enable cone limit
    PxReal yAngleLimit = PxPi / 4.0f;           ///< Y axis limit (radians)
    PxReal zAngleLimit = PxPi / 4.0f;           ///< Z axis limit (radians)
    PxReal limitRestitution = 0.0f;             ///< Limit bounce
    PxReal limitStiffness = 0.0f;               ///< Limit spring stiffness
    PxReal limitDamping = 0.0f;                 ///< Limit spring damping

    // Spring/damping for projection
    bool enableProjection = true;               ///< Enable constraint projection
    PxReal projectionLinearTolerance = 1e10f;   ///< Linear error tolerance
    PxReal projectionAngularTolerance = PxPi;   ///< Angular error tolerance
};

/**
 * @brief Fixed joint configuration
 */
struct FixedJointConfig {
    bool enableProjection = true;               ///< Enable constraint projection
    PxReal projectionLinearTolerance = 1e10f;   ///< Linear error tolerance
    PxReal projectionAngularTolerance = PxPi;   ///< Angular error tolerance
};

/**
 * @brief Revolute joint configuration (hinge)
 */
struct RevoluteJointConfig {
    bool enableLimit = false;                   ///< Enable angle limits
    PxReal lowerLimit = -PxPi / 2.0f;          ///< Lower angle limit (radians)
    PxReal upperLimit = PxPi / 2.0f;           ///< Upper angle limit (radians)
    PxReal limitRestitution = 0.0f;            ///< Limit bounce
    PxReal limitStiffness = 0.0f;              ///< Limit spring stiffness
    PxReal limitDamping = 0.0f;                ///< Limit spring damping

    bool enableDrive = false;                   ///< Enable motor drive
    PxReal driveVelocity = 0.0f;               ///< Target angular velocity
    PxReal driveForceLimit = PX_MAX_F32;       ///< Maximum drive force
    PxReal driveGearRatio = 1.0f;              ///< Drive gear ratio

    bool enableProjection = true;               ///< Enable constraint projection
    PxReal projectionLinearTolerance = 1e10f;   ///< Linear error tolerance
    PxReal projectionAngularTolerance = PxPi;   ///< Angular error tolerance
};

/**
 * @brief Prismatic joint configuration (slider)
 */
struct PrismaticJointConfig {
    bool enableLimit = false;                   ///< Enable distance limits
    PxReal lowerLimit = -10.0f;                ///< Lower distance limit
    PxReal upperLimit = 10.0f;                 ///< Upper distance limit
    PxReal limitRestitution = 0.0f;            ///< Limit bounce
    PxReal limitStiffness = 0.0f;              ///< Limit spring stiffness
    PxReal limitDamping = 0.0f;                ///< Limit spring damping

    bool enableProjection = true;               ///< Enable constraint projection
    PxReal projectionLinearTolerance = 1e10f;   ///< Linear error tolerance
    PxReal projectionAngularTolerance = PxPi;   ///< Angular error tolerance
};

/**
 * @brief Distance joint configuration
 */
struct DistanceJointConfig {
    PxReal minDistance = 0.0f;                 ///< Minimum distance
    PxReal maxDistance = 0.0f;                 ///< Maximum distance (0 = rigid)
    PxReal stiffness = 0.0f;                   ///< Spring stiffness
    PxReal damping = 0.0f;                     ///< Spring damping

    bool enableMinDistanceLimit = false;        ///< Enable min distance
    bool enableMaxDistanceLimit = true;         ///< Enable max distance

    bool enableProjection = true;               ///< Enable constraint projection
    PxReal projectionLinearTolerance = 1e10f;   ///< Linear error tolerance
    PxReal projectionAngularTolerance = PxPi;   ///< Angular error tolerance
};

/**
 * @brief D6 joint configuration (6 degree-of-freedom)
 */
struct D6JointConfig {
    // Motion settings for each axis
    PxD6Motion::Enum motionX = PxD6Motion::eLOCKED;
    PxD6Motion::Enum motionY = PxD6Motion::eLOCKED;
    PxD6Motion::Enum motionZ = PxD6Motion::eLOCKED;
    PxD6Motion::Enum motionTwist = PxD6Motion::eLOCKED;
    PxD6Motion::Enum motionSwing1 = PxD6Motion::eLOCKED;
    PxD6Motion::Enum motionSwing2 = PxD6Motion::eLOCKED;

    // Linear limits
    bool enableLinearLimit = false;
    PxReal linearLimitValue = 0.0f;
    PxReal linearLimitRestitution = 0.0f;
    PxReal linearLimitStiffness = 0.0f;
    PxReal linearLimitDamping = 0.0f;

    // Swing limits (cone)
    bool enableSwingLimit = false;
    PxReal swingYAngle = PxPi / 4.0f;
    PxReal swingZAngle = PxPi / 4.0f;
    PxReal swingLimitRestitution = 0.0f;
    PxReal swingLimitStiffness = 0.0f;
    PxReal swingLimitDamping = 0.0f;

    // Twist limits
    bool enableTwistLimit = false;
    PxReal twistLowerAngle = -PxPi / 4.0f;
    PxReal twistUpperAngle = PxPi / 4.0f;
    PxReal twistLimitRestitution = 0.0f;
    PxReal twistLimitStiffness = 0.0f;
    PxReal twistLimitDamping = 0.0f;

    // Drive settings
    bool enableDrive = false;
    PxD6Drive::Enum driveType = PxD6Drive::eSLERP;
    PxReal driveStiffness = 0.0f;
    PxReal driveDamping = 1000.0f;
    PxReal driveForceLimit = PX_MAX_F32;
    bool driveIsAcceleration = true;

    bool enableProjection = true;
    PxReal projectionLinearTolerance = 1e10f;
    PxReal projectionAngularTolerance = PxPi;
};

/**
 * @brief Joint break configuration
 */
struct JointBreakConfig {
    bool enableBreak = false;                   ///< Enable joint breaking
    PxReal breakForce = PX_MAX_F32;            ///< Force threshold
    PxReal breakTorque = PX_MAX_F32;           ///< Torque threshold
};

/**
 * @brief Joint chain configuration
 */
struct JointChainConfig {
    enum class ChainType {
        FIXED,          ///< Fixed joints (rigid chain)
        SPHERICAL,      ///< Spherical joints (rope)
        REVOLUTE,       ///< Revolute joints (articulated)
        MIXED           ///< Mixed joint types
    };

    ChainType type = ChainType::FIXED;
    PxU32 linkCount = 5;                       ///< Number of links
    PxReal linkLength = 2.0f;                  ///< Length of each link
    PxReal linkRadius = 0.5f;                  ///< Radius of each link
    PxReal linkMass = 10.0f;                   ///< Mass of each link
    bool breakable = false;                     ///< Enable joint breaking
    PxReal breakForce = PX_MAX_F32;            ///< Break force threshold
};

/**
 * @brief Joint manager class
 *
 * This class provides comprehensive joint creation and management:
 * - All PhysX joint types
 * - Joint limits and drives
 * - Breakable joints with callbacks
 * - Joint chains for ragdolls/ropes
 * - Projection and constraint stabilization
 *
 * @example
 * @code
 * // Create joint manager
 * JointManager jointMgr;
 * jointMgr.initialize(physics);
 *
 * // Create spherical joint with limits
 * SphericalJointConfig config;
 * config.enableLimit = true;
 * config.yAngleLimit = PxPi / 4.0f;
 * PxSphericalJoint* joint = jointMgr.createSphericalJoint(
 *     actor0, PxTransform(PxVec3(0, 1, 0)),
 *     actor1, PxTransform(PxVec3(0, -1, 0)),
 *     config
 * );
 *
 * // Create breakable fixed joint
 * JointBreakConfig breakConfig;
 * breakConfig.enableBreak = true;
 * breakConfig.breakForce = 1000.0f;
 * jointMgr.createFixedJoint(actor0, t0, actor1, t1, FixedJointConfig(), breakConfig);
 *
 * // Create joint chain (rope)
 * JointChainConfig chainConfig;
 * chainConfig.type = JointChainConfig::ChainType::SPHERICAL;
 * chainConfig.linkCount = 10;
 * std::vector<PxRigidDynamic*> chain = jointMgr.createJointChain(
 *     scene, startActor, PxVec3(0, 0, 1), chainConfig
 * );
 * @endcode
 */
class JointManager {
public:
    /**
     * @brief Constructor
     */
    JointManager();

    /**
     * @brief Destructor
     */
    ~JointManager();

    // No copy
    JointManager(const JointManager&) = delete;
    JointManager& operator=(const JointManager&) = delete;

    // Move allowed
    JointManager(JointManager&&) noexcept;
    JointManager& operator=(JointManager&&) noexcept;

    /**
     * @brief Initialize joint manager
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
    // Joint Creation
    // ========================================================================

    /**
     * @brief Create spherical joint (ball-and-socket)
     * @param actor0 First actor (can be NULL for world anchor)
     * @param localFrame0 Joint frame relative to actor0
     * @param actor1 Second actor
     * @param localFrame1 Joint frame relative to actor1
     * @param config Joint configuration
     * @param breakConfig Break configuration
     * @return Created joint or nullptr on failure
     */
    PxSphericalJoint* createSphericalJoint(
        PxRigidActor* actor0, const PxTransform& localFrame0,
        PxRigidActor* actor1, const PxTransform& localFrame1,
        const SphericalJointConfig& config = SphericalJointConfig(),
        const JointBreakConfig& breakConfig = JointBreakConfig()
    );

    /**
     * @brief Create fixed joint (welded)
     * @param actor0 First actor (can be NULL for world anchor)
     * @param localFrame0 Joint frame relative to actor0
     * @param actor1 Second actor
     * @param localFrame1 Joint frame relative to actor1
     * @param config Joint configuration
     * @param breakConfig Break configuration
     * @return Created joint or nullptr on failure
     */
    PxFixedJoint* createFixedJoint(
        PxRigidActor* actor0, const PxTransform& localFrame0,
        PxRigidActor* actor1, const PxTransform& localFrame1,
        const FixedJointConfig& config = FixedJointConfig(),
        const JointBreakConfig& breakConfig = JointBreakConfig()
    );

    /**
     * @brief Create revolute joint (hinge)
     * @param actor0 First actor (can be NULL for world anchor)
     * @param localFrame0 Joint frame relative to actor0
     * @param actor1 Second actor
     * @param localFrame1 Joint frame relative to actor1
     * @param config Joint configuration
     * @param breakConfig Break configuration
     * @return Created joint or nullptr on failure
     */
    PxRevoluteJoint* createRevoluteJoint(
        PxRigidActor* actor0, const PxTransform& localFrame0,
        PxRigidActor* actor1, const PxTransform& localFrame1,
        const RevoluteJointConfig& config = RevoluteJointConfig(),
        const JointBreakConfig& breakConfig = JointBreakConfig()
    );

    /**
     * @brief Create prismatic joint (slider)
     * @param actor0 First actor (can be NULL for world anchor)
     * @param localFrame0 Joint frame relative to actor0
     * @param actor1 Second actor
     * @param localFrame1 Joint frame relative to actor1
     * @param config Joint configuration
     * @param breakConfig Break configuration
     * @return Created joint or nullptr on failure
     */
    PxPrismaticJoint* createPrismaticJoint(
        PxRigidActor* actor0, const PxTransform& localFrame0,
        PxRigidActor* actor1, const PxTransform& localFrame1,
        const PrismaticJointConfig& config = PrismaticJointConfig(),
        const JointBreakConfig& breakConfig = JointBreakConfig()
    );

    /**
     * @brief Create distance joint
     * @param actor0 First actor (can be NULL for world anchor)
     * @param localFrame0 Joint frame relative to actor0
     * @param actor1 Second actor
     * @param localFrame1 Joint frame relative to actor1
     * @param config Joint configuration
     * @param breakConfig Break configuration
     * @return Created joint or nullptr on failure
     */
    PxDistanceJoint* createDistanceJoint(
        PxRigidActor* actor0, const PxTransform& localFrame0,
        PxRigidActor* actor1, const PxTransform& localFrame1,
        const DistanceJointConfig& config = DistanceJointConfig(),
        const JointBreakConfig& breakConfig = JointBreakConfig()
    );

    /**
     * @brief Create D6 joint (six degree-of-freedom)
     * @param actor0 First actor (can be NULL for world anchor)
     * @param localFrame0 Joint frame relative to actor0
     * @param actor1 Second actor
     * @param localFrame1 Joint frame relative to actor1
     * @param config Joint configuration
     * @param breakConfig Break configuration
     * @return Created joint or nullptr on failure
     */
    PxD6Joint* createD6Joint(
        PxRigidActor* actor0, const PxTransform& localFrame0,
        PxRigidActor* actor1, const PxTransform& localFrame1,
        const D6JointConfig& config = D6JointConfig(),
        const JointBreakConfig& breakConfig = JointBreakConfig()
    );

    // ========================================================================
    // Joint Chains
    // ========================================================================

    /**
     * @brief Create joint chain (rope, ragdoll, etc.)
     * @param scene Scene to add actors to
     * @param startActor Starting actor (anchor point, can be NULL)
     * @param direction Chain direction
     * @param config Chain configuration
     * @param material Material for link shapes
     * @return Vector of created link actors
     */
    std::vector<PxRigidDynamic*> createJointChain(
        PxScene* scene,
        PxRigidActor* startActor,
        const PxVec3& direction,
        const JointChainConfig& config,
        PxMaterial* material = nullptr
    );

    // ========================================================================
    // Joint Management
    // ========================================================================

    /**
     * @brief Release joint and remove from tracking
     * @param joint Joint to release
     */
    void releaseJoint(PxJoint* joint);

    /**
     * @brief Get all managed joints
     * @return Vector of joints
     */
    std::vector<PxJoint*> getAllJoints() const;

    /**
     * @brief Get joint count
     * @return Number of managed joints
     */
    PxU32 getJointCount() const;

    /**
     * @brief Set joint break callback
     * @param callback Callback function
     */
    void setJointBreakCallback(JointBreakCallback callback);

    /**
     * @brief Notify joint break (called internally)
     * @param joint Broken joint
     * @param force Applied force
     * @param torque Applied torque
     */
    void notifyJointBreak(PxJoint* joint, PxReal force, PxReal torque);

    /**
     * @brief Get last error message
     * @return Error message or empty string
     */
    std::string getLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    void applyBreakConfig(PxJoint* joint, const JointBreakConfig& config);
};

} // namespace PhysXWrapper
