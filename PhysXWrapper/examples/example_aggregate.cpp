/**
 * @file example_aggregate.cpp
 * @brief Example demonstrating AggregateManager usage
 *
 * This example shows how to use the AggregateManager class for
 * grouping actors for improved performance, including ragdolls,
 * debris piles, and vehicle assemblies.
 */

#include "PhysXCore.h"
#include "RigidBody/AggregateManager.h"
#include <iostream>
#include <vector>

using namespace PhysXWrapper;

// ============================================================================
// Helper Functions
// ============================================================================

void printSeparator(const std::string& title)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================\n" << std::endl;
}

PxRigidDynamic* createDynamicBox(PxPhysics* physics, const PxTransform& transform,
                                  const PxVec3& halfExtents, PxMaterial* material)
{
    PxRigidDynamic* body = PxCreateDynamic(*physics, transform,
                                            PxBoxGeometry(halfExtents), *material, 10.0f);
    return body;
}

PxRigidDynamic* createDynamicSphere(PxPhysics* physics, const PxTransform& transform,
                                     PxReal radius, PxMaterial* material)
{
    PxRigidDynamic* body = PxCreateDynamic(*physics, transform,
                                            PxSphereGeometry(radius), *material, 10.0f);
    return body;
}

void createGround(PxPhysics* physics, PxScene* scene)
{
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.1f);
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);
}

// ============================================================================
// Test 1: Basic Aggregate
// ============================================================================

void test_BasicAggregate()
{
    printSeparator("Test 1: Basic Aggregate");

    // Initialize PhysX
    PhysXCore core;
    if (!core.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    // Initialize aggregate manager
    AggregateManager manager;
    if (!manager.initialize(physics, scene)) {
        std::cerr << "Failed to initialize aggregate manager" << std::endl;
        return;
    }

    std::cout << "Creating aggregate with 10 actors..." << std::endl;

    // Create aggregate
    PxAggregate* aggregate = manager.createAggregate(10, true);
    if (!aggregate) {
        std::cerr << "Failed to create aggregate" << std::endl;
        return;
    }

    // Create actors
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);
    std::vector<PxRigidDynamic*> actors;

    for (int i = 0; i < 5; i++) {
        PxTransform transform(PxVec3(i * 2.0f, 5.0f, 0.0f));
        PxRigidDynamic* actor = createDynamicBox(physics, transform,
                                                  PxVec3(0.5f, 0.5f, 0.5f), material);
        actors.push_back(actor);
    }

    // Add actors to aggregate
    PxU32 count = manager.addActors(aggregate, std::vector<PxActor*>(actors.begin(), actors.end()));
    std::cout << "Added " << count << " actors to aggregate" << std::endl;

    // Add aggregate to scene
    if (manager.addToScene(aggregate)) {
        std::cout << "Aggregate added to scene" << std::endl;
    }

    // Print statistics
    AggregateManager::printAggregate(aggregate, true);

    core.cleanup();
}

// ============================================================================
// Test 2: Ragdoll Simulation
// ============================================================================

void test_RagdollAggregate()
{
    printSeparator("Test 2: Ragdoll Simulation");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    AggregateManager manager;
    manager.initialize(physics, scene);

    std::cout << "Creating ragdoll aggregate..." << std::endl;

    // Create ragdoll aggregate (no self-collision)
    PxAggregate* ragdoll = manager.createRagdollAggregate(15);

    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.3f);

    // Create simplified ragdoll parts
    struct BodyPart {
        PxVec3 position;
        PxVec3 halfExtents;
    };

    std::vector<BodyPart> parts = {
        {{0, 12, 0}, {0.3f, 0.5f, 0.2f}},   // Head
        {{0, 11, 0}, {0.4f, 0.6f, 0.3f}},   // Torso
        {{0, 10, 0}, {0.3f, 0.5f, 0.2f}},   // Pelvis
        {{-0.5f, 10.5f, 0}, {0.15f, 0.5f, 0.15f}},  // Left arm
        {{0.5f, 10.5f, 0}, {0.15f, 0.5f, 0.15f}},   // Right arm
        {{-0.3f, 9, 0}, {0.2f, 0.6f, 0.2f}},        // Left leg upper
        {{0.3f, 9, 0}, {0.2f, 0.6f, 0.2f}},         // Right leg upper
        {{-0.3f, 8, 0}, {0.15f, 0.5f, 0.15f}},      // Left leg lower
        {{0.3f, 8, 0}, {0.15f, 0.5f, 0.15f}},       // Right leg lower
    };

    std::cout << "Creating " << parts.size() << " body parts..." << std::endl;

    for (const auto& part : parts) {
        PxTransform transform(part.position);
        PxRigidDynamic* actor = createDynamicBox(physics, transform, part.halfExtents, material);
        manager.addActor(ragdoll, actor);
    }

    manager.addToScene(ragdoll);

    std::cout << "Ragdoll created with " << AggregateManager::getActorCount(ragdoll) << " parts" << std::endl;
    AggregateManager::printAggregate(ragdoll);

    core.cleanup();
}

