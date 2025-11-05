/**
 * @file example_trigger.cpp
 * @brief Trigger volume example using RigidBodyTrigger
 *
 * This example demonstrates:
 * - Creating trigger volumes (box, sphere, capsule)
 * - Detecting enter/exit events
 * - Using different trigger implementations
 * - Combining triggers with CCD
 * - Trigger-trigger overlap detection
 *
 * Based on SnippetTriggers from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include "RigidBody/RigidBodyTrigger.h"
#include <iostream>
#include <iomanip>

using namespace PhysXWrapper;
using namespace physx;

// Global stats
struct TriggerStats {
    int enterCount = 0;
    int exitCount = 0;
    int triggerTriggerCount = 0;
};

TriggerStats g_stats;

/**
 * @brief Test basic trigger functionality
 */
void testBasicTrigger(PhysXCore& physics) {
    std::cout << "\n=== TEST 1: Basic Trigger (Native Implementation) ===" << std::endl;

    g_stats = TriggerStats();

    // Create trigger manager
    RigidBodyTrigger triggerManager;

    // Set callback
    triggerManager.setTriggerCallback([](const TriggerEvent& event) {
        if (event.type == TriggerEventType::ENTER) {
            std::cout << "  -> Object ENTERED trigger!" << std::endl;
            g_stats.enterCount++;
        } else {
            std::cout << "  -> Object EXITED trigger!" << std::endl;
            g_stats.exitCount++;
        }
    });

    // Create scene with trigger support
    std::cout << "Creating scene with native triggers..." << std::endl;
    TriggerConfig config;
    config.implementation = TriggerImplementation::NATIVE;
    config.enableCCD = false;

    PxTolerancesScale scale;
    PxSceneDesc sceneDesc = triggerManager.createSceneDesc(scale, config);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);

    PxScene* scene = physics.getPhysics()->createScene(sceneDesc);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(
        *physics.getPhysics(),
        PxPlane(0, 1, 0, 0),
        *physics.getDefaultMaterial()
    );
    scene->addActor(*ground);

    // Create box trigger at height 10
    std::cout << "Creating box trigger at (0, 10, 0)..." << std::endl;
    PxRigidStatic* triggerActor = triggerManager.createBoxTrigger(
        physics.getPhysics(),
        scene,
        physics.getDefaultMaterial(),
        PxVec3(0, 10, 0),
        PxVec3(10, 1, 10),
        config
    );

    // Create falling dynamic box
    std::cout << "Creating falling box at (0, 20, 0)..." << std::endl;
    PxShape* boxShape = physics.getPhysics()->createShape(
        PxBoxGeometry(1, 1, 1),
        *physics.getDefaultMaterial()
    );
    PxRigidDynamic* dynamicBox = physics.getPhysics()->createRigidDynamic(
        PxTransform(0, 20, 0)
    );
    dynamicBox->attachShape(*boxShape);
    PxRigidBodyExt::updateMassAndInertia(*dynamicBox, 1.0f);
    scene->addActor(*dynamicBox);
    boxShape->release();

    // Simulate
    std::cout << "\nRunning simulation..." << std::endl;
    for (int i = 0; i < 300; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        // Print position every 60 frames
        if (i % 60 == 0) {
            PxTransform transform = dynamicBox->getGlobalPose();
            std::cout << "Frame " << i << ": Y = "
                      << std::fixed << std::setprecision(2)
                      << transform.p.y << std::endl;
        }
    }

    std::cout << "\nFinal stats:" << std::endl;
    std::cout << "  Enter events: " << g_stats.enterCount << std::endl;
    std::cout << "  Exit events: " << g_stats.exitCount << std::endl;

    // Cleanup
    scene->release();
}

/**
 * @brief Test trigger with CCD
 */
