/**
 * @file example_jointdrive.cpp
 * @brief D6 Joint Drive System Example
 *
 * This example demonstrates the use of PxD6Joint drives for motion control:
 * - Linear drives (moving along X/Y/Z axes)
 * - Angular drives (TWIST, SWING, SLERP)
 * - Drive velocity and target position control
 * - Acceleration vs Force-based drives
 *
 * Based on PhysX Snippet: SnippetJointDrive
 *
 * Usage:
 *   Press 1-6 to switch between different drive scenarios
 *   Space: Pause/Resume simulation
 *
 * Scenarios:
 *   1. Linear X-axis drive with velocity target
 *   2. TWIST angular drive (rotation around X-axis)
 *   3. SWING1 angular drive (rotation around Y-axis)
 *   4. SLERP angular drive (spherical interpolation)
 *   5. Position-based linear drive
 *   6. Combined linear and angular drives
 */

#include "PhysXCore.h"
#include "Joint/JointManager.h"
#include <iostream>
#include <iomanip>

using namespace PhysXWrapper;

class JointDriveExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;
    JointManager* jointManager;

    PxD6Joint* currentJoint;
    PxRigidDynamic* drivenActor;

    int currentScenario;
    bool paused;

public:
    JointDriveExample()
        : physics(nullptr)
        , scene(nullptr)
        , material(nullptr)
        , jointManager(nullptr)
        , currentJoint(nullptr)
        , drivenActor(nullptr)
        , currentScenario(1)
        , paused(false)
    {}

    ~JointDriveExample() {
        cleanup();
    }

    bool initialize() {
        // Initialize PhysX with zero gravity for drive demonstration
        PhysXCore::Config config;
        config.gravity = PxVec3(0.0f);  // No gravity - motion only from drives
        config.numThreads = 2;

        if (!core.initialize(config)) {
            std::cerr << "Failed to initialize PhysX" << std::endl;
            return false;
        }

        physics = core.getPhysics();
        scene = core.getScene();

        // Create material
        material = physics->createMaterial(0.5f, 0.5f, 0.1f);

        // Initialize joint manager
        jointManager = new JointManager();
        jointManager->initialize(physics, scene);

        // Enable joint visualization
        scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);

        std::cout << "PhysX Joint Drive Example Initialized" << std::endl;
        std::cout << "Press 1-6 to switch scenarios, Space to pause/resume" << std::endl;

        // Create initial scenario
        createScenario(1);

        return true;
    }

    void createScenario(int scenario) {
        // Clean up previous scenario
        if (currentJoint) {
            currentJoint->release();
            currentJoint = nullptr;
        }
        if (drivenActor) {
            drivenActor->release();
            drivenActor = nullptr;
        }

        currentScenario = scenario;

        // Create base actor (anchor point)
        PxBoxGeometry boxGeom(0.5f, 0.5f, 0.5f);
        PxTransform basePose(PxVec3(0.0f, 0.0f, 0.0f));

        PxRigidStatic* baseActor = PxCreateStatic(*physics, basePose, boxGeom, *material);
        scene->addActor(*baseActor);

        // Create driven actor
        PxTransform drivenPose(PxVec3(2.0f, 0.0f, 0.0f));
        drivenActor = PxCreateDynamic(*physics, drivenPose, boxGeom, *material, 1.0f);
        scene->addActor(*drivenActor);

        // Setup D6 joint with all DOFs free initially
        PxTransform localFrame0(PxVec3(0.5f, 0, 0));  // Offset from base
        PxTransform localFrame1(PxVec3(-0.5f, 0, 0)); // Offset from driven

        // Create D6 joint with drive configuration based on scenario
        JointManager::D6JointConfig config;

        // Free all DOFs so drive can work without interference
        config.motionX = PxD6Motion::eFREE;
        config.motionY = PxD6Motion::eFREE;
        config.motionZ = PxD6Motion::eFREE;
        config.motionSwing1 = PxD6Motion::eFREE;
        config.motionSwing2 = PxD6Motion::eFREE;
        config.motionTwist = PxD6Motion::eFREE;

        // Configure drive based on scenario
        config.enableDrive = true;
        config.driveStiffness = 0.0f;       // Pure velocity drive
        config.driveDamping = 1000.0f;      // High damping for velocity control
        config.driveForceLimit = PX_MAX_F32;
        config.driveIsAcceleration = true;   // Acceleration-based

        switch (scenario) {
            case 1:
                std::cout << "\n=== Scenario 1: Linear X-axis Drive (Velocity) ===" << std::endl;
                std::cout << "Actor will move along X-axis with constant velocity" << std::endl;
                config.driveType = PxD6Drive::eX;
                break;

            case 2:
                std::cout << "\n=== Scenario 2: TWIST Angular Drive ===" << std::endl;
                std::cout << "Actor will rotate around X-axis (twist)" << std::endl;
                config.driveType = PxD6Drive::eTWIST;
                break;

            case 3:
                std::cout << "\n=== Scenario 3: SWING1 Angular Drive ===" << std::endl;
                std::cout << "Actor will rotate around Y-axis (swing)" << std::endl;
                config.driveType = PxD6Drive::eSWING1;
                break;

            case 4:
                std::cout << "\n=== Scenario 4: SLERP Angular Drive ===" << std::endl;
                std::cout << "Actor will rotate using spherical interpolation" << std::endl;
                config.driveType = PxD6Drive::eSLERP;
                break;

            case 5:
                std::cout << "\n=== Scenario 5: Position-based Linear Drive ===" << std::endl;
                std::cout << "Actor will move to target position" << std::endl;
                config.driveType = PxD6Drive::eX;
                config.driveStiffness = 1000.0f;  // Add stiffness for position drive
                break;

            case 6:
                std::cout << "\n=== Scenario 6: Combined Linear + Angular Drive ===" << std::endl;
                std::cout << "Actor will move and rotate simultaneously" << std::endl;
                config.driveType = PxD6Drive::eX;
                break;
        }

        currentJoint = jointManager->createD6Joint(
            baseActor, localFrame0,
            drivenActor, localFrame1,
            config
        );

        if (!currentJoint) {
            std::cerr << "Failed to create D6 joint" << std::endl;
            return;
        }

        // Set drive velocities/targets based on scenario
        applyDriveParameters(scenario);

        std::cout << "Scenario " << scenario << " created successfully" << std::endl;
    }

    void applyDriveParameters(int scenario) {
        if (!currentJoint) return;

        switch (scenario) {
            case 1:
                // Linear velocity drive along X
                currentJoint->setDriveVelocity(
                    PxVec3(2.0f, 0.0f, 0.0f),  // Linear velocity
                    PxVec3(0.0f),               // Angular velocity
                    true                        // Wake up actor
                );
                break;

            case 2:
                // TWIST angular drive
                currentJoint->setAngularDriveConfig(PxD6AngularDriveConfig::eSWING_TWIST);
                currentJoint->setDrive(PxD6Drive::eTWIST,
                    PxD6JointDrive(0.0f, 1000.0f, PX_MAX_F32, true));
                currentJoint->setDriveVelocity(
                    PxVec3(0.0f),                    // No linear velocity
                    PxVec3(1.0f, 0.0f, 0.0f),       // Twist around X
                    true
                );
                break;

            case 3:
                // SWING1 angular drive
                currentJoint->setAngularDriveConfig(PxD6AngularDriveConfig::eSWING_TWIST);
                currentJoint->setDrive(PxD6Drive::eSWING1,
                    PxD6JointDrive(0.0f, 1000.0f, PX_MAX_F32, true));
                currentJoint->setDriveVelocity(
                    PxVec3(0.0f),
                    PxVec3(0.0f, 1.0f, 0.0f),       // Swing around Y
                    true
                );
                break;

            case 4:
                // SLERP drive
                currentJoint->setAngularDriveConfig(PxD6AngularDriveConfig::eSLERP);
                currentJoint->setDrive(PxD6Drive::eSLERP,
                    PxD6JointDrive(0.0f, 1000.0f, PX_MAX_F32, true));
                currentJoint->setDriveVelocity(
                    PxVec3(0.0f),
                    PxVec3(0.0f, 1.0f, 0.0f),       // Rotate
                    true
                );
                break;

            case 5:
                // Position drive
                currentJoint->setDrivePosition(
                    PxTransform(PxVec3(5.0f, 0.0f, 0.0f)),  // Target position
                    true
                );
                break;

            case 6:
                // Combined: both linear and angular
                currentJoint->setAngularDriveConfig(PxD6AngularDriveConfig::eSWING_TWIST);
                currentJoint->setDrive(PxD6Drive::eX,
                    PxD6JointDrive(0.0f, 1000.0f, PX_MAX_F32, true));
                currentJoint->setDrive(PxD6Drive::eTWIST,
                    PxD6JointDrive(0.0f, 1000.0f, PX_MAX_F32, true));
                currentJoint->setDriveVelocity(
                    PxVec3(1.0f, 0.0f, 0.0f),       // Move along X
                    PxVec3(0.5f, 0.0f, 0.0f),       // Rotate around X
                    true
                );
                break;
        }
    }

    void simulate(PxReal dt) {
        if (paused) return;

        scene->simulate(dt);
        scene->fetchResults(true);
    }

    void printStatus() {
        if (!drivenActor) return;

        PxTransform pose = drivenActor->getGlobalPose();
        PxVec3 vel = drivenActor->getLinearVelocity();
        PxVec3 angVel = drivenActor->getAngularVelocity();

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Position: (" << pose.p.x << ", " << pose.p.y << ", " << pose.p.z << ")  ";
        std::cout << "Velocity: (" << vel.x << ", " << vel.y << ", " << vel.z << ")  ";
        std::cout << "AngVel: (" << angVel.x << ", " << angVel.y << ", " << angVel.z << ")";
        std::cout << (paused ? " [PAUSED]" : "") << std::endl;
    }

    void switchScenario(int scenario) {
        if (scenario >= 1 && scenario <= 6) {
            createScenario(scenario);
        }
    }

    void togglePause() {
        paused = !paused;
        std::cout << (paused ? "PAUSED" : "RESUMED") << std::endl;
    }

    void cleanup() {
        if (jointManager) {
            delete jointManager;
            jointManager = nullptr;
        }

        core.cleanup();
    }

    void run() {
        const PxReal dt = 1.0f / 60.0f;
        int frameCount = 0;

        std::cout << "\nStarting simulation..." << std::endl;
        std::cout << "Commands: 1-6 (switch scenario), Space (pause), Q (quit)" << std::endl;

        // Run for limited time per scenario
        while (frameCount < 600) {  // 10 seconds at 60fps
            simulate(dt);

            // Print status every 60 frames (1 second)
            if (frameCount % 60 == 0) {
                std::cout << "Frame " << frameCount << " - ";
                printStatus();
            }

            // Auto-switch scenarios for demonstration
            if (frameCount > 0 && frameCount % 120 == 0 && !paused) {
                int nextScenario = (currentScenario % 6) + 1;
                switchScenario(nextScenario);
            }

            frameCount++;
        }

        std::cout << "\nSimulation complete!" << std::endl;
    }
};

int main() {
    JointDriveExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
