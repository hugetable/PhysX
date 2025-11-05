/**
 * @file example_helloworld.cpp
 * @brief Hello World example using PhysXCore wrapper
 *
 * This example demonstrates the basic usage of PhysXCore class.
 * It creates a simple scene with:
 * - A ground plane
 * - A stack of boxes
 * - A sphere projectile
 *
 * Based on SnippetHelloWorld from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include <iostream>
#include <cmath>

using namespace PhysXWrapper;
using namespace physx;

/**
 * @brief Create a stack of boxes
 */
void createStack(PhysXCore& physics, const PxTransform& t, PxU32 size, PxReal halfExtent)
{
    PxPhysics* physicsSDK = physics.getPhysics();
    PxScene* scene = physics.getScene();
    PxMaterial* material = physics.getDefaultMaterial();

    if (!physicsSDK || !scene || !material) {
        std::cerr << "Physics not initialized" << std::endl;
        return;
    }

    // Create a shared shape
    PxShape* shape = physicsSDK->createShape(
        PxBoxGeometry(halfExtent, halfExtent, halfExtent),
        *material
    );

    for (PxU32 i = 0; i < size; i++) {
        for (PxU32 j = 0; j < size - i; j++) {
            PxTransform localTm(
                PxVec3(
                    PxReal(j * 2) - PxReal(size - i),
                    PxReal(i * 2 + 1),
                    0
                ) * halfExtent
            );

            PxRigidDynamic* body = physicsSDK->createRigidDynamic(t.transform(localTm));
            body->attachShape(*shape);
            PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
            scene->addActor(*body);
        }
    }

    shape->release();
}

/**
 * @brief Print simulation statistics
 */
void printStats(PxScene* scene, int frame)
{
    if (!scene) return;

    PxU32 nbActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);

    if (frame % 60 == 0) {  // Print every second (at 60 FPS)
        std::cout << "Frame " << frame
                  << ": " << nbActors << " dynamic actors"
                  << std::endl;
    }
}

int main(int argc, char** argv)
{
    std::cout << "=== PhysXWrapper - Hello World Example ===" << std::endl;
    std::cout << std::endl;

    // Configure PhysX
    PhysXCoreConfig config;
    config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    config.numThreads = 2;
    config.enablePVD = false;  // Set to true to connect to PhysX Visual Debugger

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
    std::cout << std::endl;

    // Create ground plane
    std::cout << "Creating ground plane..." << std::endl;
    PxRigidStatic* groundPlane = physics.createGroundPlane(PxVec3(0, 1, 0), 0);
    if (!groundPlane) {
        std::cerr << "Failed to create ground plane" << std::endl;
        return 1;
    }

    // Create stacks of boxes
    std::cout << "Creating box stacks..." << std::endl;
    PxReal stackZ = 10.0f;
    for (PxU32 i = 0; i < 5; i++) {
        createStack(physics, PxTransform(PxVec3(0, 0, stackZ -= 10.0f)), 10, 2.0f);
    }

    // Create projectile sphere
    std::cout << "Creating projectile..." << std::endl;
    PxRigidDynamic* sphere = physics.createDynamic(
        PxTransform(PxVec3(0, 40, 100)),
        PxSphereGeometry(10),
        10.0f,
        PxVec3(0, -50, -100)
    );

    if (!sphere) {
        std::cerr << "Failed to create sphere" << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "Running simulation..." << std::endl;
    std::cout << "Simulating 5 seconds (300 frames at 60 FPS)" << std::endl;
    std::cout << std::endl;

    // Simulation loop
    const float timeStep = 1.0f / 60.0f;  // 60 FPS
    const int frameCount = 300;  // 5 seconds

    for (int i = 0; i < frameCount; i++) {
        if (!physics.update(timeStep)) {
            std::cerr << "Simulation failed at frame " << i << ": "
                      << physics.getLastError() << std::endl;
            return 1;
        }

        printStats(physics.getScene(), i);
    }

    std::cout << std::endl;
    std::cout << "Simulation completed successfully!" << std::endl;
    std::cout << std::endl;

    // Cleanup is automatic (RAII)
    std::cout << "Cleaning up..." << std::endl;

    return 0;
}
