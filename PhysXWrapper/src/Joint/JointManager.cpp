/**
 * @file JointManager.cpp
 * @brief Implementation of JointManager class
 */

#include "Joint/JointManager.h"
#include "extensions/PxRigidBodyExt.h"
#include <algorithm>
#include <string>

namespace PhysXWrapper {

/**
 * @brief Private implementation
 */
class JointManager::Impl {
public:
    Impl()
        : m_physics(nullptr)
        , m_initialized(false)
    {}

    PxPhysics* m_physics;
    bool m_initialized;
    std::string m_lastError;

    std::vector<PxJoint*> m_joints;
    JointBreakCallback m_breakCallback;

    void setError(const std::string& error) {
        m_lastError = error;
    }

    void clearError() {
        m_lastError.clear();
    }

    void trackJoint(PxJoint* joint) {
        if (joint) {
            m_joints.push_back(joint);
        }
    }

    void untrackJoint(PxJoint* joint) {
        auto it = std::find(m_joints.begin(), m_joints.end(), joint);
        if (it != m_joints.end()) {
            m_joints.erase(it);
        }
    }
};

// ============================================================================
// Construction / Destruction
// ============================================================================

JointManager::JointManager()
    : m_impl(std::make_unique<Impl>())
{
}

JointManager::~JointManager() {
    cleanup();
}

JointManager::JointManager(JointManager&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

JointManager& JointManager::operator=(JointManager&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

// ============================================================================
// Initialization
// ============================================================================

bool JointManager::initialize(PxPhysics* physics) {
    if (!physics) {
        m_impl->setError("Invalid physics instance");
        return false;
    }

    if (m_impl->m_initialized) {
        m_impl->setError("Already initialized");
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_initialized = true;
    m_impl->clearError();

    return true;
}

void JointManager::cleanup() {
    if (!m_impl->m_initialized) return;

    // Release all tracked joints
    for (PxJoint* joint : m_impl->m_joints) {
        if (joint) {
            joint->release();
        }
    }
    m_impl->m_joints.clear();

    m_impl->m_physics = nullptr;
    m_impl->m_initialized = false;
    m_impl->clearError();
}

bool JointManager::isInitialized() const {
    return m_impl->m_initialized;
}

// ============================================================================
// Spherical Joint
// ============================================================================

PxSphericalJoint* JointManager::createSphericalJoint(
    PxRigidActor* actor0, const PxTransform& localFrame0,
    PxRigidActor* actor1, const PxTransform& localFrame1,
    const SphericalJointConfig& config,
    const JointBreakConfig& breakConfig)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("JointManager not initialized");
        return nullptr;
    }

    if (!actor1) {
        m_impl->setError("actor1 cannot be NULL");
        return nullptr;
    }

    // Create joint
    PxSphericalJoint* joint = PxSphericalJointCreate(
        *m_impl->m_physics,
        actor0, localFrame0,
        actor1, localFrame1
    );

    if (!joint) {
        m_impl->setError("Failed to create spherical joint");
        return nullptr;
    }

    // Apply configuration
    if (config.enableLimit) {
        PxJointLimitCone limit(config.yAngleLimit, config.zAngleLimit);
        limit.restitution = config.limitRestitution;
        limit.stiffness = config.limitStiffness;
        limit.damping = config.limitDamping;
        joint->setLimitCone(limit);
        joint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
    }

    // PhysX 5.x: Projection API has been removed/changed
    // Joint projection settings are no longer configurable this way
    /*
    if (config.enableProjection) {
        // PhysX 5.x: setProjectionLinearTolerance removed
        // joint->setProjectionLinearTolerance(config.projectionLinearTolerance);
        // PhysX 5.x: ePROJECTION flag removed
        // joint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
    }
    */

    // Apply break configuration
    applyBreakConfig(joint, breakConfig);

    // Track joint
    m_impl->trackJoint(joint);
    m_impl->clearError();

    return joint;
}

// ============================================================================
// Fixed Joint
// ============================================================================

PxFixedJoint* JointManager::createFixedJoint(
    PxRigidActor* actor0, const PxTransform& localFrame0,
    PxRigidActor* actor1, const PxTransform& localFrame1,
    const FixedJointConfig& config,
    const JointBreakConfig& breakConfig)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("JointManager not initialized");
        return nullptr;
    }

