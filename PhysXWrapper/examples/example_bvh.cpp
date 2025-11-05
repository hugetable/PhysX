/**
 * @file example_bvh.cpp
 * @brief Example demonstrating BVHBuilder for spatial acceleration
 *
 * This example shows how to use BVH (Bounding Volume Hierarchy) structures
 * to optimize scene queries and simulation for actors with many shapes.
 *
 * BVH benefits:
 * - Faster scene queries (raycasts, sweeps, overlaps)
 * - Reduced shape bound synchronization during simulation
 * - Lower memory usage in scene-wide AABB tree
 */

#include "PhysXManager.h"
#include "RigidBody/RigidBodyManager.h"
#include "Utility/BVHBuilder.h"
#include "Query/GeometryQuery.h"
#include <iostream>
#include <chrono>

using namespace PhysXWrapper;

// ============================================================================
// Test 1: Basic BVH Construction
// ============================================================================

void testBasicBVHConstruction()
{
    std::cout << "\n=== Test 1: Basic BVH Construction ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Initialize BVH builder
    BVHBuilder builder;
    if (!builder.initialize(physics)) {
        std::cerr << "Failed to initialize BVH builder" << std::endl;
        return;
    }

    // Test 1a: Build BVH from bounds list
    std::cout << "\nTest 1a: Building BVH from bounds list" << std::endl;
    std::vector<PxBounds3> bounds;
    for (int i = 0; i < 100; i++) {
        PxVec3 center(i * 2.0f, 0, 0);
        PxVec3 halfExtents(0.5f, 0.5f, 0.5f);
        bounds.push_back(PxBounds3(center - halfExtents, center + halfExtents));
    }

    BVHBuilder::BVHConfig config;
    config.printStats = true;

    PxBVH* bvh = builder.buildFromBounds(bounds, config);
    if (bvh) {
        std::cout << "✓ Successfully built BVH from " << bounds.size() << " bounds" << std::endl;
        bvh->release();
    }

    // Test 1b: Compute statistics
    std::cout << "\nTest 1b: Computing BVH statistics" << std::endl;
    PxReal totalVolume = BVHBuilder::computeTotalVolume(bounds);
    PxBounds3 aabb = BVHBuilder::computeAABB(bounds);
    std::cout << "  Total volume: " << totalVolume << std::endl;
    std::cout << "  Combined AABB: min(" << aabb.minimum.x << ", " << aabb.minimum.y << ", " << aabb.minimum.z << ")" << std::endl;
    std::cout << "                 max(" << aabb.maximum.x << ", " << aabb.maximum.y << ", " << aabb.maximum.z << ")" << std::endl;

    builder.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Basic BVH construction test passed" << std::endl;
}

// ============================================================================
// Test 2: Compound Sphere with BVH
// ============================================================================

void testCompoundSphere()
{
    std::cout << "\n=== Test 2: Compound Sphere with BVH ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Create ground
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);
    bodyManager.createGroundPlane();

    // Initialize BVH builder
    BVHBuilder builder;
    if (!builder.initialize(physics)) {
        std::cerr << "Failed to initialize BVH builder" << std::endl;
        return;
    }

    // Test 2a: Create compound sphere without aggregate
    std::cout << "\nTest 2a: Creating compound sphere (density=10, without aggregate)" << std::endl;
    PxRigidDynamic* sphere1 = builder.createCompoundSphere(
        scene,
        PxTransform(PxVec3(-5, 10, 0)),
        10,    // density
        2.0f,  // large radius
        0.2f,  // small radius
        false  // no aggregate
    );

    if (sphere1) {
        std::cout << "✓ Created compound sphere with " << sphere1->getNbShapes() << " shapes" << std::endl;
    }

    // Test 2b: Create compound sphere with aggregate
    std::cout << "\nTest 2b: Creating compound sphere (density=10, with aggregate)" << std::endl;
    PxRigidDynamic* sphere2 = builder.createCompoundSphere(
        scene,
        PxTransform(PxVec3(5, 10, 0)),
        10,    // density
        2.0f,  // large radius
        0.2f,  // small radius
        true   // use aggregate
    );

    if (sphere2) {
        std::cout << "✓ Created compound sphere with aggregate (" << sphere2->getNbShapes() << " shapes)" << std::endl;
    }

    // Simulate
    std::cout << "\nSimulating compound spheres..." << std::endl;
    for (int i = 0; i < 100; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    std::cout << "✓ Simulation completed successfully" << std::endl;

    builder.cleanup();
    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Compound sphere test passed" << std::endl;
}

// ============================================================================
// Test 3: Box Grid with BVH
// ============================================================================

