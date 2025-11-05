/**
 * @file example_pointdistance.cpp
 * @brief Example demonstrating point distance queries
 *
 * This example shows how to use PointDistanceQuery for distance computations:
 * 1. Basic point-to-geometry distance queries
 * 2. Batch queries for multiple points
 * 3. Finding nearest actors in a scene
 * 4. Distance field generation
 * 5. Geometric primitive queries
 */

#include <iostream>
#include <iomanip>
#include <Core/PhysXCore.h>
#include <Query/PointDistanceQuery.h>
#include <PxPhysicsAPI.h>

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

void printVector(const std::string& name, const PxVec3& v) {
    std::cout << name << ": (" << std::fixed << std::setprecision(3)
              << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
}

void printResult(const PointDistanceResult& result) {
    std::cout << "Query result:" << std::endl;
    printVector("  Query point", result.queryPoint);
    printVector("  Closest point", result.closestPoint);
    std::cout << "  Distance: " << std::fixed << std::setprecision(3)
              << result.distance << std::endl;
    std::cout << "  Success: " << (result.success ? "Yes" : "No") << std::endl;
}

// ============================================================================
// Test 1: Basic Geometry Queries
// ============================================================================

void test1_BasicGeometryQueries() {
    printSeparator("Test 1: Basic Geometry Queries");

    std::cout << "Testing distance queries to various geometries..." << std::endl;

    // Test 1a: Box
    std::cout << "\n--- Box Geometry ---" << std::endl;
    PxBoxGeometry box(1, 2, 0.5f);
    PxTransform boxPose(PxVec3(5, 0, 0));

    PxVec3 queryPoint(0, 0, 0);
    printVector("Query point", queryPoint);
    printVector("Box center", boxPose.p);
    std::cout << "Box half-extents: (1, 2, 0.5)" << std::endl;

    PointDistanceResult boxResult = PointDistanceQuery::queryGeometry(
        queryPoint, box, boxPose);
    printResult(boxResult);

    // Test 1b: Sphere
    std::cout << "\n--- Sphere Geometry ---" << std::endl;
    PxSphereGeometry sphere(1.5f);
    PxTransform spherePose(PxVec3(3, 0, 0));

    printVector("Query point", queryPoint);
    printVector("Sphere center", spherePose.p);
    std::cout << "Sphere radius: 1.5" << std::endl;

    PointDistanceResult sphereResult = PointDistanceQuery::queryGeometry(
        queryPoint, sphere, spherePose);
    printResult(sphereResult);

    // Test 1c: Capsule
    std::cout << "\n--- Capsule Geometry ---" << std::endl;
    PxCapsuleGeometry capsule(0.5f, 1.0f);
    PxTransform capsulePose(PxVec3(0, 4, 0));

    printVector("Query point", queryPoint);
    printVector("Capsule center", capsulePose.p);
    std::cout << "Capsule radius: 0.5, half-height: 1.0" << std::endl;

    PointDistanceResult capsuleResult = PointDistanceQuery::queryGeometry(
        queryPoint, capsule, capsulePose);
    printResult(capsuleResult);

    // Test 1d: Point inside geometry
    std::cout << "\n--- Point Inside Geometry ---" << std::endl;
    PxVec3 insidePoint(5, 0, 0); // Inside the box
    printVector("Query point (inside box)", insidePoint);

    PointDistanceResult insideResult = PointDistanceQuery::queryGeometry(
        insidePoint, box, boxPose);
    printResult(insideResult);

    bool isInside = PointDistanceQuery::isPointInside(insidePoint, box, boxPose);
    std::cout << "Is point inside box? " << (isInside ? "Yes" : "No") << std::endl;

    std::cout << "\n✓ Test 1 completed successfully!" << std::endl;
}

// ============================================================================
// Test 2: Batch Queries
// ============================================================================

