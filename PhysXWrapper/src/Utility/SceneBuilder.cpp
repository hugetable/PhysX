/**
 * @file SceneBuilder.cpp
 * @brief Implementation of SceneBuilder class
 */

#include "Utility/SceneBuilder.h"
#include <cmath>

namespace PhysXWrapper {

SceneBuilder::SceneBuilder(PxPhysics* physics, PxScene* scene, PxMaterial* defaultMaterial)
    : m_physics(physics)
    , m_scene(scene)
    , m_defaultMaterial(defaultMaterial)
    , m_ownsDefaultMaterial(false)
{
    if (!m_defaultMaterial && m_physics) {
        // Create default material if not provided
        m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.5f);
        m_ownsDefaultMaterial = true;
    }
}

SceneBuilder::~SceneBuilder()
{
    // Note: We don't release the material here as it might be shared
    // The PhysX SDK will release it during cleanup
}

// ============================================================================
// Material Creation
// ============================================================================

PxMaterial* SceneBuilder::createMaterial(const MaterialPreset& preset)
{
    if (!m_physics) return nullptr;
    return m_physics->createMaterial(
        preset.staticFriction,
        preset.dynamicFriction,
        preset.restitution
    );
}

PxMaterial* SceneBuilder::getDefaultMaterial()
{
    return m_defaultMaterial;
}

// ============================================================================
// Basic Shape Creation (Dynamic)
// ============================================================================

PxRigidDynamic* SceneBuilder::createDynamicBox(
    const PxVec3& position,
    const PxVec3& halfExtents,
    PxReal density,
    PxMaterial* material,
    const PxVec3& initialVelocity)
{
    if (!m_physics || !m_scene) return nullptr;
    if (!material) material = m_defaultMaterial;
    if (!material) return nullptr;

    PxRigidDynamic* actor = m_physics->createRigidDynamic(PxTransform(position));
    if (!actor) return nullptr;

    PxShape* shape = m_physics->createShape(PxBoxGeometry(halfExtents), *material);
    if (!shape) {
        actor->release();
        return nullptr;
    }

    actor->attachShape(*shape);
    shape->release();

    PxRigidBodyExt::updateMassAndInertia(*actor, density);
    actor->setLinearVelocity(initialVelocity);

    m_scene->addActor(*actor);
    return actor;
}

PxRigidDynamic* SceneBuilder::createDynamicSphere(
    const PxVec3& position,
    PxReal radius,
    PxReal density,
    PxMaterial* material,
    const PxVec3& initialVelocity)
{
    if (!m_physics || !m_scene) return nullptr;
    if (!material) material = m_defaultMaterial;
    if (!material) return nullptr;

    PxRigidDynamic* actor = m_physics->createRigidDynamic(PxTransform(position));
    if (!actor) return nullptr;

    PxShape* shape = m_physics->createShape(PxSphereGeometry(radius), *material);
    if (!shape) {
        actor->release();
        return nullptr;
    }

    actor->attachShape(*shape);
    shape->release();

    PxRigidBodyExt::updateMassAndInertia(*actor, density);
    actor->setLinearVelocity(initialVelocity);

    m_scene->addActor(*actor);
    return actor;
}

PxRigidDynamic* SceneBuilder::createDynamicCapsule(
    const PxVec3& position,
    PxReal radius,
    PxReal halfHeight,
    PxReal density,
    PxMaterial* material,
    const PxVec3& initialVelocity)
{
    if (!m_physics || !m_scene) return nullptr;
    if (!material) material = m_defaultMaterial;
    if (!material) return nullptr;

    PxRigidDynamic* actor = m_physics->createRigidDynamic(PxTransform(position));
    if (!actor) return nullptr;

    PxShape* shape = m_physics->createShape(PxCapsuleGeometry(radius, halfHeight), *material);
    if (!shape) {
        actor->release();
        return nullptr;
    }

    actor->attachShape(*shape);
    shape->release();

    PxRigidBodyExt::updateMassAndInertia(*actor, density);
    actor->setLinearVelocity(initialVelocity);

    m_scene->addActor(*actor);
    return actor;
}

// ============================================================================
// Basic Shape Creation (Static)
// ============================================================================

PxRigidStatic* SceneBuilder::createStaticBox(
    const PxVec3& position,
    const PxVec3& halfExtents,
    PxMaterial* material)
{
    if (!m_physics || !m_scene) return nullptr;
    if (!material) material = m_defaultMaterial;
    if (!material) return nullptr;

    PxRigidStatic* actor = m_physics->createRigidStatic(PxTransform(position));
    if (!actor) return nullptr;

    PxShape* shape = m_physics->createShape(PxBoxGeometry(halfExtents), *material);
    if (!shape) {
        actor->release();
        return nullptr;
    }

    actor->attachShape(*shape);
    shape->release();

    m_scene->addActor(*actor);
    return actor;
}

PxRigidStatic* SceneBuilder::createStaticSphere(
    const PxVec3& position,
    PxReal radius,
    PxMaterial* material)
{
    if (!m_physics || !m_scene) return nullptr;
    if (!material) material = m_defaultMaterial;
    if (!material) return nullptr;

    PxRigidStatic* actor = m_physics->createRigidStatic(PxTransform(position));
    if (!actor) return nullptr;

    PxShape* shape = m_physics->createShape(PxSphereGeometry(radius), *material);
    if (!shape) {
        actor->release();
        return nullptr;
    }

    actor->attachShape(*shape);
    shape->release();

    m_scene->addActor(*actor);
    return actor;
}

