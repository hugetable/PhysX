/**
 * @file example_frustum.cpp
 * @brief Example demonstrating view-frustum culling
 *
 * This example shows how to use FrustumQuery for visibility culling:
 * 1. Basic frustum culling with custom objects
 * 2. BVH-accelerated culling for large scenes
 * 3. PhysX scene culling
 * 4. Camera movement and frustum updates
 *
 * Frustum culling is essential for rendering optimization in games.
 */

#include <iostream>
#include <Core/PhysXCore.h>
#include <Query/FrustumQuery.h>
#include <PxPhysicsAPI.h>
#include <iomanip>

using namespace PhysXWrapper;
using namespace physx;

// ============================================================================
// Helper Functions
// ============================================================================

void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================" << std::endl;
}

void printFrustum(const Frustum& frustum) {
    const char* planeNames[] = {"LEFT", "RIGHT", "TOP", "BOTTOM", "NEAR", "FAR"};
    std::cout << "Frustum planes:" << std::endl;
    for (int i = 0; i < 6; ++i) {
        const PxPlane& plane = frustum.planes[i];
        std::cout << "  " << std::setw(7) << planeNames[i] << ": "
                  << "n=(" << plane.n.x << ", " << plane.n.y << ", " << plane.n.z << ") "
                  << "d=" << plane.d << std::endl;
    }
}

void printQueryResult(const FrustumQueryResult& result) {
    std::cout << "Query result:" << std::endl;
    std::cout << "  Total objects: " << result.totalObjects << std::endl;
    std::cout << "  Visible objects: " << result.visibleIndices.size() << std::endl;
    std::cout << "  Culled objects: " << result.culledObjects << std::endl;
    std::cout << "  Query time: " << std::fixed << std::setprecision(3)
              << result.queryTime << " ms" << std::endl;
    std::cout << "  Culling efficiency: " << std::fixed << std::setprecision(1)
              << (result.culledObjects * 100.0f / result.totalObjects) << "%" << std::endl;
}

// ============================================================================
// Test 1: Basic Frustum Culling
// ============================================================================

void test1_BasicFrustumCulling() {
    printSeparator("Test 1: Basic Frustum Culling");

    std::cout << "Creating frustum from camera configuration..." << std::endl;

    // Create camera
    CameraConfig camera;
    camera.position = PxVec3(0, 5, 10);
    camera.target = PxVec3(0, 0, 0);
    camera.up = PxVec3(0, 1, 0);
    camera.fov = 60.0f;
    camera.aspectRatio = 16.0f / 9.0f;
    camera.nearPlane = 0.1f;
    camera.farPlane = 100.0f;

    std::cout << "Camera configuration:" << std::endl;
    std::cout << "  Position: (" << camera.position.x << ", "
              << camera.position.y << ", " << camera.position.z << ")" << std::endl;
    std::cout << "  Target: (" << camera.target.x << ", "
              << camera.target.y << ", " << camera.target.z << ")" << std::endl;
    std::cout << "  FOV: " << camera.fov << "°" << std::endl;
    std::cout << "  Aspect: " << camera.aspectRatio << std::endl;

    // Create frustum
    Frustum frustum = FrustumQuery::createFrustum(camera);
    std::cout << "\n✓ Created frustum" << std::endl;
    printFrustum(frustum);

    // Create query and add objects
    FrustumQuery query;

    std::cout << "\nAdding objects..." << std::endl;

    // Objects in front of camera (should be visible)
    query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(PxVec3(0, 0, 0)));
    query.addObject(PxSphereGeometry(1), PxTransform(PxVec3(2, 0, 0)));
    query.addObject(PxCapsuleGeometry(0.5f, 1), PxTransform(PxVec3(-2, 0, 0)));

    // Objects behind camera (should be culled)
    query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(PxVec3(0, 0, 20)));
    query.addObject(PxSphereGeometry(1), PxTransform(PxVec3(5, 0, 20)));

    // Objects too far (should be culled)
    query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(PxVec3(0, 0, -200)));

    std::cout << "✓ Added " << query.getObjectCount() << " objects" << std::endl;

    // Perform culling (without BVH)
    std::cout << "\nPerforming culling without BVH..." << std::endl;
    FrustumQueryResult result = query.cull(frustum);
    printQueryResult(result);

    std::cout << "\nVisible object indices: ";
    for (PxU32 idx : result.visibleIndices) {
        std::cout << idx << " ";
    }
    std::cout << std::endl;

    std::cout << "\n✓ Test 1 completed successfully!" << std::endl;
}

// ============================================================================
// Test 2: BVH-Accelerated Culling
// ============================================================================

