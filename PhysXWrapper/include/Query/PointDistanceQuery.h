/**
 * @file PointDistanceQuery.h
 * @brief Point-to-geometry distance query system for PhysX
 *
 * This class provides utilities for computing closest point distances:
 * - Find closest point on a geometry from a query point
 * - Compute distance from point to geometry
 * - Query PhysX scenes for nearest actors
 * - Batch query multiple points
 * - Support all geometry types (box, sphere, capsule, convex, mesh)
 *
 * Point distance queries are useful for:
 * - AI pathfinding (finding nearest obstacles)
 * - Collision prediction
 * - Proximity detection
 * - Surface snapping
 * - Distance field generation
 *
 * Based on SnippetPointDistanceQuery from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>
#include <memory>
#include <functional>
#include <limits>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Result of a point distance query
 */
struct PointDistanceResult {
    PxVec3 queryPoint;                              ///< Original query point
    PxVec3 closestPoint;                            ///< Closest point on geometry
    PxReal distance = PX_MAX_F32;                   ///< Distance to closest point
    PxReal distanceSquared = PX_MAX_F32;            ///< Squared distance (for performance)
    bool success = false;                           ///< Query succeeded
};

/**
 * @brief Result of scene actor query
 */
struct NearestActorResult {
    PxRigidActor* actor = nullptr;                  ///< Nearest actor
    PxShape* shape = nullptr;                       ///< Nearest shape on actor
    PxVec3 queryPoint;                              ///< Original query point
    PxVec3 closestPoint;                            ///< Closest point on actor
    PxReal distance = PX_MAX_F32;                   ///< Distance to actor
    bool success = false;                           ///< Query succeeded
};

/**
 * @brief Configuration for point distance queries
 */
struct PointDistanceConfig {
    PxReal maxDistance = PX_MAX_F32;                ///< Maximum search distance
    bool computeClosestPoint = true;                ///< Compute closest point (vs just distance)
    bool usePreciseDistance = false;                ///< Use precise distance (vs squared for performance)
};

/**
 * @brief Batch query result
 */
struct BatchQueryResult {
    std::vector<PointDistanceResult> results;       ///< Individual results
    PxU32 successCount = 0;                         ///< Number of successful queries
    PxReal averageDistance = 0.0f;                  ///< Average distance
    PxReal minDistance = PX_MAX_F32;                ///< Minimum distance found
    PxReal maxDistance = 0.0f;                      ///< Maximum distance found
};

/**
 * @brief Callback for batch queries
 */
using PointDistanceCallback = std::function<void(const PointDistanceResult&)>;

/**
 * @brief Point distance query manager class
 *
 * This class provides comprehensive point-to-geometry distance queries:
 * - Query single points against geometries
 * - Query multiple points in batch
 * - Find nearest actors in PhysX scenes
 * - Support all geometry types
 * - Optional distance thresholds
 * - Performance optimized (uses squared distances when possible)
 *
 * @example
 * @code
 * // Query point to box
 * PxBoxGeometry box(1, 1, 1);
 * PxTransform pose(PxVec3(5, 0, 0));
 * PxVec3 queryPoint(0, 0, 0);
 *
 * PointDistanceResult result = PointDistanceQuery::queryGeometry(
 *     queryPoint, box, pose);
 *
 * std::cout << "Distance: " << result.distance << std::endl;
 * std::cout << "Closest point: (" << result.closestPoint.x << ", "
 *           << result.closestPoint.y << ", " << result.closestPoint.z << ")" << std::endl;
 *
 * // Find nearest actor in scene
 * NearestActorResult nearestActor = PointDistanceQuery::findNearestActor(
 *     scene, queryPoint);
 *
 * if (nearestActor.success) {
 *     std::cout << "Nearest actor at distance: " << nearestActor.distance << std::endl;
 * }
 * @endcode
 */
class PointDistanceQuery {
public:
    // ========================================================================
    // Single Point Queries
    // ========================================================================

    /**
     * @brief Query distance from point to geometry
     * @param point Query point
     * @param geometry Target geometry
     * @param pose Geometry pose
     * @param config Query configuration
     * @return Query result
     */
    static PointDistanceResult queryGeometry(
        const PxVec3& point,
        const PxGeometry& geometry,
        const PxTransform& pose,
        const PointDistanceConfig& config = PointDistanceConfig());

    /**
     * @brief Query distance from point to actor
     * @param point Query point
     * @param actor Target actor
     * @param config Query configuration
     * @return Query result (minimum distance to any shape on actor)
     */
    static PointDistanceResult queryActor(
        const PxVec3& point,
        PxRigidActor* actor,
        const PointDistanceConfig& config = PointDistanceConfig());

    /**
     * @brief Query distance from point to specific shape
     * @param point Query point
     * @param shape Target shape
     * @param config Query configuration
     * @return Query result
     */
    static PointDistanceResult queryShape(
        const PxVec3& point,
        PxShape* shape,
        const PointDistanceConfig& config = PointDistanceConfig());

    // ========================================================================
    // Batch Queries
    // ========================================================================

    /**
     * @brief Query multiple points against same geometry
     * @param points Query points
     * @param geometry Target geometry
     * @param pose Geometry pose
     * @param config Query configuration
     * @return Batch query result
     */
    static BatchQueryResult queryGeometryBatch(
        const std::vector<PxVec3>& points,
        const PxGeometry& geometry,
        const PxTransform& pose,
        const PointDistanceConfig& config = PointDistanceConfig());