PxRigidStatic* SceneBuilder::createStaticCapsule(
    const PxVec3& position,
    PxReal radius,
    PxReal halfHeight,
    PxMaterial* material)
{
    if (!m_physics || !m_scene) return nullptr;
    if (!material) material = m_defaultMaterial;
    if (!material) return nullptr;

    PxRigidStatic* actor = m_physics->createRigidStatic(PxTransform(position));
    if (!actor) return nullptr;

    PxShape* shape = m_physics->createShape(PxCapsuleGeometry(radius, halfHeight), *material);
    if (!shape) {
        actor->release();
        return nullptr;
    }

    actor->attachShape(*shape);
    shape->release();

    m_scene->addActor(*actor);
    return actor;
}

// ============================================================================
// Scene Elements
// ============================================================================

PxRigidStatic* SceneBuilder::createGround(
    const PxVec3& normal,
    PxReal distance,
    PxMaterial* material)
{
    if (!m_physics || !m_scene) return nullptr;
    if (!material) material = m_defaultMaterial;
    if (!material) return nullptr;

    PxRigidStatic* ground = PxCreatePlane(*m_physics, PxPlane(normal, distance), *material);
    if (ground) {
        m_scene->addActor(*ground);
    }
    return ground;
}

std::vector<PxRigidDynamic*> SceneBuilder::createBoxStack(
    const PxVec3& basePosition,
    PxU32 size,
    PxReal boxHalfExtent,
    PxReal density,
    PxMaterial* material)
{
    std::vector<PxRigidDynamic*> actors;
    if (!m_physics || !m_scene) return actors;

    for (PxU32 i = 0; i < size; i++) {
        for (PxU32 j = 0; j < size - i; j++) {
            PxVec3 position = basePosition + PxVec3(
                PxReal(j * 2) - PxReal(size - i),
                PxReal(i * 2 + 1),
                0
            ) * boxHalfExtent;

            PxRigidDynamic* box = createDynamicBox(
                position,
                PxVec3(boxHalfExtent),
                density,
                material
            );

            if (box) {
                actors.push_back(box);
            }
        }
    }

    return actors;
}

std::vector<PxRigidDynamic*> SceneBuilder::createBoxWall(
    const PxVec3& basePosition,
    PxU32 width,
    PxU32 height,
    PxReal boxHalfExtent,
    PxReal density,
    PxMaterial* material)
{
    std::vector<PxRigidDynamic*> actors;
    if (!m_physics || !m_scene) return actors;

    for (PxU32 i = 0; i < height; i++) {
        for (PxU32 j = 0; j < width; j++) {
            PxVec3 position = basePosition + PxVec3(
                PxReal(j) * boxHalfExtent * 2.0f,
                PxReal(i) * boxHalfExtent * 2.0f + boxHalfExtent,
                0
            );

            PxRigidDynamic* box = createDynamicBox(
                position,
                PxVec3(boxHalfExtent),
                density,
                material
            );

            if (box) {
                actors.push_back(box);
            }
        }
    }

    return actors;
}

std::vector<PxRigidStatic*> SceneBuilder::createObstacles(
    const PxVec3& startPosition,
    PxU32 count,
    PxReal spacing,
    const PxVec3& boxHalfExtents,
    PxMaterial* material)
{
    std::vector<PxRigidStatic*> actors;
    if (!m_physics || !m_scene) return actors;

    for (PxU32 i = 0; i < count; i++) {
        PxVec3 position = startPosition + PxVec3(i * spacing, boxHalfExtents.y, 0);

        PxRigidStatic* box = createStaticBox(
            position,
            boxHalfExtents,
            material
        );

        if (box) {
            actors.push_back(box);
        }
    }

    return actors;
}

std::vector<PxRigidStatic*> SceneBuilder::createStairs(
    const PxVec3& startPosition,
    PxU32 stepCount,
    PxReal stepWidth,
    PxReal stepHeight,
    PxReal stepDepth,
    PxMaterial* material)
{
    std::vector<PxRigidStatic*> actors;
    if (!m_physics || !m_scene) return actors;

    for (PxU32 i = 0; i < stepCount; i++) {
        PxReal cumulativeHeight = stepHeight * (i + 1);
        PxVec3 position = startPosition + PxVec3(
            i * stepDepth,
            cumulativeHeight * 0.5f,
            0
        );

        PxVec3 halfExtents(stepDepth * 0.5f, cumulativeHeight * 0.5f, stepWidth * 0.5f);

        PxRigidStatic* step = createStaticBox(
            position,
            halfExtents,
            material
        );

        if (step) {
            actors.push_back(step);
        }
    }

    return actors;
}

PxRigidStatic* SceneBuilder::createSlope(
    const PxVec3& position,
    PxReal slopeLength,
    PxReal slopeWidth,
    PxReal slopeAngleDegrees,
    PxMaterial* material)
{
    if (!m_physics || !m_scene) return nullptr;
    if (!material) material = m_defaultMaterial;
    if (!material) return nullptr;

    // Convert angle to radians
    PxReal angleRad = slopeAngleDegrees * PxPi / 180.0f;

    // Create rotation quaternion around Z axis
    PxQuat rotation(angleRad, PxVec3(0, 0, 1));

    // Create transform
    PxTransform transform(position, rotation);

    // Create static actor with box shape
    PxRigidStatic* actor = m_physics->createRigidStatic(transform);
    if (!actor) return nullptr;

    PxVec3 halfExtents(slopeLength * 0.5f, 0.1f, slopeWidth * 0.5f);
    PxShape* shape = m_physics->createShape(PxBoxGeometry(halfExtents), *material);
    if (!shape) {
        actor->release();
        return nullptr;
    }

    actor->attachShape(*shape);
    shape->release();

    m_scene->addActor(*actor);
    return actor;
}

} // namespace PhysXWrapper
