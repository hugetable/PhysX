/**
 * @file RigidBodyCCD.cpp
 * @brief Implementation of RigidBodyCCD class
 */

#include "RigidBody/RigidBodyCCD.h"

namespace PhysXWrapper {

// CCD filter shader implementation
static PxFilterFlags ccdFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    PX_UNUSED(attributes0);
    PX_UNUSED(filterData0);
    PX_UNUSED(attributes1);
    PX_UNUSED(filterData1);
    PX_UNUSED(constantBlock);
    PX_UNUSED(constantBlockSize);

    // Enable contact solving, discrete and CCD contact detection
    pairFlags = PxPairFlag::eSOLVE_CONTACT |
                PxPairFlag::eDETECT_DISCRETE_CONTACT |
                PxPairFlag::eDETECT_CCD_CONTACT;

    return PxFilterFlags();
}

/**
 * @brief Private implementation
 */
class RigidBodyCCD::Impl {
public:
    Impl() : m_config() {}

    CCDConfig m_config;
};

// RigidBodyCCD implementation
RigidBodyCCD::RigidBodyCCD()
    : m_impl(std::make_unique<Impl>())
{
}

RigidBodyCCD::~RigidBodyCCD() = default;

PxSceneDesc RigidBodyCCD::createSceneDesc(
    const PxTolerancesScale& scale,
    const CCDConfig& config,
    const PxVec3& gravity)
{
    m_impl->m_config = config;

    PxSceneDesc sceneDesc(scale);
    sceneDesc.gravity = gravity;

    // Configure based on CCD algorithm
    if (config.algorithm == CCDAlgorithm::LINEAR ||
        config.algorithm == CCDAlgorithm::FULL) {
        // Enable linear CCD
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        sceneDesc.filterShader = ccdFilterShader;
    }
    else if (config.algorithm == CCDAlgorithm::SPECULATIVE) {
        // Speculative CCD doesn't need scene flag
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    }
    else if (config.algorithm == CCDAlgorithm::RAYCAST) {
        // Raycast CCD uses default settings, managed externally
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    }
    else {
        // No CCD
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    }

    // Set CCD parameters
    sceneDesc.ccdMaxPasses = config.maxCCDPasses;
    sceneDesc.ccdThreshold = config.ccdThreshold;
    sceneDesc.maxBiasCoefficient = config.maxBiasCoefficient;

    return sceneDesc;
}

bool RigidBodyCCD::enableCCD(
    PxRigidDynamic* actor,
    const CCDConfig& config)
{
    if (!actor) return false;

    m_impl->m_config = config;

    // Enable CCD flags based on algorithm
    if (config.algorithm == CCDAlgorithm::LINEAR ||
        config.algorithm == CCDAlgorithm::FULL) {
        actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    }

    if (config.algorithm == CCDAlgorithm::SPECULATIVE ||
        config.algorithm == CCDAlgorithm::FULL) {
        actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, true);
    }

    // Note: Raycast CCD requires external manager registration
    // This is handled by extensions/PxRaycastCCD.h

    return true;
}

bool RigidBodyCCD::disableCCD(PxRigidDynamic* actor)
{
    if (!actor) return false;

    actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, false);
    actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, false);

    return true;
}

bool RigidBodyCCD::isCCDEnabled(PxRigidDynamic* actor) const
{
    if (!actor) return false;

    PxRigidBodyFlags flags = actor->getRigidBodyFlags();
    return (flags & PxRigidBodyFlag::eENABLE_CCD) ||
           (flags & PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD);
}

void RigidBodyCCD::setShapeCCDThreshold(PxShape* shape, PxReal threshold)
{
    if (shape) {
        shape->setRestOffset(threshold);
    }
}

PxReal RigidBodyCCD::getShapeCCDThreshold(PxShape* shape) const
{
    if (shape) {
        return shape->getRestOffset();
    }
    return 0.0f;
}

PxU32 RigidBodyCCD::enableCCDForAllDynamics(
    PxScene* scene,
    const CCDConfig& config)
{
    if (!scene) return 0;

    PxU32 count = 0;
    PxActorTypeFlags flags = PxActorTypeFlag::eRIGID_DYNAMIC;

    PxU32 numActors = scene->getNbActors(flags);
    if (numActors == 0) return 0;

    PxActor** actors = new PxActor*[numActors];
    scene->getActors(flags, actors, numActors);

    for (PxU32 i = 0; i < numActors; i++) {
        PxRigidDynamic* dynamic = actors[i]->is<PxRigidDynamic>();
        if (dynamic) {
            if (enableCCD(dynamic, config)) {
                count++;
            }
        }
    }

    delete[] actors;
    return count;
}

PxRigidDynamic* RigidBodyCCD::createFastMovingDynamic(
    PxPhysics* physics,
    PxScene* scene,
    const PxTransform& transform,
    const PxGeometry& geometry,
    PxMaterial* material,
    const PxVec3& velocity,
    PxReal density,
    const CCDConfig& config)
{
    if (!physics || !scene || !material) return nullptr;

    // Create dynamic actor
    PxRigidDynamic* actor = physics->createRigidDynamic(transform);
    if (!actor) return nullptr;

    // Create and attach shape
    PxShape* shape = physics->createShape(geometry, *material);
    if (!shape) {
        actor->release();
        return nullptr;
    }

    actor->attachShape(*shape);
    shape->release();

    // Set mass and inertia
    PxRigidBodyExt::updateMassAndInertia(*actor, density);

    // Set velocity
    actor->setLinearVelocity(velocity);

    // Enable CCD
    enableCCD(actor, config);

    // Add to scene
    scene->addActor(*actor);

    return actor;
}

CCDStats RigidBodyCCD::getCCDStats(PxScene* scene) const
{
    CCDStats stats;

    if (!scene) return stats;

    // Get simulation statistics
    PxSimulationStatistics simStats;
    scene->getSimulationStatistics(simStats);

    // Note: PhysX doesn't directly expose CCD-specific statistics
    // These would need to be tracked separately in a real implementation
    stats.ccdPairs = 0;  // Would need custom tracking
    stats.ccdContacts = 0;  // Would need custom tracking
    stats.ccdTime = 0.0f;  // Would need custom timing

    return stats;
}

PxSimulationFilterShader RigidBodyCCD::getCCDFilterShader()
{
    return ccdFilterShader;
}

PxReal RigidBodyCCD::calculateCCDThreshold(
    const PxVec3& velocity,
    PxReal size,
    PxReal timeStep)
{
    // Calculate distance object will travel in one time step
    PxReal distance = velocity.magnitude() * timeStep;

    // Threshold should be smaller than the object size
    // to detect potential tunneling
    PxReal threshold = PxMin(distance * 0.5f, size * 0.5f);

    return threshold;
}

const CCDConfig& RigidBodyCCD::getConfig() const
{
    return m_impl->m_config;
}

void RigidBodyCCD::setConfig(const CCDConfig& config)
{
    m_impl->m_config = config;
}

} // namespace PhysXWrapper
