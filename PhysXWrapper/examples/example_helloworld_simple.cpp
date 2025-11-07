/**
 * @file example_helloworld_simple.cpp
 * @brief Simplified Hello World example using SceneBuilder
 *
 * This example demonstrates the simplified API using SceneBuilder.
 * Compare this with example_helloworld.cpp to see the difference!
 *
 * Before (example_helloworld.cpp):
 *   - Manual shape creation and attachment
 *   - Manual mass/inertia calculation
 *   - Manual scene addition
 *   - Lots of boilerplate code
 *
 * After (this file):
 *   - One-line shape creation
 *   - No manual mass calculation needed
 *   - Cleaner, more readable code
 */

#include "Core/PhysXCore.h"
#include "Utility/SceneBuilder.h"
#include <iostream>

using namespace PhysXWrapper;
using namespace physx;

int main(int argc, char** argv)
{
    std::cout << "=== PhysXWrapper - Hello World (Simplified) ===" << std::endl;
    std::cout << std::endl;

    // Configure PhysX
    PhysXCoreConfig config;
    config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    config.numThreads = 2;
    config.enablePVD = false;

    // Check for PVD flag
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

    // Create scene builder
    SceneBuilder builder(physics.getPhysics(), physics.getScene(), physics.getDefaultMaterial());

    // Create ground plane (ONE LINE!)
    std::cout << "\nCreating scene..." << std::endl;
    builder.createGround();
    std::cout << "  - Ground plane created" << std::endl;

    // Create box stack (ONE LINE!)
    auto stack = builder.createBoxStack(
        PxVec3(0, 0, 0),  // Base position
        5,                 // 5 boxes at base (pyramid)
        0.5f               // Box half extent
    );
    std::cout << "  - Box stack created (" << stack.size() << " boxes)" << std::endl;

    // Create a sphere projectile (ONE LINE!)
    PxRigidDynamic* sphere = builder.createDynamicSphere(
        PxVec3(0, 10, -10),      // Position
        1.0f,                    // Radius
        10.0f,                   // Density
        nullptr,                 // Use default material
        PxVec3(0, 0, 50)        // Initial velocity (shoot forward!)
    );
    std::cout << "  - Sphere projectile created (velocity: 50 m/s)" << std::endl;

    // Optional: Create some obstacles
    auto obstacles = builder.createObstacles(
        PxVec3(-10, 0, 20),  // Start position
        5,                    // 5 obstacles
        4.0f,                 // Spacing
        PxVec3(1, 2, 1)      // Size
    );
    std::cout << "  - Obstacles created (" << obstacles.size() << " boxes)" << std::endl;

    std::cout << "\nRunning simulation..." << std::endl;
    std::cout << "The sphere will hit the box stack!\n" << std::endl;

    // Simulation loop
    const float timeStep = 1.0f / 60.0f;  // 60 FPS
    const int numFrames = 300;            // 5 seconds at 60 FPS

    for (int frame = 0; frame < numFrames; frame++) {
        // Update physics
        physics.update(timeStep);

        // Print stats every second
        if (frame % 60 == 0) {
            PxU32 nbDynamic = physics.getScene()->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
            PxU32 nbStatic = physics.getScene()->getNbActors(PxActorTypeFlag::eRIGID_STATIC);

            std::cout << "Frame " << frame << " (t=" << (frame * timeStep) << "s): "
                      << nbDynamic << " dynamic actors, "
                      << nbStatic << " static actors" << std::endl;

            // Print sphere position
            if (sphere) {
                PxVec3 pos = sphere->getGlobalPose().p;
                std::cout << "  Sphere at (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
            }
        }
    }

    std::cout << "\n✓ Simulation completed successfully!" << std::endl;
    std::cout << "\nCompare this code with example_helloworld.cpp!" << std::endl;
    std::cout << "  - Much shorter and cleaner" << std::endl;
    std::cout << "  - No boilerplate code" << std::endl;
    std::cout << "  - Easy to understand and modify" << std::endl;

    // Cleanup is automatic via RAII!
    return 0;
}
