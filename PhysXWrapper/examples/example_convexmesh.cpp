/**
 * @file example_convexmesh.cpp
 * @brief Convex mesh creation example using ConvexMeshBuilder
 *
 * This example demonstrates:
 * - Creating convex meshes from point clouds
 * - Using different cooking configurations
 * - Creating predefined shapes (box, cylinder, cone)
 * - Direct insertion vs stream serialization
 * - Performance comparison of different settings
 *
 * Based on SnippetConvexMeshCreate from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include "Utility/ConvexMeshBuilder.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>

using namespace PhysXWrapper;
using namespace physx;

/**
 * @brief Test creating convex mesh from random point cloud
 */
void testRandomPointCloud(PhysXCore& physics, ConvexMeshBuilder& builder) {
    std::cout << "\n=== TEST 1: Random Point Cloud ===" << std::endl;

    // Generate random points
    std::cout << "Generating 64 random points..." << std::endl;
    std::vector<PxVec3> points = generateRandomPointCloud(
        64,
        PxVec3(-10.0f, -10.0f, -10.0f),
        PxVec3(10.0f, 10.0f, 10.0f)
    );

    // Test with different configurations
    std::cout << "\n1.1 Offline cooking (with gauss map):" << std::endl;
    ConvexMeshConfig offlineConfig = ConvexMeshBuilder::getOfflineConfig();
    offlineConfig.directInsertion = true;  // Override for this test
    ConvexMeshResult result1 = builder.createConvexMesh(points, offlineConfig);

    if (result1.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Output vertices: " << result1.numVertices << std::endl;
        std::cout << "  Output polygons: " << result1.numPolygons << std::endl;
        std::cout << "  Cooking time: " << std::fixed << std::setprecision(3)
                  << result1.cookingTime << " ms" << std::endl;

        // Clean up
        if (result1.mesh) result1.mesh->release();
    } else {
        std::cerr << "  FAILED: " << result1.error << std::endl;
    }

    std::cout << "\n1.2 Runtime cooking (no gauss map):" << std::endl;
    ConvexMeshConfig runtimeConfig = ConvexMeshBuilder::getRuntimeConfig();
    ConvexMeshResult result2 = builder.createConvexMesh(points, runtimeConfig);

    if (result2.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Output vertices: " << result2.numVertices << std::endl;
        std::cout << "  Output polygons: " << result2.numPolygons << std::endl;
        std::cout << "  Cooking time: " << std::fixed << std::setprecision(3)
                  << result2.cookingTime << " ms" << std::endl;
        std::cout << "  Speed improvement: "
                  << ((result1.cookingTime / result2.cookingTime - 1.0f) * 100.0f)
                  << "%" << std::endl;

        // Clean up
        if (result2.mesh) result2.mesh->release();
    } else {
        std::cerr << "  FAILED: " << result2.error << std::endl;
    }
}

/**
 * @brief Test creating predefined shapes
 */
void testPredefinedShapes(PhysXCore& physics, ConvexMeshBuilder& builder) {
    std::cout << "\n=== TEST 2: Predefined Shapes ===" << std::endl;

    ConvexMeshConfig config;
    config.directInsertion = true;

    // Create box
    std::cout << "\n2.1 Box (2x2x2):" << std::endl;
    ConvexMeshResult boxResult = builder.createBox(PxVec3(1.0f, 1.0f, 1.0f), config);
    if (boxResult.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Vertices: " << boxResult.numVertices << std::endl;
        std::cout << "  Polygons: " << boxResult.numPolygons << std::endl;
        std::cout << "  Cooking time: " << boxResult.cookingTime << " ms" << std::endl;
        if (boxResult.mesh) boxResult.mesh->release();
    } else {
        std::cerr << "  FAILED: " << boxResult.error << std::endl;
    }

    // Create cylinder
    std::cout << "\n2.2 Cylinder (radius=1, height=2, 16 segments):" << std::endl;
    ConvexMeshResult cylResult = builder.createCylinder(1.0f, 1.0f, 16, config);
    if (cylResult.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Vertices: " << cylResult.numVertices << std::endl;
        std::cout << "  Polygons: " << cylResult.numPolygons << std::endl;
        std::cout << "  Cooking time: " << cylResult.cookingTime << " ms" << std::endl;
        if (cylResult.mesh) cylResult.mesh->release();
    } else {
        std::cerr << "  FAILED: " << cylResult.error << std::endl;
    }

    // Create cone
    std::cout << "\n2.3 Cone (radius=1, height=2, 16 segments):" << std::endl;
    ConvexMeshResult coneResult = builder.createCone(1.0f, 2.0f, 16, config);
    if (coneResult.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Vertices: " << coneResult.numVertices << std::endl;
        std::cout << "  Polygons: " << coneResult.numPolygons << std::endl;
        std::cout << "  Cooking time: " << coneResult.cookingTime << " ms" << std::endl;
        if (coneResult.mesh) coneResult.mesh->release();
    } else {
        std::cerr << "  FAILED: " << coneResult.error << std::endl;
    }
}

/**
 * @brief Test stream serialization
 */