    if (!actor1) {
        m_impl->setError("actor1 cannot be NULL");
        return nullptr;
    }

    // Create joint
    PxFixedJoint* joint = PxFixedJointCreate(
        *m_impl->m_physics,
        actor0, localFrame0,
        actor1, localFrame1
    );

    if (!joint) {
        m_impl->setError("Failed to create fixed joint");
        return nullptr;
    }

    // Apply projection
    if (config.enableProjection) {
        // PhysX 5.x: setProjectionLinearTolerance removed
        // joint->setProjectionLinearTolerance(config.projectionLinearTolerance);
        // PhysX 5.x: setProjectionAngularTolerance removed
        // joint->setProjectionAngularTolerance(config.projectionAngularTolerance);
        // PhysX 5.x: ePROJECTION flag removed
        // joint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
    }

    // Apply break configuration
    applyBreakConfig(joint, breakConfig);

    // Track joint
    m_impl->trackJoint(joint);
    m_impl->clearError();

    return joint;
}

// ============================================================================
// Revolute Joint
// ============================================================================

PxRevoluteJoint* JointManager::createRevoluteJoint(
    PxRigidActor* actor0, const PxTransform& localFrame0,
    PxRigidActor* actor1, const PxTransform& localFrame1,
    const RevoluteJointConfig& config,
    const JointBreakConfig& breakConfig)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("JointManager not initialized");
        return nullptr;
    }

    if (!actor1) {
        m_impl->setError("actor1 cannot be NULL");
        return nullptr;
    }

    // Create joint
    PxRevoluteJoint* joint = PxRevoluteJointCreate(
        *m_impl->m_physics,
        actor0, localFrame0,
        actor1, localFrame1
    );

    if (!joint) {
        m_impl->setError("Failed to create revolute joint");
        return nullptr;
    }

    // Apply limits
    if (config.enableLimit) {
        PxJointAngularLimitPair limit(config.lowerLimit, config.upperLimit);
        limit.restitution = config.limitRestitution;
        limit.stiffness = config.limitStiffness;
        limit.damping = config.limitDamping;
        joint->setLimit(limit);
        joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
    }

    // Apply drive
    if (config.enableDrive) {
        joint->setDriveVelocity(config.driveVelocity);
        joint->setDriveForceLimit(config.driveForceLimit);
        joint->setDriveGearRatio(config.driveGearRatio);
        joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
    }

    // Apply projection
    if (config.enableProjection) {
        // PhysX 5.x: setProjectionLinearTolerance removed
        // joint->setProjectionLinearTolerance(config.projectionLinearTolerance);
        // PhysX 5.x: setProjectionAngularTolerance removed
        // joint->setProjectionAngularTolerance(config.projectionAngularTolerance);
        // PhysX 5.x: ePROJECTION flag removed
        // joint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
    }

    // Apply break configuration
    applyBreakConfig(joint, breakConfig);

    // Track joint
    m_impl->trackJoint(joint);
    m_impl->clearError();

    return joint;
}

// ============================================================================
// Prismatic Joint
// ============================================================================

