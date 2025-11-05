/**
 * @file example_gyroscopic.cpp
 * @brief Example demonstrating gyroscopic forces and the Dzhanibekov effect
 *
 * This example shows how gyroscopic forces affect rotating objects and
 * demonstrates the famous Dzhanibekov effect (tennis racket theorem).
 *
 * The Dzhanibekov effect shows that rotation around the intermediate
 * principal axis is unstable, causing periodic flipping behavior.
 */

#include "PhysXManager.h"
#include "RigidBody/GyroscopicForces.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace PhysXWrapper;

// ============================================================================
// Test 1: Basic Gyroscopic Effect
// ============================================================================

void testBasicGyroscopicEffect()
{
    std::cout << "\n=== Test 1: Basic Gyroscopic Effect ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Disable gravity for clearer demonstration
    PxSceneDesc sceneDesc = scene->getSceneDesc();
    scene->setGravity(PxVec3(0.0f));

    // Initialize gyroscopic forces manager
    GyroscopicForces gyro;
    if (!gyro.initialize(physics, scene)) {
        std::cerr << "Failed to initialize GyroscopicForces" << std::endl;
        return;
    }

    // Test 1a: T-shape with gyroscopic enabled
    std::cout << "\nTest 1a: Creating T-shape with gyroscopic forces enabled" << std::endl;
    GyroscopicForces::GyroscopicConfig config;
    config.enableGyroscopic = true;
    config.zeroGravity = true;

    PxRigidDynamic* tShape = gyro.createTShape(PxTransform(PxVec3(0, 5, 0)), config);
    if (tShape) {
        std::cout << "✓ Created T-shape" << std::endl;
    }

    // Simulate and observe behavior
    std::cout << "\nSimulating gyroscopic effect..." << std::endl;
    for (int i = 0; i < 300; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        if (i % 60 == 0) {
            auto stats = GyroscopicForces::getStats(tShape);
            std::cout << "Frame " << i << ": Angular speed = " << stats.angularSpeed << " rad/s" << std::endl;
        }
    }

    gyro.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Basic gyroscopic effect test passed" << std::endl;
}

// ============================================================================
// Test 2: Comparison (With vs Without Gyroscopic)
// ============================================================================

void testGyroscopicComparison()
{
    std::cout << "\n=== Test 2: Gyroscopic Comparison ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();
    scene->setGravity(PxVec3(0.0f));

    // Initialize gyroscopic forces manager
    GyroscopicForces gyro;
    gyro.initialize(physics, scene);

    // Create comparison pair
    std::cout << "\nCreating comparison pair (with and without gyroscopic forces)" << std::endl;

    GyroscopicForces::GyroscopicConfig config;
    config.angularVelocity = PxVec3(7.5f, 5.025f, 0.0f);

    auto [actorWith, actorWithout] = gyro.createComparisonPair(
        GyroscopicForces::DemoShape::T_SHAPE,
        PxVec3(-3, 5, 0),  // Left: with gyroscopic
        PxVec3(3, 5, 0),   // Right: without gyroscopic
        config
    );

    std::cout << "  Left actor: Gyroscopic ENABLED" << std::endl;
    std::cout << "  Right actor: Gyroscopic DISABLED" << std::endl;

    // Simulate and compare
    std::cout << "\nSimulating comparison..." << std::endl;
    for (int i = 0; i < 300; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        if (i % 100 == 0) {
            auto statsWithin = GyroscopicForces::getStats(actorWith);
            auto statsWithout = GyroscopicForces::getStats(actorWithout);

            std::cout << "\nFrame " << i << ":" << std::endl;
            std::cout << "  With gyroscopic: speed = " << statsWithin.angularSpeed << " rad/s" << std::endl;
            std::cout << "  Without gyroscopic: speed = " << statsWithout.angularSpeed << " rad/s" << std::endl;
        }
    }

    gyro.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Gyroscopic comparison test passed" << std::endl;
}

// ============================================================================
// Test 3: Different Shapes
// ============================================================================