// ============================================================================
// Test 3: Debris Pile
// ============================================================================

void test_DebrisAggregate()
{
    printSeparator("Test 3: Debris Pile");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    AggregateManager manager;
    manager.initialize(physics, scene);

    std::cout << "Creating debris aggregate with 50 pieces..." << std::endl;

    // Create debris aggregate (with self-collision)
    PxAggregate* debris = manager.createDebrisAggregate(50);

    PxMaterial* material = physics->createMaterial(0.6f, 0.6f, 0.2f);

    // Create debris pieces in a pile
    for (int i = 0; i < 50; i++) {
        PxReal x = (rand() % 100 - 50) / 50.0f * 2.0f;
        PxReal y = 5.0f + (rand() % 100) / 50.0f * 3.0f;
        PxReal z = (rand() % 100 - 50) / 50.0f * 2.0f;

        PxTransform transform(PxVec3(x, y, z));
        PxRigidDynamic* piece = createDynamicBox(physics, transform,
                                                  PxVec3(0.2f, 0.2f, 0.2f), material);
        manager.addActor(debris, piece);
    }

    manager.addToScene(debris);

    std::cout << "Debris pile created" << std::endl;
    AggregateManager::printAggregate(debris);

    // Simulate for a bit
    std::cout << "\nSimulating debris settling..." << std::endl;
    for (int i = 0; i < 100; i++) {
        scene->simulate(0.016f);
        scene->fetchResults(true);
    }

    std::cout << "Simulation complete" << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 4: Vehicle Assembly
// ============================================================================

void test_VehicleAggregate()
{
    printSeparator("Test 4: Vehicle Assembly");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    AggregateManager manager;
    manager.initialize(physics, scene);

    std::cout << "Creating vehicle aggregate..." << std::endl;

    // Create vehicle aggregate (chassis + 4 wheels)
    PxAggregate* vehicle = manager.createVehicleAggregate(4);

    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.3f);

    // Create chassis
    PxTransform chassisTransform(PxVec3(0, 3, 0));
    PxRigidDynamic* chassis = createDynamicBox(physics, chassisTransform,
                                                PxVec3(1.5f, 0.5f, 2.0f), material);
    manager.addActor(vehicle, chassis);

    // Create wheels
    PxVec3 wheelPositions[] = {
        {1.2f, 2.5f, 1.5f},   // Front left
        {-1.2f, 2.5f, 1.5f},  // Front right
        {1.2f, 2.5f, -1.5f},  // Rear left
        {-1.2f, 2.5f, -1.5f}  // Rear right
    };

    for (int i = 0; i < 4; i++) {
        PxTransform wheelTransform(wheelPositions[i]);
        PxRigidDynamic* wheel = createDynamicSphere(physics, wheelTransform, 0.4f, material);
        manager.addActor(vehicle, wheel);
    }

    manager.addToScene(vehicle);

    std::cout << "Vehicle created with chassis and 4 wheels" << std::endl;
    AggregateManager::printAggregate(vehicle, true);

    core.cleanup();
}

// ============================================================================
// Test 5: Aggregate Splitting and Merging
// ============================================================================

