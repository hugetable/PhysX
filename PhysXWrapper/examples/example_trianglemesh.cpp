/**
 * @file example_trianglemesh.cpp
 * @brief Triangle mesh creation example using TriangleMeshBuilder
 *
 * This example demonstrates:
 * - Creating triangle meshes from vertices and indices
 * - Using different midphase algorithms (BVH33, BVH34)
 * - Using different cooking configurations
 * - Creating predefined shapes (plane, terrain)
 * - Performance comparison of different settings
 * - Using triangle meshes in simulation
 *
 * Based on SnippetTriangleMeshCreate from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include "Utility/TriangleMeshBuilder.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>

using namespace PhysXWrapper;
using namespace physx;

/**
 * @brief Test creating triangle meshes with different midphase algorithms
 */
void testMidphaseComparison(PhysXCore& physics, TriangleMeshBuilder& builder) {
    std::cout << "\n=== TEST 1: Midphase Algorithm Comparison ===" << std::endl;

    // Create test terrain data (medium size)
    const PxU32 numRows = 64;
    const PxU32 numCols = 64;

    std::cout << "\nGenerating terrain (" << numRows << "x" << numCols << " = "
              << numRows * numCols * 2 << " triangles)..." << std::endl;

    // Test BVH33
    std::cout << "\n1.1 BVH33 Midphase (default settings):" << std::endl;
    TriangleMeshConfig config33;
    config33.midphase = TriangleMeshMidphase::BVH33;
    config33.directInsertion = true;

    TriangleMeshResult result33 = builder.createTerrain(
        PxVec3(0, 0, 0), numRows, numCols, 1.0f, 1.0f, 2.0f, config33
    );

    if (result33.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Output vertices: " << result33.numVertices << std::endl;
        std::cout << "  Output triangles: " << result33.numTriangles << std::endl;
        std::cout << "  Cooking time: " << std::fixed << std::setprecision(3)
                  << result33.cookingTime << " ms" << std::endl;
        if (result33.mesh) result33.mesh->release();
    } else {
        std::cerr << "  FAILED: " << result33.error << std::endl;
    }

    // Test BVH34
    std::cout << "\n1.2 BVH34 Midphase (4 tris per leaf):" << std::endl;
    TriangleMeshConfig config34;
    config34.midphase = TriangleMeshMidphase::BVH34;
    config34.directInsertion = true;
    config34.numTrisPerLeaf = 4;

    TriangleMeshResult result34 = builder.createTerrain(
        PxVec3(0, 0, 0), numRows, numCols, 1.0f, 1.0f, 2.0f, config34
    );

    if (result34.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Output vertices: " << result34.numVertices << std::endl;
        std::cout << "  Output triangles: " << result34.numTriangles << std::endl;
        std::cout << "  Cooking time: " << std::fixed << std::setprecision(3)
                  << result34.cookingTime << " ms" << std::endl;
        if (result34.mesh) result34.mesh->release();
    } else {
        std::cerr << "  FAILED: " << result34.error << std::endl;
    }
}

/**
 * @brief Test cooking configuration performance
 */
void testCookingConfigurations(PhysXCore& physics, TriangleMeshBuilder& builder) {
    std::cout << "\n=== TEST 2: Cooking Configuration Performance ===" << std::endl;

    const PxU32 numRows = 128;
    const PxU32 numCols = 128;

    std::cout << "\nTerrain size: " << numRows << "x" << numCols << " = "
              << numRows * numCols * 2 << " triangles" << std::endl;

    // Test offline config (high quality)
    std::cout << "\n2.1 Offline cooking (high quality, slower):" << std::endl;
    TriangleMeshConfig offlineConfig = TriangleMeshBuilder::getOfflineConfig();
    offlineConfig.directInsertion = true;  // Override for this test

    TriangleMeshResult offlineResult = builder.createTerrain(
        PxVec3(0, 0, 0), numRows, numCols, 1.0f, 1.0f, 1.0f, offlineConfig
    );

    if (offlineResult.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Vertices: " << offlineResult.numVertices << std::endl;
        std::cout << "  Triangles: " << offlineResult.numTriangles << std::endl;
        std::cout << "  Cooking time: " << std::fixed << std::setprecision(3)
                  << offlineResult.cookingTime << " ms" << std::endl;
        if (offlineResult.mesh) offlineResult.mesh->release();
    } else {
        std::cerr << "  FAILED: " << offlineResult.error << std::endl;
    }

    // Test runtime config (fast cooking)
    std::cout << "\n2.2 Runtime cooking (fast, lower quality):" << std::endl;
    TriangleMeshConfig runtimeConfig = TriangleMeshBuilder::getRuntimeConfig();

    TriangleMeshResult runtimeResult = builder.createTerrain(
        PxVec3(0, 0, 0), numRows, numCols, 1.0f, 1.0f, 1.0f, runtimeConfig
    );

    if (runtimeResult.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Vertices: " << runtimeResult.numVertices << std::endl;
        std::cout << "  Triangles: " << runtimeResult.numTriangles << std::endl;
        std::cout << "  Cooking time: " << std::fixed << std::setprecision(3)
                  << runtimeResult.cookingTime << " ms" << std::endl;

        if (offlineResult.success && offlineResult.cookingTime > 0) {
            float speedup = offlineResult.cookingTime / runtimeResult.cookingTime;
            std::cout << "  Speedup: " << std::fixed << std::setprecision(2)
                      << speedup << "x faster" << std::endl;
        }

        if (runtimeResult.mesh) runtimeResult.mesh->release();
    } else {
        std::cerr << "  FAILED: " << runtimeResult.error << std::endl;
    }
}

