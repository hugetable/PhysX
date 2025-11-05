/**
 * @file FrustumQuery.h
 * @brief View-frustum culling query system for PhysX
 *
 * This class provides utilities for view-frustum culling:
 * - Extract frustum planes from camera matrices
 * - Perform frustum-based culling queries
 * - Use BVH acceleration for fast culling
 * - Query both custom BVH and PhysX scenes
 *
 * Frustum culling is useful for:
 * - Rendering optimization (don't render invisible objects)
 * - AI visibility checks
 * - Camera-based spatial queries
 * - Level of detail (LOD) systems
 *
 * Based on SnippetFrustumQuery from PhysX SDK.
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
 * @brief Frustum plane indices
 */
enum class FrustumPlane {
    LEFT = 0,
    RIGHT = 1,
    TOP = 2,
    BOTTOM = 3,
    NEAR = 4,
    FAR = 5,
    COUNT = 6
};

/**
 * @brief Camera configuration for frustum extraction
 */
struct CameraConfig {
    PxVec3 position = PxVec3(0, 0, 0);        ///< Camera position
    PxVec3 target = PxVec3(0, 0, -1);         ///< Look-at target
    PxVec3 up = PxVec3(0, 1, 0);              ///< Up vector

    PxReal fov = 60.0f;                        ///< Field of view (degrees)
    PxReal aspectRatio = 16.0f / 9.0f;         ///< Aspect ratio (width/height)
    PxReal nearPlane = 0.1f;                   ///< Near clipping plane
    PxReal farPlane = 1000.0f;                 ///< Far clipping plane
};

/**
 * @brief Frustum definition (6 planes)
 */
struct Frustum {
    PxPlane planes[6];                         ///< 6 frustum planes

    /**
     * @brief Test if point is inside frustum
     */
    bool containsPoint(const PxVec3& point) const;

    /**
     * @brief Test if sphere is inside frustum
     */
    bool containsSphere(const PxVec3& center, PxReal radius) const;

    /**
     * @brief Test if box is inside frustum
     */
    bool containsBox(const PxBounds3& bounds) const;
};

/**
 * @brief Culling result callback
 */
using CullingCallback = std::function<void(PxU32 objectIndex)>;

/**
 * @brief Frustum query result
 */
struct FrustumQueryResult {
    std::vector<PxU32> visibleIndices;         ///< Indices of visible objects
    PxU32 totalObjects = 0;                    ///< Total objects tested
    PxU32 culledObjects = 0;                   ///< Number of culled objects
    PxReal queryTime = 0.0f;                   ///< Query time (milliseconds)
};

/**
 * @brief Object data for custom BVH culling
 */
struct CullableObject {
    PxGeometryHolder geometry;                 ///< Object geometry
    PxTransform transform;                     ///< Object transform
    PxBounds3 bounds;                          ///< Object bounds
    void* userData = nullptr;                  ///< Optional user data
};

/**
 * @brief Frustum query manager class
 *
 * This class provides comprehensive frustum culling functionality:
 * - Extract frustum from camera parameters or matrices
 * - Perform culling queries on custom objects or PhysX scenes
 * - Use BVH acceleration for fast queries
 * - Support various query modes and filters
 *
 * @example
 * @code
 * // Create frustum from camera
 * CameraConfig camera;
 * camera.position = PxVec3(0, 5, 10);
 * camera.target = PxVec3(0, 0, 0);
 * camera.fov = 60.0f;
 *
 * Frustum frustum = FrustumQuery::createFrustum(camera);
 *
 * // Add objects
 * FrustumQuery query;
 * query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(PxVec3(0, 0, 0)));
 * query.addObject(PxSphereGeometry(1), PxTransform(PxVec3(5, 0, 0)));
 *
 * // Build BVH for acceleration
 * query.buildBVH();
 *
 * // Perform culling
 * FrustumQueryResult result = query.cull(frustum);
 * std::cout << result.visibleIndices.size() << " visible objects" << std::endl;
 *
 * // Or use callback
 * query.cullWithCallback(frustum, [](PxU32 index) {
 *     std::cout << "Object " << index << " is visible" << std::endl;
 * });
 * @endcode
 */
class FrustumQuery {
public:
    /**
     * @brief Constructor
     */
    FrustumQuery();

    /**
     * @brief Destructor
     */
    ~FrustumQuery();

    // No copy
    FrustumQuery(const FrustumQuery&) = delete;
    FrustumQuery& operator=(const FrustumQuery&) = delete;

    // Move allowed
    FrustumQuery(FrustumQuery&&) noexcept;
    FrustumQuery& operator=(FrustumQuery&&) noexcept;

    // ========================================================================
    // Frustum Creation
    // ========================================================================

    /**
     * @brief Create frustum from camera configuration
     * @param camera Camera configuration
     * @return Frustum definition
     */
    static Frustum createFrustum(const CameraConfig& camera);

    /**
     * @brief Create frustum from view and projection matrices
     * @param viewMatrix View matrix (4x4)
     * @param projMatrix Projection matrix (4x4)
     * @return Frustum definition
     */
    static Frustum createFrustum(const PxMat44& viewMatrix, const PxMat44& projMatrix);

    /**
     * @brief Create frustum from combined view-projection matrix
     * @param viewProjMatrix Combined view-projection matrix
     * @return Frustum definition
     */
    static Frustum createFrustum(const PxMat44& viewProjMatrix);

