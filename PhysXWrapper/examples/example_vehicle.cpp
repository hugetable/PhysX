/**
 * @file example_vehicle.cpp
 * @brief Example demonstrating VehicleManager usage
 *
 * This example shows how to use the VehicleManager class for
 * creating and controlling wheeled vehicles with realistic physics,
 * including throttle, brake, steering, and different drive types.
 */

#include "PhysXCore.h"
#include "Vehicle/VehicleManager.h"
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
    PxMaterial* material = physics->createMaterial(0.8f, 0.8f, 0.1f);
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);
}

void createRamp(PxPhysics* physics, PxScene* scene, const PxVec3& position, PxReal angle)
{
    PxMaterial* material = physics->createMaterial(0.6f, 0.6f, 0.1f);
    PxQuat rotation(angle, PxVec3(0, 0, 1));

    PxRigidStatic* ramp = PxCreateStatic(
        *physics,
        PxTransform(position, rotation),
        PxBoxGeometry(5.0f, 0.2f, 5.0f),
        *material
    );
    scene->addActor(*ramp);
}

void createObstacles(PxPhysics* physics, PxScene* scene)
{
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    // Create some obstacles
    for (int i = 0; i < 5; i++) {
        PxVec3 pos(i * 8.0f, 1.0f, 20.0f);
        PxRigidStatic* obstacle = PxCreateStatic(
            *physics,
            PxTransform(pos),
            PxBoxGeometry(1.0f, 2.0f, 1.0f),
            *material
        );
        scene->addActor(*obstacle);
    }
}

// ============================================================================
// Test 1: Basic Vehicle Control
// ============================================================================

void test_BasicVehicleControl()
{
    printSeparator("Test 1: Basic Vehicle Control");

    // Initialize PhysX
    PhysXCore core;
    if (!core.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    // Initialize vehicle manager
    VehicleManager manager;
    if (!manager.initialize(physics, scene)) {
        std::cerr << "Failed to initialize vehicle manager" << std::endl;
        return;
    }

    std::cout << "Creating simple vehicle..." << std::endl;

    // Create vehicle
    VehicleManager::Vehicle* vehicle = manager.createSimpleVehicle(PxVec3(0, 2, 0));
    if (!vehicle) {
        std::cerr << "Failed to create vehicle" << std::endl;
        return;
    }

    manager.printVehicleInfo(vehicle);

    PxReal deltaTime = 0.016f; // 60 FPS

    // Wait for vehicle to settle
    std::cout << "\nSettling vehicle..." << std::endl;
    for (int i = 0; i < 60; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    // Test acceleration
    std::cout << "\nAccelerating..." << std::endl;
    manager.setThrottle(vehicle, 1.0f);

    for (int i = 0; i < 300; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);

        if (i % 60 == 0) {
            VehicleManager::VehicleState state = manager.getState(vehicle);
            std::cout << "t=" << (i * deltaTime) << "s: Speed = "
                      << state.speed << " m/s (" << (state.speed * 3.6f) << " km/h)" << std::endl;
        }
    }

    // Test braking
    std::cout << "\nBraking..." << std::endl;
    manager.setThrottle(vehicle, 0.0f);
    manager.setBrake(vehicle, 1.0f);

    for (int i = 0; i < 120; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);

        if (i % 30 == 0) {
            VehicleManager::VehicleState state = manager.getState(vehicle);
            std::cout << "t=" << (i * deltaTime) << "s: Speed = "
                      << state.speed << " m/s" << std::endl;
        }
    }

    manager.printVehicleInfo(vehicle);

    core.cleanup();
}

// ============================================================================
// Test 2: Steering Control
// ============================================================================