void test2_BatchQueries() {
    printSeparator("Test 2: Batch Queries");

    std::cout << "Querying multiple points against a sphere..." << std::endl;

    // Create target geometry
    PxSphereGeometry sphere(2.0f);
    PxTransform pose(PxVec3(0, 0, 0));

    std::cout << "Sphere: center (0, 0, 0), radius 2.0" << std::endl;

    // Create query points in a grid
    std::vector<PxVec3> points;
    for (int x = -3; x <= 3; ++x) {
        for (int y = -3; y <= 3; ++y) {
            points.push_back(PxVec3(x, y, 0));
        }
    }

    std::cout << "Querying " << points.size() << " points..." << std::endl;

    // Perform batch query
    BatchQueryResult batchResult = PointDistanceQuery::queryGeometryBatch(
        points, sphere, pose);

    std::cout << "\nBatch query results:" << std::endl;
    std::cout << "  Total queries: " << batchResult.results.size() << std::endl;
    std::cout << "  Successful: " << batchResult.successCount << std::endl;
    std::cout << "  Average distance: " << std::fixed << std::setprecision(3)
              << batchResult.averageDistance << std::endl;
    std::cout << "  Min distance: " << batchResult.minDistance << std::endl;
    std::cout << "  Max distance: " << batchResult.maxDistance << std::endl;

    // Count points inside sphere
    PxU32 insideCount = 0;
    for (const auto& result : batchResult.results) {
        if (result.distance <= 0.0f) {
            insideCount++;
        }
    }
    std::cout << "  Points inside sphere: " << insideCount << std::endl;

    // Test with callback
    std::cout << "\nTesting batch query with callback..." << std::endl;
    PxU32 callbackCount = 0;
    PxReal totalDist = 0.0f;

    PointDistanceQuery::queryGeometryBatchWithCallback(
        points, sphere, pose,
        [&callbackCount, &totalDist](const PointDistanceResult& result) {
            callbackCount++;
            totalDist += result.distance;
        });

    std::cout << "  Callback invoked " << callbackCount << " times" << std::endl;
    std::cout << "  Average distance: " << std::fixed << std::setprecision(3)
              << (totalDist / callbackCount) << std::endl;

    std::cout << "\n✓ Test 2 completed successfully!" << std::endl;
}

// ============================================================================
// Test 3: Scene Queries
// ============================================================================

