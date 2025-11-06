/**
 * @file GeometryQuery.cpp
 * @brief Implementation of GeometryQuery class
 */

#include "Query/GeometryQuery.h"
#include <algorithm>

namespace PhysXWrapper {

GeometryQuery::GeometryQuery(PxScene* scene)
    : m_scene(scene)
{
}

GeometryQuery::~GeometryQuery() = default;

//=== Raycast Methods ===//

bool GeometryQuery::raycastSingle(
    const PxVec3& origin,
    const PxVec3& direction,
    PxReal maxDistance,
    RaycastHit& hit,
    const QueryFilter& filter)
{
    if (!m_scene) return false;

    PxRaycastBuffer hitBuffer;
    PxQueryFilterData filterData;
    filterData.flags = filter.flags;
    filterData.data = filter.data;

    bool status = m_scene->raycast(
        origin, direction, maxDistance,
        hitBuffer,
        PxHitFlag::eDEFAULT,
        filterData,
        filter.callback
    );

    if (status && hitBuffer.hasBlock) {
        const PxRaycastHit& pxHit = hitBuffer.block;
        hit.position = pxHit.position;
        hit.normal = pxHit.normal;
        hit.distance = pxHit.distance;
        hit.actor = pxHit.actor;
        hit.shape = pxHit.shape;
        hit.faceIndex = pxHit.faceIndex;
        hit.uv = PxVec2(pxHit.u, pxHit.v);
        return true;
    }

    return false;
}

int GeometryQuery::raycastMultiple(
    const PxVec3& origin,
    const PxVec3& direction,
    PxReal maxDistance,
    std::vector<RaycastHit>& hits,
    int maxHits,
    const QueryFilter& filter)
{
    if (!m_scene || maxHits <= 0) return 0;

    hits.clear();

    // Allocate buffer for hits
    std::vector<PxRaycastHit> hitBuffer(maxHits);
    PxRaycastBuffer buffer(hitBuffer.data(), maxHits);

    PxQueryFilterData filterData;
    filterData.flags = filter.flags;  // eMULTIPLE flag removed - use buffer size instead
    filterData.data = filter.data;

    bool status = m_scene->raycast(
        origin, direction, maxDistance,
        buffer,
        PxHitFlag::eDEFAULT,
        filterData,
        filter.callback
    );

    if (status) {
        hits.reserve(buffer.nbTouches);
        for (PxU32 i = 0; i < buffer.nbTouches; i++) {
            const PxRaycastHit& pxHit = buffer.touches[i];
            RaycastHit hit;
            hit.position = pxHit.position;
            hit.normal = pxHit.normal;
            hit.distance = pxHit.distance;
            hit.actor = pxHit.actor;
            hit.shape = pxHit.shape;
            hit.faceIndex = pxHit.faceIndex;
            hit.uv = PxVec2(pxHit.u, pxHit.v);
            hits.push_back(hit);
        }
    }

    return static_cast<int>(hits.size());
}

int GeometryQuery::raycastAll(
    const PxVec3& origin,
    const PxVec3& direction,
    PxReal maxDistance,
    std::vector<RaycastHit>& hits,
    const QueryFilter& filter)
{
    // Start with a reasonable buffer size
    return raycastMultiple(origin, direction, maxDistance, hits, 128, filter);
}

bool GeometryQuery::raycastAny(
    const PxVec3& origin,
    const PxVec3& direction,
    PxReal maxDistance,
    const QueryFilter& filter)
{
    if (!m_scene) return false;

    PxRaycastBuffer hitBuffer;
    PxQueryFilterData filterData;
    filterData.flags = filter.flags | PxQueryFlag::eANY_HIT;
    filterData.data = filter.data;

    return m_scene->raycast(
        origin, direction, maxDistance,
        hitBuffer,
        PxHitFlag::eDEFAULT,
        filterData,
        filter.callback
    );
}

//=== Sweep Methods ===//

bool GeometryQuery::sweepSphere(
    const PxVec3& origin,
    const PxVec3& direction,
    PxReal radius,
    PxReal maxDistance,
    SweepHit& hit,
    const QueryFilter& filter)
{
    if (!m_scene) return false;

    PxSphereGeometry sphere(radius);
    PxTransform pose(origin);

    PxSweepBuffer hitBuffer;
    PxQueryFilterData filterData;
    filterData.flags = filter.flags;
    filterData.data = filter.data;

    bool status = m_scene->sweep(
        sphere, pose,
        direction, maxDistance,
        hitBuffer,
        PxHitFlag::eDEFAULT,
        filterData,
        filter.callback
    );

    if (status && hitBuffer.hasBlock) {
        const PxSweepHit& pxHit = hitBuffer.block;
        hit.position = pxHit.position;
        hit.normal = pxHit.normal;
        hit.distance = pxHit.distance;
        hit.actor = pxHit.actor;
        hit.shape = pxHit.shape;
        hit.faceIndex = pxHit.faceIndex;
        return true;
    }

    return false;
}

bool GeometryQuery::sweepBox(
    const PxVec3& origin,
    const PxVec3& direction,
    const PxVec3& halfExtents,
    const PxQuat& rotation,
    PxReal maxDistance,
    SweepHit& hit,
    const QueryFilter& filter)
{
    if (!m_scene) return false;

    PxBoxGeometry box(halfExtents);
    PxTransform pose(origin, rotation);

    PxSweepBuffer hitBuffer;
    PxQueryFilterData filterData;
    filterData.flags = filter.flags;
    filterData.data = filter.data;

    bool status = m_scene->sweep(
        box, pose,
        direction, maxDistance,
        hitBuffer,
        PxHitFlag::eDEFAULT,
        filterData,
        filter.callback
    );

    if (status && hitBuffer.hasBlock) {
        const PxSweepHit& pxHit = hitBuffer.block;
        hit.position = pxHit.position;
        hit.normal = pxHit.normal;
        hit.distance = pxHit.distance;
        hit.actor = pxHit.actor;
        hit.shape = pxHit.shape;
        hit.faceIndex = pxHit.faceIndex;
        return true;
    }

    return false;
}

bool GeometryQuery::sweepCapsule(
    const PxVec3& origin,
    const PxVec3& direction,
    PxReal radius,
    PxReal halfHeight,
    const PxQuat& rotation,
    PxReal maxDistance,
    SweepHit& hit,
    const QueryFilter& filter)
{
    if (!m_scene) return false;

    PxCapsuleGeometry capsule(radius, halfHeight);
    PxTransform pose(origin, rotation);

    PxSweepBuffer hitBuffer;
    PxQueryFilterData filterData;
    filterData.flags = filter.flags;
    filterData.data = filter.data;

    bool status = m_scene->sweep(
        capsule, pose,
        direction, maxDistance,
        hitBuffer,
        PxHitFlag::eDEFAULT,
        filterData,
        filter.callback
    );

    if (status && hitBuffer.hasBlock) {
        const PxSweepHit& pxHit = hitBuffer.block;
        hit.position = pxHit.position;
        hit.normal = pxHit.normal;
        hit.distance = pxHit.distance;
        hit.actor = pxHit.actor;
        hit.shape = pxHit.shape;
        hit.faceIndex = pxHit.faceIndex;
        return true;
    }

    return false;
}

//=== Overlap Methods ===//

bool GeometryQuery::overlapSphere(
    const PxVec3& origin,
    PxReal radius,
    std::vector<OverlapHit>& hits,
    int maxOverlaps,
    const QueryFilter& filter)
{
    if (!m_scene || maxOverlaps <= 0) return false;

    hits.clear();

    PxSphereGeometry sphere(radius);
    PxTransform pose(origin);

    // Allocate buffer for overlaps
    std::vector<PxOverlapHit> hitBuffer(maxOverlaps);
    PxOverlapBuffer buffer(hitBuffer.data(), maxOverlaps);

    PxQueryFilterData filterData;
    filterData.flags = filter.flags;
    filterData.data = filter.data;

    bool status = m_scene->overlap(
        sphere, pose,
        buffer,
        filterData,
        filter.callback
    );

    if (status) {
        hits.reserve(buffer.nbTouches);
        for (PxU32 i = 0; i < buffer.nbTouches; i++) {
            const PxOverlapHit& pxHit = buffer.touches[i];
            OverlapHit hit;
            hit.actor = pxHit.actor;
            hit.shape = pxHit.shape;
            hits.push_back(hit);
        }
    }

    return !hits.empty();
}

bool GeometryQuery::overlapBox(
    const PxVec3& origin,
    const PxVec3& halfExtents,
    const PxQuat& rotation,
    std::vector<OverlapHit>& hits,
    int maxOverlaps,
    const QueryFilter& filter)
{
    if (!m_scene || maxOverlaps <= 0) return false;

    hits.clear();

    PxBoxGeometry box(halfExtents);
    PxTransform pose(origin, rotation);

    std::vector<PxOverlapHit> hitBuffer(maxOverlaps);
    PxOverlapBuffer buffer(hitBuffer.data(), maxOverlaps);

    PxQueryFilterData filterData;
    filterData.flags = filter.flags;
    filterData.data = filter.data;

    bool status = m_scene->overlap(
        box, pose,
        buffer,
        filterData,
        filter.callback
    );

    if (status) {
        hits.reserve(buffer.nbTouches);
        for (PxU32 i = 0; i < buffer.nbTouches; i++) {
            const PxOverlapHit& pxHit = buffer.touches[i];
            OverlapHit hit;
            hit.actor = pxHit.actor;
            hit.shape = pxHit.shape;
            hits.push_back(hit);
        }
    }

    return !hits.empty();
}

bool GeometryQuery::overlapCapsule(
    const PxVec3& origin,
    PxReal radius,
    PxReal halfHeight,
    const PxQuat& rotation,
    std::vector<OverlapHit>& hits,
    int maxOverlaps,
    const QueryFilter& filter)
{
    if (!m_scene || maxOverlaps <= 0) return false;

    hits.clear();

    PxCapsuleGeometry capsule(radius, halfHeight);
    PxTransform pose(origin, rotation);

    std::vector<PxOverlapHit> hitBuffer(maxOverlaps);
    PxOverlapBuffer buffer(hitBuffer.data(), maxOverlaps);

    PxQueryFilterData filterData;
    filterData.flags = filter.flags;
    filterData.data = filter.data;

    bool status = m_scene->overlap(
        capsule, pose,
        buffer,
        filterData,
        filter.callback
    );

    if (status) {
        hits.reserve(buffer.nbTouches);
        for (PxU32 i = 0; i < buffer.nbTouches; i++) {
            const PxOverlapHit& pxHit = buffer.touches[i];
            OverlapHit hit;
            hit.actor = pxHit.actor;
            hit.shape = pxHit.shape;
            hits.push_back(hit);
        }
    }

    return !hits.empty();
}

bool GeometryQuery::overlapAny(
    const PxVec3& origin,
    PxReal radius,
    const QueryFilter& filter)
{
    if (!m_scene) return false;

    PxSphereGeometry sphere(radius);
    PxTransform pose(origin);

    PxOverlapBuffer hitBuffer;
    PxQueryFilterData filterData;
    filterData.flags = filter.flags | PxQueryFlag::eANY_HIT;
    filterData.data = filter.data;

    return m_scene->overlap(
        sphere, pose,
        hitBuffer,
        filterData,
        filter.callback
    );
}

void GeometryQuery::setScene(PxScene* scene) {
    m_scene = scene;
}

} // namespace PhysXWrapper
