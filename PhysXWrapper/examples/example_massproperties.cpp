/**
 * @file example_massproperties.cpp
 * @brief Mass Properties Calculation and Manipulation Example
 *
 * This example demonstrates PhysX's mass properties system:
 * - Computing mass from shapes and density
 * - Setting custom mass, center of mass, and inertia tensors
 * - Using PxRigidBodyExt utility functions
 * - Demonstrating effects of mass distribution on simulation
 * - Mass properties from compound shapes
 * - Hollow vs solid shapes
 * - Anisotropic inertia tensors
 *
 * Based on PhysX Snippet: SnippetMassProperties
 *
 * Key Concepts:
 * - Mass: Total mass of the rigid body (kg)
 * - Center of Mass: Balance point of the body
 * - Inertia Tensor: 3x3 matrix describing rotational resistance
 * - Mass Frame: Local coordinate system for mass properties
 *
 * Physics Background:
 * - Inertia tensor affects angular acceleration: τ = I·α
 * - Different inertia components cause different rotation behaviors
 * - Low inertia = easy to rotate, high inertia = hard to rotate
 * - Diagonal inertia tensor = principal axes aligned with body frame
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace PhysXWrapper;

class MassPropertiesExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    struct TestActor {
        PxRigidDynamic* actor;
        std::string description;
        PxVec3 initialAngularVel;
    };

    std::vector<TestActor> testActors;

public:
    MassPropertiesExample()
        : physics(nullptr)
        , scene(nullptr)
        , material(nullptr)
    {}

    ~MassPropertiesExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "===================================================" << std::endl;
        std::cout << "PhysX Mass Properties Example" << std::endl;
        std::cout << "===================================================" << std::endl;

        // Initialize PhysX with gravity
        PhysXCore::Config config;
        config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        config.numThreads = 2;

        if (!core.initialize(config)) {
            std::cerr << "Failed to initialize PhysX" << std::endl;
            return false;
        }

        physics = core.getPhysics();
        scene = core.getScene();
        material = physics->createMaterial(0.5f, 0.5f, 0.3f);

        // Create ground plane
        PxRigidStatic* groundPlane = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
        scene->addActor(*groundPlane);

        std::cout << "\nThis example demonstrates different mass configurations:" << std::endl;
        std::cout << "  1. Default mass from density" << std::endl;
        std::cout << "  2. Custom mass with automatic inertia" << std::endl;
        std::cout << "  3. Custom center of mass offset" << std::endl;
        std::cout << "  4. Anisotropic inertia tensor (different rotation resistance)" << std::endl;
        std::cout << "  5. Hollow shape vs solid shape" << std::endl;
        std::cout << "  6. Compound shapes with multiple components" << std::endl;
        std::cout << "  7. Very high inertia (hard to rotate)" << std::endl;
        std::cout << "  8. Very low inertia (easy to rotate)" << std::endl;

        return true;
    }

    /**
     * Test 1: Default mass properties from density
     * PhysX automatically computes mass and inertia from shape and density
     */
    void createDefaultMassActor() {
        PxBoxGeometry boxGeom(1.0f, 1.0f, 1.0f);
        PxTransform pose(PxVec3(-15.0f, 10.0f, 0.0f));

        PxRigidDynamic* actor = PxCreateDynamic(*physics, pose, boxGeom, *material, 1.0f);

        // The density parameter (1.0f) causes automatic mass calculation
        // Mass = density * volume
        // For a 2x2x2 box: volume = 8, so mass = 8 kg

        scene->addActor(*actor);

        PxRigidBodyExt::setMassAndUpdateInertia(*actor, 10.0f);

        TestActor test;
        test.actor = actor;
        test.description = "Default (mass from density)";
        test.initialAngularVel = PxVec3(1.0f, 1.0f, 1.0f);
        testActors.push_back(test);

        printMassProperties(actor, test.description);
    }

    /**
     * Test 2: Custom mass with automatic inertia
     * Set specific mass, let PhysX compute appropriate inertia
     */
    void createCustomMassActor() {
        PxBoxGeometry boxGeom(1.0f, 1.0f, 1.0f);
        PxTransform pose(PxVec3(-10.0f, 10.0f, 0.0f));

        PxRigidDynamic* actor = PxCreateDynamic(*physics, pose, boxGeom, *material, 1.0f);

        // Set custom mass of 100 kg (very heavy)
        // Inertia tensor will be recomputed to match this mass
        PxRigidBodyExt::setMassAndUpdateInertia(*actor, 100.0f);

        scene->addActor(*actor);

        TestActor test;
        test.actor = actor;
        test.description = "Heavy (100 kg)";
        test.initialAngularVel = PxVec3(1.0f, 1.0f, 1.0f);
        testActors.push_back(test);

        printMassProperties(actor, test.description);
    }

    /**
     * Test 3: Custom center of mass offset
     * Move center of mass away from geometric center
     */
    void createOffsetCenterOfMassActor() {
        PxBoxGeometry boxGeom(1.0f, 1.0f, 1.0f);
        PxTransform pose(PxVec3(-5.0f, 10.0f, 0.0f));

        PxRigidDynamic* actor = PxCreateDynamic(*physics, pose, boxGeom, *material, 1.0f);

        // Set mass properties with offset center of mass
        // This simulates an unbalanced object (like a hammer)
        PxRigidBodyExt::setMassAndUpdateInertia(*actor, 10.0f);

        // Move center of mass to one side
        PxVec3 comOffset(0.5f, 0.0f, 0.0f);  // Offset by 0.5m in X direction
        actor->setCMassLocalPose(PxTransform(comOffset));

        scene->addActor(*actor);

        TestActor test;
        test.actor = actor;
        test.description = "Offset center of mass (unbalanced)";
        test.initialAngularVel = PxVec3(0.0f, 2.0f, 0.0f);
        testActors.push_back(test);

        printMassProperties(actor, test.description);
    }

    /**
     * Test 4: Anisotropic inertia tensor
     * Different resistance to rotation around different axes
     */
    void createAnisotropicInertiaActor() {
        PxBoxGeometry boxGeom(1.0f, 1.0f, 1.0f);
        PxTransform pose(PxVec3(0.0f, 10.0f, 0.0f));

        PxRigidDynamic* actor = PxCreateDynamic(*physics, pose, boxGeom, *material, 1.0f);

        // Start with default mass and inertia
        PxRigidBodyExt::setMassAndUpdateInertia(*actor, 10.0f);

        // Get current inertia tensor
        PxVec3 inertia = actor->getMassSpaceInertiaTensor();

        // Make rotation around X-axis very easy (low inertia)
        // Make rotation around Y-axis very hard (high inertia)
        // Keep Z-axis normal
        PxVec3 customInertia(
            inertia.x * 0.1f,   // 10x easier to rotate around X
            inertia.y * 10.0f,  // 10x harder to rotate around Y
            inertia.z           // Normal rotation around Z
        );

        actor->setMassSpaceInertiaTensor(customInertia);

        scene->addActor(*actor);

        TestActor test;
        test.actor = actor;
        test.description = "Anisotropic inertia (X easy, Y hard)";
        test.initialAngularVel = PxVec3(2.0f, 2.0f, 0.0f);
        testActors.push_back(test);

        printMassProperties(actor, test.description);
    }

    /**
     * Test 5: Hollow shape simulation
     * Higher inertia for same mass (mass distributed far from center)
     */
    void createHollowShapeActor() {
        PxBoxGeometry boxGeom(1.0f, 1.0f, 1.0f);
        PxTransform pose(PxVec3(5.0f, 10.0f, 0.0f));

        PxRigidDynamic* actor = PxCreateDynamic(*physics, pose, boxGeom, *material, 1.0f);

        // Set mass
        PxRigidBodyExt::setMassAndUpdateInertia(*actor, 10.0f);

        // Get solid inertia
        PxVec3 solidInertia = actor->getMassSpaceInertiaTensor();

        // Hollow objects have higher inertia for same mass
        // Because mass is distributed further from center
        // Approximate hollow box: I_hollow ≈ I_solid * 1.5
        PxVec3 hollowInertia = solidInertia * 1.5f;
        actor->setMassSpaceInertiaTensor(hollowInertia);

        scene->addActor(*actor);

        TestActor test;
        test.actor = actor;
        test.description = "Hollow shape (higher inertia)";
        test.initialAngularVel = PxVec3(1.0f, 1.0f, 1.0f);
        testActors.push_back(test);

        printMassProperties(actor, test.description);
    }

    /**
     * Test 6: Compound shapes with multiple components
     * Multiple shapes contribute to total mass and inertia
     */
    void createCompoundShapeActor() {
        PxTransform pose(PxVec3(10.0f, 10.0f, 0.0f));
        PxRigidDynamic* actor = physics->createRigidDynamic(pose);

        // Create a compound shape: box body + sphere head
        PxBoxGeometry bodyGeom(0.5f, 1.0f, 0.5f);
        PxSphereGeometry headGeom(0.5f);

        PxShape* bodyShape = PxRigidActorExt::createExclusiveShape(*actor, bodyGeom, *material);
        PxShape* headShape = PxRigidActorExt::createExclusiveShape(*actor, headGeom, *material);

        // Position head on top of body
        PxTransform headLocalPose(PxVec3(0.0f, 1.5f, 0.0f));
        headShape->setLocalPose(headLocalPose);

        // Update mass and inertia from all shapes
        // This considers both shapes and their positions
        PxRigidBodyExt::updateMassAndInertia(*actor, 1.0f);  // 1.0 kg/unit³ density

        scene->addActor(*actor);

        TestActor test;
        test.actor = actor;
        test.description = "Compound (body + head)";
        test.initialAngularVel = PxVec3(0.0f, 0.0f, 2.0f);
        testActors.push_back(test);

        printMassProperties(actor, test.description);
    }

    /**
     * Test 7: Very high inertia (hard to rotate)
     * Demonstrates massive rotating resistance
     */
    void createHighInertiaActor() {
        PxBoxGeometry boxGeom(1.0f, 1.0f, 1.0f);
        PxTransform pose(PxVec3(15.0f, 10.0f, 0.0f));

        PxRigidDynamic* actor = PxCreateDynamic(*physics, pose, boxGeom, *material, 1.0f);

        PxRigidBodyExt::setMassAndUpdateInertia(*actor, 10.0f);

        // Multiply inertia by 100 (very hard to rotate)
        PxVec3 inertia = actor->getMassSpaceInertiaTensor();
        actor->setMassSpaceInertiaTensor(inertia * 100.0f);

        scene->addActor(*actor);

        TestActor test;
        test.actor = actor;
        test.description = "Very high inertia (hard to rotate)";
        test.initialAngularVel = PxVec3(2.0f, 2.0f, 2.0f);
        testActors.push_back(test);

        printMassProperties(actor, test.description);
    }

    /**
     * Test 8: Very low inertia (easy to rotate)
     * Demonstrates minimal rotating resistance
     */
    void createLowInertiaActor() {
        PxBoxGeometry boxGeom(1.0f, 1.0f, 1.0f);
        PxTransform pose(PxVec3(20.0f, 10.0f, 0.0f));

        PxRigidDynamic* actor = PxCreateDynamic(*physics, pose, boxGeom, *material, 1.0f);

        PxRigidBodyExt::setMassAndUpdateInertia(*actor, 10.0f);

        // Divide inertia by 10 (very easy to rotate)
        PxVec3 inertia = actor->getMassSpaceInertiaTensor();
        actor->setMassSpaceInertiaTensor(inertia * 0.1f);

        scene->addActor(*actor);

        TestActor test;
        test.actor = actor;
        test.description = "Very low inertia (easy to rotate)";
        test.initialAngularVel = PxVec3(2.0f, 2.0f, 2.0f);
        testActors.push_back(test);

        printMassProperties(actor, test.description);
    }

    void printMassProperties(PxRigidDynamic* actor, const std::string& description) {
        PxReal mass = actor->getMass();
        PxVec3 inertia = actor->getMassSpaceInertiaTensor();
        PxTransform comPose = actor->getCMassLocalPose();

        std::cout << "\n" << description << ":" << std::endl;
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "  Mass: " << mass << " kg" << std::endl;
        std::cout << "  Inertia: (" << inertia.x << ", " << inertia.y << ", " << inertia.z << ")" << std::endl;
        std::cout << "  CoM offset: (" << comPose.p.x << ", " << comPose.p.y << ", " << comPose.p.z << ")" << std::endl;
    }

    void printActorState(const TestActor& test) {
        PxTransform pose = test.actor->getGlobalPose();
        PxVec3 angVel = test.actor->getAngularVelocity();
        PxVec3 linVel = test.actor->getLinearVelocity();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << test.description << ":" << std::endl;
        std::cout << "  Pos: (" << pose.p.x << ", " << pose.p.y << ", " << pose.p.z << ")  ";
        std::cout << "  AngVel: (" << angVel.x << ", " << angVel.y << ", " << angVel.z << ")" << std::endl;
    }

    void createAllTests() {
        std::cout << "\n=== Creating Test Actors ===" << std::endl;

        createDefaultMassActor();
        createCustomMassActor();
        createOffsetCenterOfMassActor();
        createAnisotropicInertiaActor();
        createHollowShapeActor();
        createCompoundShapeActor();
        createHighInertiaActor();
        createLowInertiaActor();

        // Apply initial angular velocities to all actors
        for (auto& test : testActors) {
            test.actor->setAngularVelocity(test.initialAngularVel);
        }
    }

    void demonstrateMassPropertiesAPI() {
        std::cout << "\n=== Mass Properties API Demonstration ===" << std::endl;

        // Create a test actor
        PxBoxGeometry boxGeom(1.0f, 2.0f, 0.5f);  // Non-uniform box
        PxTransform pose(PxVec3(0.0f, 20.0f, 0.0f));
        PxRigidDynamic* actor = PxCreateDynamic(*physics, pose, boxGeom, *material, 1.0f);

        std::cout << "\n1. Initial properties (from density 1.0):" << std::endl;
        printMassProperties(actor, "Initial state");

        // Method 1: Set mass and update inertia proportionally
        std::cout << "\n2. After setMassAndUpdateInertia(50.0):" << std::endl;
        PxRigidBodyExt::setMassAndUpdateInertia(*actor, 50.0f);
        printMassProperties(actor, "After mass update");

        // Method 2: Add multiple shapes and recompute
        PxSphereGeometry sphereGeom(0.5f);
        PxShape* extraShape = PxRigidActorExt::createExclusiveShape(*actor, sphereGeom, *material);
        extraShape->setLocalPose(PxTransform(PxVec3(0.0f, 2.5f, 0.0f)));

        std::cout << "\n3. After adding sphere and updateMassAndInertia():" << std::endl;
        PxRigidBodyExt::updateMassAndInertia(*actor, 1.0f);  // Recompute from all shapes
        printMassProperties(actor, "After shape addition");

        // Method 3: Manual inertia tensor setting
        PxVec3 customInertia(10.0f, 50.0f, 20.0f);  // Custom values
        actor->setMassSpaceInertiaTensor(customInertia);

        std::cout << "\n4. After manual inertia tensor setting:" << std::endl;
        printMassProperties(actor, "Custom inertia");

        // Method 4: Set complete mass properties at once
        PxMassProperties massProps;
        massProps.mass = 100.0f;
        massProps.centerOfMass = PxVec3(0.5f, 0.0f, 0.0f);
        massProps.inertiaTensor = PxMat33(PxVec3(20.0f, 0, 0),
                                          PxVec3(0, 80.0f, 0),
                                          PxVec3(0, 0, 30.0f));

        actor->setMass(massProps.mass);
        actor->setCMassLocalPose(PxTransform(massProps.centerOfMass));
        actor->setMassSpaceInertiaTensor(PxVec3(massProps.inertiaTensor.column0.x,
                                                 massProps.inertiaTensor.column1.y,
                                                 massProps.inertiaTensor.column2.z));

        std::cout << "\n5. After setting complete mass properties:" << std::endl;
        printMassProperties(actor, "Complete custom properties");

        scene->addActor(*actor);
    }

    void simulate(PxReal dt) {
        scene->simulate(dt);
        scene->fetchResults(true);
    }

    void run() {
        std::cout << "\n=== Starting Mass Properties Simulation ===" << std::endl;

        // First demonstrate the API
        demonstrateMassPropertiesAPI();

        // Then create all test actors
        createAllTests();

        std::cout << "\n=== Running Simulation ===" << std::endl;
        std::cout << "All actors start with angular velocity applied" << std::endl;
        std::cout << "Observe how different mass properties affect rotation" << std::endl;

        const PxReal dt = 1.0f / 60.0f;
        const int totalFrames = 300;  // 5 seconds

        for (int frame = 0; frame < totalFrames; frame++) {
            simulate(dt);

            // Print status every 60 frames (1 second)
            if (frame % 60 == 0) {
                std::cout << "\n=== Frame " << frame << " (t=" << (frame * dt) << "s) ===" << std::endl;
                for (const auto& test : testActors) {
                    printActorState(test);
                }
            }
        }

        std::cout << "\n=== Simulation Complete ===" << std::endl;
        std::cout << "\nKey Observations:" << std::endl;
        std::cout << "  • Heavy objects fall at same rate (gravity is mass-independent)" << std::endl;
        std::cout << "  • Higher inertia = slower angular deceleration" << std::endl;
        std::cout << "  • Anisotropic inertia = different rotation speeds per axis" << std::endl;
        std::cout << "  • Offset CoM = wobbling/precession during rotation" << std::endl;
        std::cout << "  • Compound shapes = combined mass properties from all components" << std::endl;
    }

    void cleanup() {
        // Actors are automatically released by PhysXCore
        if (material) material->release();
        core.cleanup();
    }
};

int main() {
    MassPropertiesExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
