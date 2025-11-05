/**
 * @file ConvexMeshBuilder.h
 * @brief Simplified convex mesh creation
 *
 * This class provides a simplified interface for creating convex meshes from
 * point clouds. Based on SnippetConvexMeshCreate from PhysX SDK.
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
 * @brief Configuration for convex mesh cooking
 */
struct ConvexMeshConfig {
    /** Cooking type (default: eQUICKHULL) */
    PxConvexMeshCookingType::Enum cookingType = PxConvexMeshCookingType::eQUICKHULL;

    /** Gauss map limit (higher = no gauss map, lower = gauss map included) */
    PxU32 gaussMapLimit = 256;

    /** Use direct insertion into PhysX (faster for runtime cooking) */
    bool directInsertion = true;

    /** Inflate the convex mesh by a small amount for better collision detection */
    PxReal inflation = 0.0f;

    /** Vertex weld tolerance */
    PxReal vertexWeldTolerance = 0.0f;

    /** Area test epsilon */
    PxReal areaTestEpsilon = 0.06f;

    /** Plane tolerance */
    PxReal planeTolerance = 0.0007f;

    /** Quantize input vertices */
    bool quantizeInput = true;

    /** Check for zero area triangles */
    bool checkZeroAreaTriangles = true;

    /** Build GPU data (for GPU rigid bodies) */
    bool buildGPUData = false;
};

/**
 * @brief Result of convex mesh creation
 */
struct ConvexMeshResult {
    /** Created convex mesh (null if failed) */
    PxConvexMesh* mesh = nullptr;

    /** Number of vertices in the output hull */
    PxU32 numVertices = 0;

    /** Number of polygons in the output hull */
    PxU32 numPolygons = 0;

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
 * @brief Convex mesh builder class
 *
 * This class simplifies the creation of convex meshes from point clouds.
 * It provides:
 * - Easy-to-use interface for creating convex hulls
 * - Multiple cooking options and optimizations
 * - Direct insertion or stream serialization
 * - Detailed result information
 *
 * @example
 * @code
 * // Create builder
 * ConvexMeshBuilder builder(physics);
 *
 * // Prepare point cloud
 * std::vector<PxVec3> points = {
 *     PxVec3(0, 0, 0),
 *     PxVec3(1, 0, 0),
 *     PxVec3(0, 1, 0),
 *     PxVec3(0, 0, 1),
 *     // ... more points
 * };
 *
 * // Create convex mesh
 * ConvexMeshConfig config;
 * config.directInsertion = true;  // For runtime cooking
 * ConvexMeshResult result = builder.createConvexMesh(points, config);
 *
 * if (result.success) {
 *     // Use the mesh
 *     PxConvexMeshGeometry geom(result.mesh);
 *     PxRigidStatic* actor = physics->createRigidStatic(PxTransform(PxIdentity));
 *     PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, geom, *material);
 *     scene->addActor(*actor);
 * }
 *
 * // Mesh will be automatically released by PhysX when actors using it are released
 * @endcode
 */
class ConvexMeshBuilder {
public:
    /**
     * @brief Constructor
     * @param physics Pointer to PxPhysics instance
     */
    explicit ConvexMeshBuilder(PxPhysics* physics);

    /**
     * @brief Destructor
     */
    ~ConvexMeshBuilder();

    // Disable copy
    ConvexMeshBuilder(const ConvexMeshBuilder&) = delete;
    ConvexMeshBuilder& operator=(const ConvexMeshBuilder&) = delete;

    /**
     * @brief Create convex mesh from point cloud
     * @param points Vector of 3D points
     * @param config Cooking configuration
     * @return Result containing the created mesh and statistics
     */
    ConvexMeshResult createConvexMesh(
        const std::vector<PxVec3>& points,
        const ConvexMeshConfig& config = ConvexMeshConfig()
    );