/**
 * @brief Test predefined shapes
 */
void testPredefinedShapes(PhysXCore& physics, TriangleMeshBuilder& builder) {
    std::cout << "\n=== TEST 3: Predefined Shapes ===" << std::endl;

    TriangleMeshConfig config;
    config.directInsertion = true;

    // Create plane
    std::cout << "\n3.1 Plane mesh (10x10, 4x4 segments):" << std::endl;
    TriangleMeshResult planeResult = builder.createPlane(10.0f, 10.0f, 4, 4, config);

    if (planeResult.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Vertices: " << planeResult.numVertices << std::endl;
        std::cout << "  Triangles: " << planeResult.numTriangles << std::endl;
        std::cout << "  Cooking time: " << planeResult.cookingTime << " ms" << std::endl;
        if (planeResult.mesh) planeResult.mesh->release();
    } else {
        std::cerr << "  FAILED: " << planeResult.error << std::endl;
    }

    // Create small terrain
    std::cout << "\n3.2 Terrain mesh (16x16 grid, height variation 5.0):" << std::endl;
    TriangleMeshResult terrainResult = builder.createTerrain(
        PxVec3(-8.0f, 0.0f, -8.0f),
        16, 16,
        1.0f, 1.0f,
        5.0f,
        config
    );

    if (terrainResult.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Vertices: " << terrainResult.numVertices << std::endl;
        std::cout << "  Triangles: " << terrainResult.numTriangles << std::endl;
        std::cout << "  Cooking time: " << terrainResult.cookingTime << " ms" << std::endl;
        if (terrainResult.mesh) terrainResult.mesh->release();
    } else {
        std::cerr << "  FAILED: " << terrainResult.error << std::endl;
    }
}

/**
 * @brief Test stream serialization
 */
void testStreamSerialization(PhysXCore& physics, TriangleMeshBuilder& builder) {
    std::cout << "\n=== TEST 4: Stream Serialization ===" << std::endl;

    // Create vertices and indices for terrain
    std::vector<PxVec3> vertices;
    std::vector<PxU32> indices;

    // Generate simple terrain (32x32 grid)
    const PxU32 numRows = 32;
    const PxU32 numCols = 32;
    const PxU32 numX = numCols + 1;
    const PxU32 numZ = numRows + 1;

    // Generate vertices with random heights
    for (PxU32 z = 0; z < numZ; z++) {
        for (PxU32 x = 0; x < numX; x++) {
            float height = 2.0f * (2.0f * (float(rand()) / float(RAND_MAX)) - 1.0f);
            vertices.push_back(PxVec3(x * 1.0f, height, z * 1.0f));
        }
    }

    // Generate indices
    for (PxU32 z = 0; z < numRows; z++) {
        for (PxU32 x = 0; x < numCols; x++) {
            PxU32 base = z * numX + x;
            indices.push_back(base + 1);
            indices.push_back(base);
            indices.push_back(base + numX);
            indices.push_back(base + numX + 1);
            indices.push_back(base + 1);
            indices.push_back(base + numX);
        }
    }

    // Cook to stream
    std::cout << "\n4.1 Cooking terrain to stream..." << std::endl;
    PxDefaultMemoryOutputStream stream;
    TriangleMeshConfig config = TriangleMeshBuilder::getOfflineConfig();

    TriangleMeshResult cookResult = builder.createTriangleMeshToStream(
        vertices, indices, stream, config
    );

    if (cookResult.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Stream size: " << stream.getSize() << " bytes" << std::endl;
        std::cout << "  Cooking time: " << cookResult.cookingTime << " ms" << std::endl;

        // Load from stream
        std::cout << "\n4.2 Loading from stream..." << std::endl;
        PxDefaultMemoryInputData inStream(stream.getData(), stream.getSize());
        PxTriangleMesh* loadedMesh = builder.loadTriangleMeshFromStream(inStream);

        if (loadedMesh) {
            std::cout << "  SUCCESS!" << std::endl;
            std::cout << "  Loaded vertices: " << loadedMesh->getNbVertices() << std::endl;
            std::cout << "  Loaded triangles: " << loadedMesh->getNbTriangles() << std::endl;
            loadedMesh->release();
        } else {
            std::cerr << "  FAILED to load from stream" << std::endl;
        }
    } else {
        std::cerr << "  FAILED: " << cookResult.error << std::endl;
    }
}

