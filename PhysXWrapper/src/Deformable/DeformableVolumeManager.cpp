/**
 * @file DeformableVolumeManager.cpp
 * @brief Implementation of DeformableVolumeManager class
 */

#include "Deformable/DeformableVolumeManager.h"
#include "extensions/PxTetMakerExt.h"
#include "extensions/PxDeformableVolumeExt.h"
#include "extensions/PxRemeshingExt.h"
#include <algorithm>

namespace PhysXWrapper {

/**
 * @brief Private implementation
 */
class DeformableVolumeManager::Impl {
public:
    Impl()
        : m_physics(nullptr)
        , m_cudaContextManager(nullptr)
        , m_initialized(false)
    {}

    PxPhysics* m_physics;
    PxCudaContextManager* m_cudaContextManager;
    bool m_initialized;
    std::string m_lastError;

    std::vector<DeformableVolumeHandle*> m_volumes;

    void setError(const std::string& error) {
        m_lastError = error;
    }

    void clearError() {
        m_lastError.clear();
    }

    void trackVolume(DeformableVolumeHandle* handle) {
        if (handle) {
            m_volumes.push_back(handle);
        }
    }

    void untrackVolume(DeformableVolumeHandle* handle) {
        auto it = std::find(m_volumes.begin(), m_volumes.end(), handle);
        if (it != m_volumes.end()) {
            m_volumes.erase(it);
        }
    }
};

// ============================================================================
// Construction / Destruction
// ============================================================================

DeformableVolumeManager::DeformableVolumeManager()
    : m_impl(std::make_unique<Impl>())
{
}

DeformableVolumeManager::~DeformableVolumeManager() {
    cleanup();
}

DeformableVolumeManager::DeformableVolumeManager(DeformableVolumeManager&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

DeformableVolumeManager& DeformableVolumeManager::operator=(DeformableVolumeManager&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

// ============================================================================
// Initialization
// ============================================================================

bool DeformableVolumeManager::initialize(PxPhysics* physics, PxCudaContextManager* cudaContextManager) {
    if (!physics) {
        m_impl->setError("Invalid physics instance");
        return false;
    }

    if (m_impl->m_initialized) {
        m_impl->setError("Already initialized");
        return false;
    }

    // CUDA context manager is optional but required for deformable volumes
    if (!cudaContextManager) {
        m_impl->setError("CUDA context manager is required for deformable volumes. GPU/CUDA not available.");
        return false;
    }

    if (!cudaContextManager->contextIsValid()) {
        m_impl->setError("CUDA context is not valid. GPU initialization failed.");
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_cudaContextManager = cudaContextManager;
    m_impl->m_initialized = true;
    m_impl->clearError();

    return true;
}

void DeformableVolumeManager::cleanup() {
    if (!m_impl->m_initialized) return;

    // Release all volumes
    for (DeformableVolumeHandle* handle : m_impl->m_volumes) {
        if (handle) {
            // Free GPU buffers
            if (m_impl->m_cudaContextManager) {
                if (handle->simPositionInvMass) {
                    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, handle->simPositionInvMass);
                }
                if (handle->simVelocity) {
                    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, handle->simVelocity);
                }
                if (handle->collPositionInvMass) {
                    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, handle->collPositionInvMass);
                }
                if (handle->restPosition) {
                    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, handle->restPosition);
                }
            }

            // Release actor
            if (handle->actor) {
                handle->actor->release();
            }

            // Release mesh
            if (handle->mesh) {
                handle->mesh->release();
            }

            delete handle;
        }
    }
    m_impl->m_volumes.clear();

    m_impl->m_physics = nullptr;
    m_impl->m_cudaContextManager = nullptr;
    m_impl->m_initialized = false;
    m_impl->clearError();
}

bool DeformableVolumeManager::isInitialized() const {
    return m_impl->m_initialized;
}

bool DeformableVolumeManager::isGPUAvailable() const {
    return m_impl->m_cudaContextManager && m_impl->m_cudaContextManager->contextIsValid();
}