    /**
     * @brief Create orthographic frustum
     * @param left Left plane
     * @param right Right plane
     * @param bottom Bottom plane
     * @param top Top plane
     * @param nearPlane Near plane
     * @param farPlane Far plane
     * @return Frustum definition
     */
    static Frustum createOrthographicFrustum(
        PxReal left, PxReal right,
        PxReal bottom, PxReal top,
        PxReal nearPlane, PxReal farPlane);

    // ========================================================================
    // Object Management
    // ========================================================================

    /**
     * @brief Add object for culling
     * @param geometry Object geometry
     * @param transform Object transform
     * @param userData Optional user data
     * @return Object index
     */
    PxU32 addObject(const PxGeometry& geometry, const PxTransform& transform, void* userData = nullptr);

    /**
     * @brief Add object with pre-computed bounds
     * @param bounds Object bounds
     * @param userData Optional user data
     * @return Object index
     */
    PxU32 addObject(const PxBounds3& bounds, void* userData = nullptr);

    /**
     * @brief Remove all objects
     */
    void clearObjects();

    /**
     * @brief Get number of objects
     */
    PxU32 getObjectCount() const;

    /**
     * @brief Get object by index
     */
    const CullableObject* getObject(PxU32 index) const;

    /**
     * @brief Update object transform
     * @param index Object index
     * @param transform New transform
     */
    void updateObjectTransform(PxU32 index, const PxTransform& transform);

    // ========================================================================
    // BVH Management
    // ========================================================================

    /**
     * @brief Build BVH acceleration structure
     * @param enlargement Bounds enlargement factor (0 = no enlargement)
     */
    void buildBVH(PxReal enlargement = 0.0f);

    /**
     * @brief Check if BVH is built
     */
    bool hasBVH() const;

    /**
     * @brief Release BVH
     */
    void releaseBVH();

    /**
     * @brief Rebuild BVH (after object updates)
     */
    void rebuildBVH();

    // ========================================================================
    // Culling Queries
    // ========================================================================

    /**
     * @brief Perform frustum culling query
     * @param frustum Frustum to cull against
     * @return Query result with visible object indices
     */
    FrustumQueryResult cull(const Frustum& frustum) const;

    /**
     * @brief Perform frustum culling with callback
     * @param frustum Frustum to cull against
     * @param callback Called for each visible object
     */
    void cullWithCallback(const Frustum& frustum, CullingCallback callback) const;

    /**
     * @brief Cull PhysX scene actors
     * @param scene PhysX scene
     * @param frustum Frustum to cull against
     * @return Vector of visible actors
     */
    static std::vector<PxRigidActor*> cullScene(PxScene* scene, const Frustum& frustum);

    /**
     * @brief Cull PhysX scene with callback
     * @param scene PhysX scene
     * @param frustum Frustum to cull against
     * @param callback Called for each visible actor
     */
    static void cullSceneWithCallback(
        PxScene* scene,
        const Frustum& frustum,
        std::function<void(PxRigidActor*)> callback);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /**
     * @brief Test if bounds are visible in frustum
     * @param frustum Frustum
     * @param bounds Bounds to test
     * @return True if visible
     */
    static bool isVisible(const Frustum& frustum, const PxBounds3& bounds);

    /**
     * @brief Test if sphere is visible in frustum
     * @param frustum Frustum
     * @param center Sphere center
     * @param radius Sphere radius
     * @return True if visible
     */
    static bool isVisible(const Frustum& frustum, const PxVec3& center, PxReal radius);

    /**
     * @brief Compute object bounds
     * @param geometry Geometry
     * @param transform Transform
     * @return Bounds
     */
    static PxBounds3 computeBounds(const PxGeometry& geometry, const PxTransform& transform);

    /**
     * @brief Get statistics for last query
     * @return Last query result
     */
    const FrustumQueryResult& getLastQueryResult() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    // Helper methods
    static void extractFrustumPlanes(const PxMat44& viewProj, PxPlane* planes);
    static bool testBoundsAgainstPlane(const PxBounds3& bounds, const PxPlane& plane);
};

/**
 * @brief Helper class for camera matrix generation
 */
class CameraMatrixHelper {
public:
    /**
     * @brief Create view matrix (look-at)
     * @param eye Camera position
     * @param target Look-at target
     * @param up Up vector
     * @return View matrix
     */
    static PxMat44 createViewMatrix(const PxVec3& eye, const PxVec3& target, const PxVec3& up);

    /**
     * @brief Create perspective projection matrix
     * @param fovY Field of view Y (radians)
     * @param aspectRatio Aspect ratio (width/height)
     * @param nearPlane Near clipping plane
     * @param farPlane Far clipping plane
     * @return Projection matrix
     */
    static PxMat44 createPerspectiveMatrix(
        PxReal fovY, PxReal aspectRatio,
        PxReal nearPlane, PxReal farPlane);

    /**
     * @brief Create orthographic projection matrix
     * @param left Left plane
     * @param right Right plane
     * @param bottom Bottom plane
     * @param top Top plane
     * @param nearPlane Near plane
     * @param farPlane Far plane
     * @return Projection matrix
     */
    static PxMat44 createOrthographicMatrix(
        PxReal left, PxReal right,
        PxReal bottom, PxReal top,
        PxReal nearPlane, PxReal farPlane);
};

} // namespace PhysXWrapper