/**
 * @brief Test using triangle mesh in simulation
 */
void testInSimulation(PhysXCore& physics, TriangleMeshBuilder& builder) {
    std::cout << "\n=== TEST 5: Using Triangle Mesh in Simulation ===" << std::endl;

    PxScene* scene = physics.getScene();
    PxPhysics* physicsSDK = physics.getPhysics();
    PxMaterial* material = physics.getDefaultMaterial();

    if (!scene || !physicsSDK || !material) {
        std::cerr << "Physics not properly initialized" << std::endl;
        return;
    }

    // Create terrain mesh
    std::cout << "\nCreating terrain (32x32 grid)..." << std::endl;
    TriangleMeshConfig config;
    config.directInsertion = true;

    TriangleMeshResult terrainResult = builder.createTerrain(
        PxVec3(-16.0f, 0.0f, -16.0f),
        32, 32,
        1.0f, 1.0f,
        3.0f,  // Height variation
        config
    );

    if (!terrainResult.success) {
        std::cerr << "Failed to create terrain: " << terrainResult.error << std::endl;
        return;
    }

    std::cout << "Terrain created successfully!" << std::endl;
    std::cout << "  Vertices: " << terrainResult.numVertices << std::endl;
    std::cout << "  Triangles: " << terrainResult.numTriangles << std::endl;

    // Create static actor with the triangle mesh
    PxTriangleMeshGeometry geom(terrainResult.mesh);
    PxRigidStatic* terrainActor = physicsSDK->createRigidStatic(PxTransform(PxIdentity));
    PxShape* terrainShape = physicsSDK->createShape(geom, *material);
    terrainActor->attachShape(*terrainShape);
    scene->addActor(*terrainActor);
    terrainShape->release();

    std::cout << "Terrain added to scene!" << std::endl;

    // Create some dynamic spheres to fall onto the terrain
    std::cout << "\nAdding 5 dynamic spheres..." << std::endl;
    for (int i = 0; i < 5; i++) {
        PxVec3 pos(
            -8.0f + i * 4.0f,
            15.0f + i * 2.0f,
            0.0f
        );
        PxRigidDynamic* sphere = physics.createDynamic(
            PxTransform(pos),
            PxSphereGeometry(0.5f),
            1.0f
        );
        if (sphere) {
            scene->addActor(*sphere);
        }
    }

    std::cout << "\nRunning simulation (5 seconds)..." << std::endl;

    // Simulate
    const float timeStep = 1.0f / 60.0f;
    const int frameCount = 300;

    for (int i = 0; i < frameCount; i++) {
        physics.update(timeStep);

        // Print status every second
        if (i % 60 == 0) {
            PxU32 numDynamic = 0;
            PxU32 numStatic = 0;

            PxActor* actors[128];
            PxU32 numActors = scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                                                actors, 128);

            for (PxU32 j = 0; j < numActors; j++) {
                if (actors[j]->is<PxRigidDynamic>()) numDynamic++;
                if (actors[j]->is<PxRigidStatic>()) numStatic++;
            }

            std::cout << "  Frame " << i << ": "
                      << numStatic << " static, "
                      << numDynamic << " dynamic actors" << std::endl;
        }
    }

    std::cout << "\nSimulation complete!" << std::endl;

    // Note: mesh will be released automatically when PhysX cleans up
}

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - Triangle Mesh Builder Example ===" << std::endl;
    std::cout << std::endl;

    // Seed random number generator
    srand(static_cast<unsigned>(time(nullptr)));

    // Configure PhysX
    PhysXCoreConfig config;
    config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    config.numThreads = 2;
    config.enablePVD = false;

    // Check if user wants PVD
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--pvd") {
            config.enablePVD = true;
            std::cout << "PVD enabled. Connect PhysX Visual Debugger to localhost:5425" << std::endl;
        }
    }

    // Create and initialize PhysX
    PhysXCore physics;

    std::cout << "Initializing PhysX..." << std::endl;
    if (!physics.initialize(config)) {
        std::cerr << "Failed to initialize PhysX: " << physics.getLastError() << std::endl;
        return 1;
    }
    std::cout << "PhysX initialized successfully!" << std::endl;

    // Create mesh builder
    TriangleMeshBuilder builder(physics.getPhysics());

    // Run tests
    testMidphaseComparison(physics, builder);
    testCookingConfigurations(physics, builder);
    testPredefinedShapes(physics, builder);
    testStreamSerialization(physics, builder);
    testInSimulation(physics, builder);

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "\nThis example demonstrated:" << std::endl;
    std::cout << "  - Different midphase algorithms (BVH33, BVH34)" << std::endl;
    std::cout << "  - Runtime vs offline cooking configurations" << std::endl;
    std::cout << "  - Predefined shapes (plane, terrain)" << std::endl;
    std::cout << "  - Stream serialization and deserialization" << std::endl;
    std::cout << "  - Using triangle meshes for static terrain in simulation" << std::endl;

    std::cout << "\nCleaning up..." << std::endl;

    return 0;
}