PxPrismaticJoint* JointManager::createPrismaticJoint(
    PxRigidActor* actor0, const PxTransform& localFrame0,
    PxRigidActor* actor1, const PxTransform& localFrame1,
    const PrismaticJointConfig& config,
    const JointBreakConfig& breakConfig)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("JointManager not initialized");
        return nullptr;
    }

    if (!actor1) {
        m_impl->setError("actor1 cannot be NULL");
        return nullptr;
    }

    // Create joint
    PxPrismaticJoint* joint = PxPrismaticJointCreate(
        *m_impl->m_physics,
        actor0, localFrame0,
        actor1, localFrame1
    );

    if (!joint) {
        m_impl->setError("Failed to create prismatic joint");
        return nullptr;
    }

    // Apply limits
    if (config.enableLimit) {
        PxJointLinearLimitPair limit(m_impl->m_physics->getTolerancesScale(),
                                      config.lowerLimit, config.upperLimit);
        limit.restitution = config.limitRestitution;
        limit.stiffness = config.limitStiffness;
        limit.damping = config.limitDamping;
        joint->setLimit(limit);
        joint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, true);
    }

    // Apply projection
    if (config.enableProjection) {
        // PhysX 5.x: setProjectionLinearTolerance removed
        // joint->setProjectionLinearTolerance(config.projectionLinearTolerance);
        // PhysX 5.x: setProjectionAngularTolerance removed
        // joint->setProjectionAngularTolerance(config.projectionAngularTolerance);
        // PhysX 5.x: ePROJECTION flag removed
        // joint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
    }

    // Apply break configuration
    applyBreakConfig(joint, breakConfig);

    // Track joint
    m_impl->trackJoint(joint);
    m_impl->clearError();

    return joint;
}

// ============================================================================
// Distance Joint
// ============================================================================

PxDistanceJoint* JointManager::createDistanceJoint(
    PxRigidActor* actor0, const PxTransform& localFrame0,
    PxRigidActor* actor1, const PxTransform& localFrame1,
    const DistanceJointConfig& config,
    const JointBreakConfig& breakConfig)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("JointManager not initialized");
        return nullptr;
    }

    if (!actor1) {
        m_impl->setError("actor1 cannot be NULL");
        return nullptr;
    }

    // Create joint
    PxDistanceJoint* joint = PxDistanceJointCreate(
        *m_impl->m_physics,
        actor0, localFrame0,
        actor1, localFrame1
    );

    if (!joint) {
        m_impl->setError("Failed to create distance joint");
        return nullptr;
    }

    // Apply distance limits
    joint->setMinDistance(config.minDistance);
    joint->setMaxDistance(config.maxDistance);

    // Apply stiffness and damping
    joint->setStiffness(config.stiffness);
    joint->setDamping(config.damping);

    // Enable limits
    joint->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED,
                                 config.enableMinDistanceLimit);
    joint->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED,
                                 config.enableMaxDistanceLimit);

    // Spring behavior (if stiffness > 0)
    if (config.stiffness > 0.0f || config.damping > 0.0f) {
        joint->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, true);
    }

    // Apply projection
    if (config.enableProjection) {
        // PhysX 5.x: ePROJECTION flag removed
        // joint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
    }

    // Apply break configuration
    applyBreakConfig(joint, breakConfig);

    // Track joint
    m_impl->trackJoint(joint);
    m_impl->clearError();

    return joint;
}

// ============================================================================
// D6 Joint
// ============================================================================

