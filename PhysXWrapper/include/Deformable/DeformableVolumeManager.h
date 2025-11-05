/**
 * @file DeformableVolumeManager.h
 * @brief Comprehensive soft body deformable volume system for PhysX
 *
 * This class provides utilities for creating and managing deformable volumes (soft bodies):
 * - GPU-accelerated soft body simulation
 * - Tetrahedral mesh generation from triangle meshes
 * - Material properties (Young's modulus, Poisson ratio, damping)
 * - Collision mesh and simulation mesh management
 * - Self-collision control
 * - Attachment to rigid bodies
 *
 * IMPORTANT: Deformable volumes require GPU/CUDA support in PhysX 5.x.
 * This feature will not work without a compatible NVIDIA GPU.
 *
 * Based on SnippetDeformableVolume from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>
#include <memory>
#include <string>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Deformable volume material configuration
 */
struct DeformableVolumeMaterialConfig {
    PxReal youngsModulus = 2.0e5f;        ///< Young's modulus (stiffness) in Pa
    PxReal poissonRatio = 0.3f;           ///< Poisson ratio (0-0.5, 0.5 = incompressible)
    PxReal damping = 0.1f;                ///< Damping coefficient
};

/**
 * @brief Deformable volume configuration
 */
struct DeformableVolumeConfig {
    PxReal density = 100.0f;              ///< Density (kg/m^3)
    PxReal scale = 1.0f;                  ///< Scale factor for mesh
    PxU32 solverIterationCount = 30;      ///< Solver iteration count (higher = more stable/slower)
    PxU32 numVoxelsAlongLongestAxis = 8;  ///< Voxel resolution for simulation mesh
    bool enableSelfCollision = false;      ///< Enable self-collision
    bool useCollisionMeshForSimulation = false;  ///< Use collision mesh as simulation mesh (faster but less accurate)
    PxReal maxInvMassRatio = 50.0f;       ///< Maximum inverse mass ratio
};

/**
 * @brief Simple mesh data structure
 */
struct SimpleMesh {
    std::vector<PxVec3> vertices;         ///< Vertex positions
    std::vector<PxU32> indices;           ///< Triangle indices (3 per triangle)
};

/**
 * @brief Deformable volume handle
 */
struct DeformableVolumeHandle {
    PxDeformableVolume* actor = nullptr;             ///< PhysX deformable volume actor
    PxDeformableVolumeMesh* mesh = nullptr;          ///< Deformable volume mesh
    PxVec4* simPositionInvMass = nullptr;            ///< Simulation position/inverse mass buffer (GPU)
    PxVec4* simVelocity = nullptr;                   ///< Simulation velocity buffer (GPU)
    PxVec4* collPositionInvMass = nullptr;           ///< Collision position/inverse mass buffer (GPU)
    PxVec4* restPosition = nullptr;                  ///< Rest position buffer (GPU)
    std::string name;                                 ///< Optional name
};

/**
 * @brief Deformable volume manager class
 *
 * This class provides comprehensive deformable volume (soft body) management:
 * - Create soft bodies from triangle meshes
 * - GPU-accelerated simulation
 * - Material property configuration
 * - Collision and self-collision
 * - Attachment to rigid bodies
 * - Query deformed state
 *
 * REQUIREMENTS:
 * - NVIDIA GPU with CUDA support
 * - PhysX compiled with GPU support
 * - Scene configured with GPU dynamics enabled
 *
 * @example
 * @code
 * // Initialize manager with CUDA context
 * DeformableVolumeManager defMgr;
 * if (!defMgr.initialize(physics, cudaContextManager)) {
 *     std::cerr << "Failed to initialize: " << defMgr.getLastError() << std::endl;
 *     // GPU not available
 *     return;
 * }
 *
 * // Create simple cube mesh
 * SimpleMesh mesh = defMgr.createCubeMesh(PxVec3(0, 5, 0), 2.0f);
 *
 * // Configure deformable volume
 * DeformableVolumeConfig config;
 * config.density = 100.0f;
 * config.solverIterationCount = 30;
 *
 * DeformableVolumeMaterialConfig matConfig;
 * matConfig.youngsModulus = 1.0e5f;  // Softer material
 * matConfig.poissonRatio = 0.4f;
 *
 * // Create deformable volume
 * DeformableVolumeHandle* handle = defMgr.createDeformableVolume(
 *     scene, mesh, config, matConfig
 * );
 *
 * if (handle) {
 *     std::cout << "Soft body created successfully" << std::endl;
 * }
 *
 * // Update simulation (call after scene->fetchResults())
 * defMgr.updateDeformedMeshes();
 * @endcode
 */
class DeformableVolumeManager {
public:
    /**
     * @brief Constructor
     */
    DeformableVolumeManager();

    /**
     * @brief Destructor
     */
    ~DeformableVolumeManager();

