/**
 * @file TriangleMeshBuilder.cpp
 * @brief Implementation of TriangleMeshBuilder class
 */

#include "Utility/TriangleMeshBuilder.h"
#include <cmath>
#include <cstdlib>
#include <chrono>

namespace PhysXWrapper {

TriangleMeshBuilder::TriangleMeshBuilder(PxPhysics* physics)
    : m_physics(physics)
{
}

TriangleMeshBuilder::~TriangleMeshBuilder() = default;

TriangleMeshResult TriangleMeshBuilder::createTriangleMesh(
    const std::vector<PxVec3>& vertices,
    const std::vector<PxU32>& indices,
    const TriangleMeshConfig& config)
{
    if (vertices.empty() || indices.empty()) {
        TriangleMeshResult result;
        result.success = false;
        result.error = "Vertices or indices are empty";
        m_lastError = result.error;
        return result;
    }

    if (indices.size() % 3 != 0) {
        TriangleMeshResult result;
        result.success = false;
        result.error = "Index count must be a multiple of 3";
        m_lastError = result.error;
        return result;
    }

    PxU32 numTriangles = static_cast<PxU32>(indices.size() / 3);
    return createTriangleMesh(
        vertices.data(),
        static_cast<PxU32>(vertices.size()),
        indices.data(),
        numTriangles,
        config
    );
}

TriangleMeshResult TriangleMeshBuilder::createTriangleMesh(
    const PxVec3* vertices,
    PxU32 numVertices,
    const PxU32* indices,
    PxU32 numTriangles,
    const TriangleMeshConfig& config)
{
    return cookTriangleMesh(vertices, numVertices, indices, numTriangles, config, nullptr);
}

TriangleMeshResult TriangleMeshBuilder::createTriangleMeshToStream(
    const std::vector<PxVec3>& vertices,
    const std::vector<PxU32>& indices,
    PxOutputStream& stream,
    const TriangleMeshConfig& config)
{
    if (vertices.empty() || indices.empty()) {
        TriangleMeshResult result;
        result.success = false;
        result.error = "Vertices or indices are empty";
        m_lastError = result.error;
        return result;
    }

    if (indices.size() % 3 != 0) {
        TriangleMeshResult result;
        result.success = false;
        result.error = "Index count must be a multiple of 3";
        m_lastError = result.error;
        return result;
    }

    PxU32 numTriangles = static_cast<PxU32>(indices.size() / 3);
    return cookTriangleMesh(
        vertices.data(),
        static_cast<PxU32>(vertices.size()),
        indices.data(),
        numTriangles,
        config,
        &stream
    );
}

PxTriangleMesh* TriangleMeshBuilder::loadTriangleMeshFromStream(PxInputStream& stream)
{
    if (!m_physics) {
        m_lastError = "PhysX not initialized";
        return nullptr;
    }

    PxTriangleMesh* mesh = m_physics->createTriangleMesh(stream);
    if (!mesh) {
        m_lastError = "Failed to load triangle mesh from stream";
    }

    return mesh;
}

bool TriangleMeshBuilder::validateMeshData(
    const PxVec3* vertices,
    PxU32 numVertices,
    const PxU32* indices,
    PxU32 numTriangles) const
{
    if (!vertices || !indices || numVertices < 3 || numTriangles < 1) {
        return false;
    }

    // Check for NaN or infinite values in vertices
    for (PxU32 i = 0; i < numVertices; i++) {
        const PxVec3& v = vertices[i];
        if (!PxIsFinite(v.x) || !PxIsFinite(v.y) || !PxIsFinite(v.z)) {
            return false;
        }
    }

    // Check that all indices are valid
    for (PxU32 i = 0; i < numTriangles * 3; i++) {
        if (indices[i] >= numVertices) {
            return false;
        }
    }

    return true;
}

TriangleMeshResult TriangleMeshBuilder::createPlane(
    PxReal width,
    PxReal depth,
    PxU32 widthSegments,
    PxU32 depthSegments,
    const TriangleMeshConfig& config)
{
    if (widthSegments < 1 || depthSegments < 1) {
        TriangleMeshResult result;
        result.success = false;
        result.error = "Segments must be >= 1";
        m_lastError = result.error;
        return result;
    }

    // Generate vertices
    PxU32 numX = widthSegments + 1;
    PxU32 numZ = depthSegments + 1;
    PxU32 numVertices = numX * numZ;
    std::vector<PxVec3> vertices(numVertices);

    PxReal halfWidth = width * 0.5f;
    PxReal halfDepth = depth * 0.5f;
    PxReal xStep = width / widthSegments;
    PxReal zStep = depth / depthSegments;

    for (PxU32 z = 0; z < numZ; z++) {
        for (PxU32 x = 0; x < numX; x++) {
            PxU32 idx = z * numX + x;
            vertices[idx] = PxVec3(
                -halfWidth + x * xStep,
                0.0f,
                -halfDepth + z * zStep
            );
        }
    }

    // Generate indices
    PxU32 numTriangles = widthSegments * depthSegments * 2;
    std::vector<PxU32> indices(numTriangles * 3);

    PxU32 idx = 0;
    for (PxU32 z = 0; z < depthSegments; z++) {
        for (PxU32 x = 0; x < widthSegments; x++) {
            PxU32 base = z * numX + x;

            // Triangle 1
            indices[idx++] = base;
            indices[idx++] = base + numX;
            indices[idx++] = base + 1;

            // Triangle 2
            indices[idx++] = base + 1;
            indices[idx++] = base + numX;
            indices[idx++] = base + numX + 1;
        }
    }

    return createTriangleMesh(vertices, indices, config);
}

TriangleMeshResult TriangleMeshBuilder::createTerrain(
    const PxVec3& origin,
    PxU32 numRows,
    PxU32 numColumns,
    PxReal cellSizeRow,
    PxReal cellSizeCol,
    PxReal heightScale,
    const TriangleMeshConfig& config)
{
    if (numRows < 1 || numColumns < 1) {
        TriangleMeshResult result;
        result.success = false;
        result.error = "Rows and columns must be >= 1";
        m_lastError = result.error;
        return result;
    }

    // Generate vertices
    PxU32 numX = numColumns + 1;
    PxU32 numZ = numRows + 1;
    PxU32 numVertices = numX * numZ;
    std::vector<PxVec3> vertices(numVertices);

    // Create grid
    for (PxU32 z = 0; z < numZ; z++) {
        for (PxU32 x = 0; x < numX; x++) {
            PxU32 idx = z * numX + x;
            vertices[idx] = PxVec3(
                origin.x + x * cellSizeRow,
                origin.y,
                origin.z + z * cellSizeCol
            );
        }
    }

    // Add random height variation
    for (PxU32 i = 0; i < numVertices; i++) {
        PxReal randomHeight = heightScale * (2.0f * (float(rand()) / float(RAND_MAX)) - 1.0f);
        vertices[i].y += randomHeight;
    }

    // Generate indices
    PxU32 numTriangles = numRows * numColumns * 2;
    std::vector<PxU32> indices(numTriangles * 3);

    PxU32 idx = 0;
    for (PxU32 z = 0; z < numRows; z++) {
        for (PxU32 x = 0; x < numColumns; x++) {
            PxU32 base = z * numX + x;

            // Triangle 1
            indices[idx++] = base + 1;
            indices[idx++] = base;
            indices[idx++] = base + numX;

            // Triangle 2
            indices[idx++] = base + numX + 1;
            indices[idx++] = base + 1;
            indices[idx++] = base + numX;
        }
    }

    return createTriangleMesh(vertices, indices, config);
}

TriangleMeshConfig TriangleMeshBuilder::getRuntimeConfig()
{
    TriangleMeshConfig config;
    config.midphase = TriangleMeshMidphase::BVH33;
    config.directInsertion = true;
    config.cleanMesh = false;  // Skip for speed
    config.precomputeActiveEdges = false;  // Skip for speed
    config.cookingPerformance = true;  // Optimize for cooking speed
    config.meshSizePerformanceTradeoff = 1.0f;  // Favor speed over size
    return config;
}

TriangleMeshConfig TriangleMeshBuilder::getOfflineConfig()
{
    TriangleMeshConfig config;
    config.midphase = TriangleMeshMidphase::BVH33;
    config.directInsertion = false;  // Serialize to stream
    config.cleanMesh = true;
    config.precomputeActiveEdges = true;
    config.cookingPerformance = false;  // Optimize for runtime performance
    config.meshSizePerformanceTradeoff = 0.0f;  // Favor smaller mesh size
    return config;
}

const std::string& TriangleMeshBuilder::getLastError() const
{
    return m_lastError;
}

TriangleMeshResult TriangleMeshBuilder::cookTriangleMesh(
    const PxVec3* vertices,
    PxU32 numVertices,
    const PxU32* indices,
    PxU32 numTriangles,
    const TriangleMeshConfig& config,
    PxOutputStream* stream)
{
    TriangleMeshResult result;

    if (!m_physics) {
        result.success = false;
        result.error = "PhysX not initialized";
        m_lastError = result.error;
        return result;
    }

    if (!validateMeshData(vertices, numVertices, indices, numTriangles)) {
        result.success = false;
        result.error = "Invalid mesh data";
        m_lastError = result.error;
        return result;
    }

    // Create cooking parameters
    PxCookingParams cookingParams = createCookingParams(config);

    // Setup triangle mesh descriptor
    PxTriangleMeshDesc meshDesc;
    meshDesc.points.count = numVertices;
    meshDesc.points.data = vertices;
    meshDesc.points.stride = sizeof(PxVec3);
    meshDesc.triangles.count = numTriangles;
    meshDesc.triangles.data = indices;
    meshDesc.triangles.stride = 3 * sizeof(PxU32);

    if (config.use16BitIndices) {
        meshDesc.flags |= PxMeshFlag::e16_BIT_INDICES;
    }

    // Validate mesh if cleanup is disabled (in debug/checked builds)
#if defined(PX_CHECKED) || defined(PX_DEBUG)
    if (!config.cleanMesh) {
        if (!PxValidateTriangleMesh(cookingParams, meshDesc)) {
            result.success = false;
            result.error = "Mesh validation failed";
            m_lastError = result.error;
            return result;
        }
    }
#endif

    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    PxTriangleMesh* triangleMesh = nullptr;

    if (stream) {
        // Cook to stream
        bool cookResult = PxCookTriangleMesh(cookingParams, meshDesc, *stream);
        if (!cookResult) {
            result.success = false;
            result.error = "Failed to cook triangle mesh to stream";
            m_lastError = result.error;
            return result;
        }

        result.success = true;
    } else if (config.directInsertion) {
        // Direct insertion
        triangleMesh = PxCreateTriangleMesh(cookingParams, meshDesc, m_physics->getPhysicsInsertionCallback());
        if (!triangleMesh) {
            result.success = false;
            result.error = "Failed to create triangle mesh with direct insertion";
            m_lastError = result.error;
            return result;
        }

        result.mesh = triangleMesh;
        result.numVertices = triangleMesh->getNbVertices();
        result.numTriangles = triangleMesh->getNbTriangles();
        result.success = true;
    } else {
        // Cook to temporary stream and create mesh
        PxDefaultMemoryOutputStream outStream;
        bool cookResult = PxCookTriangleMesh(cookingParams, meshDesc, outStream);
        if (!cookResult) {
            result.success = false;
            result.error = "Failed to cook triangle mesh";
            m_lastError = result.error;
            return result;
        }

        result.meshSize = outStream.getSize();

        // Create mesh from stream
        PxDefaultMemoryInputData inStream(outStream.getData(), outStream.getSize());
        triangleMesh = m_physics->createTriangleMesh(inStream);
        if (!triangleMesh) {
            result.success = false;
            result.error = "Failed to create triangle mesh from cooked data";
            m_lastError = result.error;
            return result;
        }

        result.mesh = triangleMesh;
        result.numVertices = triangleMesh->getNbVertices();
        result.numTriangles = triangleMesh->getNbTriangles();
        result.success = true;
    }

    // End timing
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    result.cookingTime = duration.count() / 1000.0f;  // Convert to milliseconds

    m_lastError.clear();
    return result;
}

PxCookingParams TriangleMeshBuilder::createCookingParams(const TriangleMeshConfig& config) const
{
    PxTolerancesScale scale;
    PxCookingParams params(scale);

    // Set midphase
    if (config.midphase == TriangleMeshMidphase::BVH34) {
        params.midphaseDesc = PxMeshMidPhase::eBVH34;
        params.midphaseDesc.mBVH34Desc.numPrimsPerLeaf = config.numTrisPerLeaf;
    } else {
        params.midphaseDesc = PxMeshMidPhase::eBVH33;

        // Set cooking hint
        if (config.cookingPerformance) {
            params.midphaseDesc.mBVH33Desc.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;
        } else {
            params.midphaseDesc.mBVH33Desc.meshCookingHint = PxMeshCookingHint::eSIM_PERFORMANCE;
        }

        // Set mesh size/performance tradeoff
        params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff = config.meshSizePerformanceTradeoff;
    }

    // Setup common parameters
    setupCommonCookingParams(params, config);

    if (config.vertexWeldTolerance > 0.0f) {
        params.meshWeldTolerance = config.vertexWeldTolerance;
    }

    if (config.buildGPUData) {
        params.buildGPUData = true;
    }

    return params;
}

void TriangleMeshBuilder::setupCommonCookingParams(
    PxCookingParams& params,
    const TriangleMeshConfig& config) const
{
    // Suppress remap table if requested
    if (config.suppressRemapTable) {
        params.suppressTriangleMeshRemapTable = true;
    }

    // Mesh cleanup
    if (!config.cleanMesh) {
        params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
    } else {
        params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);
    }

    // Active edges precomputation
    if (!config.precomputeActiveEdges) {
        params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
    } else {
        params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);
    }
}

} // namespace PhysXWrapper