PxD6Joint* JointManager::createD6Joint(
    PxRigidActor* actor0, const PxTransform& localFrame0,
    PxRigidActor* actor1, const PxTransform& localFrame1,
    const D6JointConfig& config,
    const JointBreakConfig& breakConfig)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("JointManager not initialized");
        return nullptr;
    }

    if (!actor1) {
        m_impl->setError("actor1 cannot be NULL");
        return nullptr;
    }

    // Create joint
    PxD6Joint* joint = PxD6JointCreate(
        *m_impl->m_physics,
        actor0, localFrame0,
        actor1, localFrame1
    );

    if (!joint) {
        m_impl->setError("Failed to create D6 joint");
        return nullptr;
    }

    // Set motion constraints
    joint->setMotion(PxD6Axis::eX, config.motionX);
    joint->setMotion(PxD6Axis::eY, config.motionY);
    joint->setMotion(PxD6Axis::eZ, config.motionZ);
    joint->setMotion(PxD6Axis::eTWIST, config.motionTwist);
    joint->setMotion(PxD6Axis::eSWING1, config.motionSwing1);
    joint->setMotion(PxD6Axis::eSWING2, config.motionSwing2);

    // Apply linear limits (PhysX 5.x: constructor no longer takes TolerancesScale)
    if (config.enableLinearLimit) {
        PxJointLinearLimit limit(config.linearLimitValue);
        limit.restitution = config.linearLimitRestitution;
        limit.stiffness = config.linearLimitStiffness;
        limit.damping = config.linearLimitDamping;
        joint->setLinearLimit(limit);
    }

    // Apply swing limits
    if (config.enableSwingLimit) {
        PxJointLimitCone limit(config.swingYAngle, config.swingZAngle);
        limit.restitution = config.swingLimitRestitution;
        limit.stiffness = config.swingLimitStiffness;
        limit.damping = config.swingLimitDamping;
        joint->setSwingLimit(limit);
    }

    // Apply twist limits
    if (config.enableTwistLimit) {
        PxJointAngularLimitPair limit(config.twistLowerAngle, config.twistUpperAngle);
        limit.restitution = config.twistLimitRestitution;
        limit.stiffness = config.twistLimitStiffness;
        limit.damping = config.twistLimitDamping;
        joint->setTwistLimit(limit);
    }

    // Apply drive
    if (config.enableDrive) {
        PxD6JointDrive drive(config.driveStiffness, config.driveDamping,
                            config.driveForceLimit, config.driveIsAcceleration);
        joint->setDrive(config.driveType, drive);
    }

    // Apply projection
    if (config.enableProjection) {
        // PhysX 5.x: setProjectionLinearTolerance removed
        // joint->setProjectionLinearTolerance(config.projectionLinearTolerance);
        // PhysX 5.x: setProjectionAngularTolerance removed
        // joint->setProjectionAngularTolerance(config.projectionAngularTolerance);
        // PhysX 5.x: ePROJECTION flag removed
        // joint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
    }

    // Apply break configuration
    applyBreakConfig(joint, breakConfig);

    // Track joint
    m_impl->trackJoint(joint);
    m_impl->clearError();

    return joint;
}

// ============================================================================
// Joint Chain
// ============================================================================