// ============================================================================
// Mesh Generation Helpers
// ============================================================================

SimpleMesh DeformableVolumeManager::createCubeMesh(const PxVec3& center, PxReal size, PxReal maxEdgeLength) {
    SimpleMesh mesh;

    // Create cube vertices
    PxReal halfSize = size * 0.5f;
    PxVec3 corners[8] = {
        center + PxVec3(-halfSize, -halfSize, -halfSize),
        center + PxVec3( halfSize, -halfSize, -halfSize),
        center + PxVec3( halfSize,  halfSize, -halfSize),
        center + PxVec3(-halfSize,  halfSize, -halfSize),
        center + PxVec3(-halfSize, -halfSize,  halfSize),
        center + PxVec3( halfSize, -halfSize,  halfSize),
        center + PxVec3( halfSize,  halfSize,  halfSize),
        center + PxVec3(-halfSize,  halfSize,  halfSize)
    };

    // Create cube faces (12 triangles)
    PxU32 faces[36] = {
        0,1,2, 0,2,3,  // front
        1,5,6, 1,6,2,  // right
        5,4,7, 5,7,6,  // back
        4,0,3, 4,3,7,  // left
        3,2,6, 3,6,7,  // top
        4,5,1, 4,1,0   // bottom
    };

    mesh.vertices.assign(corners, corners + 8);
    mesh.indices.assign(faces, faces + 36);

    // Subdivide if needed
    if (maxEdgeLength > 0.0f) {
        PxArray<PxVec3> verts;
        PxArray<PxU32> indices;
        for (const auto& v : mesh.vertices) verts.pushBack(v);
        for (const auto& i : mesh.indices) indices.pushBack(i);

        PxRemeshingExt::limitMaxEdgeLength(indices, verts, maxEdgeLength);

        mesh.vertices.assign(verts.begin(), verts.end());
        mesh.indices.assign(indices.begin(), indices.end());
    }

    return mesh;
}

SimpleMesh DeformableVolumeManager::createSphereMesh(const PxVec3& center, PxReal radius, PxReal maxEdgeLength) {
    SimpleMesh mesh;

    // Create icosphere (subdivided icosahedron)
    const PxReal t = (1.0f + PxSqrt(5.0f)) * 0.5f;

    // Icosahedron vertices
    PxVec3 vertices[12] = {
        PxVec3(-1,  t,  0), PxVec3( 1,  t,  0), PxVec3(-1, -t,  0), PxVec3( 1, -t,  0),
        PxVec3( 0, -1,  t), PxVec3( 0,  1,  t), PxVec3( 0, -1, -t), PxVec3( 0,  1, -t),
        PxVec3( t,  0, -1), PxVec3( t,  0,  1), PxVec3(-t,  0, -1), PxVec3(-t,  0,  1)
    };

    // Normalize and scale
    for (int i = 0; i < 12; i++) {
        vertices[i].normalize();
        vertices[i] = center + vertices[i] * radius;
        mesh.vertices.push_back(vertices[i]);
    }

    // Icosahedron faces
    PxU32 faces[60] = {
        0,11,5,  0,5,1,   0,1,7,   0,7,10,  0,10,11,
        1,5,9,   5,11,4,  11,10,2,  10,7,6,  7,1,8,
        3,9,4,   3,4,2,   3,2,6,    3,6,8,   3,8,9,
        4,9,5,   2,4,11,  6,2,10,   8,6,7,   9,8,1
    };

    mesh.indices.assign(faces, faces + 60);

    // Subdivide if needed
    if (maxEdgeLength > 0.0f) {
        PxArray<PxVec3> verts;
        PxArray<PxU32> indices;
        for (const auto& v : mesh.vertices) verts.pushBack(v);
        for (const auto& i : mesh.indices) indices.pushBack(i);

        PxRemeshingExt::limitMaxEdgeLength(indices, verts, maxEdgeLength);

        // Re-project to sphere
        for (PxU32 i = 0; i < verts.size(); i++) {
            PxVec3 dir = verts[i] - center;
            dir.normalize();
            verts[i] = center + dir * radius;
        }

        mesh.vertices.assign(verts.begin(), verts.end());
        mesh.indices.assign(indices.begin(), indices.end());
    }

    return mesh;
}

