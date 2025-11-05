/**
 * @file GyroscopicForces.cpp
 * @brief Implementation of GyroscopicForces class
 */

#include "RigidBody/GyroscopicForces.h"
#include <iostream>
#include <cmath>

namespace PhysXWrapper {

// ============================================================================
// GyroscopicStats Implementation
// ============================================================================

void GyroscopicForces::GyroscopicStats::print() const
{
    std::cout << "Gyroscopic Statistics:" << std::endl;
    std::cout << "  Angular velocity: (" << angularVelocity.x << ", "
              << angularVelocity.y << ", " << angularVelocity.z << ")" << std::endl;
    std::cout << "  Angular speed: " << angularSpeed << " rad/s" << std::endl;
    std::cout << "  Moment of inertia: (" << momentOfInertia.x << ", "
              << momentOfInertia.y << ", " << momentOfInertia.z << ")" << std::endl;
    std::cout << "  Kinetic energy: " << kineticEnergy << " J" << std::endl;
    std::cout << "  Flip count: " << flipCount << std::endl;
}

// ============================================================================
// GyroscopicForces::Impl
// ============================================================================

class GyroscopicForces::Impl {
public:
    PxPhysics* m_physics = nullptr;
    PxScene* m_scene = nullptr;
    PxMaterial* m_defaultMaterial = nullptr;

    bool m_initialized = false;
};

// ============================================================================
// Construction/Destruction
// ============================================================================

GyroscopicForces::GyroscopicForces()
    : m_impl(std::make_unique<Impl>())
{
}

GyroscopicForces::~GyroscopicForces()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool GyroscopicForces::initialize(PxPhysics* physics, PxScene* scene)
{
    if (!physics || !scene) {
        std::cerr << "GyroscopicForces::initialize: physics or scene is null" << std::endl;
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_scene = scene;

    // Create default material
    m_impl->m_defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.25f);
    if (!m_impl->m_defaultMaterial) {
        std::cerr << "GyroscopicForces::initialize: Failed to create default material" << std::endl;
        return false;
    }

    m_impl->m_initialized = true;
    return true;
}

void GyroscopicForces::cleanup()
{
    if (m_impl->m_defaultMaterial) {
        m_impl->m_defaultMaterial->release();
        m_impl->m_defaultMaterial = nullptr;
    }

    m_impl->m_initialized = false;
}

bool GyroscopicForces::isInitialized() const
{
    return m_impl->m_initialized;
}

// ============================================================================
// Actor Creation
// ============================================================================

PxRigidDynamic* GyroscopicForces::createTShape(const PxTransform& transform,
                                                const GyroscopicConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "GyroscopicForces::createTShape: Not initialized" << std::endl;
        return nullptr;
    }

    PxRigidDynamic* actor = createActor(transform, config);
    if (!actor) return nullptr;

    // Create T-shape: vertical bar + horizontal bar
    // Vertical bar (stem)
    PxShape* shape0 = m_impl->m_physics->createShape(
        PxBoxGeometry(PxVec3(0.05f, 0.5f, 0.05f)),
        *m_impl->m_defaultMaterial,
        true
    );
    actor->attachShape(*shape0);
    shape0->release();

    // Horizontal bar (top)
    PxShape* shape1 = m_impl->m_physics->createShape(
        PxBoxGeometry(PxVec3(0.1f, 0.05f, 0.05f)),
        *m_impl->m_defaultMaterial,
        true
    );
    shape1->setLocalPose(PxTransform(PxVec3(0.1f, 0.0f, 0.0f)));
    actor->attachShape(*shape1);
    shape1->release();

    configureActor(actor, config);
    m_impl->m_scene->addActor(*actor);

    return actor;
}