void test2_BVHAcceleratedCulling() {
    printSeparator("Test 2: BVH-Accelerated Culling");

    std::cout << "Creating large scene for performance testing..." << std::endl;

    // Create camera
    CameraConfig camera;
    camera.position = PxVec3(0, 10, 20);
    camera.target = PxVec3(0, 0, 0);
    camera.fov = 60.0f;

    Frustum frustum = FrustumQuery::createFrustum(camera);

    // Create query
    FrustumQuery query;

    // Add many objects in a grid
    const int gridSize = 20;
    const float spacing = 5.0f;

    std::cout << "Adding " << (gridSize * gridSize * gridSize) << " objects..." << std::endl;

    for (int x = 0; x < gridSize; ++x) {
        for (int y = 0; y < gridSize; ++y) {
            for (int z = 0; z < gridSize; ++z) {
                PxVec3 pos(
                    (x - gridSize / 2) * spacing,
                    (y - gridSize / 2) * spacing,
                    (z - gridSize / 2) * spacing
                );

                // Alternate between different geometries
                if ((x + y + z) % 3 == 0) {
                    query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(pos));
                } else if ((x + y + z) % 3 == 1) {
                    query.addObject(PxSphereGeometry(1), PxTransform(pos));
                } else {
                    query.addObject(PxCapsuleGeometry(0.5f, 1), PxTransform(pos));
                }
            }
        }
    }

    std::cout << "✓ Added " << query.getObjectCount() << " objects" << std::endl;

    // Test WITHOUT BVH
    std::cout << "\n--- Testing WITHOUT BVH ---" << std::endl;
    FrustumQueryResult resultNoBVH = query.cull(frustum);
    printQueryResult(resultNoBVH);

    // Build BVH
    std::cout << "\nBuilding BVH acceleration structure..." << std::endl;
    query.buildBVH();
    std::cout << "✓ BVH built" << std::endl;

    // Test WITH BVH
    std::cout << "\n--- Testing WITH BVH ---" << std::endl;
    FrustumQueryResult resultWithBVH = query.cull(frustum);
    printQueryResult(resultWithBVH);

    // Compare performance
    std::cout << "\n--- Performance Comparison ---" << std::endl;
    std::cout << "Without BVH: " << resultNoBVH.queryTime << " ms" << std::endl;
    std::cout << "With BVH:    " << resultWithBVH.queryTime << " ms" << std::endl;

    if (resultNoBVH.queryTime > 0.0f) {
        float speedup = resultNoBVH.queryTime / resultWithBVH.queryTime;
        std::cout << "Speedup:     " << std::fixed << std::setprecision(2)
                  << speedup << "x faster" << std::endl;
    }

    std::cout << "\n✓ Test 2 completed successfully!" << std::endl;
}

// ============================================================================
// Test 3: PhysX Scene Culling
// ============================================================================

void test3_PhysXSceneCulling() {
    printSeparator("Test 3: PhysX Scene Culling");

    std::cout << "Creating PhysX scene with actors..." << std::endl;

    // Initialize PhysX
    PhysXCore physx;
    physx.initialize();
    physx.createScene();

    PxPhysics* physics = physx.getPhysics();
    PxScene* scene = physx.getScene();
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    // Add actors to scene
    std::cout << "Adding actors to scene..." << std::endl;

    // Ground plane
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Add dynamic actors in a pattern
    for (int x = -5; x <= 5; x += 2) {
        for (int z = -5; z <= 5; z += 2) {
            PxShape* shape = physics->createShape(PxBoxGeometry(0.5f, 0.5f, 0.5f), *material);
            PxRigidDynamic* body = physics->createRigidDynamic(
                PxTransform(PxVec3(x, 1.0f, z)));
            body->attachShape(*shape);
            PxRigidBodyExt::setMassAndUpdateInertia(*body, 10.0f);
            scene->addActor(*body);
            shape->release();
        }
    }

    std::cout << "✓ Added " << scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC |
                                                    PxActorTypeFlag::eRIGID_STATIC)
              << " actors to scene" << std::endl;

    // Create camera looking at scene
    CameraConfig camera;
    camera.position = PxVec3(0, 10, 15);
    camera.target = PxVec3(0, 0, 0);
    camera.fov = 60.0f;

    std::cout << "\nCamera position: (" << camera.position.x << ", "
              << camera.position.y << ", " << camera.position.z << ")" << std::endl;

    // Create frustum
    Frustum frustum = FrustumQuery::createFrustum(camera);

    // Perform scene culling
    std::cout << "\nPerforming frustum culling on scene..." << std::endl;
    std::vector<PxRigidActor*> visibleActors = FrustumQuery::cullScene(scene, frustum);

    std::cout << "✓ Culling complete:" << std::endl;
    std::cout << "  Visible actors: " << visibleActors.size() << std::endl;

    // Show details of visible actors
    std::cout << "\nVisible actor positions:" << std::endl;
    int count = 0;
    for (PxRigidActor* actor : visibleActors) {
        PxVec3 pos = actor->getGlobalPose().p;
        std::cout << "  Actor " << count++ << ": ("
                  << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;

        if (count >= 10) {
            std::cout << "  ... and " << (visibleActors.size() - 10) << " more" << std::endl;
            break;
        }
    }

    // Test with callback
    std::cout << "\nTesting with callback..." << std::endl;
    int callbackCount = 0;
    FrustumQuery::cullSceneWithCallback(scene, frustum, [&callbackCount](PxRigidActor* actor) {
        callbackCount++;
    });
    std::cout << "✓ Callback invoked " << callbackCount << " times" << std::endl;

    std::cout << "\n✓ Test 3 completed successfully!" << std::endl;
}