void testStreamSerialization(PhysXCore& physics, ConvexMeshBuilder& builder) {
    std::cout << "\n=== TEST 3: Stream Serialization ===" << std::endl;

    // Generate points
    std::vector<PxVec3> points = generateRandomPointCloud(
        32,
        PxVec3(-5.0f, -5.0f, -5.0f),
        PxVec3(5.0f, 5.0f, 5.0f)
    );

    // Cook to stream
    std::cout << "\n3.1 Cooking to stream..." << std::endl;
    PxDefaultMemoryOutputStream stream;
    ConvexMeshConfig config = ConvexMeshBuilder::getOfflineConfig();
    ConvexMeshResult cookResult = builder.createConvexMeshToStream(points, stream, config);

    if (cookResult.success) {
        std::cout << "  SUCCESS!" << std::endl;
        std::cout << "  Stream size: " << stream.getSize() << " bytes" << std::endl;
        std::cout << "  Cooking time: " << cookResult.cookingTime << " ms" << std::endl;

        // Load from stream
        std::cout << "\n3.2 Loading from stream..." << std::endl;
        PxDefaultMemoryInputData inStream(stream.getData(), stream.getSize());
        PxConvexMesh* loadedMesh = builder.loadConvexMeshFromStream(inStream);

        if (loadedMesh) {
            std::cout << "  SUCCESS!" << std::endl;
            std::cout << "  Loaded vertices: " << loadedMesh->getNbVertices() << std::endl;
            std::cout << "  Loaded polygons: " << loadedMesh->getNbPolygons() << std::endl;
            loadedMesh->release();
        } else {
            std::cerr << "  FAILED to load from stream" << std::endl;
        }
    } else {
        std::cerr << "  FAILED: " << cookResult.error << std::endl;
    }
}

/**
 * @brief Test using convex meshes in simulation
 */
void testInSimulation(PhysXCore& physics, ConvexMeshBuilder& builder) {
    std::cout << "\n=== TEST 4: Using Convex Mesh in Simulation ===" << std::endl;

    PxScene* scene = physics.getScene();
    PxPhysics* physicsSDK = physics.getPhysics();
    PxMaterial* material = physics.getDefaultMaterial();

    if (!scene || !physicsSDK || !material) {
        std::cerr << "Physics not properly initialized" << std::endl;
        return;
    }

    // Create ground plane
    std::cout << "\nCreating ground plane..." << std::endl;
    physics.createGroundPlane(PxVec3(0, 1, 0), 0);

    // Create a custom convex mesh (diamond shape)
    std::cout << "Creating custom convex mesh (diamond)..." << std::endl;
    std::vector<PxVec3> diamondPoints = {
        PxVec3(0, 2, 0),    // top
        PxVec3(0, -2, 0),   // bottom
        PxVec3(1, 0, 0),    // right
        PxVec3(-1, 0, 0),   // left
        PxVec3(0, 0, 1),    // front
        PxVec3(0, 0, -1)    // back
    };

    ConvexMeshConfig config;
    config.directInsertion = true;
    ConvexMeshResult result = builder.createConvexMesh(diamondPoints, config);

    if (!result.success) {
        std::cerr << "Failed to create diamond mesh: " << result.error << std::endl;
        return;
    }

    std::cout << "Diamond mesh created successfully!" << std::endl;
    std::cout << "  Vertices: " << result.numVertices << std::endl;
    std::cout << "  Polygons: " << result.numPolygons << std::endl;

    // Create dynamic actor with the convex mesh
    PxConvexMeshGeometry geom(result.mesh);
    PxRigidDynamic* actor = physicsSDK->createRigidDynamic(PxTransform(PxVec3(0, 10, 0)));
    PxShape* shape = physicsSDK->createShape(geom, *material);
    actor->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*actor, 1.0f);
    scene->addActor(*actor);
    shape->release();

    std::cout << "\nRunning simulation (3 seconds)..." << std::endl;

    // Simulate
    const float timeStep = 1.0f / 60.0f;
    const int frameCount = 180;

    for (int i = 0; i < frameCount; i++) {
        physics.update(timeStep);

        // Print position every second
        if (i % 60 == 0) {
            PxTransform transform = actor->getGlobalPose();
            std::cout << "  Frame " << i << ": Y position = "
                      << std::fixed << std::setprecision(2)
                      << transform.p.y << std::endl;
        }
    }

    std::cout << "\nSimulation complete!" << std::endl;

    // Note: mesh will be released automatically when PhysX cleans up
}

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - Convex Mesh Builder Example ===" << std::endl;
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
    ConvexMeshBuilder builder(physics.getPhysics());

    // Run tests
    testRandomPointCloud(physics, builder);
    testPredefinedShapes(physics, builder);
    testStreamSerialization(physics, builder);
    testInSimulation(physics, builder);

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "\nThis example demonstrated:" << std::endl;
    std::cout << "  - Creating convex meshes from random point clouds" << std::endl;
    std::cout << "  - Runtime vs offline cooking configurations" << std::endl;
    std::cout << "  - Predefined shapes (box, cylinder, cone)" << std::endl;
    std::cout << "  - Stream serialization and deserialization" << std::endl;
    std::cout << "  - Using convex meshes in physics simulation" << std::endl;

    std::cout << "\nCleaning up..." << std::endl;

    return 0;
}
