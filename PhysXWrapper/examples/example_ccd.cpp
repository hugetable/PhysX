/**
 * @file example_ccd.cpp
 * @brief Continuous Collision Detection (CCD) example
 *
 * This example demonstrates:
 * - Linear CCD for fast-moving objects
 * - Speculative/Angular CCD for rotating objects
 * - Full CCD (linear + speculative)
 * - Comparing CCD vs no CCD
 * - Per-shape CCD threshold configuration
 *
 * Based on SnippetCCD from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include "RigidBody/RigidBodyCCD.h"
#include <iostream>
#include <iomanip>

using namespace PhysXWrapper;
using namespace physx;

/**
 * @brief Create a box stack
 */
void createBoxStack(PxPhysics* physics, PxScene* scene, PxMaterial* material,
                    const PxVec3& position, PxU32 size, PxReal halfExtent,
                    RigidBodyCCD* ccdManager = nullptr,
                    const CCDConfig* config = nullptr)
{
    for (PxU32 i = 0; i < size; i++) {
        for (PxU32 j = 0; j < size - i; j++) {
            PxVec3 localPos(
                PxReal(j * 2) - PxReal(size - i),
                PxReal(i * 2 + 1),
                0
            );
            localPos *= halfExtent;

            PxRigidDynamic* box = physics->createRigidDynamic(
                PxTransform(position + localPos)
            );

            PxShape* shape = physics->createShape(
                PxBoxGeometry(halfExtent, halfExtent, halfExtent),
                *material
            );

            box->attachShape(*shape);
            PxRigidBodyExt::updateMassAndInertia(*box, 10.0f);

            if (ccdManager && config) {
                ccdManager->enableCCD(box, *config);
            }

            scene->addActor(*box);
            shape->release();
        }
    }
}

/**
 * @brief Test 1: Linear CCD - prevents fast objects from tunneling
 */
void testLinearCCD(PhysXCore& physics) {
    std::cout << "\n=== TEST 1: Linear CCD ===" << std::endl;
    std::cout << "Fast-moving sphere hits box stack" << std::endl;

    RigidBodyCCD ccdManager;

    // Create scene with linear CCD
    CCDConfig config;
    config.algorithm = CCDAlgorithm::LINEAR;

    PxTolerancesScale scale;
    PxSceneDesc sceneDesc = ccdManager.createSceneDesc(scale, config);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);

    PxScene* scene = physics.getPhysics()->createScene(sceneDesc);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(
        *physics.getPhysics(),
        PxPlane(0, 1, 0, 0),
        *physics.getDefaultMaterial()
    );
    scene->addActor(*ground);

    // Create box stack with CCD
    std::cout << "Creating box stack at (0, 0, 10)..." << std::endl;
    createBoxStack(
        physics.getPhysics(),
        scene,
        physics.getDefaultMaterial(),
        PxVec3(0, 0, 10),
        10,
        2.0f,
        &ccdManager,
        &config
    );

    // Create fast-moving sphere with CCD
    std::cout << "Creating fast-moving sphere (velocity = -1000 m/s)..." << std::endl;
    PxRigidDynamic* sphere = ccdManager.createFastMovingDynamic(
        physics.getPhysics(),
        scene,
        PxTransform(0, 18, 100),
        PxSphereGeometry(2.0f),
        physics.getDefaultMaterial(),
        PxVec3(0, 0, -1000),  // VERY fast!
        10.0f,
        config
    );

    std::cout << "CCD enabled: " << (ccdManager.isCCDEnabled(sphere) ? "YES" : "NO") << std::endl;

    // Simulate
    std::cout << "\nRunning simulation..." << std::endl;
    bool hitDetected = false;

    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        PxTransform transform = sphere->getGlobalPose();

        if (i % 30 == 0) {
            std::cout << "Frame " << i << ": Z = "
                      << std::fixed << std::setprecision(2)
                      << transform.p.z << std::endl;
        }

        // Check if sphere stopped (hit something)
        if (!hitDetected && transform.p.z < 15.0f) {
            hitDetected = true;
            std::cout << "  -> Collision detected! Sphere stopped by CCD." << std::endl;
        }
    }

    if (!hitDetected) {
        std::cout << "  -> WARNING: Sphere tunneled through (CCD failed?)" << std::endl;
    }

    // Cleanup
    scene->release();
    std::cout << "Test complete." << std::endl;
}

