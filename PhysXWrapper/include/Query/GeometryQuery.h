/**
 * @file GeometryQuery.h
 * @brief Geometry query wrapper for raycasts, sweeps, and overlaps
 *
 * This class provides simplified interfaces for scene queries including
 * raycasts, shape sweeps, and overlap tests.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>
#include <memory>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Raycast hit information
 */
struct RaycastHit {
    /** Hit position in world space */
    PxVec3 position;

    /** Hit normal */
    PxVec3 normal;

    /** Distance from ray origin to hit point */
    PxReal distance;

    /** Hit actor */
    PxRigidActor* actor;

    /** Hit shape */
    PxShape* shape;

    /** Face index for triangle meshes (0xffffffff if not applicable) */
    PxU32 faceIndex;

    /** Barycentric coordinates for triangle hit (if applicable) */
    PxVec2 uv;
};

/**
 * @brief Sweep hit information
 */
struct SweepHit {
    /** Hit position in world space */
    PxVec3 position;

    /** Hit normal */
    PxVec3 normal;

    /** Distance along sweep direction (0 to 1) */
    PxReal distance;

    /** Hit actor */
    PxRigidActor* actor;

    /** Hit shape */
    PxShape* shape;

    /** Face index for triangle meshes */
    PxU32 faceIndex;
};

/**
 * @brief Overlap hit information
 */
struct OverlapHit {
    /** Overlapping actor */
    PxRigidActor* actor;

    /** Overlapping shape */
    PxShape* shape;
};

/**
 * @brief Query filter data
 */
struct QueryFilter {
    /** Filter flags */
    PxQueryFlags flags;

    /** Pre-filter callback (optional) */
    PxQueryFilterCallback* callback;

    /** Filter data */
    PxFilterData data;

    /**
     * @brief Default constructor with common settings
     */
    QueryFilter()
        : flags(PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER)
        , callback(nullptr)
    {
    }
};

/**
 * @brief Geometry query class
 *
 * Provides simplified interfaces for PhysX scene queries:
 * - Raycasts: Cast a ray through the scene
 * - Sweeps: Move a shape through the scene
 * - Overlaps: Test if a shape overlaps with scene geometry
 *
 * All methods are thread-safe when used with different PxScene instances.
 *
 * @example
 * @code
 * GeometryQuery query(scene);
 *
 * // Simple raycast
 * RaycastHit hit;
 * if (query.raycastSingle(origin, direction, maxDistance, hit)) {
 *     std::cout << "Hit at distance: " << hit.distance << "\n";
 * }
 *
 * // Multiple hits
 * std::vector<RaycastHit> hits;
 * int numHits = query.raycastMultiple(origin, direction, maxDistance, hits, 10);
 *
 * // Sphere sweep
 * SweepHit sweepHit;
 * if (query.sweepSphere(start, direction, radius, distance, sweepHit)) {
 *     std::cout << "Sweep hit at: " << sweepHit.distance << "\n";
 * }
 *
 * // Overlap test
 * std::vector<OverlapHit> overlaps;
 * if (query.overlapSphere(center, radius, overlaps)) {
 *     std::cout << "Found " << overlaps.size() << " overlapping objects\n";
 * }
 * @endcode
 */
class GeometryQuery {
public:
    /**
     * @brief Constructor
     * @param scene PhysX scene to query
     */
    explicit GeometryQuery(PxScene* scene);

    /**
     * @brief Destructor
     */
    ~GeometryQuery();

    // Disable copy
    GeometryQuery(const GeometryQuery&) = delete;
    GeometryQuery& operator=(const GeometryQuery&) = delete;

    //=== Raycast Methods ===//

    /**
     * @brief Raycast that returns the closest hit
     * @param origin Ray origin
     * @param direction Ray direction (should be normalized)
     * @param maxDistance Maximum ray distance
     * @param hit Output hit information
     * @param filter Query filter (optional)
     * @return true if hit, false otherwise
     */
    bool raycastSingle(
        const PxVec3& origin,
        const PxVec3& direction,
        PxReal maxDistance,
        RaycastHit& hit,
        const QueryFilter& filter = QueryFilter()
    );

    /**
     * @brief Raycast that returns multiple hits
     * @param origin Ray origin
     * @param direction Ray direction (should be normalized)
     * @param maxDistance Maximum ray distance
     * @param hits Output vector of hits
     * @param maxHits Maximum number of hits to return
     * @param filter Query filter (optional)
     * @return Number of hits found
     */
    int raycastMultiple(
        const PxVec3& origin,
        const PxVec3& direction,
        PxReal maxDistance,
        std::vector<RaycastHit>& hits,
        int maxHits = 32,
        const QueryFilter& filter = QueryFilter()
    );

    /**
     * @brief Raycast that returns all hits along the ray
     * @param origin Ray origin
     * @param direction Ray direction (should be normalized)
     * @param maxDistance Maximum ray distance
     * @param hits Output vector of all hits
     * @param filter Query filter (optional)
     * @return Number of hits found
     */
    int raycastAll(
        const PxVec3& origin,
        const PxVec3& direction,
        PxReal maxDistance,
        std::vector<RaycastHit>& hits,
        const QueryFilter& filter = QueryFilter()
    );

