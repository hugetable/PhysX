/**
 * @file SceneBuilder.h
 * @brief High-level scene building utilities for PhysX
 *
 * This class provides convenient helper functions to create common scene elements,
 * reducing boilerplate code in examples and applications.
 *
 * @author PhysXWrapper
 * @date 2025-11-07
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Material presets for common use cases
 */
struct MaterialPreset {
    float staticFriction;
    float dynamicFriction;
    float restitution;

    // Common presets
    static MaterialPreset Default()  { return {0.5f, 0.5f, 0.5f}; }
    static MaterialPreset Bouncy()   { return {0.3f, 0.3f, 0.9f}; }
    static MaterialPreset Slippery() { return {0.05f, 0.05f, 0.3f}; }
    static MaterialPreset Sticky()   { return {0.9f, 0.9f, 0.1f}; }
    static MaterialPreset Ice()      { return {0.02f, 0.01f, 0.2f}; }
    static MaterialPreset Rubber()   { return {0.8f, 0.7f, 0.85f}; }
    static MaterialPreset Metal()    { return {0.4f, 0.3f, 0.4f}; }
    static MaterialPreset Wood()     { return {0.6f, 0.5f, 0.3f}; }
};

/**
 * @brief SceneBuilder - High-level helper class for building physics scenes
 *
 * This class provides convenient functions to create common scene elements like:
 * - Basic shapes (boxes, spheres, capsules)
 * - Scene elements (ground, obstacles, stairs, slopes, stacks)
 * - Material presets
 *
 * @example
 * @code
 * SceneBuilder builder(physics, scene);
 *
 * // Create ground
 * builder.createGround();
 *
 * // Create a box stack
 * builder.createBoxStack(PxVec3(0, 0, 0), 5, 0.5f);
 *
 * // Create obstacles
 * builder.createObstacles(PxVec3(10, 0, 0), 10);
 * @endcode
 */
class SceneBuilder {
public:
    /**
     * @brief Constructor
     * @param physics Pointer to PxPhysics instance
     * @param scene Pointer to PxScene instance
     * @param defaultMaterial Default material to use (optional)
     */
    SceneBuilder(PxPhysics* physics, PxScene* scene, PxMaterial* defaultMaterial = nullptr);

    /**
     * @brief Destructor
     */
    ~SceneBuilder();

    // ============================================================================
    // Material Creation
    // ============================================================================

    /**
     * @brief Create material from preset
     * @param preset Material preset
     * @return Created material
     */
    PxMaterial* createMaterial(const MaterialPreset& preset);

    /**
     * @brief Get or create default material
     * @return Default material
     */
    PxMaterial* getDefaultMaterial();

    // ============================================================================
    // Basic Shape Creation (Dynamic)
    // ============================================================================

    /**
     * @brief Create a dynamic box
     * @param position Position
     * @param halfExtents Half extents (size/2)
     * @param density Density in kg/m^3 (default: 10.0)
     * @param material Material (default: default material)
     * @param initialVelocity Initial velocity (default: zero)
     * @return Created box actor
     */
    PxRigidDynamic* createDynamicBox(
        const PxVec3& position,
        const PxVec3& halfExtents,
        PxReal density = 10.0f,
        PxMaterial* material = nullptr,
        const PxVec3& initialVelocity = PxVec3(0)
    );

    /**
     * @brief Create a dynamic sphere
     * @param position Position
     * @param radius Radius
     * @param density Density in kg/m^3 (default: 10.0)
     * @param material Material (default: default material)
     * @param initialVelocity Initial velocity (default: zero)
     * @return Created sphere actor
     */
    PxRigidDynamic* createDynamicSphere(
        const PxVec3& position,
        PxReal radius,
        PxReal density = 10.0f,
        PxMaterial* material = nullptr,
        const PxVec3& initialVelocity = PxVec3(0)
    );

    /**
     * @brief Create a dynamic capsule
     * @param position Position
     * @param radius Radius
     * @param halfHeight Half height
     * @param density Density in kg/m^3 (default: 10.0)
     * @param material Material (default: default material)
     * @param initialVelocity Initial velocity (default: zero)
     * @return Created capsule actor
     */
    PxRigidDynamic* createDynamicCapsule(
        const PxVec3& position,
        PxReal radius,
        PxReal halfHeight,
        PxReal density = 10.0f,
        PxMaterial* material = nullptr,
        const PxVec3& initialVelocity = PxVec3(0)
    );

    // ============================================================================
    // Basic Shape Creation (Static)
    // ============================================================================

