/**
 * @file ArticulationManager.cpp
 * @brief Implementation of ArticulationManager class
 */

#include "Articulation/ArticulationManager.h"
#include "extensions/PxRigidBodyExt.h"
#include <algorithm>

namespace PhysXWrapper {

/**
 * @brief Private implementation
 */
class ArticulationManager::Impl {
public:
    Impl()
        : m_physics(nullptr)
        , m_initialized(false)
    {}

    PxPhysics* m_physics;
    PxMaterial* m_defaultMaterial;
    bool m_initialized;
    std::string m_lastError;

    std::vector<PxArticulationReducedCoordinate*> m_articulations;

    void setError(const std::string& error) {
        m_lastError = error;
    }

    void clearError() {
        m_lastError.clear();
    }

    void trackArticulation(PxArticulationReducedCoordinate* art) {
        if (art) {
            m_articulations.push_back(art);
        }
    }

    void untrackArticulation(PxArticulationReducedCoordinate* art) {
        auto it = std::find(m_articulations.begin(), m_articulations.end(), art);
        if (it != m_articulations.end()) {
            m_articulations.erase(it);
        }
    }
};

// ============================================================================
// Construction / Destruction
// ============================================================================

ArticulationManager::ArticulationManager()
    : m_impl(std::make_unique<Impl>())
{
}

ArticulationManager::~ArticulationManager() {
    cleanup();
}

ArticulationManager::ArticulationManager(ArticulationManager&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

ArticulationManager& ArticulationManager::operator=(ArticulationManager&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

// ============================================================================
// Initialization
// ============================================================================

bool ArticulationManager::initialize(PxPhysics* physics) {
    if (!physics) {
        m_impl->setError("Invalid physics instance");
        return false;
    }

    if (m_impl->m_initialized) {
        m_impl->setError("Already initialized");
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);
    m_impl->m_initialized = true;
    m_impl->clearError();

    return true;
}

void ArticulationManager::cleanup() {
    if (!m_impl->m_initialized) return;

    // Release all tracked articulations
    for (PxArticulationReducedCoordinate* art : m_impl->m_articulations) {
        if (art) {
            art->release();
        }
    }
    m_impl->m_articulations.clear();

    if (m_impl->m_defaultMaterial) {
        m_impl->m_defaultMaterial->release();
        m_impl->m_defaultMaterial = nullptr;
    }

    m_impl->m_physics = nullptr;
    m_impl->m_initialized = false;
    m_impl->clearError();
}

bool ArticulationManager::isInitialized() const {
    return m_impl->m_initialized;
}

// ============================================================================
// Articulation Creation
// ============================================================================

PxArticulationReducedCoordinate* ArticulationManager::createArticulation(
    PxScene* scene,
    const ArticulationConfig& config)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("ArticulationManager not initialized");
        return nullptr;
    }

    if (!scene) {
        m_impl->setError("Invalid scene");
        return nullptr;
    }

    // Create articulation
    PxArticulationReducedCoordinate* articulation =
        m_impl->m_physics->createArticulationReducedCoordinate();

    if (!articulation) {
        m_impl->setError("Failed to create articulation");
        return nullptr;
    }

    // Apply configuration
    applyArticulationConfig(articulation, config);

    // Add to scene
    scene->addArticulation(*articulation);

    // Track articulation
    m_impl->trackArticulation(articulation);
    m_impl->clearError();

    return articulation;
}

void ArticulationManager::releaseArticulation(PxArticulationReducedCoordinate* articulation) {
    if (!articulation) return;

    m_impl->untrackArticulation(articulation);
    articulation->release();
}

// ============================================================================
// Link Creation
// ============================================================================

PxArticulationLink* ArticulationManager::createLink(
    PxArticulationReducedCoordinate* articulation,
    PxArticulationLink* parent,
    const ArticulationLinkConfig& linkConfig,
    const ArticulationJointConfig& jointConfig,
    PxMaterial* material)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("ArticulationManager not initialized");
        return nullptr;
    }

    if (!articulation) {
        m_impl->setError("Invalid articulation");
        return nullptr;
    }

    if (!linkConfig.geometry) {
        m_impl->setError("Invalid geometry");
        return nullptr;
    }

    // Use default material if not provided
    PxMaterial* mat = material ? material : m_impl->m_defaultMaterial;

    // Create link
    PxArticulationLink* link = articulation->createLink(parent, linkConfig.globalPose);
    if (!link) {
        m_impl->setError("Failed to create link");
        return nullptr;
    }

    // Create shape
    PxShape* shape = PxRigidActorExt::createExclusiveShape(
        *link, *linkConfig.geometry, *mat
    );

    if (!shape) {
        m_impl->setError("Failed to create shape");
        return nullptr;
    }

    // Set mass and inertia
    PxRigidBodyExt::updateMassAndInertia(*link, linkConfig.density);

    // Set velocity limits and damping
    link->setLinearDamping(linkConfig.linearDamping);
    link->setAngularDamping(linkConfig.angularDamping);
    link->setMaxLinearVelocity(linkConfig.maxLinearVelocity);
    link->setMaxAngularVelocity(linkConfig.maxAngularVelocity);

    // Configure joint (if not root link)
    if (parent) {
        PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
        if (joint) {
            applyJointConfig(joint, jointConfig);
        }
    }

    m_impl->clearError();
    return link;
}