PxRigidDynamic* GyroscopicForces::createLShape(const PxTransform& transform,
                                                const GyroscopicConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "GyroscopicForces::createLShape: Not initialized" << std::endl;
        return nullptr;
    }

    PxRigidDynamic* actor = createActor(transform, config);
    if (!actor) return nullptr;

    // Create L-shape: vertical bar + horizontal bar
    // Vertical bar
    PxShape* shape0 = m_impl->m_physics->createShape(
        PxBoxGeometry(PxVec3(0.05f, 0.4f, 0.05f)),
        *m_impl->m_defaultMaterial,
        true
    );
    shape0->setLocalPose(PxTransform(PxVec3(0.0f, -0.2f, 0.0f)));
    actor->attachShape(*shape0);
    shape0->release();

    // Horizontal bar
    PxShape* shape1 = m_impl->m_physics->createShape(
        PxBoxGeometry(PxVec3(0.3f, 0.05f, 0.05f)),
        *m_impl->m_defaultMaterial,
        true
    );
    shape1->setLocalPose(PxTransform(PxVec3(0.3f, 0.15f, 0.0f)));
    actor->attachShape(*shape1);
    shape1->release();

    configureActor(actor, config);
    m_impl->m_scene->addActor(*actor);

    return actor;
}

PxRigidDynamic* GyroscopicForces::createHammer(const PxTransform& transform,
                                                const GyroscopicConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "GyroscopicForces::createHammer: Not initialized" << std::endl;
        return nullptr;
    }

    PxRigidDynamic* actor = createActor(transform, config);
    if (!actor) return nullptr;

    // Handle
    PxShape* handle = m_impl->m_physics->createShape(
        PxCapsuleGeometry(0.03f, 0.4f),
        *m_impl->m_defaultMaterial,
        true
    );
    actor->attachShape(*handle);
    handle->release();

    // Hammer head
    PxShape* head = m_impl->m_physics->createShape(
        PxBoxGeometry(PxVec3(0.15f, 0.1f, 0.08f)),
        *m_impl->m_defaultMaterial,
        true
    );
    head->setLocalPose(PxTransform(PxVec3(0.0f, 0.5f, 0.0f)));
    actor->attachShape(*head);
    head->release();

    configureActor(actor, config);
    m_impl->m_scene->addActor(*actor);

    return actor;
}

PxRigidDynamic* GyroscopicForces::createDumbbell(const PxTransform& transform,
                                                  const GyroscopicConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "GyroscopicForces::createDumbbell: Not initialized" << std::endl;
        return nullptr;
    }

    PxRigidDynamic* actor = createActor(transform, config);
    if (!actor) return nullptr;

    // Bar
    PxShape* bar = m_impl->m_physics->createShape(
        PxCapsuleGeometry(0.02f, 0.3f),
        *m_impl->m_defaultMaterial,
        true
    );
    actor->attachShape(*bar);
    bar->release();

    // End sphere 1
    PxShape* sphere1 = m_impl->m_physics->createShape(
        PxSphereGeometry(0.15f),
        *m_impl->m_defaultMaterial,
        true
    );
    sphere1->setLocalPose(PxTransform(PxVec3(0.0f, 0.45f, 0.0f)));
    actor->attachShape(*sphere1);
    sphere1->release();

    // End sphere 2
    PxShape* sphere2 = m_impl->m_physics->createShape(
        PxSphereGeometry(0.15f),
        *m_impl->m_defaultMaterial,
        true
    );
    sphere2->setLocalPose(PxTransform(PxVec3(0.0f, -0.45f, 0.0f)));
    actor->attachShape(*sphere2);
    sphere2->release();

    configureActor(actor, config);
    m_impl->m_scene->addActor(*actor);

    return actor;
}

PxRigidDynamic* GyroscopicForces::createCross(const PxTransform& transform,
                                               const GyroscopicConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "GyroscopicForces::createCross: Not initialized" << std::endl;
        return nullptr;
    }

    PxRigidDynamic* actor = createActor(transform, config);
    if (!actor) return nullptr;

    // Horizontal bar
    PxShape* hBar = m_impl->m_physics->createShape(
        PxBoxGeometry(PxVec3(0.5f, 0.05f, 0.05f)),
        *m_impl->m_defaultMaterial,
        true
    );
    actor->attachShape(*hBar);
    hBar->release();

    // Vertical bar
    PxShape* vBar = m_impl->m_physics->createShape(
        PxBoxGeometry(PxVec3(0.05f, 0.5f, 0.05f)),
        *m_impl->m_defaultMaterial,
        true
    );
    actor->attachShape(*vBar);
    vBar->release();

    configureActor(actor, config);
    m_impl->m_scene->addActor(*actor);

    return actor;
}