SimpleMesh DeformableVolumeManager::createCylinderMesh(const PxVec3& center, PxReal radius, PxReal height, PxReal maxEdgeLength) {
    SimpleMesh mesh;

    const PxU32 segments = 16;
    const PxReal halfHeight = height * 0.5f;

    // Create cylinder vertices
    for (PxU32 i = 0; i <= segments; i++) {
        PxReal angle = (i * PxTwoPi) / segments;
        PxReal x = PxCos(angle) * radius;
        PxReal z = PxSin(angle) * radius;

        mesh.vertices.push_back(center + PxVec3(x, -halfHeight, z));
        mesh.vertices.push_back(center + PxVec3(x,  halfHeight, z));
    }

    // Add center vertices for caps
    PxU32 bottomCenter = mesh.vertices.size();
    mesh.vertices.push_back(center + PxVec3(0, -halfHeight, 0));
    PxU32 topCenter = mesh.vertices.size();
    mesh.vertices.push_back(center + PxVec3(0,  halfHeight, 0));

    // Create side faces
    for (PxU32 i = 0; i < segments; i++) {
        PxU32 i0 = i * 2;
        PxU32 i1 = i0 + 1;
        PxU32 i2 = (i0 + 2) % ((segments + 1) * 2);
        PxU32 i3 = i2 + 1;

        mesh.indices.push_back(i0); mesh.indices.push_back(i2); mesh.indices.push_back(i1);
        mesh.indices.push_back(i1); mesh.indices.push_back(i2); mesh.indices.push_back(i3);
    }

    // Create bottom cap
    for (PxU32 i = 0; i < segments; i++) {
        mesh.indices.push_back(bottomCenter);
        mesh.indices.push_back(i * 2);
        mesh.indices.push_back(((i + 1) % (segments + 1)) * 2);
    }

    // Create top cap
    for (PxU32 i = 0; i < segments; i++) {
        mesh.indices.push_back(topCenter);
        mesh.indices.push_back(((i + 1) % (segments + 1)) * 2 + 1);
        mesh.indices.push_back(i * 2 + 1);
    }

    // Subdivide if needed
    if (maxEdgeLength > 0.0f) {
        PxArray<PxVec3> verts;
        PxArray<PxU32> indices;
        for (const auto& v : mesh.vertices) verts.pushBack(v);
        for (const auto& i : mesh.indices) indices.pushBack(i);

        PxRemeshingExt::limitMaxEdgeLength(indices, verts, maxEdgeLength);

        mesh.vertices.assign(verts.begin(), verts.end());
        mesh.indices.assign(indices.begin(), indices.end());
    }

    return mesh;
}

// ============================================================================
// Deformable Volume Creation
// ============================================================================