// ============================================================================
// Joint Configuration
// ============================================================================

bool ArticulationManager::configureJoint(
    PxArticulationLink* link,
    const ArticulationJointConfig& jointConfig)
{
    if (!link) {
        m_impl->setError("Invalid link");
        return false;
    }

    PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
    if (!joint) {
        m_impl->setError("Link has no inbound joint (root link?)");
        return false;
    }

    applyJointConfig(joint, jointConfig);
    m_impl->clearError();
    return true;
}

bool ArticulationManager::setJointMotion(
    PxArticulationLink* link,
    PxArticulationAxis::Enum axis,
    PxArticulationMotion::Enum motion)
{
    if (!link) {
        m_impl->setError("Invalid link");
        return false;
    }

    PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
    if (!joint) {
        m_impl->setError("Link has no inbound joint");
        return false;
    }

    joint->setMotion(axis, motion);
    m_impl->clearError();
    return true;
}

bool ArticulationManager::setJointLimits(
    PxArticulationLink* link,
    PxArticulationAxis::Enum axis,
    PxReal low,
    PxReal high)
{
    if (!link) {
        m_impl->setError("Invalid link");
        return false;
    }

    PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
    if (!joint) {
        m_impl->setError("Link has no inbound joint");
        return false;
    }

    joint->setLimitParams(axis, PxArticulationLimit(low, high));
    m_impl->clearError();
    return true;
}

bool ArticulationManager::setJointDrive(
    PxArticulationLink* link,
    PxArticulationAxis::Enum axis,
    PxReal stiffness,
    PxReal damping,
    PxReal maxForce,
    bool isAcceleration)
{
    if (!link) {
        m_impl->setError("Invalid link");
        return false;
    }

    PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
    if (!joint) {
        m_impl->setError("Link has no inbound joint");
        return false;
    }

    joint->setDriveParams(axis, PxArticulationDrive(stiffness, damping, maxForce, isAcceleration ? PxArticulationDriveType::eACCELERATION : PxArticulationDriveType::eFORCE));
    m_impl->clearError();
    return true;
}

bool ArticulationManager::setDriveTarget(
    PxArticulationLink* link,
    PxArticulationAxis::Enum axis,
    PxReal target)
{
    if (!link) {
        m_impl->setError("Invalid link");
        return false;
    }

    PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
    if (!joint) {
        m_impl->setError("Link has no inbound joint");
        return false;
    }

    joint->setDriveTarget(axis, target);
    m_impl->clearError();
    return true;
}

bool ArticulationManager::setDriveVelocity(
    PxArticulationLink* link,
    PxArticulationAxis::Enum axis,
    PxReal targetVelocity)
{
    if (!link) {
        m_impl->setError("Invalid link");
        return false;
    }

    PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
    if (!joint) {
        m_impl->setError("Link has no inbound joint");
        return false;
    }

    joint->setDriveVelocity(axis, targetVelocity);
    m_impl->clearError();
    return true;
}

// ============================================================================
// Helper Builders
// ============================================================================