void test_SteeringControl()
{
    printSeparator("Test 2: Steering Control");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    VehicleManager manager;
    manager.initialize(physics, scene);

    VehicleManager::Vehicle* vehicle = manager.createSimpleVehicle(PxVec3(0, 2, 0));

    PxReal deltaTime = 0.016f;

    // Settle
    for (int i = 0; i < 60; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Driving in a circle..." << std::endl;

    // Drive in circle
    manager.setThrottle(vehicle, 0.6f);
    manager.setSteering(vehicle, 0.5f); // Turn left

    PxVec3 startPos = manager.getPosition(vehicle);

    for (int i = 0; i < 600; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);

        if (i % 100 == 0) {
            PxVec3 pos = manager.getPosition(vehicle);
            std::cout << "Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        }
    }

    PxVec3 endPos = manager.getPosition(vehicle);
    PxReal distance = (endPos - startPos).magnitude();
    std::cout << "Distance from start: " << distance << " m" << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 3: Different Drive Types
// ============================================================================

void test_DriveTypes()
{
    printSeparator("Test 3: Different Drive Types");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    VehicleManager manager;
    manager.initialize(physics, scene);

    // Create vehicles with different drive types
    std::cout << "Creating vehicles with different drive types..." << std::endl;

    VehicleManager::VehicleDesc desc4WD;
    desc4WD.initialTransform = PxTransform(PxVec3(-10, 2, 0));
    desc4WD.driveType = VehicleManager::DriveType::FOUR_WHEEL_DRIVE;
    VehicleManager::Vehicle* vehicle4WD = manager.createVehicle(desc4WD);

    VehicleManager::VehicleDesc descFWD;
    descFWD.initialTransform = PxTransform(PxVec3(0, 2, 0));
    descFWD.driveType = VehicleManager::DriveType::FRONT_WHEEL_DRIVE;
    VehicleManager::Vehicle* vehicleFWD = manager.createVehicle(descFWD);

    VehicleManager::VehicleDesc descRWD;
    descRWD.initialTransform = PxTransform(PxVec3(10, 2, 0));
    descRWD.driveType = VehicleManager::DriveType::REAR_WHEEL_DRIVE;
    VehicleManager::Vehicle* vehicleRWD = manager.createVehicle(descRWD);

    PxReal deltaTime = 0.016f;

    // Settle
    for (int i = 0; i < 60; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "\nAccelerating all vehicles..." << std::endl;

    manager.setThrottle(vehicle4WD, 1.0f);
    manager.setThrottle(vehicleFWD, 1.0f);
    manager.setThrottle(vehicleRWD, 1.0f);

    for (int i = 0; i < 300; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);

        if (i % 100 == 0) {
            std::cout << "\nt=" << (i * deltaTime) << "s:" << std::endl;
            std::cout << "4WD speed: " << manager.getSpeed(vehicle4WD) << " m/s" << std::endl;
            std::cout << "FWD speed: " << manager.getSpeed(vehicleFWD) << " m/s" << std::endl;
            std::cout << "RWD speed: " << manager.getSpeed(vehicleRWD) << " m/s" << std::endl;
        }
    }

    core.cleanup();
}

// ============================================================================
// Test 4: Obstacle Avoidance
// ============================================================================

void test_ObstacleAvoidance()
{
    printSeparator("Test 4: Obstacle Avoidance");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);
    createObstacles(physics, scene);

    VehicleManager manager;
    manager.initialize(physics, scene);

    VehicleManager::Vehicle* vehicle = manager.createSimpleVehicle(PxVec3(0, 2, 0));

    PxReal deltaTime = 0.016f;

    // Settle
    for (int i = 0; i < 60; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Driving toward obstacles and steering around them..." << std::endl;

    manager.setThrottle(vehicle, 0.7f);

    for (int i = 0; i < 600; i++) {
        // Simple steering logic: alternate steering
        if (i > 200 && i < 300) {
            manager.setSteering(vehicle, 0.8f); // Turn left
        } else if (i > 300 && i < 400) {
            manager.setSteering(vehicle, -0.8f); // Turn right
        } else {
            manager.setSteering(vehicle, 0.0f);
        }

        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);

        if (i % 100 == 0) {
            PxVec3 pos = manager.getPosition(vehicle);
            std::cout << "Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        }
    }

    core.cleanup();
}

// ============================================================================
// Test 5: Handbrake and Drift
// ============================================================================

void test_HandbrakeControl()
{
    printSeparator("Test 5: Handbrake and Drift");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    VehicleManager manager;
    manager.initialize(physics, scene);

    VehicleManager::Vehicle* vehicle = manager.createSimpleVehicle(PxVec3(0, 2, 0));

    PxReal deltaTime = 0.016f;

    // Settle
    for (int i = 0; i < 60; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    // Accelerate
    std::cout << "Accelerating..." << std::endl;
    manager.setThrottle(vehicle, 1.0f);

    for (int i = 0; i < 200; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    std::cout << "Speed before handbrake: " << manager.getSpeed(vehicle) << " m/s" << std::endl;

    // Apply handbrake
    std::cout << "\nApplying handbrake..." << std::endl;
    manager.setThrottle(vehicle, 0.0f);
    manager.setHandbrake(vehicle, 1.0f);

    for (int i = 0; i < 120; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);

        if (i % 30 == 0) {
            std::cout << "Speed: " << manager.getSpeed(vehicle) << " m/s" << std::endl;
        }
    }

    std::cout << "Final speed: " << manager.getSpeed(vehicle) << " m/s" << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 6: Multiple Vehicles
// ============================================================================

void test_MultipleVehicles()
{
    printSeparator("Test 6: Multiple Vehicles");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGround(physics, scene);

    VehicleManager manager;
    manager.initialize(physics, scene);

    std::cout << "Creating 5 vehicles..." << std::endl;

    std::vector<VehicleManager::Vehicle*> vehicles;
    for (int i = 0; i < 5; i++) {
        PxVec3 pos(i * 5.0f, 2, 0);
        VehicleManager::Vehicle* vehicle = manager.createSimpleVehicle(pos);
        vehicles.push_back(vehicle);
    }

    std::cout << "Total vehicles: " << manager.getVehicleCount() << std::endl;

    PxReal deltaTime = 0.016f;

    // Settle
    for (int i = 0; i < 60; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    // Set different controls for each vehicle
    std::cout << "\nSetting different controls..." << std::endl;
    manager.setThrottle(vehicles[0], 1.0f);
    manager.setThrottle(vehicles[1], 0.8f);
    manager.setThrottle(vehicles[2], 0.6f);
    manager.setThrottle(vehicles[3], 0.4f);
    manager.setThrottle(vehicles[4], 0.2f);

    // Run simulation
    for (int i = 0; i < 300; i++) {
        manager.update(deltaTime);
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

    // Print final states
    std::cout << "\nFinal states:" << std::endl;
    for (size_t i = 0; i < vehicles.size(); i++) {
        std::cout << "\nVehicle " << i << ":" << std::endl;
        manager.printVehicleInfo(vehicles[i]);
    }

    // Print all vehicles summary
    std::cout << "\n";
    manager.printAllVehicles();

    core.cleanup();
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "PhysXWrapper - VehicleManager Example" << std::endl;
    std::cout << "=====================================\n" << std::endl;

    try {
        test_BasicVehicleControl();
        test_SteeringControl();
        test_DriveTypes();
        test_ObstacleAvoidance();
        test_HandbrakeControl();
        test_MultipleVehicles();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
