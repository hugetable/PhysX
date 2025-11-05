/**
 * @file example_geometryquery.cpp
 * @brief Geometry query example demonstrating raycast, sweep, and overlap
 *
 * This example demonstrates:
 * - Raycasting (single, multiple, any)
 * - Shape sweeping (sphere, box, capsule)
 * - Overlap queries (sphere, box, capsule)
 * - Query filtering
 *
 * Based on PhysX SDK spatial query features.
 */

#include "Core/PhysXCore.h"
#include "Query/GeometryQuery.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace PhysXWrapper;
using namespace physx;

/**
 * @brief Create a simple scene with various objects
 */
void createTestScene(PhysXCore& physics) {
    PxPhysics* physicsSDK = physics.getPhysics();
    PxScene* scene = physics.getScene();
    PxMaterial* material = physics.getDefaultMaterial();

    if (!physicsSDK || !scene || !material) {
        std::cerr << "Physics not initialized" << std::endl;
        return;
    }

    // Create ground plane
    physics.createGroundPlane(PxVec3(0, 1, 0), 0);

    // Create a few static boxes at different heights
    PxBoxGeometry boxGeom(1.0f, 1.0f, 1.0f);

    // Box at (0, 1, 0)
    PxRigidStatic* box1 = physicsSDK->createRigidStatic(PxTransform(PxVec3(0, 1, 0)));
    PxShape* shape1 = physicsSDK->createShape(boxGeom, *material);
    box1->attachShape(*shape1);
    scene->addActor(*box1);
    shape1->release();

    // Box at (5, 1, 0)
    PxRigidStatic* box2 = physicsSDK->createRigidStatic(PxTransform(PxVec3(5, 1, 0)));
    PxShape* shape2 = physicsSDK->createShape(boxGeom, *material);
    box2->attachShape(*shape2);
    scene->addActor(*box2);
    shape2->release();

    // Box at (10, 1, 0)
    PxRigidStatic* box3 = physicsSDK->createRigidStatic(PxTransform(PxVec3(10, 1, 0)));
    PxShape* shape3 = physicsSDK->createShape(boxGeom, *material);
    box3->attachShape(*shape3);
    scene->addActor(*box3);
    shape3->release();

    // Create some dynamic spheres
    PxSphereGeometry sphereGeom(0.5f);

    PxRigidDynamic* sphere1 = physicsSDK->createRigidDynamic(PxTransform(PxVec3(2, 3, 0)));
    PxShape* sphereShape1 = physicsSDK->createShape(sphereGeom, *material);
    sphere1->attachShape(*sphereShape1);
    PxRigidBodyExt::updateMassAndInertia(*sphere1, 1.0f);
    scene->addActor(*sphere1);
    sphereShape1->release();

    PxRigidDynamic* sphere2 = physicsSDK->createRigidDynamic(PxTransform(PxVec3(7, 4, 0)));
    PxShape* sphereShape2 = physicsSDK->createShape(sphereGeom, *material);
    sphere2->attachShape(*sphereShape2);
    PxRigidBodyExt::updateMassAndInertia(*sphere2, 1.0f);
    scene->addActor(*sphere2);
    sphereShape2->release();
}

/**
 * @brief Test raycast queries
 */