PxArticulationReducedCoordinate* ArticulationManager::createRobotArm(
    PxScene* scene,
    const RobotArmConfig& config,
    PxMaterial* material)
{
    if (!m_impl->m_initialized || !scene) {
        m_impl->setError("Invalid parameters");
        return nullptr;
    }

    // Create articulation
    ArticulationConfig artConfig;
    artConfig.solverIterationCounts = 32;
    artConfig.fixBase = true;

    PxArticulationReducedCoordinate* articulation = createArticulation(scene, artConfig);
    if (!articulation) {
        return nullptr;
    }

    // Normalize direction
    PxVec3 direction = config.armDirection;
    direction.normalize();

    // Create base link
    ArticulationLinkConfig baseLinkConfig;
    PxBoxGeometry baseGeom(0.5f, 0.5f, 0.5f);
    baseLinkConfig.geometry = &baseGeom;
    baseLinkConfig.globalPose = PxTransform(config.basePosition);
    baseLinkConfig.density = config.linkMass * 2.0f;  // Heavier base

    PxArticulationLink* prevLink = createLink(articulation, nullptr, baseLinkConfig, ArticulationJointConfig(), material);
    if (!prevLink) {
        releaseArticulation(articulation);
        return nullptr;
    }

    // Create arm links
    for (PxU32 i = 0; i < config.numJoints; i++) {
        PxVec3 linkPos = config.basePosition + direction * config.linkLength * (i + 1);

        // Create link
        ArticulationLinkConfig linkConfig;
        PxCapsuleGeometry linkGeom(config.linkRadius, config.linkLength * 0.5f);
        linkConfig.geometry = &linkGeom;
        linkConfig.globalPose = PxTransform(linkPos);
        linkConfig.density = config.linkMass;

        // Configure joint
        ArticulationJointConfig jointConfig;
        jointConfig.type = PxArticulationJointType::eREVOLUTE;
        jointConfig.motionTwist = PxArticulationMotion::eLIMITED;

        if (config.enableJointLimits) {
            jointConfig.enableLimits = true;
            jointConfig.limitLow = config.jointLimitLow;
            jointConfig.limitHigh = config.jointLimitHigh;
        }

        jointConfig.parentPose = PxTransform(PxVec3(0, 0, 0));
        jointConfig.childPose = PxTransform(PxVec3(0, -config.linkLength * 0.5f, 0));

        // Enable drive
        jointConfig.enableDrive = true;
        jointConfig.driveStiffness = 1000.0f;
        jointConfig.driveDamping = 100.0f;

        PxArticulationLink* link = createLink(articulation, prevLink, linkConfig, jointConfig, material);
        if (!link) {
            releaseArticulation(articulation);
            return nullptr;
        }

        prevLink = link;
    }

    m_impl->clearError();
    return articulation;
}

PxArticulationReducedCoordinate* ArticulationManager::createChain(
    PxScene* scene,
    const PxVec3& basePosition,
    const PxVec3& direction,
    PxU32 numLinks,
    PxReal linkLength,
    PxReal linkRadius,
    PxMaterial* material)
{
    if (!m_impl->m_initialized || !scene || numLinks == 0) {
        m_impl->setError("Invalid parameters");
        return nullptr;
    }

    // Create articulation
    ArticulationConfig artConfig;
    artConfig.solverIterationCounts = 16;

    PxArticulationReducedCoordinate* articulation = createArticulation(scene, artConfig);
    if (!articulation) {
        return nullptr;
    }

    // Normalize direction
    PxVec3 dir = direction;
    dir.normalize();

    // Create first link (base)
    ArticulationLinkConfig baseLinkConfig;
    PxSphereGeometry baseGeom(linkRadius * 2.0f);
    baseLinkConfig.geometry = &baseGeom;
    baseLinkConfig.globalPose = PxTransform(basePosition);
    baseLinkConfig.density = 10.0f;

    PxArticulationLink* prevLink = createLink(articulation, nullptr, baseLinkConfig, ArticulationJointConfig(), material);
    if (!prevLink) {
        releaseArticulation(articulation);
        return nullptr;
    }

    // Create chain links
    for (PxU32 i = 0; i < numLinks; i++) {
        PxVec3 linkPos = basePosition + dir * linkLength * (i + 1);

        // Create link
        ArticulationLinkConfig linkConfig;
        PxCapsuleGeometry linkGeom(linkRadius, linkLength * 0.5f);
        linkConfig.geometry = &linkGeom;
        linkConfig.globalPose = PxTransform(linkPos);
        linkConfig.density = 5.0f;

        // Configure joint (spherical for flexible chain)
        ArticulationJointConfig jointConfig;
        jointConfig.type = PxArticulationJointType::eSPHERICAL;
        jointConfig.motionSwing1 = PxArticulationMotion::eLIMITED;
        jointConfig.motionSwing2 = PxArticulationMotion::eLIMITED;
        jointConfig.motionTwist = PxArticulationMotion::eFREE;

        jointConfig.enableLimits = true;
        jointConfig.limitLow = -PxPi / 4.0f;
        jointConfig.limitHigh = PxPi / 4.0f;

        jointConfig.parentPose = PxTransform(PxVec3(0, 0, 0));
        jointConfig.childPose = PxTransform(PxVec3(0, -linkLength * 0.5f, 0));

        PxArticulationLink* link = createLink(articulation, prevLink, linkConfig, jointConfig, material);
        if (!link) {
            releaseArticulation(articulation);
            return nullptr;
        }

        prevLink = link;
    }

    m_impl->clearError();
    return articulation;
}

