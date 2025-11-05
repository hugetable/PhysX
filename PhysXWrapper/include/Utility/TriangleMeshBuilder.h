/**
 * @file TriangleMeshBuilder.h
 * @brief Simplified triangle mesh creation
 *
 * This class provides a simplified interface for creating triangle meshes from
 * vertices and indices. Based on SnippetTriangleMeshCreate from PhysX SDK.
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
 * @brief Midphase algorithm selection
 */
enum class TriangleMeshMidphase {
    BVH33,  ///< BVH33 midphase (default, good balance)
    BVH34   ///< BVH34 midphase (newer, potentially better performance)
};

/**
 * @brief Configuration for triangle mesh cooking
 */
struct TriangleMeshConfig {
    /** Midphase algorithm (default: BVH33) */
    TriangleMeshMidphase midphase = TriangleMeshMidphase::BVH33;

    /** Use direct insertion into PhysX (faster for runtime cooking) */
    bool directInsertion = true;

    /** Clean the mesh (remove duplicate vertices, etc.) */
    bool cleanMesh = true;

    /** Precompute active edges for better collision */
    bool precomputeActiveEdges = true;

    /** Cooking performance hint (true = faster cooking, false = better runtime) */
    bool cookingPerformance = false;

    /** Mesh size/performance tradeoff (0.0 = smaller mesh, 1.0 = faster runtime) */
    PxReal meshSizePerformanceTradeoff = 0.55f;

    /** Number of triangles per leaf (BVH34 only, default: 4) */
    PxU32 numTrisPerLeaf = 4;

    /** Suppress triangle mesh remap table */
    bool suppressRemapTable = true;

    /** Vertex weld tolerance */
    PxReal vertexWeldTolerance = 0.0f;

    /** Build 16-bit indices (saves memory but limits vertex count) */
    bool use16BitIndices = false;

    /** Build GPU data (for GPU rigid bodies) */
    bool buildGPUData = false;
};

/**
 * @brief Result of triangle mesh creation
 */
struct TriangleMeshResult {
    /** Created triangle mesh (null if failed) */
    PxTriangleMesh* mesh = nullptr;

    /** Number of vertices in the output mesh */
    PxU32 numVertices = 0;

    /** Number of triangles in the output mesh */
    PxU32 numTriangles = 0;

    /** Size of serialized mesh data (only if directInsertion=false) */
    PxU32 meshSize = 0;

    /** Cooking time in milliseconds */
    float cookingTime = 0.0f;

    /** Success flag */
    bool success = false;

    /** Error message (if any) */
    std::string error;
};

/**
 * @brief Triangle mesh builder class
 *
 * This class simplifies the creation of triangle meshes for static collision geometry.
 * It provides:
 * - Easy-to-use interface for creating triangle meshes
 * - Multiple midphase algorithms (BVH33, BVH34)
 * - Various cooking options and optimizations
 * - Direct insertion or stream serialization
 * - Detailed result information
 *
 * @example
 * @code
 * // Create builder
 * TriangleMeshBuilder builder(physics);
 *
 * // Prepare mesh data (simple terrain)
 * std::vector<PxVec3> vertices;
 * std::vector<PxU32> indices;
 * // ... fill with vertex and index data
 *
 * // Create triangle mesh
 * TriangleMeshConfig config;
 * config.directInsertion = true;  // For runtime cooking
 * TriangleMeshResult result = builder.createTriangleMesh(vertices, indices, config);
 *
 * if (result.success) {
 *     // Use the mesh
 *     PxTriangleMeshGeometry geom(result.mesh);
 *     PxRigidStatic* actor = physics->createRigidStatic(PxTransform(PxIdentity));
 *     PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, geom, *material);
 *     scene->addActor(*actor);
 * }
 *
 * // Mesh will be automatically released by PhysX when actors using it are released
 * @endcode
 */
class TriangleMeshBuilder {
public:
    /**
     * @brief Constructor
     * @param physics Pointer to PxPhysics instance
     */
    explicit TriangleMeshBuilder(PxPhysics* physics);

    /**
     * @brief Destructor
     */
    ~TriangleMeshBuilder();

    // Disable copy
    TriangleMeshBuilder(const TriangleMeshBuilder&) = delete;
    TriangleMeshBuilder& operator=(const TriangleMeshBuilder&) = delete;

