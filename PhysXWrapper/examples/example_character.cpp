/**
 * @file example_character.cpp
 * @brief Example demonstrating CharacterController usage
 *
 * This example shows how to use the CharacterController class for
 * kinematic character movement with collision detection, jumping,
 * slope climbing, and obstacle handling.
 */

#include "PhysXCore.h"
#include "Character/CharacterController.h"
#include <iostream>
#include <thread>
#include <chrono>

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

void createGround(PxPhysics* physics, PxScene* scene)
{
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.1f);
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);
}

void createObstacles(PxPhysics* physics, PxScene* scene)
{
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    // Create boxes as obstacles
    for (int i = 0; i < 5; i++) {
        PxRigidStatic* box = PxCreateStatic(
            *physics,
            PxTransform(PxVec3(i * 4.0f, 0.5f, 5.0f)),
            PxBoxGeometry(1.0f, 1.0f, 1.0f),
            *material
        );
        scene->addActor(*box);
    }
}

void createStairs(PxPhysics* physics, PxScene* scene)
{
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    // Create stairs
    for (int i = 0; i < 10; i++) {
        PxReal height = 0.25f * (i + 1);
        PxRigidStatic* step = PxCreateStatic(
            *physics,
            PxTransform(PxVec3(-5.0f - i * 0.5f, height * 0.5f, 0.0f)),
            PxBoxGeometry(0.5f, height * 0.5f, 2.0f),
            *material
        );
        scene->addActor(*step);
    }
}

void createSlope(PxPhysics* physics, PxScene* scene)
{
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    // Create sloped plane
    PxQuat rotation(PxPi / 6.0f, PxVec3(0, 0, 1)); // 30 degrees
    PxRigidStatic* slope = PxCreateStatic(
        *physics,
        PxTransform(PxVec3(10.0f, 0.0f, 0.0f), rotation),
        PxBoxGeometry(5.0f, 0.1f, 5.0f),
        *material
    );
    scene->addActor(*slope);
}

// ============================================================================
// Test 1: Basic Movement
// ============================================================================