void testRaycastQueries(GeometryQuery& query) {
    std::cout << "\n=== RAYCAST TESTS ===" << std::endl;

    // Test 1: Single raycast downward from above
    std::cout << "\n1. Raycast downward from (0, 10, 0):" << std::endl;
    RaycastHit hit;
    if (query.raycastSingle(
        PxVec3(0, 10, 0),  // origin
        PxVec3(0, -1, 0),  // direction (down)
        100.0f,            // max distance
        hit
    )) {
        std::cout << "  HIT!" << std::endl;
        std::cout << "  Position: (" << std::fixed << std::setprecision(2)
                  << hit.position.x << ", " << hit.position.y << ", " << hit.position.z << ")" << std::endl;
        std::cout << "  Distance: " << hit.distance << std::endl;
        std::cout << "  Normal: (" << hit.normal.x << ", " << hit.normal.y << ", " << hit.normal.z << ")" << std::endl;
    } else {
        std::cout << "  No hit" << std::endl;
    }

    // Test 2: Horizontal raycast that might hit multiple objects
    std::cout << "\n2. Horizontal raycast through multiple boxes:" << std::endl;
    std::vector<RaycastHit> hits;
    int hitCount = query.raycastMultiple(
        PxVec3(-5, 1, 0),  // origin
        PxVec3(1, 0, 0),   // direction (right)
        20.0f,             // max distance
        hits,
        10                 // max hits
    );
    std::cout << "  Found " << hitCount << " hits" << std::endl;
    for (int i = 0; i < hitCount; i++) {
        std::cout << "  Hit " << (i + 1) << ": distance = "
                  << std::fixed << std::setprecision(2) << hits[i].distance << std::endl;
    }

    // Test 3: Check if any object exists in a direction
    std::cout << "\n3. Raycast any - quick existence check:" << std::endl;
    bool anyHit = query.raycastAny(
        PxVec3(0, 5, 0),
        PxVec3(0, -1, 0),
        10.0f
    );
    std::cout << "  Object exists below? " << (anyHit ? "YES" : "NO") << std::endl;

    // Test 4: Raycast with no expected hit
    std::cout << "\n4. Raycast into empty space:" << std::endl;
    anyHit = query.raycastAny(
        PxVec3(0, 5, 0),
        PxVec3(0, 1, 0),  // upward
        10.0f
    );
    std::cout << "  Object exists above? " << (anyHit ? "YES" : "NO") << std::endl;
}

/**
 * @brief Test sweep queries
 */
void testSweepQueries(GeometryQuery& query) {
    std::cout << "\n=== SWEEP TESTS ===" << std::endl;

    // Test 1: Sphere sweep downward
    std::cout << "\n1. Sphere sweep (radius 0.5) downward from (5, 10, 0):" << std::endl;
    SweepHit sweepHit;
    if (query.sweepSphere(
        PxVec3(5, 10, 0),  // origin
        PxVec3(0, -1, 0),  // direction
        0.5f,              // radius
        20.0f,             // max distance
        sweepHit
    )) {
        std::cout << "  HIT!" << std::endl;
        std::cout << "  Position: (" << std::fixed << std::setprecision(2)
                  << sweepHit.position.x << ", " << sweepHit.position.y << ", "
                  << sweepHit.position.z << ")" << std::endl;
        std::cout << "  Distance: " << sweepHit.distance << std::endl;
        std::cout << "  Normal: (" << sweepHit.normal.x << ", "
                  << sweepHit.normal.y << ", " << sweepHit.normal.z << ")" << std::endl;
    } else {
        std::cout << "  No hit" << std::endl;
    }

    // Test 2: Box sweep horizontally
    std::cout << "\n2. Box sweep (1x1x1) horizontally from (-5, 1, 0):" << std::endl;
    if (query.sweepBox(
        PxVec3(-5, 1, 0),       // origin
        PxVec3(1, 0, 0),        // direction (right)
        PxVec3(0.5f, 0.5f, 0.5f), // half extents
        PxQuat(PxIdentity),     // rotation
        20.0f,                  // max distance
        sweepHit
    )) {
        std::cout << "  HIT!" << std::endl;
        std::cout << "  Distance to first object: " << sweepHit.distance << std::endl;
        std::cout << "  Position: (" << std::fixed << std::setprecision(2)
                  << sweepHit.position.x << ", " << sweepHit.position.y << ", "
                  << sweepHit.position.z << ")" << std::endl;
    } else {
        std::cout << "  No hit" << std::endl;
    }

    // Test 3: Capsule sweep
    std::cout << "\n3. Capsule sweep (radius 0.5, height 2.0) downward:" << std::endl;
    if (query.sweepCapsule(
        PxVec3(10, 10, 0),     // origin
        PxVec3(0, -1, 0),      // direction
        0.5f,                  // radius
        1.0f,                  // half height
        PxQuat(PxIdentity),    // rotation
        20.0f,                 // max distance
        sweepHit
    )) {
        std::cout << "  HIT!" << std::endl;
        std::cout << "  Distance: " << sweepHit.distance << std::endl;
        std::cout << "  Position: (" << std::fixed << std::setprecision(2)
                  << sweepHit.position.x << ", " << sweepHit.position.y << ", "
                  << sweepHit.position.z << ")" << std::endl;
    } else {
        std::cout << "  No hit" << std::endl;
    }
}