void test3_SceneQueries() {
    printSeparator("Test 3: Scene Queries");

    std::cout << "Finding nearest actors in a PhysX scene..." << std::endl;

    // Initialize PhysX
    PhysXCore physx;
    physx.initialize();
    physx.createScene();

    PxPhysics* physics = physx.getPhysics();
    PxScene* scene = physx.getScene();
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    // Create actors at various positions
    std::cout << "\nCreating actors in scene..." << std::endl;

    struct ActorInfo {
        PxRigidDynamic* actor;
        PxVec3 position;
        std::string name;
    };

    std::vector<ActorInfo> actorInfos;

    // Actor 1: Box at (5, 0, 0)
    {
        PxShape* shape = physics->createShape(PxBoxGeometry(1, 1, 1), *material);
        PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(PxVec3(5, 0, 0)));
        actor->attachShape(*shape);
        scene->addActor(*actor);
        shape->release();
        actorInfos.push_back({actor, PxVec3(5, 0, 0), "Box"});
    }

    // Actor 2: Sphere at (3, 4, 0)
    {
        PxShape* shape = physics->createShape(PxSphereGeometry(1), *material);
        PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(PxVec3(3, 4, 0)));
        actor->attachShape(*shape);
        scene->addActor(*actor);
        shape->release();
        actorInfos.push_back({actor, PxVec3(3, 4, 0), "Sphere"});
    }

    // Actor 3: Capsule at (-2, 2, 0)
    {
        PxShape* shape = physics->createShape(PxCapsuleGeometry(0.5f, 1), *material);
        PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(PxVec3(-2, 2, 0)));
        actor->attachShape(*shape);
        scene->addActor(*actor);
        shape->release();
        actorInfos.push_back({actor, PxVec3(-2, 2, 0), "Capsule"});
    }

    // Actor 4: Box at (10, 0, 0) - far away
    {
        PxShape* shape = physics->createShape(PxBoxGeometry(1, 1, 1), *material);
        PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(PxVec3(10, 0, 0)));
        actor->attachShape(*shape);
        scene->addActor(*actor);
        shape->release();
        actorInfos.push_back({actor, PxVec3(10, 0, 0), "Box (far)"});
    }

    std::cout << "✓ Created " << actorInfos.size() << " actors" << std::endl;

    // Test 3a: Find nearest actor
    std::cout << "\n--- Finding Nearest Actor ---" << std::endl;
    PxVec3 queryPoint(0, 0, 0);
    printVector("Query point", queryPoint);

    NearestActorResult nearest = PointDistanceQuery::findNearestActor(
        scene, queryPoint);

    if (nearest.success) {
        std::cout << "Nearest actor found!" << std::endl;
        printVector("  Closest point", nearest.closestPoint);
        std::cout << "  Distance: " << std::fixed << std::setprecision(3)
                  << nearest.distance << std::endl;

        // Find which actor it is
        for (const auto& info : actorInfos) {
            if (info.actor == nearest.actor) {
                std::cout << "  Actor: " << info.name << " at ";
                printVector("", info.position);
                break;
            }
        }
    }

    // Test 3b: Find actors within distance
    std::cout << "\n--- Finding Actors Within Distance ---" << std::endl;
    PxReal maxDist = 6.0f;
    std::cout << "Maximum distance: " << maxDist << std::endl;

    std::vector<NearestActorResult> nearbyActors =
        PointDistanceQuery::findActorsWithinDistance(scene, queryPoint, maxDist);

    std::cout << "Found " << nearbyActors.size() << " actors within distance:" << std::endl;
    for (size_t i = 0; i < nearbyActors.size(); ++i) {
        const auto& result = nearbyActors[i];
        std::cout << "  Actor " << (i + 1) << ": distance = "
                  << std::fixed << std::setprecision(3) << result.distance;

        // Find actor name
        for (const auto& info : actorInfos) {
            if (info.actor == result.actor) {
                std::cout << " (" << info.name << ")";
                break;
            }
        }
        std::cout << std::endl;
    }

    // Test 3c: Find k nearest actors
    std::cout << "\n--- Finding K Nearest Actors ---" << std::endl;
    PxU32 k = 2;
    std::cout << "Finding " << k << " nearest actors..." << std::endl;

    std::vector<NearestActorResult> kNearest =
        PointDistanceQuery::findKNearestActors(scene, queryPoint, k);

    std::cout << "Found " << kNearest.size() << " nearest actors:" << std::endl;
    for (size_t i = 0; i < kNearest.size(); ++i) {
        const auto& result = kNearest[i];
        std::cout << "  " << (i + 1) << ". Distance: "
                  << std::fixed << std::setprecision(3) << result.distance;

        // Find actor name
        for (const auto& info : actorInfos) {
            if (info.actor == result.actor) {
                std::cout << " (" << info.name << ")";
                break;
            }
        }
        std::cout << std::endl;
    }

    std::cout << "\n✓ Test 3 completed successfully!" << std::endl;
}

// ============================================================================
// Test 4: Geometric Primitives
// ============================================================================

void test4_GeometricPrimitives() {
    printSeparator("Test 4: Geometric Primitives");

    std::cout << "Testing geometric primitive distance functions..." << std::endl;

    PxVec3 queryPoint(3, 4, 0);
    printVector("\nQuery point", queryPoint);

    // Test 4a: Distance to sphere
    std::cout << "\n--- Distance to Sphere ---" << std::endl;
    PxVec3 sphereCenter(0, 0, 0);
    PxReal sphereRadius = 2.0f;

    printVector("Sphere center", sphereCenter);
    std::cout << "Sphere radius: " << sphereRadius << std::endl;

    PxReal sphereDist = PointDistanceQuery::distanceToSphere(
        queryPoint, sphereCenter, sphereRadius);
    PxVec3 closestOnSphere = PointDistanceQuery::closestPointOnSphere(
        queryPoint, sphereCenter, sphereRadius);

    std::cout << "Distance: " << std::fixed << std::setprecision(3)
              << sphereDist << std::endl;
    printVector("Closest point", closestOnSphere);

    // Test 4b: Distance to AABB
    std::cout << "\n--- Distance to AABB ---" << std::endl;
    PxBounds3 aabb(PxVec3(-1, -1, -1), PxVec3(1, 1, 1));

    std::cout << "AABB: [(-1, -1, -1), (1, 1, 1)]" << std::endl;

    PxReal aabbDist = PointDistanceQuery::distanceToAABB(queryPoint, aabb);
    PxVec3 closestOnAABB = PointDistanceQuery::closestPointOnAABB(queryPoint, aabb);

    std::cout << "Distance: " << std::fixed << std::setprecision(3)
              << aabbDist << std::endl;
    printVector("Closest point", closestOnAABB);

    // Test 4c: Distance to plane
    std::cout << "\n--- Distance to Plane ---" << std::endl;
    PxPlane plane(0, 1, 0, 0); // XZ plane at Y=0

    std::cout << "Plane: Y = 0 (XZ plane)" << std::endl;

    PxReal planeDist = PointDistanceQuery::distanceToPlane(queryPoint, plane);
    PxVec3 closestOnPlane = PointDistanceQuery::closestPointOnPlane(queryPoint, plane);

    std::cout << "Signed distance: " << std::fixed << std::setprecision(3)
              << planeDist << std::endl;
    printVector("Closest point", closestOnPlane);

    std::cout << "\n✓ Test 4 completed successfully!" << std::endl;
}

