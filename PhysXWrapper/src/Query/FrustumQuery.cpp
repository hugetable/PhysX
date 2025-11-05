/**
 * @file FrustumQuery.cpp
 * @brief Implementation of FrustumQuery class
 */

#include "Query/FrustumQuery.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace PhysXWrapper {

// ============================================================================
// Frustum Implementation
// ============================================================================

bool Frustum::containsPoint(const PxVec3& point) const {
    for (int i = 0; i < 6; ++i) {
        if (planes[i].distance(point) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool Frustum::containsSphere(const PxVec3& center, PxReal radius) const {
    for (int i = 0; i < 6; ++i) {
        if (planes[i].distance(center) < -radius) {
            return false;
        }
    }
    return true;
}

bool Frustum::containsBox(const PxBounds3& bounds) const {
    // Get box corners
    PxVec3 corners[8];
    corners[0] = PxVec3(bounds.minimum.x, bounds.minimum.y, bounds.minimum.z);
    corners[1] = PxVec3(bounds.maximum.x, bounds.minimum.y, bounds.minimum.z);
    corners[2] = PxVec3(bounds.minimum.x, bounds.maximum.y, bounds.minimum.z);
    corners[3] = PxVec3(bounds.maximum.x, bounds.maximum.y, bounds.minimum.z);
    corners[4] = PxVec3(bounds.minimum.x, bounds.minimum.y, bounds.maximum.z);
    corners[5] = PxVec3(bounds.maximum.x, bounds.minimum.y, bounds.maximum.z);
    corners[6] = PxVec3(bounds.minimum.x, bounds.maximum.y, bounds.maximum.z);
    corners[7] = PxVec3(bounds.maximum.x, bounds.maximum.y, bounds.maximum.z);

    // Test each plane
    for (int i = 0; i < 6; ++i) {
        bool allOutside = true;
        for (int j = 0; j < 8; ++j) {
            if (planes[i].distance(corners[j]) >= 0.0f) {
                allOutside = false;
                break;
            }
        }
        if (allOutside) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// FrustumQuery::Impl
// ============================================================================

class FrustumQuery::Impl {
public:
    Impl() : m_bvh(nullptr) {}

    ~Impl() {
        releaseBVH();
    }

    std::vector<CullableObject> m_objects;
    PxBVH* m_bvh;
    mutable FrustumQueryResult m_lastResult;

    void releaseBVH() {
        if (m_bvh) {
            m_bvh->release();
            m_bvh = nullptr;
        }
    }
};

// ============================================================================
// Construction / Destruction
// ============================================================================

FrustumQuery::FrustumQuery()
    : m_impl(std::make_unique<Impl>())
{
}

FrustumQuery::~FrustumQuery() {
}

FrustumQuery::FrustumQuery(FrustumQuery&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

FrustumQuery& FrustumQuery::operator=(FrustumQuery&& other) noexcept {
    if (this != &other) {
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

// ============================================================================
// Frustum Creation
// ============================================================================

Frustum FrustumQuery::createFrustum(const CameraConfig& camera) {
    // Create view matrix
    PxMat44 view = CameraMatrixHelper::createViewMatrix(
        camera.position, camera.target, camera.up);

    // Create projection matrix
    PxReal fovRadians = camera.fov * PxPi / 180.0f;
    PxMat44 proj = CameraMatrixHelper::createPerspectiveMatrix(
        fovRadians, camera.aspectRatio, camera.nearPlane, camera.farPlane);

    // Combine and extract frustum
    return createFrustum(view, proj);
}

Frustum FrustumQuery::createFrustum(const PxMat44& viewMatrix, const PxMat44& projMatrix) {
    PxMat44 viewProj = projMatrix * viewMatrix;
    return createFrustum(viewProj);
}

Frustum FrustumQuery::createFrustum(const PxMat44& viewProjMatrix) {
    Frustum frustum;
    extractFrustumPlanes(viewProjMatrix, frustum.planes);
    return frustum;
}

Frustum FrustumQuery::createOrthographicFrustum(
    PxReal left, PxReal right,
    PxReal bottom, PxReal top,
    PxReal nearPlane, PxReal farPlane)
{
    Frustum frustum;

    // Left plane
    frustum.planes[static_cast<int>(FrustumPlane::LEFT)] = PxPlane(1, 0, 0, -left);

    // Right plane
    frustum.planes[static_cast<int>(FrustumPlane::RIGHT)] = PxPlane(-1, 0, 0, right);

    // Bottom plane
    frustum.planes[static_cast<int>(FrustumPlane::BOTTOM)] = PxPlane(0, 1, 0, -bottom);

    // Top plane
    frustum.planes[static_cast<int>(FrustumPlane::TOP)] = PxPlane(0, -1, 0, top);

    // Near plane
    frustum.planes[static_cast<int>(FrustumPlane::NEAR)] = PxPlane(0, 0, 1, -nearPlane);

    // Far plane
    frustum.planes[static_cast<int>(FrustumPlane::FAR)] = PxPlane(0, 0, -1, farPlane);

    // Normalize all planes
    for (int i = 0; i < 6; ++i) {
        frustum.planes[i].normalize();
    }

    return frustum;
}

// ============================================================================
// Object Management
// ============================================================================

PxU32 FrustumQuery::addObject(const PxGeometry& geometry, const PxTransform& transform, void* userData) {
    CullableObject obj;
    obj.geometry.storeAny(geometry);
    obj.transform = transform;
    obj.bounds = computeBounds(geometry, transform);
    obj.userData = userData;

    m_impl->m_objects.push_back(obj);
    return static_cast<PxU32>(m_impl->m_objects.size() - 1);
}

PxU32 FrustumQuery::addObject(const PxBounds3& bounds, void* userData) {
    CullableObject obj;
    obj.bounds = bounds;
    obj.userData = userData;

    m_impl->m_objects.push_back(obj);
    return static_cast<PxU32>(m_impl->m_objects.size() - 1);
}

void FrustumQuery::clearObjects() {
    m_impl->m_objects.clear();
    releaseBVH();
}

PxU32 FrustumQuery::getObjectCount() const {
    return static_cast<PxU32>(m_impl->m_objects.size());
}

const CullableObject* FrustumQuery::getObject(PxU32 index) const {
    if (index >= m_impl->m_objects.size()) {
        return nullptr;
    }
    return &m_impl->m_objects[index];
}

void FrustumQuery::updateObjectTransform(PxU32 index, const PxTransform& transform) {
    if (index >= m_impl->m_objects.size()) {
        return;
    }

    CullableObject& obj = m_impl->m_objects[index];
    obj.transform = transform;

    // Recompute bounds if geometry is available
    if (obj.geometry.getType() != PxGeometryType::eINVALID) {
        obj.bounds = computeBounds(obj.geometry.any(), transform);
    }
}

// ============================================================================
// BVH Management
// ============================================================================

void FrustumQuery::buildBVH(PxReal enlargement) {
    releaseBVH();

    if (m_impl->m_objects.empty()) {
        return;
    }

    const PxU32 numObjects = static_cast<PxU32>(m_impl->m_objects.size());

    // Allocate bounds array
    PxBounds3* bounds = new PxBounds3[numObjects];
    for (PxU32 i = 0; i < numObjects; ++i) {
        bounds[i] = m_impl->m_objects[i].bounds;
    }

    // Create BVH
    PxBVHDesc bvhDesc;
    bvhDesc.bounds.count = numObjects;
    bvhDesc.bounds.data = bounds;
    bvhDesc.bounds.stride = sizeof(PxBounds3);
    bvhDesc.enlargement = enlargement;

    m_impl->m_bvh = PxCreateBVH(bvhDesc);

    delete[] bounds;
}

bool FrustumQuery::hasBVH() const {
    return m_impl->m_bvh != nullptr;
}

void FrustumQuery::releaseBVH() {
    m_impl->releaseBVH();
}

void FrustumQuery::rebuildBVH() {
    PxReal enlargement = 0.0f;
    if (m_impl->m_bvh) {
        // Preserve enlargement if possible
        enlargement = 0.0f; // Can't query this from existing BVH
    }
    buildBVH(enlargement);
}

// ============================================================================
// Culling Queries
// ============================================================================

FrustumQueryResult FrustumQuery::cull(const Frustum& frustum) const {
    FrustumQueryResult result;
    result.totalObjects = getObjectCount();

    auto startTime = std::chrono::high_resolution_clock::now();

    if (m_impl->m_bvh) {
        // Use BVH acceleration
        struct LocalCallback : PxBVH::OverlapCallback {
            LocalCallback(std::vector<PxU32>& visibles) : mVisibles(visibles) {}

            virtual bool reportHit(PxU32 boundsIndex) override {
                mVisibles.push_back(boundsIndex);
                return true;
            }

            std::vector<PxU32>& mVisibles;
        };

        LocalCallback callback(result.visibleIndices);
        m_impl->m_bvh->cull(6, frustum.planes, callback);
    } else {
        // Brute force check all objects
        for (PxU32 i = 0; i < result.totalObjects; ++i) {
            if (isVisible(frustum, m_impl->m_objects[i].bounds)) {
                result.visibleIndices.push_back(i);
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    result.queryTime = duration.count() / 1000.0f; // Convert to milliseconds

    result.culledObjects = result.totalObjects - static_cast<PxU32>(result.visibleIndices.size());

    m_impl->m_lastResult = result;
    return result;
}

void FrustumQuery::cullWithCallback(const Frustum& frustum, CullingCallback callback) const {
    if (!callback) return;

    if (m_impl->m_bvh) {
        // Use BVH acceleration
        struct LocalCallback : PxBVH::OverlapCallback {
            LocalCallback(CullingCallback cb) : mCallback(cb) {}

            virtual bool reportHit(PxU32 boundsIndex) override {
                mCallback(boundsIndex);
                return true;
            }

            CullingCallback mCallback;
        };

        LocalCallback cb(callback);
        m_impl->m_bvh->cull(6, frustum.planes, cb);
    } else {
        // Brute force check
        const PxU32 numObjects = getObjectCount();
        for (PxU32 i = 0; i < numObjects; ++i) {
            if (isVisible(frustum, m_impl->m_objects[i].bounds)) {
                callback(i);
            }
        }
    }
}

std::vector<PxRigidActor*> FrustumQuery::cullScene(PxScene* scene, const Frustum& frustum) {
    std::vector<PxRigidActor*> visibleActors;

    if (!scene) {
        return visibleActors;
    }

    // Get all actors in scene
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    if (numActors == 0) {
        return visibleActors;
    }

    std::vector<PxActor*> actors(numActors);
    scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                     actors.data(), numActors);

    // Test each actor
    for (PxActor* actor : actors) {
        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        if (!rigidActor) continue;

        // Get actor bounds
        PxBounds3 bounds = rigidActor->getWorldBounds();

        // Test against frustum
        if (isVisible(frustum, bounds)) {
            visibleActors.push_back(rigidActor);
        }
    }

    return visibleActors;
}

void FrustumQuery::cullSceneWithCallback(
    PxScene* scene,
    const Frustum& frustum,
    std::function<void(PxRigidActor*)> callback)
{
    if (!scene || !callback) {
        return;
    }

    // Get all actors
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    if (numActors == 0) {
        return;
    }

    std::vector<PxActor*> actors(numActors);
    scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                     actors.data(), numActors);

    // Test each actor
    for (PxActor* actor : actors) {
        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        if (!rigidActor) continue;

        PxBounds3 bounds = rigidActor->getWorldBounds();

        if (isVisible(frustum, bounds)) {
            callback(rigidActor);
        }
    }
}

// ============================================================================
// Utility Functions
// ============================================================================

bool FrustumQuery::isVisible(const Frustum& frustum, const PxBounds3& bounds) {
    return frustum.containsBox(bounds);
}

bool FrustumQuery::isVisible(const Frustum& frustum, const PxVec3& center, PxReal radius) {
    return frustum.containsSphere(center, radius);
}

PxBounds3 FrustumQuery::computeBounds(const PxGeometry& geometry, const PxTransform& transform) {
    PxBounds3 bounds;
    PxGeometryQuery::computeGeomBounds(bounds, geometry, transform);
    return bounds;
}

const FrustumQueryResult& FrustumQuery::getLastQueryResult() const {
    return m_impl->m_lastResult;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void FrustumQuery::extractFrustumPlanes(const PxMat44& viewProj, PxPlane* planes) {
    // Extract planes from view-projection matrix
    // Based on Gribb-Hartmann method

    // Left plane
    planes[static_cast<int>(FrustumPlane::LEFT)].n.x = -(viewProj.column0[3] + viewProj.column0[0]);
    planes[static_cast<int>(FrustumPlane::LEFT)].n.y = -(viewProj.column1[3] + viewProj.column1[0]);
    planes[static_cast<int>(FrustumPlane::LEFT)].n.z = -(viewProj.column2[3] + viewProj.column2[0]);
    planes[static_cast<int>(FrustumPlane::LEFT)].d = -(viewProj.column3[3] + viewProj.column3[0]);

    // Right plane
    planes[static_cast<int>(FrustumPlane::RIGHT)].n.x = -(viewProj.column0[3] - viewProj.column0[0]);
    planes[static_cast<int>(FrustumPlane::RIGHT)].n.y = -(viewProj.column1[3] - viewProj.column1[0]);
    planes[static_cast<int>(FrustumPlane::RIGHT)].n.z = -(viewProj.column2[3] - viewProj.column2[0]);
    planes[static_cast<int>(FrustumPlane::RIGHT)].d = -(viewProj.column3[3] - viewProj.column3[0]);

    // Top plane
    planes[static_cast<int>(FrustumPlane::TOP)].n.x = -(viewProj.column0[3] - viewProj.column0[1]);
    planes[static_cast<int>(FrustumPlane::TOP)].n.y = -(viewProj.column1[3] - viewProj.column1[1]);
    planes[static_cast<int>(FrustumPlane::TOP)].n.z = -(viewProj.column2[3] - viewProj.column2[1]);
    planes[static_cast<int>(FrustumPlane::TOP)].d = -(viewProj.column3[3] - viewProj.column3[1]);

    // Bottom plane
    planes[static_cast<int>(FrustumPlane::BOTTOM)].n.x = -(viewProj.column0[3] + viewProj.column0[1]);
    planes[static_cast<int>(FrustumPlane::BOTTOM)].n.y = -(viewProj.column1[3] + viewProj.column1[1]);
    planes[static_cast<int>(FrustumPlane::BOTTOM)].n.z = -(viewProj.column2[3] + viewProj.column2[1]);
    planes[static_cast<int>(FrustumPlane::BOTTOM)].d = -(viewProj.column3[3] + viewProj.column3[1]);

    // Near plane
    planes[static_cast<int>(FrustumPlane::NEAR)].n.x = -(viewProj.column0[3] + viewProj.column0[2]);
    planes[static_cast<int>(FrustumPlane::NEAR)].n.y = -(viewProj.column1[3] + viewProj.column1[2]);
    planes[static_cast<int>(FrustumPlane::NEAR)].n.z = -(viewProj.column2[3] + viewProj.column2[2]);
    planes[static_cast<int>(FrustumPlane::NEAR)].d = -(viewProj.column3[3] + viewProj.column3[2]);

    // Far plane
    planes[static_cast<int>(FrustumPlane::FAR)].n.x = -(viewProj.column0[3] - viewProj.column0[2]);
    planes[static_cast<int>(FrustumPlane::FAR)].n.y = -(viewProj.column1[3] - viewProj.column1[2]);
    planes[static_cast<int>(FrustumPlane::FAR)].n.z = -(viewProj.column2[3] - viewProj.column2[2]);
    planes[static_cast<int>(FrustumPlane::FAR)].d = -(viewProj.column3[3] - viewProj.column3[2]);

    // Normalize all planes
    for (int i = 0; i < 6; ++i) {
        planes[i].normalize();
    }
}

bool FrustumQuery::testBoundsAgainstPlane(const PxBounds3& bounds, const PxPlane& plane) {
    // Get positive vertex (vertex furthest in plane normal direction)
    PxVec3 pVertex;
    pVertex.x = (plane.n.x >= 0.0f) ? bounds.maximum.x : bounds.minimum.x;
    pVertex.y = (plane.n.y >= 0.0f) ? bounds.maximum.y : bounds.minimum.y;
    pVertex.z = (plane.n.z >= 0.0f) ? bounds.maximum.z : bounds.minimum.z;

    // Test if positive vertex is outside
    return plane.distance(pVertex) >= 0.0f;
}

// ============================================================================
// CameraMatrixHelper Implementation
// ============================================================================

PxMat44 CameraMatrixHelper::createViewMatrix(const PxVec3& eye, const PxVec3& target, const PxVec3& up) {
    PxVec3 zAxis = (eye - target).getNormalized();
    PxVec3 xAxis = up.cross(zAxis).getNormalized();
    PxVec3 yAxis = zAxis.cross(xAxis);

    PxMat44 view(
        PxVec4(xAxis.x, yAxis.x, zAxis.x, 0.0f),
        PxVec4(xAxis.y, yAxis.y, zAxis.y, 0.0f),
        PxVec4(xAxis.z, yAxis.z, zAxis.z, 0.0f),
        PxVec4(-xAxis.dot(eye), -yAxis.dot(eye), -zAxis.dot(eye), 1.0f)
    );

    return view;
}

PxMat44 CameraMatrixHelper::createPerspectiveMatrix(
    PxReal fovY, PxReal aspectRatio,
    PxReal nearPlane, PxReal farPlane)
{
    PxReal f = 1.0f / std::tan(fovY * 0.5f);
    PxReal nf = 1.0f / (nearPlane - farPlane);

    PxMat44 proj(
        PxVec4(f / aspectRatio, 0.0f, 0.0f, 0.0f),
        PxVec4(0.0f, f, 0.0f, 0.0f),
        PxVec4(0.0f, 0.0f, (farPlane + nearPlane) * nf, -1.0f),
        PxVec4(0.0f, 0.0f, (2.0f * farPlane * nearPlane) * nf, 0.0f)
    );

    return proj;
}

PxMat44 CameraMatrixHelper::createOrthographicMatrix(
    PxReal left, PxReal right,
    PxReal bottom, PxReal top,
    PxReal nearPlane, PxReal farPlane)
{
    PxReal rl = 1.0f / (right - left);
    PxReal tb = 1.0f / (top - bottom);
    PxReal fn = 1.0f / (farPlane - nearPlane);

    PxMat44 proj(
        PxVec4(2.0f * rl, 0.0f, 0.0f, 0.0f),
        PxVec4(0.0f, 2.0f * tb, 0.0f, 0.0f),
        PxVec4(0.0f, 0.0f, -2.0f * fn, 0.0f),
        PxVec4(-(right + left) * rl, -(top + bottom) * tb, -(farPlane + nearPlane) * fn, 1.0f)
    );

    return proj;
}

} // namespace PhysXWrapper