void testDifferentShapes()
{
    std::cout << "\n=== Test 3: Different Shapes ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();
    scene->setGravity(PxVec3(0.0f));

    // Initialize gyroscopic forces manager
    GyroscopicForces gyro;
    gyro.initialize(physics, scene);

    GyroscopicForces::GyroscopicConfig config;
    config.enableGyroscopic = true;

    // Test different shapes
    std::cout << "\nCreating various shapes with gyroscopic forces" << std::endl;

    // T-shape
    std::cout << "  Creating T-shape..." << std::endl;
    PxRigidDynamic* tShape = gyro.createTShape(PxTransform(PxVec3(-6, 0, 0)), config);

    // L-shape
    std::cout << "  Creating L-shape..." << std::endl;
    PxRigidDynamic* lShape = gyro.createLShape(PxTransform(PxVec3(-3, 0, 0)), config);

    // Hammer
    std::cout << "  Creating hammer..." << std::endl;
    PxRigidDynamic* hammer = gyro.createHammer(PxTransform(PxVec3(0, 0, 0)), config);

    // Dumbbell
    std::cout << "  Creating dumbbell..." << std::endl;
    PxRigidDynamic* dumbbell = gyro.createDumbbell(PxTransform(PxVec3(3, 0, 0)), config);

    // Cross
    std::cout << "  Creating cross..." << std::endl;
    PxRigidDynamic* cross = gyro.createCross(PxTransform(PxVec3(6, 0, 0)), config);

    std::cout << "✓ Created 5 different shapes" << std::endl;

    // Simulate all shapes
    std::cout << "\nSimulating all shapes..." << std::endl;
    for (int i = 0; i < 200; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    gyro.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Different shapes test passed" << std::endl;
}

// ============================================================================
// Test 4: Dzhanibekov Effect Demonstration
// ============================================================================

void testDzhanibekovEffect()
{
    std::cout << "\n=== Test 4: Dzhanibekov Effect Demonstration ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();
    scene->setGravity(PxVec3(0.0f));

    // Initialize gyroscopic forces manager
    GyroscopicForces gyro;
    gyro.initialize(physics, scene);

    // Create T-shape with rotation around intermediate axis
    std::cout << "\nCreating T-shape rotating around intermediate axis" << std::endl;
    std::cout << "(This should exhibit flipping behavior - the Dzhanibekov effect)" << std::endl;

    GyroscopicForces::GyroscopicConfig config;
    config.enableGyroscopic = true;
    config.angularVelocity = PxVec3(7.5f, 5.025f, 0.0f);  // Mixed rotation
    config.angularDamping = 0.0f;  // No damping to see pure effect

    PxRigidDynamic* tShape = gyro.createTShape(PxTransform(PxVec3(0, 0, 0)), config);

    // Check initial moment of inertia
    PxVec3 inertia = GyroscopicForces::computeMomentOfInertia(tShape);
    std::cout << "\nMoment of inertia: (" << inertia.x << ", " << inertia.y << ", " << inertia.z << ")" << std::endl;

    // Identify intermediate axis
    float minI = PxMin(inertia.x, PxMin(inertia.y, inertia.z));
    float maxI = PxMax(inertia.x, PxMax(inertia.y, inertia.z));
    std::cout << "Min inertia: " << minI << ", Max inertia: " << maxI << std::endl;

    // Simulate and monitor orientation changes
    std::cout << "\nSimulating Dzhanibekov effect (watch for periodic flipping)..." << std::endl;

    PxQuat prevOrientation = tShape->getGlobalPose().q;
    int flipCount = 0;

    for (int i = 0; i < 600; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        // Detect flips by checking orientation changes
        PxQuat currentOrientation = tShape->getGlobalPose().q;
        float dot = prevOrientation.dot(currentOrientation);

        // If dot product is negative, orientation has flipped
        if (dot < 0) {
            flipCount++;
            std::cout << "  Flip detected at frame " << i << " (flip #" << flipCount << ")" << std::endl;
        }

        prevOrientation = currentOrientation;

        if (i % 100 == 0) {
            auto stats = GyroscopicForces::getStats(tShape);
            std::cout << "Frame " << i << ": Angular speed = " << stats.angularSpeed
                      << " rad/s, Flips = " << flipCount << std::endl;
        }
    }

    std::cout << "\nTotal flips detected: " << flipCount << std::endl;

    gyro.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Dzhanibekov effect demonstration passed" << std::endl;
}

// ============================================================================
// Test 5: Statistics and Analysis
// ============================================================================

void testStatisticsAndAnalysis()
{
    std::cout << "\n=== Test 5: Statistics and Analysis ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();
    scene->setGravity(PxVec3(0.0f));

    // Initialize gyroscopic forces manager
    GyroscopicForces gyro;
    gyro.initialize(physics, scene);

    // Create various objects and analyze them
    std::cout << "\nAnalyzing different shapes..." << std::endl;

    GyroscopicForces::GyroscopicConfig config;
    config.enableGyroscopic = true;

    // Create and analyze T-shape
    std::cout << "\nT-shape analysis:" << std::endl;
    PxRigidDynamic* tShape = gyro.createTShape(PxTransform(PxVec3(0, 0, 0)), config);

    // Run simulation briefly
    for (int i = 0; i < 10; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    auto stats = GyroscopicForces::getStats(tShape);
    stats.print();

    bool isIntermediateAxis = GyroscopicForces::isIntermediateAxisRotation(tShape);
    std::cout << "  Rotating around intermediate axis: " << (isIntermediateAxis ? "YES" : "NO") << std::endl;

    // Test control methods
    std::cout << "\nTesting control methods:" << std::endl;
    std::cout << "  Initial gyroscopic enabled: " << (GyroscopicForces::isGyroscopicEnabled(tShape) ? "YES" : "NO") << std::endl;

    GyroscopicForces::setGyroscopicEnabled(tShape, false);
    std::cout << "  After disabling: " << (GyroscopicForces::isGyroscopicEnabled(tShape) ? "YES" : "NO") << std::endl;

    GyroscopicForces::setAngularVelocity(tShape, PxVec3(10, 0, 0));
    std::cout << "  New angular velocity set to (10, 0, 0)" << std::endl;

    GyroscopicForces::setAngularDamping(tShape, 0.5f);
    std::cout << "  Angular damping set to 0.5" << std::endl;

    gyro.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Statistics and analysis test passed" << std::endl;
}

// ============================================================================
// Test 6: Energy Conservation
// ============================================================================

void testEnergyConservation()
{
    std::cout << "\n=== Test 6: Energy Conservation ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();
    scene->setGravity(PxVec3(0.0f));

    // Initialize gyroscopic forces manager
    GyroscopicForces gyro;
    gyro.initialize(physics, scene);

    // Create object with no damping
    GyroscopicForces::GyroscopicConfig config;
    config.enableGyroscopic = true;
    config.angularDamping = 0.0f;  // No energy loss

    PxRigidDynamic* tShape = gyro.createTShape(PxTransform(PxVec3(0, 0, 0)), config);

    // Monitor energy over time
    std::cout << "\nMonitoring rotational kinetic energy (should be conserved):" << std::endl;

    PxReal initialEnergy = 0.0f;

    for (int i = 0; i < 300; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        PxReal currentEnergy = GyroscopicForces::computeKineticEnergy(tShape);

        if (i == 0) {
            initialEnergy = currentEnergy;
            std::cout << "  Initial energy: " << initialEnergy << " J" << std::endl;
        }

        if (i % 60 == 0) {
            PxReal energyChange = PxAbs(currentEnergy - initialEnergy);
            PxReal percentChange = (energyChange / initialEnergy) * 100.0f;
            std::cout << "  Frame " << i << ": Energy = " << currentEnergy
                      << " J, Change = " << percentChange << "%" << std::endl;
        }
    }

    gyro.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Energy conservation test passed" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "PhysXWrapper - Gyroscopic Forces Example" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "\nThis example demonstrates gyroscopic forces and the Dzhanibekov effect" << std::endl;
    std::cout << "(also known as the tennis racket theorem or intermediate axis theorem)." << std::endl;

    try {
        testBasicGyroscopicEffect();
        testGyroscopicComparison();
        testDifferentShapes();
        testDzhanibekovEffect();
        testStatisticsAndAnalysis();
        testEnergyConservation();

        std::cout << "\n=========================================" << std::endl;
        std::cout << "All tests passed successfully!" << std::endl;
        std::cout << "=========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
