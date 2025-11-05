/**
 * @file example_contactmodifier.cpp
 * @brief Contact modification example
 *
 * This example demonstrates:
 * - Mass ratio adjustment for stability
 * - Runtime friction modification
 * - Runtime restitution modification
 * - Custom contact modification rules
 * - Material-based modifications
 *
 * Based on SnippetContactModification from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include "RigidBody/ContactModifier.h"
#include <iostream>
#include <iomanip>

using namespace PhysXWrapper;
using namespace physx;

/**
 * @brief Test 1: Mass Ratio Adjustment
 */
void testMassRatioAdjustment(PhysXCore& physics, PxScene* scene, ContactModifier& modifier) {
    std::cout << "\n=== TEST 1: Mass Ratio Adjustment ===" << std::endl;
    std::cout << "Creating stack with extreme mass differences..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create stack with increasing mass (1x, 8x, 27x, 64x, 125x)
    // Without mass ratio adjustment, this would be highly unstable
    for (PxU32 i = 0; i < 5; i++) {
        PxReal size = 1.0f;
        PxReal y = 0.5f + i * 2.0f * size;
        PxReal mass = (i + 1) * (i + 1) * (i + 1) * 10.0f;

        PxRigidDynamic* box = physics.getPhysics()->createRigidDynamic(
            PxTransform(PxVec3(0, y, 0))
        );

        PxShape* shape = PxRigidActorExt::createExclusiveShape(
            *box, PxBoxGeometry(size, size, size), *material
        );

        PxRigidBodyExt::updateMassAndInertia(*box, mass);
        scene->addActor(*box);

        std::cout << "  Box " << i << ": mass = " << mass << " kg" << std::endl;
    }

    // Enable mass ratio adjustment
    MassRatioAdjustmentConfig massConfig;
    massConfig.maxMassRatio = 5.0f;  // Limit to 5:1 ratio
    massConfig.scaleInertia = true;
    modifier.enableMassRatioAdjustment(massConfig);

    std::cout << "  Mass ratio adjustment enabled (max ratio: 5:1)" << std::endl;
    std::cout << "  Stack should be stable despite extreme mass differences" << std::endl;
}

/**
 * @brief Test 2: Friction Modification (Ice Surface)
 */
void testFrictionModification(PhysXCore& physics, PxScene* scene, ContactModifier& modifier) {
    std::cout << "\n=== TEST 2: Friction Modification (Ice Surface) ===" << std::endl;
    std::cout << "Creating sliding objects with reduced friction..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create ramp
    PxRigidStatic* ramp = physics.getPhysics()->createRigidStatic(
        PxTransform(PxVec3(10, 2, 0), PxQuat(PxPi / 6.0f, PxVec3(0, 0, 1)))
    );
    PxShape* rampShape = PxRigidActorExt::createExclusiveShape(
        *ramp, PxBoxGeometry(5, 0.5f, 5), *material
    );
    scene->addActor(*ramp);

    std::cout << "  Created ramp" << std::endl;

    // Create sliding box
    PxRigidDynamic* box = physics.getPhysics()->createRigidDynamic(
        PxTransform(PxVec3(13, 5, 0))
    );
    PxShape* boxShape = PxRigidActorExt::createExclusiveShape(
        *box, PxBoxGeometry(0.5f, 0.5f, 0.5f), *material
    );
    PxRigidBodyExt::updateMassAndInertia(*box, 10.0f);
    scene->addActor(*box);

    std::cout << "  Created sliding box" << std::endl;

    // Reduce friction (ice surface effect)
    FrictionModificationConfig frictionConfig;
    frictionConfig.frictionMultiplier = 0.1f;  // 10% of original friction
    frictionConfig.enableForAllContacts = true;
    modifier.enableFrictionModification(frictionConfig);

    std::cout << "  Friction reduced to 10% (ice surface)" << std::endl;
    std::cout << "  Box should slide easily down the ramp" << std::endl;
}

/**
 * @brief Test 3: Restitution Modification (Super Bouncy)
 */
void testRestitutionModification(PhysXCore& physics, PxScene* scene, ContactModifier& modifier) {
    std::cout << "\n=== TEST 3: Restitution Modification (Super Bouncy) ===" << std::endl;
    std::cout << "Creating bouncy objects..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create bouncing balls
    for (PxU32 i = 0; i < 3; i++) {
        PxRigidDynamic* ball = physics.getPhysics()->createRigidDynamic(
            PxTransform(PxVec3(20 + i * 2, 8, 0))
        );

        PxShape* shape = PxRigidActorExt::createExclusiveShape(
            *ball, PxSphereGeometry(0.5f), *material
        );

        PxRigidBodyExt::updateMassAndInertia(*ball, 5.0f);
        scene->addActor(*ball);
    }

    std::cout << "  Created 3 bouncing balls" << std::endl;

    // Increase restitution (super bouncy)
    RestitutionModificationConfig restitutionConfig;
    restitutionConfig.restitutionMultiplier = 2.0f;  // 200% bounciness
    restitutionConfig.enableForAllContacts = true;
    modifier.enableRestitutionModification(restitutionConfig);

    std::cout << "  Restitution increased to 200% (super bouncy)" << std::endl;
    std::cout << "  Balls should bounce very high" << std::endl;
}

/**
 * @brief Test 4: Custom Modification (One-Way Platform)
 */