PxRigidDynamic* GyroscopicForces::createTennisRacket(const PxTransform& transform,
                                                      const GyroscopicConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "GyroscopicForces::createTennisRacket: Not initialized" << std::endl;
        return nullptr;
    }

    PxRigidDynamic* actor = createActor(transform, config);
    if (!actor) return nullptr;

    // Handle
    PxShape* handle = m_impl->m_physics->createShape(
        PxCapsuleGeometry(0.02f, 0.3f),
        *m_impl->m_defaultMaterial,
        true
    );
    handle->setLocalPose(PxTransform(PxVec3(0.0f, -0.4f, 0.0f)));
    actor->attachShape(*handle);
    handle->release();

    // Racket head (approximate with boxes)
    const int segments = 8;
    const float radius = 0.15f;
    for (int i = 0; i < segments; i++) {
        float angle = (2.0f * PxPi * i) / segments;
        float x = radius * PxCos(angle);
        float y = radius * PxSin(angle);

        PxShape* segment = m_impl->m_physics->createShape(
            PxBoxGeometry(PxVec3(0.02f, 0.05f, 0.02f)),
            *m_impl->m_defaultMaterial,
            true
        );
        segment->setLocalPose(PxTransform(PxVec3(x, y, 0.0f)));
        actor->attachShape(*segment);
        segment->release();
    }

    configureActor(actor, config);
    m_impl->m_scene->addActor(*actor);

    return actor;
}

PxRigidDynamic* GyroscopicForces::createDemoShape(DemoShape shape,
                                                   const PxTransform& transform,
                                                   const GyroscopicConfig& config)
{
    switch (shape) {
        case DemoShape::T_SHAPE:
            return createTShape(transform, config);
        case DemoShape::L_SHAPE:
            return createLShape(transform, config);
        case DemoShape::HAMMER:
            return createHammer(transform, config);
        case DemoShape::DUMBBELL:
            return createDumbbell(transform, config);
        case DemoShape::CROSS:
            return createCross(transform, config);
        case DemoShape::TENNIS_RACKET:
            return createTennisRacket(transform, config);
        default:
            return createTShape(transform, config);
    }
}

// ============================================================================
// Gyroscopic Control
// ============================================================================

void GyroscopicForces::setGyroscopicEnabled(PxRigidDynamic* actor, bool enable)
{
    if (!actor) return;
    actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_GYROSCOPIC_FORCES, enable);
}

bool GyroscopicForces::isGyroscopicEnabled(PxRigidDynamic* actor)
{
    if (!actor) return false;
    return actor->getRigidBodyFlags() & PxRigidBodyFlag::eENABLE_GYROSCOPIC_FORCES;
}

void GyroscopicForces::setAngularVelocity(PxRigidDynamic* actor, const PxVec3& angularVelocity)
{
    if (!actor) return;
    actor->setAngularVelocity(angularVelocity);
}

void GyroscopicForces::setAngularDamping(PxRigidDynamic* actor, PxReal damping)
{
    if (!actor) return;
    actor->setAngularDamping(damping);
}

// ============================================================================
// Analysis and Statistics
// ============================================================================

GyroscopicForces::GyroscopicStats GyroscopicForces::getStats(PxRigidDynamic* actor)
{
    GyroscopicStats stats;

    if (!actor) return stats;

    stats.angularVelocity = actor->getAngularVelocity();
    stats.angularSpeed = stats.angularVelocity.magnitude();
    stats.orientation = actor->getGlobalPose().q;
    stats.momentOfInertia = computeMomentOfInertia(actor);
    stats.kineticEnergy = computeKineticEnergy(actor);

    return stats;
}