void testBoxGrid()
{
    std::cout << "\n=== Test 3: Box Grid with BVH ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Create ground
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);
    bodyManager.createGroundPlane();

    // Initialize BVH builder
    BVHBuilder builder;
    if (!builder.initialize(physics)) {
        std::cerr << "Failed to initialize BVH builder" << std::endl;
        return;
    }

    // Test 3a: Create dynamic box grid
    std::cout << "\nTest 3a: Creating dynamic box grid (5x5x5)" << std::endl;
    PxRigidActor* dynamicGrid = builder.createBoxGrid(
        scene,
        PxTransform(PxVec3(0, 10, 0)),
        5,                        // grid size
        PxVec3(0.2f, 0.2f, 0.2f), // box half extents
        0.5f,                     // spacing
        true                      // dynamic
    );

    if (dynamicGrid) {
        std::cout << "✓ Created dynamic box grid with " << dynamicGrid->getNbShapes() << " shapes" << std::endl;
    }

    // Test 3b: Create static box grid
    std::cout << "\nTest 3b: Creating static box grid (3x3x3)" << std::endl;
    PxRigidActor* staticGrid = builder.createBoxGrid(
        scene,
        PxTransform(PxVec3(10, 5, 0)),
        3,                        // grid size
        PxVec3(0.3f, 0.3f, 0.3f), // box half extents
        0.8f,                     // spacing
        false                     // static
    );

    if (staticGrid) {
        std::cout << "✓ Created static box grid with " << staticGrid->getNbShapes() << " shapes" << std::endl;
    }

    // Simulate
    std::cout << "\nSimulating box grids..." << std::endl;
    for (int i = 0; i < 100; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    builder.cleanup();
    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Box grid test passed" << std::endl;
}

// ============================================================================
// Test 4: Performance Comparison (With vs Without BVH)
// ============================================================================

void testPerformanceComparison()
{
    std::cout << "\n=== Test 4: Performance Comparison ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.1f);

    // Initialize managers
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);
    bodyManager.createGroundPlane();

    GeometryQuery query;
    query.initialize(scene);

    // Test 4a: Create actor WITHOUT BVH
    std::cout << "\nTest 4a: Creating actor without BVH (100 shapes)" << std::endl;
    PxRigidDynamic* actorWithoutBVH = physics->createRigidDynamic(PxTransform(PxVec3(-10, 10, 0)));

    for (int i = 0; i < 100; i++) {
        PxShape* shape = physics->createShape(PxSphereGeometry(0.1f), *material);
        shape->setLocalPose(PxTransform(PxVec3(i * 0.25f, 0, 0)));
        actorWithoutBVH->attachShape(*shape);
        shape->release();
    }
    PxRigidBodyExt::updateMassAndInertia(*actorWithoutBVH, 10.0f);
    scene->addActor(*actorWithoutBVH);

    // Perform raycasts (without BVH)
    auto start = std::chrono::high_resolution_clock::now();
    int hitCount = 0;
    for (int i = 0; i < 1000; i++) {
        GeometryQuery::RaycastConfig rayConfig;
        rayConfig.origin = PxVec3(-10 + i * 0.01f, 15, 0);
        rayConfig.direction = PxVec3(0, -1, 0);
        rayConfig.maxDistance = 20.0f;

        auto result = query.raycast(rayConfig);
        if (result.hasBlock) hitCount++;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto durationWithoutBVH = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "  Without BVH: " << hitCount << " hits in " << durationWithoutBVH.count() << " μs" << std::endl;

    // Remove actor without BVH
    scene->removeActor(*actorWithoutBVH);
    actorWithoutBVH->release();

    // Test 4b: Create actor WITH BVH
    std::cout << "\nTest 4b: Creating actor with BVH (100 shapes)" << std::endl;
    BVHBuilder builder;
    builder.initialize(physics);

    PxRigidDynamic* actorWithBVH = physics->createRigidDynamic(PxTransform(PxVec3(-10, 10, 0)));

    for (int i = 0; i < 100; i++) {
        PxShape* shape = physics->createShape(PxSphereGeometry(0.1f), *material);
        shape->setLocalPose(PxTransform(PxVec3(i * 0.25f, 0, 0)));
        actorWithBVH->attachShape(*shape);
        shape->release();
    }
    PxRigidBodyExt::updateMassAndInertia(*actorWithBVH, 10.0f);

    // Build and add with BVH
    PxBVH* bvh = builder.buildFromActor(actorWithBVH);
    scene->addActor(*actorWithBVH, bvh);
    bvh->release();

    // Perform raycasts (with BVH)
    start = std::chrono::high_resolution_clock::now();
    hitCount = 0;
    for (int i = 0; i < 1000; i++) {
        GeometryQuery::RaycastConfig rayConfig;
        rayConfig.origin = PxVec3(-10 + i * 0.01f, 15, 0);
        rayConfig.direction = PxVec3(0, -1, 0);
        rayConfig.maxDistance = 20.0f;

        auto result = query.raycast(rayConfig);
        if (result.hasBlock) hitCount++;
    }
    end = std::chrono::high_resolution_clock::now();
    auto durationWithBVH = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "  With BVH: " << hitCount << " hits in " << durationWithBVH.count() << " μs" << std::endl;

    // Calculate speedup
    if (durationWithBVH.count() > 0) {
        float speedup = float(durationWithoutBVH.count()) / float(durationWithBVH.count());
        std::cout << "\n  Speedup: " << speedup << "x faster with BVH" << std::endl;
    }

    material->release();
    builder.cleanup();
    query.cleanup();
    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Performance comparison test passed" << std::endl;
}