/**
 * @brief Test overlap queries
 */
void testOverlapQueries(GeometryQuery& query) {
    std::cout << "\n=== OVERLAP TESTS ===" << std::endl;

    // Test 1: Sphere overlap at origin
    std::cout << "\n1. Sphere overlap (radius 3.0) at (0, 1, 0):" << std::endl;
    std::vector<OverlapHit> overlapHits;
    if (query.overlapSphere(
        PxVec3(0, 1, 0),  // origin
        3.0f,             // radius
        overlapHits,
        10                // max overlaps
    )) {
        std::cout << "  Found " << overlapHits.size() << " overlapping objects" << std::endl;
        for (size_t i = 0; i < overlapHits.size(); i++) {
            std::cout << "  Object " << (i + 1) << ": actor = " << overlapHits[i].actor << std::endl;
        }
    } else {
        std::cout << "  No overlaps found" << std::endl;
    }

    // Test 2: Box overlap
    std::cout << "\n2. Box overlap (2x2x2) at (5, 1, 0):" << std::endl;
    overlapHits.clear();
    if (query.overlapBox(
        PxVec3(5, 1, 0),        // origin
        PxVec3(1.0f, 1.0f, 1.0f), // half extents
        PxQuat(PxIdentity),     // rotation
        overlapHits,
        10
    )) {
        std::cout << "  Found " << overlapHits.size() << " overlapping objects" << std::endl;
    } else {
        std::cout << "  No overlaps found" << std::endl;
    }

    // Test 3: Capsule overlap
    std::cout << "\n3. Capsule overlap (radius 1.0, height 4.0) at (10, 1, 0):" << std::endl;
    overlapHits.clear();
    if (query.overlapCapsule(
        PxVec3(10, 1, 0),      // origin
        1.0f,                  // radius
        2.0f,                  // half height
        PxQuat(PxIdentity),    // rotation
        overlapHits,
        10
    )) {
        std::cout << "  Found " << overlapHits.size() << " overlapping objects" << std::endl;
    } else {
        std::cout << "  No overlaps found" << std::endl;
    }

    // Test 4: Quick overlap existence check
    std::cout << "\n4. Quick overlap check at (2, 3, 0):" << std::endl;
    bool hasOverlap = query.overlapAny(
        PxVec3(2, 3, 0),  // near a dynamic sphere
        0.5f              // small radius
    );
    std::cout << "  Objects nearby? " << (hasOverlap ? "YES" : "NO") << std::endl;

    // Test 5: Overlap in empty space
    std::cout << "\n5. Overlap check in empty space at (100, 100, 100):" << std::endl;
    hasOverlap = query.overlapAny(
        PxVec3(100, 100, 100),
        1.0f
    );
    std::cout << "  Objects nearby? " << (hasOverlap ? "YES" : "NO") << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - Geometry Query Example ===" << std::endl;
    std::cout << std::endl;

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

    // Create test scene
    std::cout << "\nCreating test scene with multiple objects..." << std::endl;
    createTestScene(physics);

    // Let physics settle for a moment
    std::cout << "Running initial simulation to settle objects..." << std::endl;
    for (int i = 0; i < 60; i++) {
        physics.update(1.0f / 60.0f);
    }
    std::cout << "Scene ready!" << std::endl;

    // Create geometry query interface
    GeometryQuery query(physics.getScene());

    // Run tests
    testRaycastQueries(query);
    testSweepQueries(query);
    testOverlapQueries(query);

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "\nThis example demonstrated:" << std::endl;
    std::cout << "  - Raycast queries (single, multiple, any)" << std::endl;
    std::cout << "  - Shape sweep queries (sphere, box, capsule)" << std::endl;
    std::cout << "  - Overlap queries (sphere, box, capsule, any)" << std::endl;
    std::cout << "  - Query filtering and hit data access" << std::endl;

    std::cout << "\nCleaning up..." << std::endl;

    return 0;
}
