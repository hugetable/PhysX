/**
 * @file example_scenebuilder.cpp
 * @brief Comprehensive SceneBuilder showcase
 *
 * This example demonstrates all features of the SceneBuilder class:
 * - Material presets
 * - Basic shape creation (dynamic and static)
 * - Scene elements (ground, stacks, walls, obstacles, stairs, slopes)
 * - One-line scene setup
 */

#include "Core/PhysXCore.h"
#include "Utility/SceneBuilder.h"
#include <iostream>
#include <iomanip>

using namespace PhysXWrapper;
using namespace physx;

void printSection(const std::string& title)
{
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

int main(int argc, char** argv)
{
    std::cout << "=== PhysXWrapper - SceneBuilder Showcase ===" << std::endl;

    // Initialize PhysX
    PhysXCoreConfig config;
    config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    config.numThreads = 4;
    config.enablePVD = false;

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--pvd") {
            config.enablePVD = true;
        }
    }

    PhysXCore physics;
    if (!physics.initialize(config)) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return 1;
    }

    // Create scene builder
    SceneBuilder builder(physics.getPhysics(), physics.getScene(), physics.getDefaultMaterial());

    // ========================================================================
    // 1. Material Presets
    // ========================================================================
    printSection("1. Creating Materials with Presets");

    auto bouncyMat = builder.createMaterial(MaterialPreset::Bouncy());
    auto slipperyMat = builder.createMaterial(MaterialPreset::Slippery());
    auto stickyMat = builder.createMaterial(MaterialPreset::Sticky());
    auto iceMat = builder.createMaterial(MaterialPreset::Ice());

    std::cout << "✓ Created bouncy material (restitution: 0.9)" << std::endl;
    std::cout << "✓ Created slippery material (friction: 0.05)" << std::endl;
    std::cout << "✓ Created sticky material (friction: 0.9)" << std::endl;
    std::cout << "✓ Created ice material (ultra-low friction)" << std::endl;

    // ========================================================================
    // 2. Ground Plane
    // ========================================================================
    printSection("2. Creating Ground Plane");

    builder.createGround();
    std::cout << "✓ Ground plane created" << std::endl;

    // ========================================================================
    // 3. Dynamic Shapes (Projectiles)
    // ========================================================================
    printSection("3. Creating Dynamic Shapes");

    // Bouncy sphere
    auto sphere1 = builder.createDynamicSphere(
        PxVec3(-20, 15, 0), 1.0f, 10.0f, bouncyMat
    );
    std::cout << "✓ Bouncy sphere created at (-20, 15, 0)" << std::endl;

    // Regular box
    auto box1 = builder.createDynamicBox(
        PxVec3(-15, 15, 0), PxVec3(1, 1, 1), 10.0f
    );
    std::cout << "✓ Regular box created at (-15, 15, 0)" << std::endl;

    // Capsule with initial velocity
    auto capsule1 = builder.createDynamicCapsule(
        PxVec3(-10, 15, 0), 0.5f, 1.0f, 10.0f, nullptr, PxVec3(0, 0, 10)
    );
    std::cout << "✓ Capsule with velocity created at (-10, 15, 0)" << std::endl;

    // ========================================================================
    // 4. Box Stack (Pyramid)
    // ========================================================================
    printSection("4. Creating Box Stack");

    auto stack = builder.createBoxStack(
        PxVec3(0, 0, 0),  // Base position
        6,                 // Size (6x6 base)
        0.5f              // Box half extent
    );
    std::cout << "✓ Box stack created: " << stack.size() << " boxes" << std::endl;
    std::cout << "  (6x6 pyramid)" << std::endl;

    // ========================================================================
    // 5. Box Wall
    // ========================================================================
    printSection("5. Creating Box Wall");

    auto wall = builder.createBoxWall(
        PxVec3(15, 0, 0),  // Base position
        5,                  // Width
        8,                  // Height
        0.5f               // Box half extent
    );
    std::cout << "✓ Box wall created: " << wall.size() << " boxes" << std::endl;
    std::cout << "  (5 wide × 8 high)" << std::endl;

    // ========================================================================
    // 6. Obstacles
    // ========================================================================
    printSection("6. Creating Obstacles");

    auto obstacles = builder.createObstacles(
        PxVec3(-25, 0, 10),   // Start position
        8,                     // Count
        3.0f,                  // Spacing
        PxVec3(1, 2, 1)       // Half extents
    );
    std::cout << "✓ Obstacle course created: " << obstacles.size() << " obstacles" << std::endl;
    std::cout << "  (Spanning 24 meters)" << std::endl;

    // ========================================================================
    // 7. Stairs
    // ========================================================================
    printSection("7. Creating Stairs");

    auto stairs = builder.createStairs(
        PxVec3(25, 0, 0),  // Start position
        12,                 // Step count
        3.0f,               // Width
        0.3f,               // Height per step
        0.5f                // Depth per step
    );
    std::cout << "✓ Stairs created: " << stairs.size() << " steps" << std::endl;
    std::cout << "  (Rising " << (stairs.size() * 0.3f) << " meters)" << std::endl;

    // ========================================================================
    // 8. Slopes
    // ========================================================================
    printSection("8. Creating Slopes");

    // Regular slope
    auto slope1 = builder.createSlope(
        PxVec3(35, 0, 0),  // Position
        10.0f,              // Length
        5.0f,               // Width
        30.0f               // Angle (degrees)
    );
    std::cout << "✓ Slope 1 created: 30° angle" << std::endl;

    // Icy slope
    auto slope2 = builder.createSlope(
        PxVec3(35, 0, 10),  // Position
        10.0f,               // Length
        5.0f,                // Width
        45.0f,               // Angle (degrees)
        iceMat              // Ice material!
    );
    std::cout << "✓ Slope 2 created: 45° angle with ice material!" << std::endl;

    // ========================================================================
    // 9. Static Shapes
    // ========================================================================
    printSection("9. Creating Static Shapes");

    auto staticBox = builder.createStaticBox(
        PxVec3(40, 5, 0), PxVec3(2, 2, 2)
    );
    std::cout << "✓ Static box created" << std::endl;

    auto staticSphere = builder.createStaticSphere(
        PxVec3(45, 5, 0), 2.0f
    );
    std::cout << "✓ Static sphere created" << std::endl;

    auto staticCapsule = builder.createStaticCapsule(
        PxVec3(50, 5, 0), 1.0f, 2.0f
    );
    std::cout << "✓ Static capsule created" << std::endl;

    // ========================================================================
    // Scene Statistics
    // ========================================================================
    printSection("Scene Statistics");

    PxU32 nbDynamic = physics.getScene()->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    PxU32 nbStatic = physics.getScene()->getNbActors(PxActorTypeFlag::eRIGID_STATIC);

    std::cout << "Dynamic actors: " << nbDynamic << std::endl;
    std::cout << "Static actors:  " << nbStatic << std::endl;
    std::cout << "Total actors:   " << (nbDynamic + nbStatic) << std::endl;

    // ========================================================================
    // Run Simulation
    // ========================================================================
    printSection("Running Simulation");

    const float timeStep = 1.0f / 60.0f;
    const int numFrames = 600;  // 10 seconds

    std::cout << "Simulating for 10 seconds (600 frames)..." << std::endl;
    std::cout << "Watch the bouncy sphere bounce!" << std::endl;
    std::cout << "Watch objects slide down the slopes!\n" << std::endl;

    for (int frame = 0; frame < numFrames; frame++) {
        physics.update(timeStep);

        // Print progress every 2 seconds
        if (frame % 120 == 0) {
            float time = frame * timeStep;
            std::cout << std::fixed << std::setprecision(1);
            std::cout << "  t=" << time << "s";

            // Print sphere height (should keep bouncing!)
            if (sphere1) {
                PxVec3 pos = sphere1->getGlobalPose().p;
                std::cout << " | Bouncy sphere height: " << pos.y << " m";
            }

            std::cout << std::endl;
        }
    }

    printSection("Summary");

    std::cout << "✓ Simulation completed successfully!" << std::endl;
    std::cout << "\nSceneBuilder Benefits:" << std::endl;
    std::cout << "  • Simple one-line shape creation" << std::endl;
    std::cout << "  • Material presets for common use cases" << std::endl;
    std::cout << "  • No boilerplate code" << std::endl;
    std::cout << "  • Automatic mass/inertia calculation" << std::endl;
    std::cout << "  • Easy scene element creation (stairs, slopes, etc.)" << std::endl;
    std::cout << "  • Clean, readable code" << std::endl;
    std::cout << "\nTotal scene created with ~50 lines of code!" << std::endl;

    return 0;
}