    /**
     * @brief Create a static box
     * @param position Position
     * @param halfExtents Half extents (size/2)
     * @param material Material (default: default material)
     * @return Created box actor
     */
    PxRigidStatic* createStaticBox(
        const PxVec3& position,
        const PxVec3& halfExtents,
        PxMaterial* material = nullptr
    );

    /**
     * @brief Create a static sphere
     * @param position Position
     * @param radius Radius
     * @param material Material (default: default material)
     * @return Created sphere actor
     */
    PxRigidStatic* createStaticSphere(
        const PxVec3& position,
        PxReal radius,
        PxMaterial* material = nullptr
    );

    /**
     * @brief Create a static capsule
     * @param position Position
     * @param radius Radius
     * @param halfHeight Half height
     * @param material Material (default: default material)
     * @return Created capsule actor
     */
    PxRigidStatic* createStaticCapsule(
        const PxVec3& position,
        PxReal radius,
        PxReal halfHeight,
        PxMaterial* material = nullptr
    );

    // ============================================================================
    // Scene Elements
    // ============================================================================

    /**
     * @brief Create a ground plane
     * @param normal Plane normal (default: up)
     * @param distance Distance from origin (default: 0)
     * @param material Material (default: default material)
     * @return Created ground plane
     */
    PxRigidStatic* createGround(
        const PxVec3& normal = PxVec3(0, 1, 0),
        PxReal distance = 0.0f,
        PxMaterial* material = nullptr
    );

    /**
     * @brief Create a stack of boxes (pyramid or tower)
     * @param basePosition Base position
     * @param size Number of boxes in base layer
     * @param boxHalfExtent Half extent of each box
     * @param density Density of boxes
     * @param material Material (default: default material)
     * @return Vector of created actors
     */
    std::vector<PxRigidDynamic*> createBoxStack(
        const PxVec3& basePosition,
        PxU32 size,
        PxReal boxHalfExtent = 0.5f,
        PxReal density = 10.0f,
        PxMaterial* material = nullptr
    );

    /**
     * @brief Create a wall of boxes
     * @param basePosition Base position
     * @param width Width in boxes
     * @param height Height in boxes
     * @param boxHalfExtent Half extent of each box
     * @param density Density of boxes
     * @param material Material (default: default material)
     * @return Vector of created actors
     */
    std::vector<PxRigidDynamic*> createBoxWall(
        const PxVec3& basePosition,
        PxU32 width,
        PxU32 height,
        PxReal boxHalfExtent = 0.5f,
        PxReal density = 10.0f,
        PxMaterial* material = nullptr
    );

    /**
     * @brief Create obstacles (static boxes)
     * @param startPosition Starting position
     * @param count Number of obstacles
     * @param spacing Spacing between obstacles
     * @param boxHalfExtents Half extents of each box
     * @param material Material (default: default material)
     * @return Vector of created actors
     */
    std::vector<PxRigidStatic*> createObstacles(
        const PxVec3& startPosition,
        PxU32 count,
        PxReal spacing = 4.0f,
        const PxVec3& boxHalfExtents = PxVec3(1, 1, 1),
        PxMaterial* material = nullptr
    );

    /**
     * @brief Create stairs
     * @param startPosition Starting position
     * @param stepCount Number of steps
     * @param stepWidth Step width
     * @param stepHeight Step height
     * @param stepDepth Step depth
     * @param material Material (default: default material)
     * @return Vector of created actors
     */
    std::vector<PxRigidStatic*> createStairs(
        const PxVec3& startPosition,
        PxU32 stepCount,
        PxReal stepWidth = 2.0f,
        PxReal stepHeight = 0.25f,
        PxReal stepDepth = 0.5f,
        PxMaterial* material = nullptr
    );

    /**
     * @brief Create a slope (ramp)
     * @param position Center position
     * @param slopeLength Length of slope
     * @param slopeWidth Width of slope
     * @param slopeAngleDegrees Angle in degrees (default: 30)
     * @param material Material (default: default material)
     * @return Created slope actor
     */
    PxRigidStatic* createSlope(
        const PxVec3& position,
        PxReal slopeLength,
        PxReal slopeWidth,
        PxReal slopeAngleDegrees = 30.0f,
        PxMaterial* material = nullptr
    );

    // ============================================================================
    // Utility Functions
    // ============================================================================

    /**
     * @brief Get physics instance
     * @return PxPhysics pointer
     */
    PxPhysics* getPhysics() const { return m_physics; }

    /**
     * @brief Get scene instance
     * @return PxScene pointer
     */
    PxScene* getScene() const { return m_scene; }

private:
    PxPhysics* m_physics;
    PxScene* m_scene;
    PxMaterial* m_defaultMaterial;
    bool m_ownsDefaultMaterial;
};

} // namespace PhysXWrapper