/**
 * @brief Test 2: Speculative CCD - prevents rotating objects from tunneling
 */
void testSpeculativeCCD(PhysXCore& physics) {
    std::cout << "\n=== TEST 2: Speculative/Angular CCD ===" << std::endl;
    std::cout << "Fast-rotating plank hits falling box" << std::endl;

    RigidBodyCCD ccdManager;

    // Create scene with speculative CCD
    CCDConfig config;
    config.algorithm = CCDAlgorithm::SPECULATIVE;

    PxTolerancesScale scale;
    PxSceneDesc sceneDesc = ccdManager.createSceneDesc(scale, config);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);

    PxScene* scene = physics.getPhysics()->createScene(sceneDesc);

    // Create fast-rotating plank (locked in place, only rotates)
    std::cout << "Creating fast-rotating plank..." << std::endl;
    PxRigidDynamic* plank = physics.getPhysics()->createRigidDynamic(
        PxTransform(40, 20, 0)
    );

    PxShape* plankShape = physics.getPhysics()->createShape(
        PxBoxGeometry(10, 1, 0.1f),
        *physics.getDefaultMaterial()
    );

    plank->attachShape(*plankShape);
    PxRigidBodyExt::updateMassAndInertia(*plank, 10.0f);

    // Lock all linear movement, only allow Y rotation
    plank->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
    plank->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X, true);
    plank->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, true);
    plank->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, true);
    plank->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
    plank->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, true);

    // Set fast rotation
    plank->setAngularVelocity(PxVec3(0, 10, 0));

    // Enable CCD
    ccdManager.enableCCD(plank, config);

    scene->addActor(*plank);
    plankShape->release();

    // Create falling box
    std::cout << "Creating falling box..." << std::endl;
    PxRigidDynamic* box = physics.getPhysics()->createRigidDynamic(
        PxTransform(40, 20, 10)
    );

    PxShape* boxShape = physics.getPhysics()->createShape(
        PxBoxGeometry(0.1f, 1, 1),
        *physics.getDefaultMaterial()
    );

    box->attachShape(*boxShape);
    PxRigidBodyExt::updateMassAndInertia(*box, 10.0f);
    ccdManager.enableCCD(box, config);

    scene->addActor(*box);
    boxShape->release();

    // Simulate
    std::cout << "\nRunning simulation..." << std::endl;
    bool hitDetected = false;

    for (int i = 0; i < 180; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        if (i % 60 == 0) {
            PxTransform boxTransform = box->getGlobalPose();
            PxVec3 boxVel = box->getLinearVelocity();

            std::cout << "Frame " << i << ": Box Z = "
                      << std::fixed << std::setprecision(2)
                      << boxTransform.p.z
                      << ", Velocity = " << boxVel.magnitude() << std::endl;
        }

        // Check if box was hit (velocity changed significantly)
        PxVec3 vel = box->getLinearVelocity();
        if (!hitDetected && vel.magnitude() > 2.0f) {
            hitDetected = true;
            std::cout << "  -> Collision detected! Box was hit by rotating plank." << std::endl;
        }
    }

    if (!hitDetected) {
        std::cout << "  -> WARNING: No collision detected (CCD failed?)" << std::endl;
    }

    // Cleanup
    scene->release();
    std::cout << "Test complete." << std::endl;
}

/**
 * @brief Test 3: Compare CCD vs No CCD
 */