void testTriggerWithCCD(PhysXCore& physics) {
    std::cout << "\n=== TEST 2: Trigger with CCD ===" << std::endl;

    g_stats = TriggerStats();

    // Create trigger manager
    RigidBodyTrigger triggerManager;

    triggerManager.setTriggerCallback([](const TriggerEvent& event) {
        if (event.type == TriggerEventType::ENTER) {
            std::cout << "  -> Fast object ENTERED trigger (CCD)!" << std::endl;
            g_stats.enterCount++;
        } else {
            std::cout << "  -> Fast object EXITED trigger!" << std::endl;
            g_stats.exitCount++;
        }
    });

    // Create scene with CCD enabled
    std::cout << "Creating scene with CCD-enabled triggers..." << std::endl;
    TriggerConfig config;
    config.implementation = TriggerImplementation::FILTER_SHADER;
    config.enableCCD = true;

    PxTolerancesScale scale;
    PxSceneDesc sceneDesc = triggerManager.createSceneDesc(scale, config);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);

    PxScene* scene = physics.getPhysics()->createScene(sceneDesc);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(
        *physics.getPhysics(),
        PxPlane(0, 1, 0, 0),
        *physics.getDefaultMaterial()
    );
    scene->addActor(*ground);

    // Create thin trigger (would be missed without CCD)
    std::cout << "Creating thin trigger at (0, 10, 0)..." << std::endl;
    PxRigidStatic* triggerActor = triggerManager.createBoxTrigger(
        physics.getPhysics(),
        scene,
        physics.getDefaultMaterial(),
        PxVec3(0, 10, 0),
        PxVec3(10, 0.01f, 10),  // Very thin!
        config
    );

    // Create fast-moving small box
    std::cout << "Creating fast-moving box at (0, 30, 0)..." << std::endl;
    PxShape* boxShape = physics.getPhysics()->createShape(
        PxBoxGeometry(0.1f, 0.1f, 0.1f),
        *physics.getDefaultMaterial()
    );
    PxRigidDynamic* dynamicBox = physics.getPhysics()->createRigidDynamic(
        PxTransform(0, 30, 0)
    );
    dynamicBox->attachShape(*boxShape);
    PxRigidBodyExt::updateMassAndInertia(*dynamicBox, 1.0f);
    dynamicBox->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    dynamicBox->setLinearVelocity(PxVec3(0, -140, 0));  // Very fast!
    scene->addActor(*dynamicBox);
    boxShape->release();

    // Simulate
    std::cout << "\nRunning simulation with fast-moving object..." << std::endl;
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        if (i % 30 == 0) {
            PxTransform transform = dynamicBox->getGlobalPose();
            std::cout << "Frame " << i << ": Y = "
                      << std::fixed << std::setprecision(2)
                      << transform.p.y << std::endl;
        }
    }

    std::cout << "\nFinal stats:" << std::endl;
    std::cout << "  Enter events: " << g_stats.enterCount << std::endl;
    std::cout << "  Exit events: " << g_stats.exitCount << std::endl;
    std::cout << "  (Without CCD, the fast object would tunnel through!)" << std::endl;

    // Cleanup
    scene->release();
}

/**
 * @brief Test different trigger shapes
 */
void testDifferentShapes(PhysXCore& physics) {
    std::cout << "\n=== TEST 3: Different Trigger Shapes ===" << std::endl;

    g_stats = TriggerStats();

    // Create trigger manager
    RigidBodyTrigger triggerManager;

    triggerManager.setTriggerCallback([](const TriggerEvent& event) {
        if (event.type == TriggerEventType::ENTER) {
            g_stats.enterCount++;
        } else {
            g_stats.exitCount++;
        }
    });

    // Create scene
    TriggerConfig config;
    config.implementation = TriggerImplementation::NATIVE;

    PxTolerancesScale scale;
    PxSceneDesc sceneDesc = triggerManager.createSceneDesc(scale, config);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);

    PxScene* scene = physics.getPhysics()->createScene(sceneDesc);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(
        *physics.getPhysics(),
        PxPlane(0, 1, 0, 0),
        *physics.getDefaultMaterial()
    );
    scene->addActor(*ground);

    // Create sphere trigger
    std::cout << "Creating sphere trigger at (-10, 5, 0), radius 3..." << std::endl;
    triggerManager.createSphereTrigger(
        physics.getPhysics(),
        scene,
        physics.getDefaultMaterial(),
        PxVec3(-10, 5, 0),
        3.0f,
        config
    );

    // Create box trigger
    std::cout << "Creating box trigger at (0, 5, 0), size 6x6x6..." << std::endl;
    triggerManager.createBoxTrigger(
        physics.getPhysics(),
        scene,
        physics.getDefaultMaterial(),
        PxVec3(0, 5, 0),
        PxVec3(3, 3, 3),
        config
    );

    // Create capsule trigger
    std::cout << "Creating capsule trigger at (10, 5, 0), r=2, h=4..." << std::endl;
    triggerManager.createCapsuleTrigger(
        physics.getPhysics(),
        scene,
        physics.getDefaultMaterial(),
        PxVec3(10, 5, 0),
        2.0f,
        2.0f,
        config
    );

    // Create several falling spheres
    std::cout << "Creating 3 falling spheres..." << std::endl;
    for (int i = 0; i < 3; i++) {
        PxShape* sphereShape = physics.getPhysics()->createShape(
            PxSphereGeometry(0.5f),
            *physics.getDefaultMaterial()
        );
        PxRigidDynamic* sphere = physics.getPhysics()->createRigidDynamic(
            PxTransform(-10 + i * 10, 15, 0)
        );
        sphere->attachShape(*sphereShape);
        PxRigidBodyExt::updateMassAndInertia(*sphere, 1.0f);
        scene->addActor(*sphere);
        sphereShape->release();
    }

    // Simulate
    std::cout << "\nRunning simulation..." << std::endl;
    for (int i = 0; i < 180; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    std::cout << "\nFinal stats:" << std::endl;
    std::cout << "  Total enter events: " << g_stats.enterCount << std::endl;
    std::cout << "  Total exit events: " << g_stats.exitCount << std::endl;
    std::cout << "  (3 spheres × 3 triggers = 9 expected enter/exit pairs)" << std::endl;

    // Cleanup
    scene->release();
}

/**
 * @brief Test trigger-trigger overlap detection
 */
