/**
 * @file PointDistanceQuery.cpp
 * @brief Implementation of PointDistanceQuery class
 */

#include "Query/PointDistanceQuery.h"
#include <algorithm>
#include <cmath>

namespace PhysXWrapper {

// ============================================================================
// Single Point Queries
// ============================================================================

PointDistanceResult PointDistanceQuery::queryGeometry(
    const PxVec3& point,
    const PxGeometry& geometry,
    const PxTransform& pose,
    const PointDistanceConfig& config)
{
    PointDistanceResult result;
    result.queryPoint = point;

    PxVec3 closestPoint;
    PxReal distanceSquared;

    // Use PhysX geometry query
    if (config.computeClosestPoint) {
        distanceSquared = PxGeometryQuery::pointDistance(point, geometry, pose, &closestPoint);
        result.closestPoint = closestPoint;
    } else {
        distanceSquared = PxGeometryQuery::pointDistance(point, geometry, pose, nullptr);
    }

    result.distanceSquared = distanceSquared;
    result.distance = PxSqrt(PxAbs(distanceSquared)); // Negative for inside
    result.success = true;

    // Check distance threshold
    if (result.distance > config.maxDistance) {
        result.success = false;
    }

    return result;
}

PointDistanceResult PointDistanceQuery::queryActor(
    const PxVec3& point,
    PxRigidActor* actor,
    const PointDistanceConfig& config)
{
    PointDistanceResult bestResult;
    bestResult.queryPoint = point;
    bestResult.success = false;

    if (!actor) {
        return bestResult;
    }

    // Get all shapes on actor
    PxU32 numShapes = actor->getNbShapes();
    if (numShapes == 0) {
        return bestResult;
    }

    std::vector<PxShape*> shapes(numShapes);
    actor->getShapes(shapes.data(), numShapes);

    // Query each shape and find minimum distance
    PxReal minDistSq = PX_MAX_F32;
    for (PxShape* shape : shapes) {
        PointDistanceResult shapeResult = queryShape(point, shape, config);
        if (shapeResult.success && shapeResult.distanceSquared < minDistSq) {
            minDistSq = shapeResult.distanceSquared;
            bestResult = shapeResult;
        }
    }

    return bestResult;
}

PointDistanceResult PointDistanceQuery::queryShape(
    const PxVec3& point,
    PxShape* shape,
    const PointDistanceConfig& config)
{
    PointDistanceResult result;
    result.queryPoint = point;
    result.success = false;

    if (!shape) {
        return result;
    }

    // Get actor to compute world pose
    PxRigidActor* actor = shape->getActor();
    if (!actor) {
        return result;
    }

    // Compute shape world pose
    PxTransform shapePose = PxShapeExt::getGlobalPose(*shape, *actor);

    // Get geometry
    PxGeometryHolder geometryHolder = shape->getGeometry();

    // Query
    return queryGeometry(point, geometryHolder.any(), shapePose, config);
}

// ============================================================================
// Batch Queries
// ============================================================================

BatchQueryResult PointDistanceQuery::queryGeometryBatch(
    const std::vector<PxVec3>& points,
    const PxGeometry& geometry,
    const PxTransform& pose,
    const PointDistanceConfig& config)
{
    BatchQueryResult batchResult;
    batchResult.results.reserve(points.size());

    PxReal totalDistance = 0.0f;

    for (const PxVec3& point : points) {
        PointDistanceResult result = queryGeometry(point, geometry, pose, config);
        batchResult.results.push_back(result);

        if (result.success) {
            batchResult.successCount++;
            totalDistance += result.distance;

            batchResult.minDistance = PxMin(batchResult.minDistance, result.distance);
            batchResult.maxDistance = PxMax(batchResult.maxDistance, result.distance);
        }
    }

    if (batchResult.successCount > 0) {
        batchResult.averageDistance = totalDistance / batchResult.successCount;
    }

    return batchResult;
}

void PointDistanceQuery::queryGeometryBatchWithCallback(
    const std::vector<PxVec3>& points,
    const PxGeometry& geometry,
    const PxTransform& pose,
    PointDistanceCallback callback,
    const PointDistanceConfig& config)
{
    if (!callback) return;

    for (const PxVec3& point : points) {
        PointDistanceResult result = queryGeometry(point, geometry, pose, config);
        callback(result);
    }
}

// ============================================================================
// Scene Queries
// ============================================================================

NearestActorResult PointDistanceQuery::findNearestActor(
    PxScene* scene,
    const PxVec3& point,
    const PointDistanceConfig& config)
{
    NearestActorResult nearestResult;
    nearestResult.queryPoint = point;
    nearestResult.success = false;

    if (!scene) {
        return nearestResult;
    }

    // Get all actors in scene
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    if (numActors == 0) {
        return nearestResult;
    }

    std::vector<PxActor*> actors(numActors);
    scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                     actors.data(), numActors);