    /**
     * @brief Create triangle mesh from vertices and indices
     * @param vertices Vector of 3D vertices
     * @param indices Vector of triangle indices (3 per triangle)
     * @param config Cooking configuration
     * @return Result containing the created mesh and statistics
     */
    TriangleMeshResult createTriangleMesh(
        const std::vector<PxVec3>& vertices,
        const std::vector<PxU32>& indices,
        const TriangleMeshConfig& config = TriangleMeshConfig()
    );

    /**
     * @brief Create triangle mesh from arrays
     * @param vertices Pointer to array of 3D vertices
     * @param numVertices Number of vertices
     * @param indices Pointer to array of triangle indices (3 per triangle)
     * @param numTriangles Number of triangles
     * @param config Cooking configuration
     * @return Result containing the created mesh and statistics
     */
    TriangleMeshResult createTriangleMesh(
        const PxVec3* vertices,
        PxU32 numVertices,
        const PxU32* indices,
        PxU32 numTriangles,
        const TriangleMeshConfig& config = TriangleMeshConfig()
    );

    /**
     * @brief Create triangle mesh and save to stream
     * @param vertices Vector of 3D vertices
     * @param indices Vector of triangle indices
     * @param stream Output stream to write serialized mesh
     * @param config Cooking configuration
     * @return Result containing statistics (mesh pointer will be null)
     */
    TriangleMeshResult createTriangleMeshToStream(
        const std::vector<PxVec3>& vertices,
        const std::vector<PxU32>& indices,
        PxOutputStream& stream,
        const TriangleMeshConfig& config = TriangleMeshConfig()
    );

    /**
     * @brief Load triangle mesh from stream
     * @param stream Input stream containing serialized mesh
     * @return Loaded triangle mesh (null if failed)
     */
    PxTriangleMesh* loadTriangleMeshFromStream(PxInputStream& stream);

    /**
     * @brief Validate mesh data before cooking
     * @param vertices Pointer to array of vertices
     * @param numVertices Number of vertices
     * @param indices Pointer to array of indices
     * @param numTriangles Number of triangles
     * @return True if mesh data is valid
     */
    bool validateMeshData(
        const PxVec3* vertices,
        PxU32 numVertices,
        const PxU32* indices,
        PxU32 numTriangles
    ) const;

    /**
     * @brief Create a plane mesh
     * @param width Width of the plane
     * @param depth Depth of the plane
     * @param widthSegments Number of segments along width
     * @param depthSegments Number of segments along depth
     * @param config Cooking configuration
     * @return Result containing the created mesh
     */
    TriangleMeshResult createPlane(
        PxReal width,
        PxReal depth,
        PxU32 widthSegments,
        PxU32 depthSegments,
        const TriangleMeshConfig& config = TriangleMeshConfig()
    );

    /**
     * @brief Create a terrain mesh with random heights
     * @param origin Origin position
     * @param numRows Number of rows
     * @param numColumns Number of columns
     * @param cellSizeRow Row cell size
     * @param cellSizeCol Column cell size
     * @param heightScale Height variation scale
     * @param config Cooking configuration
     * @return Result containing the created mesh
     */
    TriangleMeshResult createTerrain(
        const PxVec3& origin,
        PxU32 numRows,
        PxU32 numColumns,
        PxReal cellSizeRow,
        PxReal cellSizeCol,
        PxReal heightScale,
        const TriangleMeshConfig& config = TriangleMeshConfig()
    );

    /**
     * @brief Get default configuration for runtime cooking
     * @return Configuration optimized for runtime (fast cooking)
     */
    static TriangleMeshConfig getRuntimeConfig();

    /**
     * @brief Get default configuration for offline cooking
     * @return Configuration optimized for quality (slower cooking, better runtime)
     */
    static TriangleMeshConfig getOfflineConfig();

    /**
     * @brief Get last error message
     * @return Error message string
     */
    const std::string& getLastError() const;

private:
    PxPhysics* m_physics;
    std::string m_lastError;

    // Internal helper for cooking
    TriangleMeshResult cookTriangleMesh(
        const PxVec3* vertices,
        PxU32 numVertices,
        const PxU32* indices,
        PxU32 numTriangles,
        const TriangleMeshConfig& config,
        PxOutputStream* stream = nullptr
    );

    // Helper to create cooking params
    PxCookingParams createCookingParams(const TriangleMeshConfig& config) const;

    // Helper to setup common cooking parameters
    void setupCommonCookingParams(
        PxCookingParams& params,
        const TriangleMeshConfig& config
    ) const;
};

} // namespace PhysXWrapper