DeformableVolumeHandle* DeformableVolumeManager::createDeformableVolume(
    PxScene* scene,
    const SimpleMesh& mesh,
    const DeformableVolumeConfig& config,
    const DeformableVolumeMaterialConfig& materialConfig,
    const PxTransform& transform,
    const std::string& name)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("Manager not initialized");
        return nullptr;
    }

    if (!scene) {
        m_impl->setError("Invalid scene");
        return nullptr;
    }

    if (mesh.vertices.empty() || mesh.indices.empty()) {
        m_impl->setError("Invalid mesh data");
        return nullptr;
    }

    // Create handle
    DeformableVolumeHandle* handle = new DeformableVolumeHandle();
    handle->name = name;

    // Create deformable volume mesh
    if (!createDeformableVolumeMesh(mesh, config, handle->mesh)) {
        delete handle;
        return nullptr;
    }

    // Create deformable volume actor
    handle->actor = m_impl->m_physics->createDeformableVolume(*m_impl->m_cudaContextManager);
    if (!handle->actor) {
        m_impl->setError("Failed to create deformable volume actor");
        if (handle->mesh) handle->mesh->release();
        delete handle;
        return nullptr;
    }

    // Create material
    PxDeformableVolumeMaterial* material = m_impl->m_physics->createDeformableVolumeMaterial(
        materialConfig.youngsModulus,
        materialConfig.poissonRatio,
        materialConfig.damping
    );

    // Create shape
    PxShapeFlags shapeFlags = PxShapeFlag::eVISUALIZATION |
                              PxShapeFlag::eSCENE_QUERY_SHAPE |
                              PxShapeFlag::eSIMULATION_SHAPE;

    PxTetrahedronMeshGeometry geometry(handle->mesh->getCollisionMesh());
    PxShape* shape = m_impl->m_physics->createShape(geometry, &material, 1, true, shapeFlags);

    if (shape) {
        handle->actor->attachShape(*shape);
        shape->release();
    }

    // Attach simulation mesh
    handle->actor->attachSimulationMesh(
        *handle->mesh->getSimulationMesh(),
        *handle->mesh->getDeformableVolumeAuxData()
    );

    // Configure actor
    handle->actor->setSolverIterationCounts(config.solverIterationCount);
    handle->actor->setDeformableBodyFlag(
        PxDeformableBodyFlag::eDISABLE_SELF_COLLISION,
        !config.enableSelfCollision
    );

    // Add to scene
    scene->addActor(*handle->actor);

    // Allocate and initialize GPU buffers
    allocateAndInitializeGPUBuffers(handle, transform, config);

    // Track handle
    m_impl->trackVolume(handle);
    m_impl->clearError();

    return handle;
}

void DeformableVolumeManager::releaseDeformableVolume(DeformableVolumeHandle* handle) {
    if (!handle) return;

    m_impl->untrackVolume(handle);

    // Free GPU buffers
    if (m_impl->m_cudaContextManager) {
        if (handle->simPositionInvMass) {
            PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, handle->simPositionInvMass);
        }
        if (handle->simVelocity) {
            PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, handle->simVelocity);
        }
        if (handle->collPositionInvMass) {
            PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, handle->collPositionInvMass);
        }
        if (handle->restPosition) {
            PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, handle->restPosition);
        }
    }

    if (handle->actor) handle->actor->release();
    if (handle->mesh) handle->mesh->release();

    delete handle;
}

// ============================================================================
// Update and Query
// ============================================================================

void DeformableVolumeManager::updateDeformedMeshes() {
    for (DeformableVolumeHandle* handle : m_impl->m_volumes) {
        if (handle && handle->actor && handle->simPositionInvMass) {
            // Copy deformed positions from GPU
            PxDeformableVolumeExt::copyToHost(
                *handle->actor,
                PxDeformableVolumeDataFlag::ePOSITION_INVMASS,
                handle->simPositionInvMass,
                nullptr, nullptr, nullptr
            );
        }
    }
}

std::vector<PxVec3> DeformableVolumeManager::getDeformedVertices(DeformableVolumeHandle* handle) {
    std::vector<PxVec3> vertices;

    if (!handle || !handle->actor) return vertices;

    PxU32 numVertices = handle->actor->getSimulationMesh()->getNbVertices();
    vertices.reserve(numVertices);

    if (handle->simPositionInvMass) {
        for (PxU32 i = 0; i < numVertices; i++) {
            vertices.push_back(PxVec3(
                handle->simPositionInvMass[i].x,
                handle->simPositionInvMass[i].y,
                handle->simPositionInvMass[i].z
            ));
        }
    }

    return vertices;
}

PxU32 DeformableVolumeManager::getVertexCount(DeformableVolumeHandle* handle) const {
    if (!handle || !handle->actor) return 0;
    return handle->actor->getSimulationMesh()->getNbVertices();
}