    // Query each actor
    PxReal minDistSq = PX_MAX_F32;
    for (PxActor* actor : actors) {
        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        if (!rigidActor) continue;

        PointDistanceResult actorResult = queryActor(point, rigidActor, config);
        if (actorResult.success && actorResult.distanceSquared < minDistSq) {
            minDistSq = actorResult.distanceSquared;

            nearestResult.actor = rigidActor;
            nearestResult.closestPoint = actorResult.closestPoint;
            nearestResult.distance = actorResult.distance;
            nearestResult.success = true;

            // Find which shape was closest
            PxU32 numShapes = rigidActor->getNbShapes();
            if (numShapes > 0) {
                std::vector<PxShape*> shapes(numShapes);
                rigidActor->getShapes(shapes.data(), numShapes);

                PxReal minShapeDistSq = PX_MAX_F32;
                for (PxShape* shape : shapes) {
                    PointDistanceResult shapeResult = queryShape(point, shape, config);
                    if (shapeResult.success && shapeResult.distanceSquared < minShapeDistSq) {
                        minShapeDistSq = shapeResult.distanceSquared;
                        nearestResult.shape = shape;
                    }
                }
            }
        }
    }

    return nearestResult;
}

std::vector<NearestActorResult> PointDistanceQuery::findActorsWithinDistance(
    PxScene* scene,
    const PxVec3& point,
    PxReal maxDistance)
{
    std::vector<NearestActorResult> results;

    if (!scene) {
        return results;
    }

    PointDistanceConfig config;
    config.maxDistance = maxDistance;

    // Get all actors
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    if (numActors == 0) {
        return results;
    }

    std::vector<PxActor*> actors(numActors);
    scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                     actors.data(), numActors);

    // Query each actor
    for (PxActor* actor : actors) {
        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        if (!rigidActor) continue;

        PointDistanceResult actorResult = queryActor(point, rigidActor, config);
        if (actorResult.success) {
            NearestActorResult result;
            result.actor = rigidActor;
            result.queryPoint = point;
            result.closestPoint = actorResult.closestPoint;
            result.distance = actorResult.distance;
            result.success = true;
            results.push_back(result);
        }
    }

    // Sort by distance
    std::sort(results.begin(), results.end(),
        [](const NearestActorResult& a, const NearestActorResult& b) {
            return a.distance < b.distance;
        });

    return results;
}

std::vector<NearestActorResult> PointDistanceQuery::findKNearestActors(
    PxScene* scene,
    const PxVec3& point,
    PxU32 k)
{
    std::vector<NearestActorResult> allResults = findActorsWithinDistance(scene, point, PX_MAX_F32);

    // Return only k nearest
    if (allResults.size() > k) {
        allResults.resize(k);
    }

    return allResults;
}

// ============================================================================
// Utility Functions
// ============================================================================

bool PointDistanceQuery::isPointInside(
    const PxVec3& point,
    const PxGeometry& geometry,
    const PxTransform& pose)
{
    PxReal distSq = PxGeometryQuery::pointDistance(point, geometry, pose, nullptr);
    return distSq <= 0.0f;
}

PxVec3 PointDistanceQuery::computeClosestPoint(
    const PxVec3& point,
    const PxGeometry& geometry,
    const PxTransform& pose)
{
    PxVec3 closestPoint;
    PxGeometryQuery::pointDistance(point, geometry, pose, &closestPoint);
    return closestPoint;
}

PxReal PointDistanceQuery::computeDistance(
    const PxVec3& point,
    const PxGeometry& geometry,
    const PxTransform& pose)
{
    PxReal distSq = PxGeometryQuery::pointDistance(point, geometry, pose, nullptr);
    return PxSqrt(PxAbs(distSq));
}

PxReal PointDistanceQuery::computeDistanceSquared(
    const PxVec3& point,
    const PxGeometry& geometry,
    const PxTransform& pose)
{
    return PxGeometryQuery::pointDistance(point, geometry, pose, nullptr);
}

// ============================================================================
// Geometric Primitives
// ============================================================================