void testCustomModification(PhysXCore& physics, PxScene* scene, ContactModifier& modifier) {
    std::cout << "\n=== TEST 4: Custom Modification (One-Way Platform) ===" << std::endl;
    std::cout << "Creating one-way platform..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create platform
    PxRigidStatic* platform = physics.getPhysics()->createRigidStatic(
        PxTransform(PxVec3(30, 5, 0))
    );
    PxShape* platformShape = PxRigidActorExt::createExclusiveShape(
        *platform, PxBoxGeometry(3, 0.2f, 3), *material
    );

    // Tag platform shape for identification
    platformShape->setSimulationFilterData(PxFilterData(1, 0, 0, 0));  // ID = 1
    scene->addActor(*platform);

    std::cout << "  Created platform" << std::endl;

    // Create object falling from above
    PxRigidDynamic* box1 = physics.getPhysics()->createRigidDynamic(
        PxTransform(PxVec3(30, 8, 0))
    );
    PxShape* box1Shape = PxRigidActorExt::createExclusiveShape(
        *box1, PxBoxGeometry(0.5f, 0.5f, 0.5f), *material
    );
    PxRigidBodyExt::updateMassAndInertia(*box1, 10.0f);
    scene->addActor(*box1);

    std::cout << "  Created falling box (should pass through from bottom)" << std::endl;

    // Add custom modification for one-way platform
    modifier.addCustomModification([](const ContactModificationContext& ctx) {
        // Check if one of the shapes is our platform
        bool isPlatformContact = false;
        PxShape* otherShape = nullptr;

        PxFilterData data0 = ctx.shape0->getSimulationFilterData();
        PxFilterData data1 = ctx.shape1->getSimulationFilterData();

        if (data0.word0 == 1) {
            isPlatformContact = true;
            otherShape = ctx.shape1;
        } else if (data1.word0 == 1) {
            isPlatformContact = true;
            otherShape = ctx.shape0;
        }

        if (isPlatformContact && otherShape) {
            // Get contact normal
            PxVec3 normal = ctx.pair->contacts.getNormal(ctx.contactIndex);

            // Only allow contact if normal points up (object on top)
            // Disable contact if normal points down (object below)
            if (normal.y < 0.5f) {
                // Disable this contact by setting very high separation
                ctx.pair->contacts.setSeparation(ctx.contactIndex, 1000.0f);
            }
        }
    });

    std::cout << "  One-way platform enabled (only supports from above)" << std::endl;
}

/**
 * @brief Run simulation and print statistics
 */
void runSimulation(PhysXCore& physics, PxScene* scene, ContactModifier& modifier) {
    std::cout << "\n=== Running Simulation ===" << std::endl;

    const PxReal timeStep = 1.0f / 60.0f;
    const int frameCount = 300;  // 5 seconds

    for (int i = 0; i < frameCount; i++) {
        modifier.resetStatistics();

        scene->simulate(timeStep);
        scene->fetchResults(true);

        // Print statistics every second
        if (i % 60 == 0) {
            std::cout << "\n--- Frame " << i << " (" << (i / 60.0f) << "s) ---" << std::endl;
            std::cout << "  Modified contacts: " << modifier.getModifiedContactCount() << std::endl;
        }
    }

    std::cout << "\nSimulation complete!" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - Contact Modification Example ===" << std::endl;
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
            std::cout << "PVD enabled" << std::endl;
        }
    }

    // Initialize PhysX
    PhysXCore physics;
    std::cout << "Initializing PhysX..." << std::endl;
    if (!physics.initialize(config)) {
        std::cerr << "Failed to initialize PhysX: " << physics.getLastError() << std::endl;
        return 1;
    }

    // Get scene
    PxScene* scene = physics.getScene();
    if (!scene) {
        std::cerr << "Failed to get scene" << std::endl;
        return 1;
    }

    // IMPORTANT: Set filter shader to enable contact modification
    PxSceneDesc& sceneDesc = const_cast<PxSceneDesc&>(scene->getSceneDesc());
    sceneDesc.filterShader = ContactModificationFilterShader;

    // Create ground plane
    PxRigidStatic* ground = PxCreatePlane(
        *physics.getPhysics(),
        PxPlane(0, 1, 0, 0),
        *physics.getDefaultMaterial()
    );
    scene->addActor(*ground);
    std::cout << "Created ground plane" << std::endl;

    // Initialize contact modifier
    ContactModifier modifier;
    if (!modifier.initialize(physics.getPhysics(), scene)) {
        std::cerr << "Failed to initialize ContactModifier: "
                  << modifier.getLastError() << std::endl;
        return 1;
    }
    std::cout << "ContactModifier initialized" << std::endl;

    // Run tests
    testMassRatioAdjustment(physics, scene, modifier);
    testFrictionModification(physics, scene, modifier);
    testRestitutionModification(physics, scene, modifier);
    testCustomModification(physics, scene, modifier);

    // Run simulation
    runSimulation(physics, scene, modifier);

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "\nThis example demonstrated:" << std::endl;
    std::cout << "  - Mass ratio adjustment for stability (extreme mass differences)" << std::endl;
    std::cout << "  - Friction modification (ice surface effect)" << std::endl;
    std::cout << "  - Restitution modification (super bouncy)" << std::endl;
    std::cout << "  - Custom modifications (one-way platforms)" << std::endl;
    std::cout << "  - Runtime contact property changes" << std::endl;

    std::cout << "\nCleaning up..." << std::endl;
    modifier.cleanup();

    return 0;
}