// ============================================================================
// Query and Control
// ============================================================================

std::vector<PxArticulationLink*> ArticulationManager::getLinks(
    PxArticulationReducedCoordinate* articulation) const
{
    std::vector<PxArticulationLink*> links;

    if (!articulation) return links;

    PxU32 numLinks = articulation->getNbLinks();
    if (numLinks == 0) return links;

    links.resize(numLinks);
    articulation->getLinks(links.data(), numLinks);

    return links;
}

PxU32 ArticulationManager::getLinkCount(PxArticulationReducedCoordinate* articulation) const {
    return articulation ? articulation->getNbLinks() : 0;
}

PxArticulationLink* ArticulationManager::getLink(
    PxArticulationReducedCoordinate* articulation,
    PxU32 index) const
{
    if (!articulation || index >= articulation->getNbLinks()) {
        return nullptr;
    }

    PxArticulationLink* link = nullptr;
    articulation->getLinks(&link, 1, index);
    return link;
}

PxReal ArticulationManager::getJointPosition(
    PxArticulationLink* link,
    PxArticulationAxis::Enum axis) const
{
    if (!link) return 0.0f;

    PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
    if (!joint) return 0.0f;

    return joint->getJointPosition(axis);
}

PxReal ArticulationManager::getJointVelocity(
    PxArticulationLink* link,
    PxArticulationAxis::Enum axis) const
{
    if (!link) return 0.0f;

    PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
    if (!joint) return 0.0f;

    return joint->getJointVelocity(axis);
}

void ArticulationManager::wakeUp(PxArticulationReducedCoordinate* articulation) {
    if (articulation) {
        articulation->wakeUp();
    }
}

void ArticulationManager::putToSleep(PxArticulationReducedCoordinate* articulation) {
    if (articulation) {
        articulation->putToSleep();
    }
}

bool ArticulationManager::isSleeping(PxArticulationReducedCoordinate* articulation) const {
    return articulation ? articulation->isSleeping() : false;
}

std::vector<PxArticulationReducedCoordinate*> ArticulationManager::getArticulations() const {
    return m_impl->m_articulations;
}

PxU32 ArticulationManager::getArticulationCount() const {
    return static_cast<PxU32>(m_impl->m_articulations.size());
}

std::string ArticulationManager::getLastError() const {
    return m_impl->m_lastError;
}

// ============================================================================
// Private Helpers
// ============================================================================

void ArticulationManager::applyArticulationConfig(
    PxArticulationReducedCoordinate* articulation,
    const ArticulationConfig& config)
{
    if (!articulation) return;

    articulation->setSolverIterationCounts(
        config.solverIterationCounts,
        config.solverVelocityIterationCounts
    );

    articulation->setSleepThreshold(config.sleepThreshold);
    articulation->setStabilizationThreshold(config.stabilizationThreshold);
    articulation->setWakeCounter(config.wakeCounter);
    articulation->setMaxDepenetrationVelocity(config.maxDepenetrationVelocity);

    if (config.fixBase) {
        articulation->setArticulationFlag(
            PxArticulationFlag::eFIX_BASE,
            true
        );
    }

    if (!config.enableSelfCollision) {
        articulation->setArticulationFlag(
            PxArticulationFlag::eDISABLE_SELF_COLLISION,
            true
        );
    }
}

