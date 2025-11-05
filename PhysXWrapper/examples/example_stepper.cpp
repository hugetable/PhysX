/**
 * @file example_stepper.cpp
 * @brief Custom Time Stepper and Substepping Example
 *
 * This example demonstrates advanced time stepping techniques in PhysX:
 * - Substepping: dividing each frame into multiple smaller substeps
 * - Kinematic actor updates between substeps
 * - Improved accuracy for fast-moving objects
 * - Variable timestep handling
 *
 * Based on PhysX Snippet: SnippetStepper
 *
 * Key Concepts:
 * - Substepping improves simulation accuracy
 * - Kinematic actors can be updated between substeps
 * - Useful for precise control of moving platforms
 * - Trade-off: better accuracy vs more computation
 *
 * Scenario:
 * A kinematic platform oscillates vertically (sine wave motion)
 * A dynamic sphere bounces on the platform
 * The platform position is updated before each substep
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace PhysXWrapper;

class StepperExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    PxRigidDynamic* kinematicPlatform;
    PxRigidDynamic* dynamicSphere;

    // Substepping configuration
    static constexpr PxReal SUBSTEP_LENGTH = 1.0f / 120.0f;  // 120Hz substeps
    static constexpr int NUM_SUBSTEPS = 2;                    // 2 substeps per frame
    static constexpr PxReal FRAME_TIME = SUBSTEP_LENGTH * NUM_SUBSTEPS;  // 60Hz frame rate

    // Platform motion parameters
    PxReal totalTime;
    static constexpr PxReal PERIOD = 4.0f;      // Oscillation period in seconds
    static constexpr PxReal AMPLITUDE = 5.0f;   // Oscillation amplitude in meters

    enum class SteppingMode {
        NO_SUBSTEPPING,         // Single step per frame
        FIXED_SUBSTEPPING,      // Fixed substeps per frame
        ADAPTIVE_SUBSTEPPING    // Adaptive based on velocities
    };

    SteppingMode currentMode;

public:
    StepperExample()
        : physics(nullptr)
        , scene(nullptr)
        , material(nullptr)
        , kinematicPlatform(nullptr)
        , dynamicSphere(nullptr)
        , totalTime(0.0f)
        , currentMode(SteppingMode::FIXED_SUBSTEPPING)
    {}

    ~StepperExample() {
        cleanup();
    }

    bool initialize() {
        // Initialize PhysX
        PhysXCore::Config config;
        config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        config.numThreads = 2;

        if (!core.initialize(config)) {
            std::cerr << "Failed to initialize PhysX" << std::endl;
            return false;
        }

        physics = core.getPhysics();
        scene = core.getScene();

        // Create material with some bounciness
        material = physics->createMaterial(0.5f, 0.5f, 0.7f);

        // Create kinematic platform
        PxBoxGeometry platformGeom(5.0f, 0.5f, 5.0f);
        PxTransform platformPose(PxVec3(0.0f, 0.0f, 0.0f));

        kinematicPlatform = PxCreateDynamic(*physics, platformPose, platformGeom, *material, 1.0f);
        kinematicPlatform->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        scene->addActor(*kinematicPlatform);

        // Create dynamic sphere
        PxSphereGeometry sphereGeom(1.0f);
        PxTransform spherePose(PxVec3(0.0f, 5.0f, 0.0f));

        dynamicSphere = PxCreateDynamic(*physics, spherePose, sphereGeom, *material, 1.0f);
        dynamicSphere->setAngularDamping(0.5f);
        scene->addActor(*dynamicSphere);

        std::cout << "PhysX Stepper Example Initialized" << std::endl;
        std::cout << "Substep length: " << (SUBSTEP_LENGTH * 1000) << "ms" << std::endl;
        std::cout << "Number of substeps: " << NUM_SUBSTEPS << std::endl;
        std::cout << "Effective frame rate: " << (1.0f / FRAME_TIME) << " Hz" << std::endl;

        return true;
    }

    void updateKinematicTarget(PxReal time) {
        // Calculate platform position using sine wave
        PxReal angularVelocity = PxTwoPi / PERIOD;
        PxReal yPosition = std::sin(angularVelocity * time) * AMPLITUDE;

        // Set kinematic target
        PxTransform targetPose(PxVec3(0.0f, yPosition, 0.0f));
        kinematicPlatform->setKinematicTarget(targetPose);
    }

    void simulateWithoutSubstepping(PxReal dt) {
        // Simple single-step simulation
        scene->simulate(dt);
        scene->fetchResults(true);
    }

    void simulateWithFixedSubstepping(PxReal dt) {
        // Fixed number of substeps per frame
        int numSubsteps = NUM_SUBSTEPS;
        PxReal substepDt = dt / numSubsteps;

        for (int i = 0; i < numSubsteps; i++) {
            // Update kinematic target before each substep
            updateKinematicTarget(totalTime + i * substepDt);

            // Run substep
            scene->simulate(substepDt);
            scene->fetchResults(true);
        }
    }

    void simulateWithAdaptiveSubstepping(PxReal dt) {
        // Adaptive substepping based on sphere velocity
        PxVec3 sphereVel = dynamicSphere->getLinearVelocity();
        PxReal speed = sphereVel.magnitude();

        // More substeps for faster motion
        int numSubsteps = 1;
        if (speed > 10.0f) {
            numSubsteps = 4;
        } else if (speed > 5.0f) {
            numSubsteps = 2;
        }

        PxReal substepDt = dt / numSubsteps;

        for (int i = 0; i < numSubsteps; i++) {
            updateKinematicTarget(totalTime + i * substepDt);
            scene->simulate(substepDt);
            scene->fetchResults(true);
        }
    }

    void simulate(PxReal dt) {
        switch (currentMode) {
            case SteppingMode::NO_SUBSTEPPING:
                simulateWithoutSubstepping(dt);
                break;

            case SteppingMode::FIXED_SUBSTEPPING:
                simulateWithFixedSubstepping(dt);
                break;

            case SteppingMode::ADAPTIVE_SUBSTEPPING:
                simulateWithAdaptiveSubstepping(dt);
                break;
        }

        totalTime += dt;
    }

    void printStatus(int frame) {
        PxTransform platformPose = kinematicPlatform->getGlobalPose();
        PxTransform spherePose = dynamicSphere->getGlobalPose();
        PxVec3 sphereVel = dynamicSphere->getLinearVelocity();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Frame " << std::setw(4) << frame << " @ t=" << std::setw(5) << totalTime << "s  ";
        std::cout << "Platform Y=" << std::setw(6) << platformPose.p.y << "  ";
        std::cout << "Sphere Y=" << std::setw(6) << spherePose.p.y << "  ";
        std::cout << "Speed=" << std::setw(6) << sphereVel.magnitude() << std::endl;
    }

    void runComparison() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "COMPARING STEPPING MODES" << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Test each mode
        struct TestResult {
            std::string modeName;
            PxReal finalSphereHeight;
            PxReal avgSpeed;
            int contactCount;
        };

        std::vector<TestResult> results;

        for (int mode = 0; mode < 3; mode++) {
            // Reset scene
            dynamicSphere->setGlobalPose(PxTransform(PxVec3(0.0f, 5.0f, 0.0f)));
            dynamicSphere->setLinearVelocity(PxVec3(0.0f));
            dynamicSphere->setAngularVelocity(PxVec3(0.0f));
            totalTime = 0.0f;

            currentMode = static_cast<SteppingMode>(mode);

            std::string modeName;
            switch (currentMode) {
                case SteppingMode::NO_SUBSTEPPING:
                    modeName = "No Substepping (60Hz)";
                    break;
                case SteppingMode::FIXED_SUBSTEPPING:
                    modeName = "Fixed Substepping (2x120Hz)";
                    break;
                case SteppingMode::ADAPTIVE_SUBSTEPPING:
                    modeName = "Adaptive Substepping";
                    break;
            }

            std::cout << "\n--- Testing: " << modeName << " ---" << std::endl;

            // Simulate for 10 seconds
            const int totalFrames = 600;  // 10 seconds at 60fps
            PxReal totalSpeed = 0.0f;

            for (int frame = 0; frame < totalFrames; frame++) {
                simulate(FRAME_TIME);

                PxVec3 vel = dynamicSphere->getLinearVelocity();
                totalSpeed += vel.magnitude();

                // Print status every second
                if (frame % 60 == 0) {
                    printStatus(frame);
                }
            }

            // Record result
            TestResult result;
            result.modeName = modeName;
            result.finalSphereHeight = dynamicSphere->getGlobalPose().p.y;
            result.avgSpeed = totalSpeed / totalFrames;
            result.contactCount = 0;  // Would need contact reporting to track
            results.push_back(result);

            std::cout << "Final sphere height: " << result.finalSphereHeight << std::endl;
            std::cout << "Average speed: " << result.avgSpeed << std::endl;
        }

        // Print comparison
        std::cout << "\n========================================" << std::endl;
        std::cout << "RESULTS COMPARISON" << std::endl;
        std::cout << "========================================" << std::endl;

        for (const auto& result : results) {
            std::cout << "\n" << result.modeName << ":" << std::endl;
            std::cout << "  Final height: " << result.finalSphereHeight << std::endl;
            std::cout << "  Avg speed: " << result.avgSpeed << std::endl;
        }

        std::cout << "\nObservations:" << std::endl;
        std::cout << "- Substepping provides more accurate collision detection" << std::endl;
        std::cout << "- Kinematic updates between substeps = smoother interaction" << std::endl;
        std::cout << "- Adaptive substepping balances accuracy and performance" << std::endl;
    }

    void cleanup() {
        if (kinematicPlatform) kinematicPlatform->release();
        if (dynamicSphere) dynamicSphere->release();
        if (material) material->release();
        core.cleanup();
    }

    void run() {
        std::cout << "\nStarting Stepper Example..." << std::endl;
        runComparison();
        std::cout << "\nExample complete!" << std::endl;
    }
};

int main() {
    StepperExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