std::vector<DeformableVolumeHandle*> DeformableVolumeManager::getAllVolumes() const {
    return m_impl->m_volumes;
}

PxU32 DeformableVolumeManager::getVolumeCount() const {
    return static_cast<PxU32>(m_impl->m_volumes.size());
}

// ============================================================================
// Attachment
// ============================================================================

PxU32 DeformableVolumeManager::attachToRigidBody(
    DeformableVolumeHandle* handle,
    PxRigidActor* rigidBody,
    PxReal attachmentRadius,
    const PxVec3& attachmentPosition)
{
    if (!handle || !handle->actor || !rigidBody) {
        m_impl->setError("Invalid parameters for attachment");
        return 0;
    }

    // This is a simplified version - full implementation would use PhysX attachment API
    m_impl->setError("Attachment not fully implemented in this version");
    return 0;
}

// ============================================================================
// Error Handling
// ============================================================================

std::string DeformableVolumeManager::getLastError() const {
    return m_impl->m_lastError;
}

// ============================================================================
// Private Helpers
// ============================================================================

bool DeformableVolumeManager::createDeformableVolumeMesh(
    const SimpleMesh& mesh,
    const DeformableVolumeConfig& config,
    PxDeformableVolumeMesh*& outMesh)
{
    // Setup cooking params
    PxTolerancesScale scale;
    PxCookingParams params(scale);
    params.meshWeldTolerance = 0.001f;
    params.meshPreprocessParams = PxMeshPreprocessingFlag::eWELD_VERTICES;
    params.buildGPUData = true;

    // Convert to PhysX format
    PxSimpleTriangleMesh surfaceMesh;
    surfaceMesh.points.count = mesh.vertices.size();
    surfaceMesh.points.data = mesh.vertices.data();
    surfaceMesh.triangles.count = mesh.indices.size() / 3;
    surfaceMesh.triangles.data = mesh.indices.data();

    // Create deformable volume mesh
    if (config.useCollisionMeshForSimulation) {
        outMesh = PxDeformableVolumeExt::createDeformableVolumeMeshNoVoxels(
            params, surfaceMesh,
            m_impl->m_physics->getPhysicsInsertionCallback()
        );
    } else {
        outMesh = PxDeformableVolumeExt::createDeformableVolumeMesh(
            params, surfaceMesh,
            config.numVoxelsAlongLongestAxis,
            m_impl->m_physics->getPhysicsInsertionCallback()
        );
    }

    if (!outMesh) {
        m_impl->setError("Failed to create deformable volume mesh");
        return false;
    }

    return true;
}

void DeformableVolumeManager::allocateAndInitializeGPUBuffers(
    DeformableVolumeHandle* handle,
    const PxTransform& transform,
    const DeformableVolumeConfig& config)
{
    if (!handle || !handle->actor) return;

    // Allocate host mirror buffers
    PxDeformableVolumeExt::allocateAndInitializeHostMirror(
        *handle->actor,
        m_impl->m_cudaContextManager,
        handle->simPositionInvMass,
        handle->simVelocity,
        handle->collPositionInvMass,
        handle->restPosition
    );

    // Transform mesh
    PxDeformableVolumeExt::transform(
        *handle->actor,
        transform,
        config.scale,
        handle->simPositionInvMass,
        handle->simVelocity,
        handle->collPositionInvMass,
        handle->restPosition
    );

    // Update mass
    PxDeformableVolumeExt::updateMass(
        *handle->actor,
        config.density,
        config.maxInvMassRatio,
        handle->simPositionInvMass
    );

    // Copy to GPU
    PxDeformableVolumeExt::copyToDevice(
        *handle->actor,
        PxDeformableVolumeDataFlag::eALL,
        handle->simPositionInvMass,
        handle->simVelocity,
        handle->collPositionInvMass,
        handle->restPosition
    );
}

} // namespace PhysXWrapper