std::vector<PxRigidDynamic*> JointManager::createJointChain(
    PxScene* scene,
    PxRigidActor* startActor,
    const PxVec3& direction,
    const JointChainConfig& config,
    PxMaterial* material)
{
    std::vector<PxRigidDynamic*> links;

    if (!m_impl->m_initialized) {
        m_impl->setError("JointManager not initialized");
        return links;
    }

    if (!scene) {
        m_impl->setError("Invalid scene");
        return links;
    }

    // Use default material if none provided
    PxMaterial* mat = material;
    if (!mat) {
        mat = m_impl->m_physics->createMaterial(0.5f, 0.5f, 0.1f);
    }

    // Normalize direction
    PxVec3 dir = direction;
    dir.normalize();

    // Calculate starting position
    PxVec3 startPos(0, 0, 0);
    if (startActor) {
        startPos = startActor->getGlobalPose().p;
    }

    // Create links
    PxRigidActor* prevActor = startActor;

    for (PxU32 i = 0; i < config.linkCount; i++) {
        // Calculate link position
        PxVec3 linkPos = startPos + dir * config.linkLength * (i + 1);

        // Create dynamic actor
        PxRigidDynamic* link = m_impl->m_physics->createRigidDynamic(
            PxTransform(linkPos)
        );

        if (!link) {
            m_impl->setError("Failed to create link actor");
            break;
        }

        // Create shape (capsule aligned with direction)
        PxShape* shape = m_impl->m_physics->createShape(
            PxCapsuleGeometry(config.linkRadius, config.linkLength * 0.5f),
            *mat,
            true
        );

        if (!shape) {
            link->release();
            m_impl->setError("Failed to create link shape");
            break;
        }

        // Align capsule with direction
        PxQuat rot = PxShortestRotation(PxVec3(1, 0, 0), dir);
        shape->setLocalPose(PxTransform(rot));

        link->attachShape(*shape);
        shape->release();

        // Set mass
        PxRigidBodyExt::updateMassAndInertia(*link, config.linkMass);

        // Add to scene
        scene->addActor(*link);
        links.push_back(link);

        // Create joint to previous actor
        if (prevActor) {
            PxVec3 anchor = linkPos - dir * config.linkLength * 0.5f;

            PxTransform localFrame0 = prevActor->getGlobalPose().transformInv(PxTransform(anchor));
            PxTransform localFrame1 = link->getGlobalPose().transformInv(PxTransform(anchor));

            PxJoint* joint = nullptr;

            // Create appropriate joint type
            switch (config.type) {
                case JointChainConfig::ChainType::FIXED: {
                    FixedJointConfig jointConfig;
                    JointBreakConfig breakConfig;
                    breakConfig.enableBreak = config.breakable;
                    breakConfig.breakForce = config.breakForce;
                    joint = createFixedJoint(prevActor, localFrame0, link, localFrame1,
                                            jointConfig, breakConfig);
                    break;
                }

                case JointChainConfig::ChainType::SPHERICAL: {
                    SphericalJointConfig jointConfig;
                    JointBreakConfig breakConfig;
                    breakConfig.enableBreak = config.breakable;
                    breakConfig.breakForce = config.breakForce;
                    joint = createSphericalJoint(prevActor, localFrame0, link, localFrame1,
                                                 jointConfig, breakConfig);
                    break;
                }

                case JointChainConfig::ChainType::REVOLUTE: {
                    RevoluteJointConfig jointConfig;
                    JointBreakConfig breakConfig;
                    breakConfig.enableBreak = config.breakable;
                    breakConfig.breakForce = config.breakForce;
                    joint = createRevoluteJoint(prevActor, localFrame0, link, localFrame1,
                                               jointConfig, breakConfig);
                    break;
                }

                case JointChainConfig::ChainType::MIXED: {
                    // Alternate between spherical and fixed
                    if (i % 2 == 0) {
                        SphericalJointConfig jointConfig;
                        JointBreakConfig breakConfig;
                        breakConfig.enableBreak = config.breakable;
                        breakConfig.breakForce = config.breakForce;
                        joint = createSphericalJoint(prevActor, localFrame0, link, localFrame1,
                                                     jointConfig, breakConfig);
                    } else {
                        FixedJointConfig jointConfig;
                        JointBreakConfig breakConfig;
                        breakConfig.enableBreak = config.breakable;
                        breakConfig.breakForce = config.breakForce;
                        joint = createFixedJoint(prevActor, localFrame0, link, localFrame1,
                                                jointConfig, breakConfig);
                    }
                    break;
                }
            }

            if (!joint) {
                m_impl->setError("Failed to create chain joint");
            }
        }

        prevActor = link;
    }

    // Clean up temporary material
    if (!material && mat) {
        mat->release();
    }

    m_impl->clearError();
    return links;
}

// ============================================================================
// Joint Management
// ============================================================================

void JointManager::releaseJoint(PxJoint* joint) {
    if (!joint) return;

    m_impl->untrackJoint(joint);
    joint->release();
}

std::vector<PxJoint*> JointManager::getAllJoints() const {
    return m_impl->m_joints;
}

PxU32 JointManager::getJointCount() const {
    return static_cast<PxU32>(m_impl->m_joints.size());
}

void JointManager::setJointBreakCallback(JointBreakCallback callback) {
    m_impl->m_breakCallback = callback;
}

void JointManager::notifyJointBreak(PxJoint* joint, PxReal force, PxReal torque) {
    if (m_impl->m_breakCallback && joint) {
        JointBreakEvent event;
        event.joint = joint;
        event.forceApplied = force;
        event.torqueApplied = torque;
        // PhysX 5.x: getUserData() is now a public member userData
        event.userData = joint->userData;

        m_impl->m_breakCallback(event);
    }

    // Untrack broken joint
    m_impl->untrackJoint(joint);
}

std::string JointManager::getLastError() const {
    return m_impl->m_lastError;
}

// ============================================================================
// Private Helpers
// ============================================================================

void JointManager::applyBreakConfig(PxJoint* joint, const JointBreakConfig& config) {
    if (!joint) return;

    if (config.enableBreak) {
        joint->setBreakForce(config.breakForce, config.breakTorque);
        joint->setConstraintFlag(PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES, true);
    }
}

} // namespace PhysXWrapper