void testTriggerTriggerOverlap(PhysXCore& physics) {
    std::cout << "\n=== TEST 4: Trigger-Trigger Overlap Detection ===" << std::endl;

    g_stats = TriggerStats();

    // Create trigger manager
    RigidBodyTrigger triggerManager;

    triggerManager.setTriggerCallback([](const TriggerEvent& event) {
        bool isTrigger0 = RigidBodyTrigger::isTriggerShape(
            event.triggerShape,
            TriggerConfig{TriggerImplementation::FILTER_SHADER, false, true}
        );
        bool isTrigger1 = RigidBodyTrigger::isTriggerShape(
            event.otherShape,
            TriggerConfig{TriggerImplementation::FILTER_SHADER, false, true}
        );

        if (isTrigger0 && isTrigger1) {
            std::cout << "  -> Trigger-Trigger overlap detected!" << std::endl;
            g_stats.triggerTriggerCount++;
        } else if (event.type == TriggerEventType::ENTER) {
            g_stats.enterCount++;
        } else {
            g_stats.exitCount++;
        }
    });

    // Create scene with filter shader (allows trigger-trigger)
    std::cout << "Creating scene with trigger-trigger detection..." << std::endl;
    TriggerConfig config;
    config.implementation = TriggerImplementation::FILTER_SHADER;
    config.detectTriggerTrigger = true;

    PxTolerancesScale scale;
    PxSceneDesc sceneDesc = triggerManager.createSceneDesc(scale, config);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);

    PxScene* scene = physics.getPhysics()->createScene(sceneDesc);

    // Create two moving spheres, each with a trigger around it
    std::cout << "Creating two spheres with triggers..." << std::endl;

    // Sphere 1 (moving right)
    PxRigidDynamic* sphere1 = physics.getPhysics()->createRigidDynamic(
        PxTransform(-5, 1, 0)
    );
    PxShape* solid1 = physics.getPhysics()->createShape(
        PxSphereGeometry(1.0f),
        *physics.getDefaultMaterial(),
        false
    );
    sphere1->attachShape(*solid1);
    solid1->release();

    PxShape* trigger1 = triggerManager.createTriggerShape(
        physics.getPhysics(),
        PxSphereGeometry(4.0f),
        physics.getDefaultMaterial(),
        config,
        true
    );
    sphere1->attachShape(*trigger1);
    trigger1->release();

    PxRigidBodyExt::updateMassAndInertia(*sphere1, 1.0f);
    sphere1->setLinearVelocity(PxVec3(1, 0, 0));
    sphere1->setAngularDamping(0.5f);
    scene->addActor(*sphere1);

    // Sphere 2 (moving left)
    PxRigidDynamic* sphere2 = physics.getPhysics()->createRigidDynamic(
        PxTransform(5, 1, 0)
    );
    PxShape* solid2 = physics.getPhysics()->createShape(
        PxSphereGeometry(1.0f),
        *physics.getDefaultMaterial(),
        false
    );
    sphere2->attachShape(*solid2);
    solid2->release();

    PxShape* trigger2 = triggerManager.createTriggerShape(
        physics.getPhysics(),
        PxSphereGeometry(4.0f),
        physics.getDefaultMaterial(),
        config,
        true
    );
    sphere2->attachShape(*trigger2);
    trigger2->release();

    PxRigidBodyExt::updateMassAndInertia(*sphere2, 1.0f);
    sphere2->setLinearVelocity(PxVec3(-1, 0, 0));
    sphere2->setAngularDamping(0.5f);
    scene->addActor(*sphere2);

    // Simulate
    std::cout << "\nRunning simulation (spheres approaching)..." << std::endl;
    for (int i = 0; i < 300; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        if (i % 60 == 0) {
            PxTransform t1 = sphere1->getGlobalPose();
            PxTransform t2 = sphere2->getGlobalPose();
            float distance = (t1.p - t2.p).magnitude();
            std::cout << "Frame " << i << ": Distance between spheres = "
                      << std::fixed << std::setprecision(2)
                      << distance << std::endl;
        }
    }

    std::cout << "\nFinal stats:" << std::endl;
    std::cout << "  Trigger-trigger overlaps: " << g_stats.triggerTriggerCount << std::endl;
    std::cout << "  (Native implementation doesn't support trigger-trigger)" << std::endl;

    // Cleanup
    scene->release();
}

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - Trigger Volume Example ===" << std::endl;
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

    // Run tests
    testBasicTrigger(physics);
    testTriggerWithCCD(physics);
    testDifferentShapes(physics);
    testTriggerTriggerOverlap(physics);

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "\nThis example demonstrated:" << std::endl;
    std::cout << "  - Basic trigger volumes (enter/exit events)" << std::endl;
    std::cout << "  - Triggers with CCD for fast-moving objects" << std::endl;
    std::cout << "  - Different trigger shapes (box, sphere, capsule)" << std::endl;
    std::cout << "  - Trigger-trigger overlap detection" << std::endl;
    std::cout << "  - Multiple trigger implementations" << std::endl;

    std::cout << "\nCleaning up..." << std::endl;

    return 0;
}