// ============================================================================
// Test 4: Camera Movement and Dynamic Culling
// ============================================================================

void test4_CameraMovementCulling() {
    printSeparator("Test 4: Camera Movement and Dynamic Culling");

    std::cout << "Testing frustum culling with moving camera..." << std::endl;

    // Create static query with objects
    FrustumQuery query;

    std::cout << "Creating grid of objects..." << std::endl;
    for (int x = -10; x <= 10; x += 2) {
        for (int z = -10; z <= 10; z += 2) {
            query.addObject(PxBoxGeometry(0.5f, 0.5f, 0.5f),
                          PxTransform(PxVec3(x, 0, z)));
        }
    }

    std::cout << "✓ Added " << query.getObjectCount() << " objects" << std::endl;

    // Build BVH for performance
    query.buildBVH();
    std::cout << "✓ Built BVH" << std::endl;

    // Simulate camera movement
    const int numFrames = 5;
    std::cout << "\nSimulating " << numFrames << " camera positions..." << std::endl;

    for (int frame = 0; frame < numFrames; ++frame) {
        // Move camera in a circle
        float angle = (frame / (float)numFrames) * 2.0f * 3.14159f;
        float radius = 15.0f;

        CameraConfig camera;
        camera.position = PxVec3(
            radius * std::cos(angle),
            5.0f,
            radius * std::sin(angle)
        );
        camera.target = PxVec3(0, 0, 0);
        camera.fov = 60.0f;

        // Create frustum
        Frustum frustum = FrustumQuery::createFrustum(camera);

        // Perform culling
        FrustumQueryResult result = query.cull(frustum);

        std::cout << "\n--- Frame " << frame << " ---" << std::endl;
        std::cout << "Camera position: ("
                  << std::fixed << std::setprecision(2)
                  << camera.position.x << ", "
                  << camera.position.y << ", "
                  << camera.position.z << ")" << std::endl;
        std::cout << "Visible: " << result.visibleIndices.size()
                  << "/" << result.totalObjects
                  << " (" << (result.visibleIndices.size() * 100.0f / result.totalObjects)
                  << "%)" << std::endl;
        std::cout << "Query time: " << result.queryTime << " ms" << std::endl;
    }

    std::cout << "\n✓ Test 4 completed successfully!" << std::endl;
}

// ============================================================================
// Test 5: Orthographic Frustum
// ============================================================================

void test5_OrthographicFrustum() {
    printSeparator("Test 5: Orthographic Frustum");

    std::cout << "Creating orthographic frustum..." << std::endl;

    // Create orthographic frustum
    Frustum frustum = FrustumQuery::createOrthographicFrustum(
        -10.0f, 10.0f,  // left, right
        -10.0f, 10.0f,  // bottom, top
        0.1f, 100.0f    // near, far
    );

    std::cout << "✓ Created orthographic frustum" << std::endl;
    std::cout << "  Bounds: [-10, 10] x [-10, 10]" << std::endl;
    std::cout << "  Depth: [0.1, 100]" << std::endl;

    // Create query
    FrustumQuery query;

    // Add objects at different positions
    query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(PxVec3(0, 0, 0)));      // Inside
    query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(PxVec3(5, 5, 0)));      // Inside
    query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(PxVec3(15, 0, 0)));     // Outside (right)
    query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(PxVec3(0, -15, 0)));    // Outside (bottom)
    query.addObject(PxBoxGeometry(1, 1, 1), PxTransform(PxVec3(0, 0, 200)));    // Outside (far)

    std::cout << "✓ Added " << query.getObjectCount() << " test objects" << std::endl;

    // Perform culling
    std::cout << "\nPerforming culling..." << std::endl;
    FrustumQueryResult result = query.cull(frustum);
    printQueryResult(result);

    std::cout << "\nExpected: 2 visible objects (indices 0, 1)" << std::endl;
    std::cout << "Actual visible indices: ";
    for (PxU32 idx : result.visibleIndices) {
        std::cout << idx << " ";
    }
    std::cout << std::endl;

    std::cout << "\n✓ Test 5 completed successfully!" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "PhysXWrapper - Frustum Culling Examples" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nFrustum culling is essential for rendering optimization." << std::endl;
    std::cout << "These tests demonstrate various culling scenarios.\n" << std::endl;

    try {
        // Test 1: Basic frustum culling
        test1_BasicFrustumCulling();

        // Test 2: BVH-accelerated culling
        test2_BVHAcceleratedCulling();

        // Test 3: PhysX scene culling
        test3_PhysXSceneCulling();

        // Test 4: Camera movement culling
        test4_CameraMovementCulling();

        // Test 5: Orthographic frustum
        test5_OrthographicFrustum();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