    // No copy
    DeformableVolumeManager(const DeformableVolumeManager&) = delete;
    DeformableVolumeManager& operator=(const DeformableVolumeManager&) = delete;

    // Move allowed
    DeformableVolumeManager(DeformableVolumeManager&&) noexcept;
    DeformableVolumeManager& operator=(DeformableVolumeManager&&) noexcept;

    /**
     * @brief Initialize deformable volume manager
     * @param physics PhysX physics instance
     * @param cudaContextManager CUDA context manager (required for GPU)
     * @return True if successful
     */
    bool initialize(PxPhysics* physics, PxCudaContextManager* cudaContextManager);

    /**
     * @brief Cleanup and release resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const;

    /**
     * @brief Check if GPU/CUDA is available
     */
    bool isGPUAvailable() const;

    // ========================================================================
    // Mesh Generation Helpers
    // ========================================================================

    /**
     * @brief Create cube mesh
     * @param center Center position
     * @param size Cube size
     * @param maxEdgeLength Maximum edge length (for subdivision)
     * @return Mesh data
     */
    SimpleMesh createCubeMesh(const PxVec3& center, PxReal size, PxReal maxEdgeLength = 1.0f);

    /**
     * @brief Create sphere mesh
     * @param center Center position
     * @param radius Sphere radius
     * @param maxEdgeLength Maximum edge length (for subdivision)
     * @return Mesh data
     */
    SimpleMesh createSphereMesh(const PxVec3& center, PxReal radius, PxReal maxEdgeLength = 0.5f);

    /**
     * @brief Create cylinder mesh
     * @param center Center position
     * @param radius Cylinder radius
     * @param height Cylinder height
     * @param maxEdgeLength Maximum edge length (for subdivision)
     * @return Mesh data
     */
    SimpleMesh createCylinderMesh(const PxVec3& center, PxReal radius, PxReal height, PxReal maxEdgeLength = 0.5f);

    // ========================================================================
    // Deformable Volume Creation
    // ========================================================================

    /**
     * @brief Create deformable volume from mesh
     * @param scene Scene to add to
     * @param mesh Input triangle mesh
     * @param config Volume configuration
     * @param materialConfig Material configuration
     * @param transform Initial transform
     * @param name Optional name
     * @return Handle to deformable volume or nullptr on failure
     */
    DeformableVolumeHandle* createDeformableVolume(
        PxScene* scene,
        const SimpleMesh& mesh,
        const DeformableVolumeConfig& config = DeformableVolumeConfig(),
        const DeformableVolumeMaterialConfig& materialConfig = DeformableVolumeMaterialConfig(),
        const PxTransform& transform = PxTransform(PxIdentity),
        const std::string& name = ""
    );

    /**
     * @brief Release deformable volume
     * @param handle Handle to release
     */
    void releaseDeformableVolume(DeformableVolumeHandle* handle);

    // ========================================================================
    // Update and Query
    // ========================================================================

    /**
     * @brief Update all deformed meshes (copy from GPU)
     * Call this after scene->fetchResults()
     */
    void updateDeformedMeshes();

    /**
     * @brief Get deformed vertices for a volume
     * @param handle Volume handle
     * @return Vector of deformed vertex positions
     */
    std::vector<PxVec3> getDeformedVertices(DeformableVolumeHandle* handle);

    /**
     * @brief Get number of vertices
     * @param handle Volume handle
     * @return Vertex count
     */
    PxU32 getVertexCount(DeformableVolumeHandle* handle) const;

    /**
     * @brief Get all managed volumes
     * @return Vector of handles
     */
    std::vector<DeformableVolumeHandle*> getAllVolumes() const;

    /**
     * @brief Get volume count
     * @return Number of managed volumes
     */
    PxU32 getVolumeCount() const;

    // ========================================================================
    // Attachment
    // ========================================================================

    /**
     * @brief Attach deformable volume vertices to rigid body
     * @param handle Volume handle
     * @param rigidBody Rigid body to attach to
     * @param attachmentRadius Radius for finding vertices to attach
     * @param attachmentPosition Position to search for vertices
     * @return Number of vertices attached
     */
    PxU32 attachToRigidBody(
        DeformableVolumeHandle* handle,
        PxRigidActor* rigidBody,
        PxReal attachmentRadius,
        const PxVec3& attachmentPosition
    );

    // ========================================================================
    // Error Handling
    // ========================================================================

    /**
     * @brief Get last error message
     * @return Error message or empty string
     */
    std::string getLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    bool createDeformableVolumeMesh(
        const SimpleMesh& mesh,
        const DeformableVolumeConfig& config,
        PxDeformableVolumeMesh*& outMesh
    );

    void allocateAndInitializeGPUBuffers(
        DeformableVolumeHandle* handle,
        const PxTransform& transform,
        const DeformableVolumeConfig& config
    );
};

} // namespace PhysXWrapper
