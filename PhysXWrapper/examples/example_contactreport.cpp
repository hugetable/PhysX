/**
 * @file example_contactreport.cpp
 * @brief Contact reporting example using RigidBodyContactHandler
 *
 * This example demonstrates:
 * - Setting up contact event callbacks
 * - Handling collision events
 * - Accessing contact point data
 * - Filtering contact types
 *
 * Based on SnippetContactReport from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include "RigidBody/RigidBodyContactHandler.h"
#include <iostream>
#include <iomanip>

using namespace PhysXWrapper;
using namespace physx;

/**
 * @brief Create a stack of boxes
 */
void createStack(PhysXCore& physics, const PxTransform& t, PxU32 size, PxReal halfExtent) {
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

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - Contact Report Example ===" << std::endl;
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
    std::cout << std::endl;

    // Create contact handler
    RigidBodyContactHandler contactHandler;

    // Statistics
    int totalContacts = 0;
    int touchFoundEvents = 0;
    int touchPersistsEvents = 0;
    int touchLostEvents = 0;

    // Set up contact callback
    contactHandler.setContactCallback([&](const ContactEvent& event) {
        // Count events by type
        switch (event.type) {
            case ContactEvent::Type::TOUCH_FOUND:
                touchFoundEvents++;
                std::cout << "[TOUCH_FOUND] New collision detected!" << std::endl;
                break;
            case ContactEvent::Type::TOUCH_PERSISTS:
                touchPersistsEvents++;
                break;
            case ContactEvent::Type::TOUCH_LOST:
                touchLostEvents++;
                std::cout << "[TOUCH_LOST] Contact ended!" << std::endl;
                break;
        }

        // Display contact points for new collisions
        if (event.type == ContactEvent::Type::TOUCH_FOUND && !event.contactPoints.empty()) {
            std::cout << "  Contact points: " << event.contactPoints.size() << std::endl;

            // Show first few contact points
            size_t displayCount = std::min<size_t>(3, event.contactPoints.size());
            for (size_t i = 0; i < displayCount; i++) {
                const auto& cp = event.contactPoints[i];
                std::cout << "    Point " << i + 1 << ":" << std::endl;
                std::cout << "      Position: ("
                          << std::fixed << std::setprecision(2)
                          << cp.position.x << ", "
                          << cp.position.y << ", "
                          << cp.position.z << ")" << std::endl;
                std::cout << "      Normal: ("
                          << cp.normal.x << ", "
                          << cp.normal.y << ", "
                          << cp.normal.z << ")" << std::endl;
                std::cout << "      Impulse magnitude: "
                          << cp.impulse.magnitude() << std::endl;
                std::cout << "      Separation: "
                          << cp.separation << std::endl;
            }

            if (event.contactPoints.size() > displayCount) {
                std::cout << "    ... and " << (event.contactPoints.size() - displayCount)
                          << " more points" << std::endl;
            }
        }

        totalContacts += event.contactPoints.size();
    });

    // Recreate scene with contact reporting enabled
    // NOTE: In a real application, you would use contactHandler.createSceneDesc()
    // when initially creating the scene. Here we demonstrate the manual approach.

    // For this example, we need to manually configure the scene
    // since PhysXCore already created one. In practice, you'd pass
    // the contact handler's scene desc to PhysXCore during initialization.

    std::cout << "Note: This example uses PhysXCore which creates its own scene." << std::endl;
    std::cout << "      In a full implementation, integrate contactHandler.createSceneDesc()" << std::endl;
    std::cout << "      into PhysXCore's initialization process." << std::endl;
    std::cout << std::endl;

    // Create ground plane
    std::cout << "Creating ground plane..." << std::endl;
    PxRigidStatic* groundPlane = physics.createGroundPlane(PxVec3(0, 1, 0), 0);
    if (!groundPlane) {
        std::cerr << "Failed to create ground plane" << std::endl;
        return 1;
    }

    // Create box stack
    std::cout << "Creating box stack..." << std::endl;
    createStack(physics, PxTransform(PxVec3(0, 3.0f, 10.0f)), 5, 2.0f);

    std::cout << std::endl;
    std::cout << "Running simulation..." << std::endl;
    std::cout << "Simulating 5 seconds (300 frames at 60 FPS)" << std::endl;
    std::cout << "Watch for contact events as boxes collide!" << std::endl;
    std::cout << std::endl;

    // Simulation loop
    const float timeStep = 1.0f / 60.0f;
    const int frameCount = 300;

    for (int i = 0; i < frameCount; i++) {
        // Clear previous frame's contact data
        contactHandler.clearContactPoints();

        // Simulate
        if (!physics.update(timeStep)) {
            std::cerr << "Simulation failed at frame " << i << ": "
                      << physics.getLastError() << std::endl;
            return 1;
        }

        // Print statistics every 60 frames (1 second)
        if (i % 60 == 0 && i > 0) {
            std::cout << "\n--- Frame " << i << " Statistics ---" << std::endl;
            std::cout << "  Contact events this second:" << std::endl;
            std::cout << "    TOUCH_FOUND: " << touchFoundEvents << std::endl;
            std::cout << "    TOUCH_PERSISTS: " << touchPersistsEvents << std::endl;
            std::cout << "    TOUCH_LOST: " << touchLostEvents << std::endl;
            std::cout << "  Total contact points: " << totalContacts << std::endl;
            std::cout << std::endl;

            // Reset counters
            touchFoundEvents = 0;
            touchPersistsEvents = 0;
            touchLostEvents = 0;
            totalContacts = 0;
        }
    }

    std::cout << std::endl;
    std::cout << "Simulation completed successfully!" << std::endl;
    std::cout << std::endl;

    // Final statistics
    std::cout << "=== Final Statistics ===" << std::endl;
    std::cout << "  Total frames simulated: " << frameCount << std::endl;
    std::cout << "  All contact points collected: "
              << contactHandler.getContactPoints().size() << std::endl;
    std::cout << "  Total contact events: "
              << contactHandler.getContactEventCount() << std::endl;

    std::cout << std::endl;
    std::cout << "Cleaning up..." << std::endl;

    return 0;
}