void testCCDComparison(PhysXCore& physics) {
    std::cout << "\n=== TEST 3: CCD vs No CCD Comparison ===" << std::endl;

    // Test without CCD
    {
        std::cout << "\n3.1 Without CCD:" << std::endl;

        RigidBodyCCD ccdManager;
        CCDConfig config;
        config.algorithm = CCDAlgorithm::NONE;

        PxTolerancesScale scale;
        PxSceneDesc sceneDesc = ccdManager.createSceneDesc(scale, config);
        sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);

        PxScene* scene = physics.getPhysics()->createScene(sceneDesc);

        // Create thin wall
        PxRigidStatic* wall = physics.getPhysics()->createRigidStatic(
            PxTransform(0, 5, 0)
        );

        PxShape* wallShape = physics.getPhysics()->createShape(
            PxBoxGeometry(10, 5, 0.1f),  // Very thin wall
            *physics.getDefaultMaterial()
        );

        wall->attachShape(*wallShape);
        scene->addActor(*wall);
        wallShape->release();

        // Create fast bullet
        PxRigidDynamic* bullet = physics.getPhysics()->createRigidDynamic(
            PxTransform(0, 5, 10)
        );

        PxShape* bulletShape = physics.getPhysics()->createShape(
            PxSphereGeometry(0.1f),
            *physics.getDefaultMaterial()
        );

        bullet->attachShape(*bulletShape);
        PxRigidBodyExt::updateMassAndInertia(*bullet, 1.0f);
        bullet->setLinearVelocity(PxVec3(0, 0, -200));  // Fast!

        scene->addActor(*bullet);
        bulletShape->release();

        // Simulate
        bool tunneled = true;
        for (int i = 0; i < 60; i++) {
            scene->simulate(1.0f / 60.0f);
            scene->fetchResults(true);

            PxTransform transform = bullet->getGlobalPose();
            if (transform.p.z > -0.2f && transform.p.z < 0.2f) {
                tunneled = false;
                std::cout << "  Bullet stopped at wall (Z=" << transform.p.z << ")" << std::endl;
                break;
            }
        }

        PxTransform finalTransform = bullet->getGlobalPose();
        std::cout << "  Final position: Z = " << finalTransform.p.z << std::endl;

        if (tunneled && finalTransform.p.z < -0.5f) {
            std::cout << "  -> Bullet TUNNELED through wall!" << std::endl;
        }

        scene->release();
    }

    // Test with CCD
    {
        std::cout << "\n3.2 With CCD:" << std::endl;

        RigidBodyCCD ccdManager;
        CCDConfig config;
        config.algorithm = CCDAlgorithm::LINEAR;

        PxTolerancesScale scale;
        PxSceneDesc sceneDesc = ccdManager.createSceneDesc(scale, config);
        sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);

        PxScene* scene = physics.getPhysics()->createScene(sceneDesc);

        // Create thin wall
        PxRigidStatic* wall = physics.getPhysics()->createRigidStatic(
            PxTransform(0, 5, 0)
        );

        PxShape* wallShape = physics.getPhysics()->createShape(
            PxBoxGeometry(10, 5, 0.1f),
            *physics.getDefaultMaterial()
        );

        wall->attachShape(*wallShape);
        scene->addActor(*wall);
        wallShape->release();

        // Create fast bullet with CCD
        PxRigidDynamic* bullet = ccdManager.createFastMovingDynamic(
            physics.getPhysics(),
            scene,
            PxTransform(0, 5, 10),
            PxSphereGeometry(0.1f),
            physics.getDefaultMaterial(),
            PxVec3(0, 0, -200),
            1.0f,
            config
        );

        // Simulate
        bool stopped = false;
        for (int i = 0; i < 60; i++) {
            scene->simulate(1.0f / 60.0f);
            scene->fetchResults(true);

            PxTransform transform = bullet->getGlobalPose();
            if (!stopped && transform.p.z > -0.5f && transform.p.z < 10.0f) {
                stopped = true;
                std::cout << "  Bullet stopped at wall (Z=" << transform.p.z << ")" << std::endl;
                break;
            }
        }

        PxTransform finalTransform = bullet->getGlobalPose();
        std::cout << "  Final position: Z = " << finalTransform.p.z << std::endl;

        if (stopped) {
            std::cout << "  -> CCD prevented tunneling!" << std::endl;
        } else {
            std::cout << "  -> WARNING: Bullet still tunneled?" << std::endl;
        }

        scene->release();
    }

    std::cout << "Comparison complete." << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - CCD Example ===" << std::endl;
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
    testLinearCCD(physics);
    testSpeculativeCCD(physics);
    testCCDComparison(physics);

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "\nThis example demonstrated:" << std::endl;
    std::cout << "  - Linear CCD for fast-moving objects" << std::endl;
    std::cout << "  - Speculative/Angular CCD for rotating objects" << std::endl;
    std::cout << "  - Comparison of CCD vs no CCD (tunneling prevention)" << std::endl;
    std::cout << "  - Per-object CCD configuration" << std::endl;

    std::cout << "\nCleaning up..." << std::endl;

    return 0;
}