PxReal PointDistanceQuery::distanceToSphere(
    const PxVec3& point,
    const PxVec3& center,
    PxReal radius)
{
    PxReal dist = (point - center).magnitude();
    return dist - radius;
}

PxVec3 PointDistanceQuery::closestPointOnSphere(
    const PxVec3& point,
    const PxVec3& center,
    PxReal radius)
{
    PxVec3 dir = point - center;
    PxReal dist = dir.magnitude();

    if (dist < 1e-6f) {
        // Point at center, return any point on sphere
        return center + PxVec3(radius, 0, 0);
    }

    dir /= dist; // Normalize
    return center + dir * radius;
}

PxReal PointDistanceQuery::distanceToAABB(
    const PxVec3& point,
    const PxBounds3& bounds)
{
    PxVec3 closest = closestPointOnAABB(point, bounds);
    return (point - closest).magnitude();
}

PxVec3 PointDistanceQuery::closestPointOnAABB(
    const PxVec3& point,
    const PxBounds3& bounds)
{
    PxVec3 result;
    result.x = PxClamp(point.x, bounds.minimum.x, bounds.maximum.x);
    result.y = PxClamp(point.y, bounds.minimum.y, bounds.maximum.y);
    result.z = PxClamp(point.z, bounds.minimum.z, bounds.maximum.z);
    return result;
}

PxReal PointDistanceQuery::distanceToPlane(
    const PxVec3& point,
    const PxPlane& plane)
{
    return plane.distance(point);
}

PxVec3 PointDistanceQuery::closestPointOnPlane(
    const PxVec3& point,
    const PxPlane& plane)
{
    PxReal dist = plane.distance(point);
    return point - plane.n * dist;
}

// ============================================================================
// Distance Field Generator
// ============================================================================

std::vector<PxReal> DistanceFieldGenerator::generateDistanceField(
    const PxGeometry& geometry,
    const PxTransform& pose,
    const PxBounds3& bounds,
    PxU32 resolution)
{
    const PxU32 totalVoxels = resolution * resolution * resolution;
    std::vector<PxReal> distanceField(totalVoxels);

    const PxVec3 extents = bounds.getExtents();
    const PxVec3 cellSize = extents / static_cast<PxReal>(resolution);
    const PxVec3 minCorner = bounds.minimum;

    // Generate distance for each voxel
    for (PxU32 x = 0; x < resolution; ++x) {
        for (PxU32 y = 0; y < resolution; ++y) {
            for (PxU32 z = 0; z < resolution; ++z) {
                // Compute voxel center
                PxVec3 voxelCenter = minCorner + PxVec3(
                    (x + 0.5f) * cellSize.x,
                    (y + 0.5f) * cellSize.y,
                    (z + 0.5f) * cellSize.z
                );

                // Compute distance
                PxReal dist = PointDistanceQuery::computeDistance(voxelCenter, geometry, pose);

                // Store in linear array
                PxU32 index = x * (resolution * resolution) + y * resolution + z;
                distanceField[index] = dist;
            }
        }
    }

    return distanceField;
}

PxReal DistanceFieldGenerator::sampleDistanceField(
    const std::vector<PxReal>& distanceField,
    const PxBounds3& bounds,
    PxU32 resolution,
    const PxVec3& point)
{
    // Check if point is outside bounds
    if (!bounds.contains(point)) {
        // Return large distance
        return PX_MAX_F32;
    }

    // Compute normalized coordinates [0, 1]
    const PxVec3 extents = bounds.getExtents();
    const PxVec3 minCorner = bounds.minimum;
    const PxVec3 normalized = (point - minCorner) / (extents * 2.0f);

    // Convert to voxel coordinates
    const PxVec3 voxelCoord = normalized * static_cast<PxReal>(resolution);

    // Get integer voxel indices
    const PxU32 x0 = static_cast<PxU32>(PxClamp(voxelCoord.x, 0.0f, static_cast<PxReal>(resolution - 1)));
    const PxU32 y0 = static_cast<PxU32>(PxClamp(voxelCoord.y, 0.0f, static_cast<PxReal>(resolution - 1)));
    const PxU32 z0 = static_cast<PxU32>(PxClamp(voxelCoord.z, 0.0f, static_cast<PxReal>(resolution - 1)));

    // Simple nearest-neighbor sampling
    const PxU32 index = x0 * (resolution * resolution) + y0 * resolution + z0;
    return distanceField[index];

    // TODO: Implement trilinear interpolation for smoother results
}

} // namespace PhysXWrapper