    /**
     * @brief Query multiple points with callback
     * @param points Query points
     * @param geometry Target geometry
     * @param pose Geometry pose
     * @param callback Called for each result
     * @param config Query configuration
     */
    static void queryGeometryBatchWithCallback(
        const std::vector<PxVec3>& points,
        const PxGeometry& geometry,
        const PxTransform& pose,
        PointDistanceCallback callback,
        const PointDistanceConfig& config = PointDistanceConfig());

    // ========================================================================
    // Scene Queries
    // ========================================================================

    /**
     * @brief Find nearest actor to point in scene
     * @param scene PhysX scene
     * @param point Query point
     * @param config Query configuration
     * @return Nearest actor result
     */
    static NearestActorResult findNearestActor(
        PxScene* scene,
        const PxVec3& point,
        const PointDistanceConfig& config = PointDistanceConfig());

    /**
     * @brief Find all actors within distance threshold
     * @param scene PhysX scene
     * @param point Query point
     * @param maxDistance Maximum distance threshold
     * @return Vector of actors and distances within threshold
     */
    static std::vector<NearestActorResult> findActorsWithinDistance(
        PxScene* scene,
        const PxVec3& point,
        PxReal maxDistance);

    /**
     * @brief Find k nearest actors to point
     * @param scene PhysX scene
     * @param point Query point
     * @param k Number of nearest actors to find
     * @return Vector of k nearest actors (sorted by distance)
     */
    static std::vector<NearestActorResult> findKNearestActors(
        PxScene* scene,
        const PxVec3& point,
        PxU32 k);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /**
     * @brief Check if point is inside geometry
     * @param point Query point
     * @param geometry Target geometry
     * @param pose Geometry pose
     * @return True if point is inside (distance <= 0)
     */
    static bool isPointInside(
        const PxVec3& point,
        const PxGeometry& geometry,
        const PxTransform& pose);

    /**
     * @brief Compute closest point on geometry from point
     * @param point Query point
     * @param geometry Target geometry
     * @param pose Geometry pose
     * @return Closest point on geometry surface
     */
    static PxVec3 computeClosestPoint(
        const PxVec3& point,
        const PxGeometry& geometry,
        const PxTransform& pose);

    /**
     * @brief Compute distance from point to geometry
     * @param point Query point
     * @param geometry Target geometry
     * @param pose Geometry pose
     * @return Distance (or negative if inside)
     */
    static PxReal computeDistance(
        const PxVec3& point,
        const PxGeometry& geometry,
        const PxTransform& pose);

    /**
     * @brief Compute squared distance (faster than distance)
     * @param point Query point
     * @param geometry Target geometry
     * @param pose Geometry pose
     * @return Squared distance
     */
    static PxReal computeDistanceSquared(
        const PxVec3& point,
        const PxGeometry& geometry,
        const PxTransform& pose);

    // ========================================================================
    // Geometric Primitives
    // ========================================================================

    /**
     * @brief Distance from point to sphere
     * @param point Query point
     * @param center Sphere center
     * @param radius Sphere radius
     * @return Distance (negative if inside)
     */
    static PxReal distanceToSphere(
        const PxVec3& point,
        const PxVec3& center,
        PxReal radius);

    /**
     * @brief Closest point on sphere from point
     * @param point Query point
     * @param center Sphere center
     * @param radius Sphere radius
     * @return Closest point on sphere
     */
    static PxVec3 closestPointOnSphere(
        const PxVec3& point,
        const PxVec3& center,
        PxReal radius);

    /**
     * @brief Distance from point to AABB
     * @param point Query point
     * @param bounds AABB bounds
     * @return Distance (0 if inside)
     */
    static PxReal distanceToAABB(
        const PxVec3& point,
        const PxBounds3& bounds);

    /**
     * @brief Closest point on AABB from point
     * @param point Query point
     * @param bounds AABB bounds
     * @return Closest point on AABB
     */
    static PxVec3 closestPointOnAABB(
        const PxVec3& point,
        const PxBounds3& bounds);

    /**
     * @brief Distance from point to plane
     * @param point Query point
     * @param plane Plane
     * @return Signed distance
     */
    static PxReal distanceToPlane(
        const PxVec3& point,
        const PxPlane& plane);

    /**
     * @brief Closest point on plane from point
     * @param point Query point
     * @param plane Plane
     * @return Closest point on plane
     */
    static PxVec3 closestPointOnPlane(
        const PxVec3& point,
        const PxPlane& plane);

private:
    // Helper methods
    static void computeDistanceInternal(
        const PxVec3& point,
        const PxGeometry& geometry,
        const PxTransform& pose,
        PxVec3* closestPoint,
        PxReal* distance,
        PxReal* distanceSquared);
};

/**
 * @brief Distance field generator (advanced)
 */
class DistanceFieldGenerator {
public:
    /**
     * @brief Generate 3D distance field around geometry
     * @param geometry Target geometry
     * @param pose Geometry pose
     * @param bounds Field bounds
     * @param resolution Grid resolution (voxels per axis)
     * @return 3D distance field (linearized array)
     */
    static std::vector<PxReal> generateDistanceField(
        const PxGeometry& geometry,
        const PxTransform& pose,
        const PxBounds3& bounds,
        PxU32 resolution);

    /**
     * @brief Sample distance field at point
     * @param distanceField Generated distance field
     * @param bounds Field bounds
     * @param resolution Field resolution
     * @param point Query point
     * @return Interpolated distance at point
     */
    static PxReal sampleDistanceField(
        const std::vector<PxReal>& distanceField,
        const PxBounds3& bounds,
        PxU32 resolution,
        const PxVec3& point);
};

} // namespace PhysXWrapper
