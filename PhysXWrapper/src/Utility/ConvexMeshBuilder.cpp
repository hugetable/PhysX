/**
 * @file ConvexMeshBuilder.cpp
 * @brief Implementation of ConvexMeshBuilder class
 */

#include "Utility/ConvexMeshBuilder.h"
#include <cmath>
#include <cstdlib>
#include <chrono>

namespace PhysXWrapper {

ConvexMeshBuilder::ConvexMeshBuilder(PxPhysics* physics)
    : m_physics(physics)
{
}

ConvexMeshBuilder::~ConvexMeshBuilder() = default;

ConvexMeshResult ConvexMeshBuilder::createConvexMesh(
    const std::vector<PxVec3>& points,
    const ConvexMeshConfig& config)
{
    if (points.empty()) {
        ConvexMeshResult result;
        result.success = false;
        result.error = "Point cloud is empty";
        m_lastError = result.error;
        return result;
    }

    return createConvexMesh(points.data(), static_cast<PxU32>(points.size()), config);
}

ConvexMeshResult ConvexMeshBuilder::createConvexMesh(
    const PxVec3* points,
    PxU32 numPoints,
    const ConvexMeshConfig& config)
{
    return cookConvexMesh(points, numPoints, config, nullptr);
}

ConvexMeshResult ConvexMeshBuilder::createConvexMeshToStream(
    const std::vector<PxVec3>& points,
    PxOutputStream& stream,
    const ConvexMeshConfig& config)
{
    if (points.empty()) {
        ConvexMeshResult result;
        result.success = false;
        result.error = "Point cloud is empty";
        m_lastError = result.error;
        return result;
    }

    return cookConvexMesh(points.data(), static_cast<PxU32>(points.size()), config, &stream);
}

PxConvexMesh* ConvexMeshBuilder::loadConvexMeshFromStream(PxInputStream& stream)
{
    if (!m_physics) {
        m_lastError = "PhysX not initialized";
        return nullptr;
    }

    PxConvexMesh* mesh = m_physics->createConvexMesh(stream);
    if (!mesh) {
        m_lastError = "Failed to load convex mesh from stream";
    }

    return mesh;
}

bool ConvexMeshBuilder::validatePointCloud(const PxVec3* points, PxU32 numPoints) const
{
    if (!points || numPoints < 4) {
        return false;  // Need at least 4 points for a tetrahedron
    }

    // Check for NaN or infinite values
    for (PxU32 i = 0; i < numPoints; i++) {
        const PxVec3& p = points[i];
        if (!PxIsFinite(p.x) || !PxIsFinite(p.y) || !PxIsFinite(p.z)) {
            return false;
        }
    }

    // Check if all points are coplanar (simple check: not all points have same Z)
    bool allSameX = true, allSameY = true, allSameZ = true;
    const PxVec3& first = points[0];

    for (PxU32 i = 1; i < numPoints; i++) {
        if (PxAbs(points[i].x - first.x) > 1e-6f) allSameX = false;
        if (PxAbs(points[i].y - first.y) > 1e-6f) allSameY = false;
        if (PxAbs(points[i].z - first.z) > 1e-6f) allSameZ = false;
    }

    // If all points have same coordinate in any dimension, they're coplanar
    if (allSameX || allSameY || allSameZ) {
        return false;
    }

    return true;
}

ConvexMeshResult ConvexMeshBuilder::createBox(
    const PxVec3& halfExtents,
    const ConvexMeshConfig& config)
{
    // Create 8 vertices for a box
    std::vector<PxVec3> vertices(8);
    vertices[0] = PxVec3(-halfExtents.x, -halfExtents.y, -halfExtents.z);
    vertices[1] = PxVec3( halfExtents.x, -halfExtents.y, -halfExtents.z);
    vertices[2] = PxVec3(-halfExtents.x,  halfExtents.y, -halfExtents.z);
    vertices[3] = PxVec3( halfExtents.x,  halfExtents.y, -halfExtents.z);
    vertices[4] = PxVec3(-halfExtents.x, -halfExtents.y,  halfExtents.z);
    vertices[5] = PxVec3( halfExtents.x, -halfExtents.y,  halfExtents.z);
    vertices[6] = PxVec3(-halfExtents.x,  halfExtents.y,  halfExtents.z);
    vertices[7] = PxVec3( halfExtents.x,  halfExtents.y,  halfExtents.z);

    return createConvexMesh(vertices, config);
}

ConvexMeshResult ConvexMeshBuilder::createCylinder(
    PxReal radius,
    PxReal halfHeight,
    PxU32 numSegments,
    const ConvexMeshConfig& config)
{
    if (numSegments < 3) {
        ConvexMeshResult result;
        result.success = false;
        result.error = "Cylinder needs at least 3 segments";
        m_lastError = result.error;
        return result;
    }

    // Create vertices: top circle + bottom circle + center points
    std::vector<PxVec3> vertices;
    vertices.reserve(numSegments * 2 + 2);

    const PxReal angleStep = PxTwoPi / numSegments;

    // Top circle
    for (PxU32 i = 0; i < numSegments; i++) {
        PxReal angle = i * angleStep;
        vertices.push_back(PxVec3(
            radius * PxCos(angle),
            halfHeight,
            radius * PxSin(angle)
        ));
    }

    // Bottom circle
    for (PxU32 i = 0; i < numSegments; i++) {
        PxReal angle = i * angleStep;
        vertices.push_back(PxVec3(
            radius * PxCos(angle),
            -halfHeight,
            radius * PxSin(angle)
        ));
    }

    return createConvexMesh(vertices, config);
}

ConvexMeshResult ConvexMeshBuilder::createCone(
    PxReal radius,
    PxReal height,
    PxU32 numSegments,
    const ConvexMeshConfig& config)
{
    if (numSegments < 3) {
        ConvexMeshResult result;
        result.success = false;
        result.error = "Cone needs at least 3 segments";
        m_lastError = result.error;
        return result;
    }

    // Create vertices: apex + base circle
    std::vector<PxVec3> vertices;
    vertices.reserve(numSegments + 1);

    // Apex at top
    vertices.push_back(PxVec3(0, height, 0));

    // Base circle
    const PxReal angleStep = PxTwoPi / numSegments;
    for (PxU32 i = 0; i < numSegments; i++) {
        PxReal angle = i * angleStep;
        vertices.push_back(PxVec3(
            radius * PxCos(angle),
            0,
            radius * PxSin(angle)
        ));
    }

    return createConvexMesh(vertices, config);
}

ConvexMeshConfig ConvexMeshBuilder::getRuntimeConfig()
{
    ConvexMeshConfig config;
    config.cookingType = PxConvexMeshCookingType::eQUICKHULL;
    config.gaussMapLimit = 256;  // No gauss map for faster cooking
    config.directInsertion = true;
    config.quantizeInput = false;  // Skip quantization for speed
    config.checkZeroAreaTriangles = false;  // Skip checks for speed
    return config;
}

ConvexMeshConfig ConvexMeshBuilder::getOfflineConfig()
{
    ConvexMeshConfig config;
    config.cookingType = PxConvexMeshCookingType::eQUICKHULL;
    config.gaussMapLimit = 16;  // Include gauss map for better quality
    config.directInsertion = false;  // Serialize to stream
    config.quantizeInput = true;
    config.checkZeroAreaTriangles = true;
    return config;
}

const std::string& ConvexMeshBuilder::getLastError() const
{
    return m_lastError;
}

ConvexMeshResult ConvexMeshBuilder::cookConvexMesh(
    const PxVec3* points,
    PxU32 numPoints,
    const ConvexMeshConfig& config,
    PxOutputStream* stream)
{
    ConvexMeshResult result;

    if (!m_physics) {
        result.success = false;
        result.error = "PhysX not initialized";
        m_lastError = result.error;
        return result;
    }

    if (!validatePointCloud(points, numPoints)) {
        result.success = false;
        result.error = "Invalid point cloud";
        m_lastError = result.error;
        return result;
    }

    // Create cooking parameters
    PxCookingParams cookingParams = createCookingParams(config);

    // Setup convex mesh descriptor
    PxConvexMeshDesc desc;
    desc.points.data = points;
    desc.points.count = numPoints;
    desc.points.stride = sizeof(PxVec3);
    desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

    if (config.quantizeInput) {
        desc.flags |= PxConvexFlag::eQUANTIZE_INPUT;
    }

    if (config.checkZeroAreaTriangles) {
        desc.flags |= PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES;
    }

    // PhysX 5.x: eGPU_COMPATIBLE flag has been removed
    // GPU compatibility is handled differently in PhysX 5.x
    /*
    if (config.buildGPUData) {
        desc.flags |= PxConvexFlag::eGPU_COMPATIBLE;
    }
    */

    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    PxConvexMesh* convexMesh = nullptr;

    if (stream) {
        // Cook to stream
        bool cookResult = PxCookConvexMesh(cookingParams, desc, *stream);
        if (!cookResult) {
            result.success = false;
            result.error = "Failed to cook convex mesh to stream";
            m_lastError = result.error;
            return result;
        }

        result.success = true;
    } else if (config.directInsertion) {
        // Direct insertion
        convexMesh = PxCreateConvexMesh(cookingParams, desc, m_physics->getPhysicsInsertionCallback());
        if (!convexMesh) {
            result.success = false;
            result.error = "Failed to create convex mesh with direct insertion";
            m_lastError = result.error;
            return result;
        }

        result.mesh = convexMesh;
        result.numVertices = convexMesh->getNbVertices();
        result.numPolygons = convexMesh->getNbPolygons();
        result.success = true;
    } else {
        // Cook to temporary stream and create mesh
        PxDefaultMemoryOutputStream outStream;
        bool cookResult = PxCookConvexMesh(cookingParams, desc, outStream);
        if (!cookResult) {
            result.success = false;
            result.error = "Failed to cook convex mesh";
            m_lastError = result.error;
            return result;
        }

        result.meshSize = outStream.getSize();

        // Create mesh from stream
        PxDefaultMemoryInputData inStream(outStream.getData(), outStream.getSize());
        convexMesh = m_physics->createConvexMesh(inStream);
        if (!convexMesh) {
            result.success = false;
            result.error = "Failed to create convex mesh from cooked data";
            m_lastError = result.error;
            return result;
        }

        result.mesh = convexMesh;
        result.numVertices = convexMesh->getNbVertices();
        result.numPolygons = convexMesh->getNbPolygons();
        result.success = true;
    }

    // End timing
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    result.cookingTime = duration.count() / 1000.0f;  // Convert to milliseconds

    m_lastError.clear();
    return result;
}

PxCookingParams ConvexMeshBuilder::createCookingParams(const ConvexMeshConfig& config) const
{
    PxTolerancesScale scale;
    PxCookingParams params(scale);

    params.convexMeshCookingType = config.cookingType;
    params.gaussMapLimit = config.gaussMapLimit;

    // PhysX 5.x: skinWidth parameter has been removed from PxCookingParams
    // Inflation is now handled differently in the convex mesh descriptor
    /*
    if (config.inflation > 0.0f) {
        params.skinWidth = config.inflation;
    }
    */

    if (config.vertexWeldTolerance > 0.0f) {
        params.meshWeldTolerance = config.vertexWeldTolerance;
    }

    if (config.areaTestEpsilon > 0.0f) {
        params.areaTestEpsilon = config.areaTestEpsilon;
    }

    if (config.planeTolerance > 0.0f) {
        params.planeTolerance = config.planeTolerance;
    }

    return params;
}

std::vector<PxVec3> generateRandomPointCloud(
    PxU32 numPoints,
    const PxVec3& minBounds,
    const PxVec3& maxBounds)
{
    std::vector<PxVec3> points;
    points.reserve(numPoints);

    for (PxU32 i = 0; i < numPoints; i++) {
        PxReal x = minBounds.x + (maxBounds.x - minBounds.x) * (float(rand()) / float(RAND_MAX));
        PxReal y = minBounds.y + (maxBounds.y - minBounds.y) * (float(rand()) / float(RAND_MAX));
        PxReal z = minBounds.z + (maxBounds.z - minBounds.z) * (float(rand()) / float(RAND_MAX));
        points.push_back(PxVec3(x, y, z));
    }

    return points;
}

} // namespace PhysXWrapper