    /**
     * @brief Create convex mesh from point array
     * @param points Pointer to array of 3D points
     * @param numPoints Number of points in the array
     * @param config Cooking configuration
     * @return Result containing the created mesh and statistics
     */
    ConvexMeshResult createConvexMesh(
        const PxVec3* points,
        PxU32 numPoints,
        const ConvexMeshConfig& config = ConvexMeshConfig()
    );

    /**
     * @brief Create convex mesh and save to stream
     * @param points Vector of 3D points
     * @param stream Output stream to write serialized mesh
     * @param config Cooking configuration
     * @return Result containing statistics (mesh pointer will be null)
     */
    ConvexMeshResult createConvexMeshToStream(
        const std::vector<PxVec3>& points,
        PxOutputStream& stream,
        const ConvexMeshConfig& config = ConvexMeshConfig()
    );

    /**
     * @brief Load convex mesh from stream
     * @param stream Input stream containing serialized mesh
     * @return Loaded convex mesh (null if failed)
     */
    PxConvexMesh* loadConvexMeshFromStream(PxInputStream& stream);

    /**
     * @brief Validate point cloud before cooking
     * @param points Pointer to array of 3D points
     * @param numPoints Number of points
     * @return True if points are valid for convex mesh creation
     */
    bool validatePointCloud(const PxVec3* points, PxU32 numPoints) const;

    /**
     * @brief Create a box-shaped convex mesh
     * @param halfExtents Half extents of the box
     * @param config Cooking configuration
     * @return Result containing the created mesh
     */
    ConvexMeshResult createBox(
        const PxVec3& halfExtents,
        const ConvexMeshConfig& config = ConvexMeshConfig()
    );

    /**
     * @brief Create a cylinder-shaped convex mesh (approximation)
     * @param radius Radius of the cylinder
     * @param halfHeight Half height of the cylinder
     * @param numSegments Number of segments around the circumference
     * @param config Cooking configuration
     * @return Result containing the created mesh
     */
    ConvexMeshResult createCylinder(
        PxReal radius,
        PxReal halfHeight,
        PxU32 numSegments,
        const ConvexMeshConfig& config = ConvexMeshConfig()
    );

    /**
     * @brief Create a cone-shaped convex mesh (approximation)
     * @param radius Base radius of the cone
     * @param height Height of the cone
     * @param numSegments Number of segments around the base
     * @param config Cooking configuration
     * @return Result containing the created mesh
     */
    ConvexMeshResult createCone(
        PxReal radius,
        PxReal height,
        PxU32 numSegments,
        const ConvexMeshConfig& config = ConvexMeshConfig()
    );

    /**
     * @brief Get default configuration for runtime cooking
     * @return Configuration optimized for runtime (fast cooking)
     */
    static ConvexMeshConfig getRuntimeConfig();

    /**
     * @brief Get default configuration for offline cooking
     * @return Configuration optimized for quality (slower cooking)
     */
    static ConvexMeshConfig getOfflineConfig();

    /**
     * @brief Get last error message
     * @return Error message string
     */
    const std::string& getLastError() const;

private:
    PxPhysics* m_physics;
    std::string m_lastError;

    // Internal helper for cooking
    ConvexMeshResult cookConvexMesh(
        const PxVec3* points,
        PxU32 numPoints,
        const ConvexMeshConfig& config,
        PxOutputStream* stream = nullptr
    );

    // Helper to create cooking params
    PxCookingParams createCookingParams(const ConvexMeshConfig& config) const;
};

/**
 * @brief Helper function to generate random point cloud
 * @param numPoints Number of points to generate
 * @param minBounds Minimum bounds of the volume
 * @param maxBounds Maximum bounds of the volume
 * @return Vector of randomly generated points
 */
std::vector<PxVec3> generateRandomPointCloud(
    PxU32 numPoints,
    const PxVec3& minBounds,
    const PxVec3& maxBounds
);

} // namespace PhysXWrapper