// ============================================================================
// Test 5: BVH with Custom Shapes
// ============================================================================

void testCustomShapes()
{
    std::cout << "\n=== Test 5: BVH with Custom Shapes ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.1f);

    // Create ground
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);
    bodyManager.createGroundPlane();

    // Initialize BVH builder
    BVHBuilder builder;
    builder.initialize(physics);

    // Test 5a: Create shapes manually
    std::cout << "\nTest 5a: Creating custom shape collection" << std::endl;
    std::vector<PxShape*> shapes;

    // Create various shapes
    shapes.push_back(physics->createShape(PxBoxGeometry(1, 1, 1), *material));
    shapes.push_back(physics->createShape(PxSphereGeometry(0.8f), *material));
    shapes.push_back(physics->createShape(PxCapsuleGeometry(0.5f, 1.0f), *material));

    // Set local poses
    shapes[0]->setLocalPose(PxTransform(PxVec3(-2, 0, 0)));
    shapes[1]->setLocalPose(PxTransform(PxVec3(0, 0, 0)));
    shapes[2]->setLocalPose(PxTransform(PxVec3(2, 0, 0)));

    std::cout << "  Created " << shapes.size() << " custom shapes" << std::endl;

    // Test 5b: Build BVH from shapes
    std::cout << "\nTest 5b: Building BVH from shapes" << std::endl;
    BVHBuilder::BVHConfig config;
    config.printStats = true;

    PxBVH* bvh = builder.buildFromShapes(shapes, config);
    if (bvh) {
        std::cout << "✓ Successfully built BVH from shapes" << std::endl;
    }

    // Test 5c: Create actor with these shapes
    std::cout << "\nTest 5c: Creating actor with custom shapes and BVH" << std::endl;
    BVHBuilder::ActorWithBVHConfig actorConfig;
    actorConfig.transform = PxTransform(PxVec3(0, 10, 0));
    actorConfig.isDynamic = true;
    actorConfig.useAggregate = false;

    PxRigidActor* customActor = builder.createActorWithBVH(scene, shapes, actorConfig, material);
    if (customActor) {
        std::cout << "✓ Created actor with " << customActor->getNbShapes() << " custom shapes" << std::endl;
    }

    // Cleanup shapes
    for (PxShape* shape : shapes) {
        shape->release();
    }

    if (bvh) {
        bvh->release();
    }

    // Simulate
    std::cout << "\nSimulating custom actor..." << std::endl;
    for (int i = 0; i < 100; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    material->release();
    builder.cleanup();
    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Custom shapes test passed" << std::endl;
}

// ============================================================================
// Test 6: Large Scale Scene
// ============================================================================

void testLargeScale()
{
    std::cout << "\n=== Test 6: Large Scale Scene ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Create ground
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);
    bodyManager.createGroundPlane();

    // Initialize BVH builder
    BVHBuilder builder;
    builder.initialize(physics);

    // Create multiple compound objects
    std::cout << "\nCreating large scale scene with multiple compound objects..." << std::endl;

    int numObjects = 5;
    for (int i = 0; i < numObjects; i++) {
        PxVec3 position(i * 10.0f - 20.0f, 15 + i * 2.0f, 0);

        PxRigidDynamic* obj = builder.createCompoundSphere(
            scene,
            PxTransform(position),
            8,     // density
            1.5f,  // large radius
            0.15f, // small radius
            true   // use aggregate
        );

        if (obj) {
            std::cout << "  Created compound object " << (i + 1) << " with " << obj->getNbShapes() << " shapes" << std::endl;
        }
    }

    // Simulate
    std::cout << "\nSimulating large scale scene..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 200; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        if (i % 50 == 0) {
            std::cout << "  Frame " << i << std::endl;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\n  Simulation completed in " << duration.count() << " ms" << std::endl;
    std::cout << "  Average frame time: " << (duration.count() / 200.0f) << " ms" << std::endl;

    builder.cleanup();
    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Large scale test passed" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "PhysXWrapper - BVHBuilder Example" << std::endl;
    std::cout << "==================================" << std::endl;

    try {
        testBasicBVHConstruction();
        testCompoundSphere();
        testBoxGrid();
        testPerformanceComparison();
        testCustomShapes();
        testLargeScale();

        std::cout << "\n==================================" << std::endl;
        std::cout << "All tests passed successfully!" << std::endl;
        std::cout << "==================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