PxVec3 GyroscopicForces::computeMomentOfInertia(PxRigidDynamic* actor)
{
    if (!actor) return PxVec3(0.0f);

    PxVec3 inertia = actor->getMassSpaceInertiaTensor();
    return inertia;
}

PxReal GyroscopicForces::computeKineticEnergy(PxRigidDynamic* actor)
{
    if (!actor) return 0.0f;

    PxVec3 angularVel = actor->getAngularVelocity();
    PxVec3 inertia = computeMomentOfInertia(actor);

    // Kinetic energy = 0.5 * I * omega^2
    PxReal energy = 0.5f * (
        inertia.x * angularVel.x * angularVel.x +
        inertia.y * angularVel.y * angularVel.y +
        inertia.z * angularVel.z * angularVel.z
    );

    return energy;
}

bool GyroscopicForces::isIntermediateAxisRotation(PxRigidDynamic* actor)
{
    if (!actor) return false;

    PxVec3 inertia = computeMomentOfInertia(actor);
    PxVec3 angularVel = actor->getAngularVelocity();

    // Find which axis has intermediate moment of inertia
    float minI = PxMin(inertia.x, PxMin(inertia.y, inertia.z));
    float maxI = PxMax(inertia.x, PxMax(inertia.y, inertia.z));

    int intermediateAxis = -1;
    if (inertia.x > minI && inertia.x < maxI) intermediateAxis = 0;
    else if (inertia.y > minI && inertia.y < maxI) intermediateAxis = 1;
    else if (inertia.z > minI && inertia.z < maxI) intermediateAxis = 2;

    if (intermediateAxis == -1) return false;

    // Check if primary rotation is around intermediate axis
    float angularVelMag = angularVel.magnitude();
    if (angularVelMag < 0.01f) return false;

    PxVec3 normalizedVel = angularVel / angularVelMag;
    float dominantComponent = 0.0f;

    if (intermediateAxis == 0) dominantComponent = PxAbs(normalizedVel.x);
    else if (intermediateAxis == 1) dominantComponent = PxAbs(normalizedVel.y);
    else dominantComponent = PxAbs(normalizedVel.z);

    return dominantComponent > 0.7f; // Threshold for "primarily" rotating around this axis
}

// ============================================================================
// Utility Functions
// ============================================================================

std::pair<PxRigidDynamic*, PxRigidDynamic*> GyroscopicForces::createComparisonPair(
    DemoShape shape,
    const PxVec3& position1,
    const PxVec3& position2,
    const GyroscopicConfig& config)
{
    // Create actor with gyroscopic enabled
    GyroscopicConfig config1 = config;
    config1.enableGyroscopic = true;
    PxRigidDynamic* actor1 = createDemoShape(shape, PxTransform(position1), config1);

    // Create actor with gyroscopic disabled
    GyroscopicConfig config2 = config;
    config2.enableGyroscopic = false;
    PxRigidDynamic* actor2 = createDemoShape(shape, PxTransform(position2), config2);

    return std::make_pair(actor1, actor2);
}

PxPhysics* GyroscopicForces::getPhysics() const
{
    return m_impl->m_physics;
}

PxScene* GyroscopicForces::getScene() const
{
    return m_impl->m_scene;
}

// ============================================================================
// Helper Methods
// ============================================================================

PxRigidDynamic* GyroscopicForces::createActor(const PxTransform& transform,
                                               const GyroscopicConfig& config)
{
    PxRigidDynamic* actor = m_impl->m_physics->createRigidDynamic(transform);
    return actor;
}

void GyroscopicForces::configureActor(PxRigidDynamic* actor, const GyroscopicConfig& config)
{
    if (!actor) return;

    // Update mass and inertia
    PxRigidBodyExt::updateMassAndInertia(*actor, config.density);

    // Set angular velocity
    actor->setAngularVelocity(config.angularVelocity);

    // Set angular damping
    actor->setAngularDamping(config.angularDamping);

    // Enable/disable gyroscopic forces
    actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_GYROSCOPIC_FORCES, config.enableGyroscopic);
}

} // namespace PhysXWrapper