void test_AggregateOperations()
{
    printSeparator("Test 5: Aggregate Splitting and Merging");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    AggregateManager manager;
    manager.initialize(physics, scene);

    std::cout << "Creating mixed aggregate (dynamic and static actors)..." << std::endl;

    // Create aggregate with mixed actors
    PxAggregate* mixed = manager.createAggregate(20, true);
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    // Add dynamic actors
    for (int i = 0; i < 5; i++) {
        PxTransform transform(PxVec3(i * 1.0f, 5.0f, 0.0f));
        PxRigidDynamic* actor = createDynamicBox(physics, transform,
                                                  PxVec3(0.5f, 0.5f, 0.5f), material);
        manager.addActor(mixed, actor);
    }

    // Add static actors
    for (int i = 0; i < 5; i++) {
        PxTransform transform(PxVec3(i * 1.0f, 1.0f, 0.0f));
        PxRigidStatic* actor = PxCreateStatic(*physics, transform,
                                               PxBoxGeometry(0.5f, 0.5f, 0.5f), *material);
        manager.addActor(mixed, actor);
    }

    std::cout << "Mixed aggregate has " << AggregateManager::getActorCount(mixed) << " actors" << std::endl;

    // Split aggregate
    std::cout << "\nSplitting aggregate by type..." << std::endl;
    auto [dynamicAgg, staticAgg] = manager.splitAggregate(mixed);

    if (dynamicAgg) {
        std::cout << "Dynamic aggregate has " << AggregateManager::getActorCount(dynamicAgg) << " actors" << std::endl;
    }

    if (staticAgg) {
        std::cout << "Static aggregate has " << AggregateManager::getActorCount(staticAgg) << " actors" << std::endl;
    }

    // Create another aggregate to merge with
    PxAggregate* aggregate2 = manager.createAggregate(10, true);
    for (int i = 0; i < 3; i++) {
        PxTransform transform(PxVec3(10.0f + i * 1.0f, 5.0f, 0.0f));
        PxRigidDynamic* actor = createDynamicBox(physics, transform,
                                                  PxVec3(0.5f, 0.5f, 0.5f), material);
        manager.addActor(aggregate2, actor);
    }

    std::cout << "\nMerging two aggregates..." << std::endl;
    PxU32 count1 = AggregateManager::getActorCount(dynamicAgg);
    PxU32 count2 = AggregateManager::getActorCount(aggregate2);

    PxAggregate* merged = manager.mergeAggregates(dynamicAgg, aggregate2);
    if (merged) {
        PxU32 mergedCount = AggregateManager::getActorCount(merged);
        std::cout << "Merged aggregate has " << mergedCount << " actors (expected " << (count1 + count2) << ")" << std::endl;
    }

    core.cleanup();
}

// ============================================================================
// Test 6: Performance Comparison
// ============================================================================

void test_PerformanceComparison()
{
    printSeparator("Test 6: Performance Comparison");

    std::cout << "Creating scene with many objects..." << std::endl;

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    AggregateManager manager;
    manager.initialize(physics, scene);

    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    // Test 1: Without aggregate
    std::cout << "\nTest 1: 100 actors without aggregate" << std::endl;
    auto start1 = std::chrono::high_resolution_clock::now();

    std::vector<PxRigidDynamic*> actors1;
    for (int i = 0; i < 100; i++) {
        PxReal x = (i % 10) * 1.0f;
        PxReal y = 10.0f + (i / 10) * 1.0f;
        PxReal z = 0.0f;

        PxTransform transform(PxVec3(x, y, z));
        PxRigidDynamic* actor = createDynamicBox(physics, transform,
                                                  PxVec3(0.3f, 0.3f, 0.3f), material);
        scene->addActor(*actor);
        actors1.push_back(actor);
    }

    auto end1 = std::chrono::high_resolution_clock::now();
    float time1 = std::chrono::duration<float, std::milli>(end1 - start1).count();
    std::cout << "Creation time: " << time1 << " ms" << std::endl;

    // Clean up
    for (auto actor : actors1) {
        scene->removeActor(*actor);
        actor->release();
    }

    // Test 2: With aggregate
    std::cout << "\nTest 2: 100 actors with aggregate" << std::endl;
    auto start2 = std::chrono::high_resolution_clock::now();

    PxAggregate* aggregate = manager.createAggregate(100, true);

    for (int i = 0; i < 100; i++) {
        PxReal x = (i % 10) * 1.0f + 20.0f;
        PxReal y = 10.0f + (i / 10) * 1.0f;
        PxReal z = 0.0f;

        PxTransform transform(PxVec3(x, y, z));
        PxRigidDynamic* actor = createDynamicBox(physics, transform,
                                                  PxVec3(0.3f, 0.3f, 0.3f), material);
        manager.addActor(aggregate, actor);
    }

    manager.addToScene(aggregate);

    auto end2 = std::chrono::high_resolution_clock::now();
    float time2 = std::chrono::duration<float, std::milli>(end2 - start2).count();
    std::cout << "Creation time: " << time2 << " ms" << std::endl;

    std::cout << "\nSpeedup: " << (time1 / time2) << "x" << std::endl;

    // Print collection statistics
    std::cout << "\nAggregate collection:" << std::endl;
    AggregateManager::AggregateCollection collection = manager.getCollection();
    collection.print();

    core.cleanup();
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "PhysXWrapper - AggregateManager Example" << std::endl;
    std::cout << "======================================\n" << std::endl;

    try {
        test_BasicAggregate();
        test_RagdollAggregate();
        test_DebrisAggregate();
        test_VehicleAggregate();
        test_AggregateOperations();
        test_PerformanceComparison();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