    /**
     * @brief Test if ray hits anything (no hit info returned)
     * @param origin Ray origin
     * @param direction Ray direction
     * @param maxDistance Maximum ray distance
     * @param filter Query filter (optional)
     * @return true if ray hits something
     */
    bool raycastAny(
        const PxVec3& origin,
        const PxVec3& direction,
        PxReal maxDistance,
        const QueryFilter& filter = QueryFilter()
    );

    //=== Sweep Methods ===//

    /**
     * @brief Sphere sweep (linear cast)
     * @param origin Starting position
     * @param direction Sweep direction (should be normalized)
     * @param radius Sphere radius
     * @param maxDistance Maximum sweep distance
     * @param hit Output hit information
     * @param filter Query filter (optional)
     * @return true if hit, false otherwise
     */
    bool sweepSphere(
        const PxVec3& origin,
        const PxVec3& direction,
        PxReal radius,
        PxReal maxDistance,
        SweepHit& hit,
        const QueryFilter& filter = QueryFilter()
    );

    /**
     * @brief Box sweep (linear cast)
     * @param origin Starting position
     * @param direction Sweep direction
     * @param halfExtents Box half extents
     * @param rotation Box rotation
     * @param maxDistance Maximum sweep distance
     * @param hit Output hit information
     * @param filter Query filter (optional)
     * @return true if hit
     */
    bool sweepBox(
        const PxVec3& origin,
        const PxVec3& direction,
        const PxVec3& halfExtents,
        const PxQuat& rotation,
        PxReal maxDistance,
        SweepHit& hit,
        const QueryFilter& filter = QueryFilter()
    );

    /**
     * @brief Capsule sweep (linear cast)
     * @param origin Starting position
     * @param direction Sweep direction
     * @param radius Capsule radius
     * @param halfHeight Capsule half height
     * @param rotation Capsule rotation
     * @param maxDistance Maximum sweep distance
     * @param hit Output hit information
     * @param filter Query filter (optional)
     * @return true if hit
     */
    bool sweepCapsule(
        const PxVec3& origin,
        const PxVec3& direction,
        PxReal radius,
        PxReal halfHeight,
        const PxQuat& rotation,
        PxReal maxDistance,
        SweepHit& hit,
        const QueryFilter& filter = QueryFilter()
    );

    //=== Overlap Methods ===//

    /**
     * @brief Sphere overlap test
     * @param origin Sphere center
     * @param radius Sphere radius
     * @param hits Output vector of overlapping objects
     * @param maxOverlaps Maximum number of overlaps to return
     * @param filter Query filter (optional)
     * @return true if any overlaps found
     */
    bool overlapSphere(
        const PxVec3& origin,
        PxReal radius,
        std::vector<OverlapHit>& hits,
        int maxOverlaps = 32,
        const QueryFilter& filter = QueryFilter()
    );

    /**
     * @brief Box overlap test
     * @param origin Box center
     * @param halfExtents Box half extents
     * @param rotation Box rotation
     * @param hits Output vector of overlapping objects
     * @param maxOverlaps Maximum number of overlaps
     * @param filter Query filter (optional)
     * @return true if any overlaps found
     */
    bool overlapBox(
        const PxVec3& origin,
        const PxVec3& halfExtents,
        const PxQuat& rotation,
        std::vector<OverlapHit>& hits,
        int maxOverlaps = 32,
        const QueryFilter& filter = QueryFilter()
    );

    /**
     * @brief Capsule overlap test
     * @param origin Capsule center
     * @param radius Capsule radius
     * @param halfHeight Capsule half height
     * @param rotation Capsule rotation
     * @param hits Output vector of overlapping objects
     * @param maxOverlaps Maximum number of overlaps
     * @param filter Query filter (optional)
     * @return true if any overlaps found
     */
    bool overlapCapsule(
        const PxVec3& origin,
        PxReal radius,
        PxReal halfHeight,
        const PxQuat& rotation,
        std::vector<OverlapHit>& hits,
        int maxOverlaps = 32,
        const QueryFilter& filter = QueryFilter()
    );

    /**
     * @brief Test if sphere overlaps anything (no overlap info returned)
     * @param origin Sphere center
     * @param radius Sphere radius
     * @param filter Query filter (optional)
     * @return true if overlaps with anything
     */
    bool overlapAny(
        const PxVec3& origin,
        PxReal radius,
        const QueryFilter& filter = QueryFilter()
    );

    /**
     * @brief Set the scene to query
     * @param scene PhysX scene
     */
    void setScene(PxScene* scene);

    /**
     * @brief Get the current scene
     * @return PhysX scene pointer
     */
    PxScene* getScene() const { return m_scene; }

private:
    PxScene* m_scene;
};

} // namespace PhysXWrapper