void test_BasicMovement()
{
    printSeparator("Test 1: Basic Movement");

    // Initialize PhysX
    PhysXCore core;
    if (!core.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    // Create ground
    createGround(physics, scene);

    // Initialize character controller
    CharacterController controller;
    if (!controller.initialize(physics, scene)) {
        std::cerr << "Failed to initialize character controller" << std::endl;
        return;
    }

    // Create capsule character
    CharacterController::CapsuleDesc desc;
    desc.position = PxExtendedVec3(0, 10, 0);
    desc.radius = 0.5f;
    desc.height = 1.8f;

    if (!controller.createCapsule(desc)) {
        std::cerr << "Failed to create capsule controller" << std::endl;
        return;
    }

    std::cout << "Created character at position (0, 10, 0)" << std::endl;
    controller.printInfo();

    // Simulate movement
    PxReal deltaTime = 0.016f; // 60 FPS

    // Fall to ground
    std::cout << "\nFalling to ground..." << std::endl;
    for (int i = 0; i < 100; i++) {
        CharacterController::CollisionFlags flags = controller.moveWithGravity(PxVec3(0, 0, 0), deltaTime);

        if (flags.isGrounded()) {
            std::cout << "Landed on ground after " << (i * deltaTime) << " seconds" << std::endl;
            break;
        }

        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    // Move forward
    std::cout << "\nMoving forward..." << std::endl;
    PxVec3 forward(0, 0, 1);
    for (int i = 0; i < 60; i++) {
        controller.moveWithGravity(forward * deltaTime * 5.0f, deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    PxVec3 finalPos = controller.getPositionVec3();
    std::cout << "Final position: (" << finalPos.x << ", " << finalPos.y << ", " << finalPos.z << ")" << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 2: Jumping
// ============================================================================

void test_Jumping()
{
    printSeparator("Test 2: Jumping");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    CharacterController controller;
    controller.initialize(physics, scene);

    CharacterController::CapsuleDesc desc;
    desc.position = PxExtendedVec3(0, 2, 0);
    desc.radius = 0.5f;
    desc.height = 1.8f;
    controller.createCapsule(desc);

    PxReal deltaTime = 0.016f;

    // Wait for landing
    for (int i = 0; i < 100; i++) {
        CharacterController::CollisionFlags flags = controller.moveWithGravity(PxVec3(0, 0, 0), deltaTime);
        if (flags.isGrounded()) break;
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Character grounded" << std::endl;

    // Perform jumps
    int jumpCount = 0;
    for (int i = 0; i < 300; i++) {
        // Try to jump every 60 frames (1 second)
        if (i % 60 == 0 && controller.isGrounded()) {
            if (controller.jump()) {
                jumpCount++;
                PxVec3 pos = controller.getPositionVec3();
                std::cout << "Jump " << jumpCount << " at t=" << (i * deltaTime)
                          << "s, height=" << pos.y << std::endl;
            }
        }

        controller.moveWithGravity(PxVec3(0, 0, 0), deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Total jumps performed: " << jumpCount << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 3: Obstacle Navigation
// ============================================================================

void test_ObstacleNavigation()
{
    printSeparator("Test 3: Obstacle Navigation");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);
    createObstacles(physics, scene);

    CharacterController controller;
    controller.initialize(physics, scene);

    CharacterController::CapsuleDesc desc;
    desc.position = PxExtendedVec3(0, 2, 0);
    desc.radius = 0.5f;
    desc.height = 1.8f;
    controller.createCapsule(desc);

    PxReal deltaTime = 0.016f;

    // Wait for landing
    for (int i = 0; i < 100; i++) {
        CharacterController::CollisionFlags flags = controller.moveWithGravity(PxVec3(0, 0, 0), deltaTime);
        if (flags.isGrounded()) break;
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Moving toward obstacles..." << std::endl;

    // Move forward into obstacles
    PxVec3 direction(0, 0, 1);
    int collisionCount = 0;

    for (int i = 0; i < 400; i++) {
        CharacterController::CollisionFlags flags = controller.moveWithGravity(
            direction * deltaTime * 3.0f, deltaTime);

        if (flags.collisionSides) {
            collisionCount++;
            if (collisionCount % 10 == 0) {
                PxVec3 pos = controller.getPositionVec3();
                std::cout << "Collision at position (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
            }
        }

        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Total side collisions: " << collisionCount << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 4: Stair Climbing
// ============================================================================

void test_StairClimbing()
{
    printSeparator("Test 4: Stair Climbing");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);
    createStairs(physics, scene);

    CharacterController controller;
    controller.initialize(physics, scene);

    CharacterController::CapsuleDesc desc;
    desc.position = PxExtendedVec3(0, 2, 0);
    desc.radius = 0.5f;
    desc.height = 1.8f;
    desc.stepOffset = 0.3f; // Allow climbing steps up to 0.3m
    controller.createCapsule(desc);

    PxReal deltaTime = 0.016f;

    // Wait for landing
    for (int i = 0; i < 100; i++) {
        CharacterController::CollisionFlags flags = controller.moveWithGravity(PxVec3(0, 0, 0), deltaTime);
        if (flags.isGrounded()) break;
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Starting to climb stairs..." << std::endl;
    PxVec3 startPos = controller.getPositionVec3();

    // Move toward stairs
    PxVec3 direction(-1, 0, 0);
    for (int i = 0; i < 600; i++) {
        controller.moveWithGravity(direction * deltaTime * 2.0f, deltaTime);

        if (i % 100 == 0) {
            PxVec3 pos = controller.getPositionVec3();
            std::cout << "Position at t=" << (i * deltaTime) << "s: ("
                      << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        }

        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    PxVec3 endPos = controller.getPositionVec3();
    PxReal heightGained = endPos.y - startPos.y;
    std::cout << "Height gained: " << heightGained << " meters" << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 5: Slope Walking
// ============================================================================

void test_SlopeWalking()
{
    printSeparator("Test 5: Slope Walking");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);
    createSlope(physics, scene);

    CharacterController controller;
    controller.initialize(physics, scene);

    CharacterController::CapsuleDesc desc;
    desc.position = PxExtendedVec3(5, 2, 0);
    desc.radius = 0.5f;
    desc.height = 1.8f;
    desc.slopeLimit = 0.707f; // ~45 degrees
    controller.createCapsule(desc);

    PxReal deltaTime = 0.016f;

    // Wait for landing
    for (int i = 0; i < 100; i++) {
        CharacterController::CollisionFlags flags = controller.moveWithGravity(PxVec3(0, 0, 0), deltaTime);
        if (flags.isGrounded()) break;
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Walking up slope (30 degrees)..." << std::endl;

    // Walk toward slope
    PxVec3 direction(1, 0, 0);
    for (int i = 0; i < 400; i++) {
        controller.moveWithGravity(direction * deltaTime * 3.0f, deltaTime);

        if (i % 100 == 0) {
            PxVec3 pos = controller.getPositionVec3();
            std::cout << "Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        }

        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Successfully walked on slope" << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 6: Box Controller
// ============================================================================

void test_BoxController()
{
    printSeparator("Test 6: Box Controller");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    CharacterController controller;
    controller.initialize(physics, scene);

    // Create box character
    CharacterController::BoxDesc desc;
    desc.position = PxExtendedVec3(0, 5, 0);
    desc.halfExtents = PxVec3(0.5f, 1.0f, 0.5f);
    controller.createBox(desc);

    std::cout << "Created box controller" << std::endl;
    controller.printInfo();

    PxReal deltaTime = 0.016f;

    // Simulate movement
    for (int i = 0; i < 200; i++) {
        PxVec3 movement(0, 0, 0);

        // Move in a circle
        PxReal angle = i * 0.05f;
        movement.x = PxCos(angle) * deltaTime * 2.0f;
        movement.z = PxSin(angle) * deltaTime * 2.0f;

        controller.moveWithGravity(movement, deltaTime);

        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    PxVec3 finalPos = controller.getPositionVec3();
    std::cout << "Final position: (" << finalPos.x << ", " << finalPos.y << ", " << finalPos.z << ")" << std::endl;

    // Test resizing
    std::cout << "\nResizing box..." << std::endl;
    PxVec3 originalExtents = controller.getBoxHalfExtents();
    std::cout << "Original extents: (" << originalExtents.x << ", " << originalExtents.y << ", " << originalExtents.z << ")" << std::endl;

    controller.setBoxHalfExtents(PxVec3(0.3f, 0.8f, 0.3f));
    PxVec3 newExtents = controller.getBoxHalfExtents();
    std::cout << "New extents: (" << newExtents.x << ", " << newExtents.y << ", " << newExtents.z << ")" << std::endl;

    core.cleanup();
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "PhysXWrapper - CharacterController Example" << std::endl;
    std::cout << "==========================================\n" << std::endl;

    try {
        test_BasicMovement();
        test_Jumping();
        test_ObstacleNavigation();
        test_StairClimbing();
        test_SlopeWalking();
        test_BoxController();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