// ============================================================================
// Test 5: Distance Field Generation
// ============================================================================

void test5_DistanceFieldGeneration() {
    printSeparator("Test 5: Distance Field Generation");

    std::cout << "Generating 3D distance field around a sphere..." << std::endl;

    // Create sphere
    PxSphereGeometry sphere(1.0f);
    PxTransform pose(PxVec3(0, 0, 0));

    std::cout << "Sphere: center (0, 0, 0), radius 1.0" << std::endl;

    // Define field bounds
    PxBounds3 bounds(PxVec3(-3, -3, -3), PxVec3(3, 3, 3));
    PxU32 resolution = 16; // 16x16x16 grid

    std::cout << "Field bounds: [(-3, -3, -3), (3, 3, 3)]" << std::endl;
    std::cout << "Resolution: " << resolution << "x" << resolution << "x" << resolution
              << " = " << (resolution * resolution * resolution) << " voxels" << std::endl;

    // Generate distance field
    std::cout << "\nGenerating distance field..." << std::endl;
    std::vector<PxReal> distanceField = DistanceFieldGenerator::generateDistanceField(
        sphere, pose, bounds, resolution);

    std::cout << "✓ Generated distance field with " << distanceField.size() << " voxels" << std::endl;

    // Sample distance field
    std::cout << "\n--- Sampling Distance Field ---" << std::endl;

    std::vector<PxVec3> samplePoints = {
        PxVec3(0, 0, 0),     // Center (inside)
        PxVec3(1, 0, 0),     // On surface
        PxVec3(2, 0, 0),     // Outside
        PxVec3(1.5f, 1.5f, 0) // Outside diagonal
    };

    for (const PxVec3& point : samplePoints) {
        PxReal sampledDist = DistanceFieldGenerator::sampleDistanceField(
            distanceField, bounds, resolution, point);

        printVector("Sample point", point);
        std::cout << "  Sampled distance: " << std::fixed << std::setprecision(3)
                  << sampledDist << std::endl;
    }

    // Compute field statistics
    PxReal minDist = PX_MAX_F32;
    PxReal maxDist = -PX_MAX_F32;
    PxReal avgDist = 0.0f;

    for (PxReal dist : distanceField) {
        minDist = PxMin(minDist, dist);
        maxDist = PxMax(maxDist, dist);
        avgDist += dist;
    }
    avgDist /= distanceField.size();

    std::cout << "\n--- Distance Field Statistics ---" << std::endl;
    std::cout << "  Min distance: " << std::fixed << std::setprecision(3) << minDist << std::endl;
    std::cout << "  Max distance: " << maxDist << std::endl;
    std::cout << "  Average distance: " << avgDist << std::endl;

    std::cout << "\n✓ Test 5 completed successfully!" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "PhysXWrapper - Point Distance Query Examples" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "\nPoint distance queries are useful for:" << std::endl;
    std::cout << "- AI pathfinding (finding nearest obstacles)" << std::endl;
    std::cout << "- Collision prediction" << std::endl;
    std::cout << "- Proximity detection" << std::endl;
    std::cout << "- Surface snapping" << std::endl;
    std::cout << "\nStarting tests...\n" << std::endl;

    try {
        // Test 1: Basic geometry queries
        test1_BasicGeometryQueries();

        // Test 2: Batch queries
        test2_BatchQueries();

        // Test 3: Scene queries
        test3_SceneQueries();

        // Test 4: Geometric primitives
        test4_GeometricPrimitives();

        // Test 5: Distance field generation
        test5_DistanceFieldGeneration();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