void ArticulationManager::applyJointConfig(
    PxArticulationJointReducedCoordinate* joint,
    const ArticulationJointConfig& config)
{
    if (!joint) return;

    // Set joint type
    joint->setJointType(config.type);

    // Set parent and child poses
    joint->setParentPose(config.parentPose);
    joint->setChildPose(config.childPose);

    // Set motion for each axis
    joint->setMotion(PxArticulationAxis::eX, config.motionX);
    joint->setMotion(PxArticulationAxis::eY, config.motionY);
    joint->setMotion(PxArticulationAxis::eZ, config.motionZ);
    joint->setMotion(PxArticulationAxis::eTWIST, config.motionTwist);
    joint->setMotion(PxArticulationAxis::eSWING1, config.motionSwing1);
    joint->setMotion(PxArticulationAxis::eSWING2, config.motionSwing2);

    // Apply limits if enabled
    if (config.enableLimits) {
        PxArticulationLimit limit(config.limitLow, config.limitHigh);

        // Apply to all non-locked axes
        if (config.motionX == PxArticulationMotion::eLIMITED)
            joint->setLimitParams(PxArticulationAxis::eX, limit);
        if (config.motionY == PxArticulationMotion::eLIMITED)
            joint->setLimitParams(PxArticulationAxis::eY, limit);
        if (config.motionZ == PxArticulationMotion::eLIMITED)
            joint->setLimitParams(PxArticulationAxis::eZ, limit);
        if (config.motionTwist == PxArticulationMotion::eLIMITED)
            joint->setLimitParams(PxArticulationAxis::eTWIST, limit);
        if (config.motionSwing1 == PxArticulationMotion::eLIMITED)
            joint->setLimitParams(PxArticulationAxis::eSWING1, limit);
        if (config.motionSwing2 == PxArticulationMotion::eLIMITED)
            joint->setLimitParams(PxArticulationAxis::eSWING2, limit);
    }

    // Apply drives if enabled
    if (config.enableDrive) {
        PxArticulationDrive drive(
            config.driveStiffness,
            config.driveDamping,
            config.driveMaxForce,
            PxArticulationDriveType::eACCELERATION
        );

        // Apply to all free or limited axes
        if (config.motionX != PxArticulationMotion::eLOCKED) {
            joint->setDriveParams(PxArticulationAxis::eX, drive);
            joint->setDriveTarget(PxArticulationAxis::eX, config.driveTarget);
            joint->setDriveVelocity(PxArticulationAxis::eX, config.driveTargetVelocity);
        }
        if (config.motionY != PxArticulationMotion::eLOCKED) {
            joint->setDriveParams(PxArticulationAxis::eY, drive);
            joint->setDriveTarget(PxArticulationAxis::eY, config.driveTarget);
            joint->setDriveVelocity(PxArticulationAxis::eY, config.driveTargetVelocity);
        }
        if (config.motionZ != PxArticulationMotion::eLOCKED) {
            joint->setDriveParams(PxArticulationAxis::eZ, drive);
            joint->setDriveTarget(PxArticulationAxis::eZ, config.driveTarget);
            joint->setDriveVelocity(PxArticulationAxis::eZ, config.driveTargetVelocity);
        }
        if (config.motionTwist != PxArticulationMotion::eLOCKED) {
            joint->setDriveParams(PxArticulationAxis::eTWIST, drive);
            joint->setDriveTarget(PxArticulationAxis::eTWIST, config.driveTarget);
            joint->setDriveVelocity(PxArticulationAxis::eTWIST, config.driveTargetVelocity);
        }
        if (config.motionSwing1 != PxArticulationMotion::eLOCKED) {
            joint->setDriveParams(PxArticulationAxis::eSWING1, drive);
            joint->setDriveTarget(PxArticulationAxis::eSWING1, config.driveTarget);
            joint->setDriveVelocity(PxArticulationAxis::eSWING1, config.driveTargetVelocity);
        }
        if (config.motionSwing2 != PxArticulationMotion::eLOCKED) {
            joint->setDriveParams(PxArticulationAxis::eSWING2, drive);
            joint->setDriveTarget(PxArticulationAxis::eSWING2, config.driveTarget);
            joint->setDriveVelocity(PxArticulationAxis::eSWING2, config.driveTargetVelocity);
        }
    }

    // Apply friction
    joint->setFrictionCoefficient(config.friction);
}

} // namespace PhysXWrapper
