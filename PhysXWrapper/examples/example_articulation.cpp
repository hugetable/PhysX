/**
 * @file example_articulation.cpp
 * @brief Articulation system example
 *
 * This example demonstrates:
 * - Creating reduced coordinate articulations
 * - Building robot arms with revolute joints
 * - Creating flexible chains (rope/snake-like)
 * - Joint drives and control
 * - Joint limits and configuration
 * - Querying joint states
 *
 * Articulations provide more stable simulation than regular joint chains
 * for multi-body systems like robots and characters.
 *
 * Based on SnippetArticulationRC from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include "Articulation/ArticulationManager.h"
#include <iostream>
#include <iomanip>

using namespace PhysXWrapper;
using namespace physx;

/**
 * @brief Test 1: Simple Robot Arm
 */
void testRobotArm(PhysXCore& physics, PxScene* scene, ArticulationManager& artMgr) {
    std::cout << "\n=== TEST 1: Robot Arm ===" << std::endl;
    std::cout << "Creating 6-joint robot arm..." << std::endl;

    // Configure robot arm
    RobotArmConfig config;
    config.numJoints = 6;
    config.linkLength = 1.0f;
    config.linkRadius = 0.15f;
    config.linkMass = 2.0f;
    config.basePosition = PxVec3(0, 1, 0);
    config.armDirection = PxVec3(0, 1, 0);  // Arm points up
    config.enableJointLimits = true;
    config.jointLimitLow = -PxPi / 3.0f;   // -60 degrees
    config.jointLimitHigh = PxPi / 3.0f;   // +60 degrees

    PxArticulationReducedCoordinate* robotArm = artMgr.createRobotArm(
        scene, config, physics.getDefaultMaterial()
    );

    if (robotArm) {
        std::cout << "  Created robot arm with " << artMgr.getLinkCount(robotArm)
                  << " links" << std::endl;

        // Get all links
        std::vector<PxArticulationLink*> links = artMgr.getLinks(robotArm);

        // Set drive targets for joints (create reaching motion)
        for (size_t i = 1; i < links.size(); i++) {  // Skip base
            PxReal target = (i % 2 == 0) ? PxPi / 6.0f : -PxPi / 6.0f;
            artMgr.setDriveTarget(links[i], PxArticulationAxis::eTWIST, target);

            std::cout << "  Joint " << i << " drive target set to "
                      << (target * 180.0f / PxPi) << " degrees" << std::endl;
        }

        std::cout << "  Robot arm configured successfully" << std::endl;
    } else {
        std::cout << "  ERROR: " << artMgr.getLastError() << std::endl;
    }
}

/**
 * @brief Test 2: Flexible Chain
 */
void testFlexibleChain(PhysXCore& physics, PxScene* scene, ArticulationManager& artMgr) {
    std::cout << "\n=== TEST 2: Flexible Chain (Rope/Snake) ===" << std::endl;
    std::cout << "Creating articulated chain..." << std::endl;

    PxVec3 chainStart(10, 10, 0);
    PxVec3 chainDirection(0, -1, 0);  // Hanging down

    PxArticulationReducedCoordinate* chain = artMgr.createChain(
        scene,
        chainStart,
        chainDirection,
        10,                    // 10 links
        1.0f,                  // 1m per link
        0.1f,                  // 0.1m radius
        physics.getDefaultMaterial()
    );

    if (chain) {
        std::cout << "  Created chain with " << artMgr.getLinkCount(chain)
                  << " links" << std::endl;

        // Apply force to last link (make it swing)
        std::vector<PxArticulationLink*> links = artMgr.getLinks(chain);
        if (!links.empty()) {
            PxArticulationLink* lastLink = links.back();
            lastLink->addForce(PxVec3(500, 0, 0), PxForceMode::eIMPULSE);
            std::cout << "  Applied impulse to last link - chain should swing" << std::endl;
        }

        std::cout << "  Chain configured successfully" << std::endl;
    } else {
        std::cout << "  ERROR: " << artMgr.getLastError() << std::endl;
    }
}

/**
 * @brief Test 3: Custom Articulation with Different Joint Types
 */
void testCustomArticulation(PhysXCore& physics, PxScene* scene, ArticulationManager& artMgr) {
    std::cout << "\n=== TEST 3: Custom Articulation ===" << std::endl;
    std::cout << "Creating custom articulation with mixed joint types..." << std::endl;

    // Create articulation
    ArticulationConfig artConfig;
    artConfig.solverIterationCounts = 32;
    artConfig.fixBase = true;

    PxArticulationReducedCoordinate* articulation = artMgr.createArticulation(scene, artConfig);
    if (!articulation) {
        std::cout << "  ERROR: Failed to create articulation" << std::endl;
        return;
    }

    // Create base link (fixed)
    ArticulationLinkConfig baseLinkConfig;
    PxBoxGeometry baseGeom(0.5f, 0.5f, 0.5f);
    baseLinkConfig.geometry = &baseGeom;
    baseLinkConfig.globalPose = PxTransform(PxVec3(20, 5, 0));
    baseLinkConfig.density = 10.0f;

    PxArticulationLink* base = artMgr.createLink(
        articulation, nullptr, baseLinkConfig,
        ArticulationJointConfig(), physics.getDefaultMaterial()
    );

    if (!base) {
        std::cout << "  ERROR: Failed to create base link" << std::endl;
        return;
    }

    std::cout << "  Created base link" << std::endl;

    // Create link 1 with REVOLUTE joint (hinge)
    ArticulationLinkConfig link1Config;
    PxCapsuleGeometry capsule1(0.1f, 0.5f);
    link1Config.geometry = &capsule1;
    link1Config.globalPose = PxTransform(PxVec3(20, 6, 0));
    link1Config.density = 5.0f;

    ArticulationJointConfig joint1Config;
    joint1Config.type = PxArticulationJointType::eREVOLUTE;
    joint1Config.motionTwist = PxArticulationMotion::eLIMITED;
    joint1Config.enableLimits = true;
    joint1Config.limitLow = -PxPi / 2.0f;
    joint1Config.limitHigh = PxPi / 2.0f;
    joint1Config.enableDrive = true;
    joint1Config.driveStiffness = 1000.0f;
    joint1Config.driveDamping = 100.0f;
    joint1Config.driveTarget = PxPi / 4.0f;  // 45 degrees
    joint1Config.parentPose = PxTransform(PxVec3(0, 0.5f, 0));
    joint1Config.childPose = PxTransform(PxVec3(0, -0.5f, 0));

    PxArticulationLink* link1 = artMgr.createLink(
        articulation, base, link1Config, joint1Config,
        physics.getDefaultMaterial()
    );

    if (link1) {
        std::cout << "  Created link 1 with REVOLUTE joint" << std::endl;
    }

    // Create link 2 with PRISMATIC joint (slider)
    ArticulationLinkConfig link2Config;
    PxBoxGeometry box2(0.2f, 0.2f, 0.2f);
    link2Config.geometry = &box2;
    link2Config.globalPose = PxTransform(PxVec3(20, 7, 0));
    link2Config.density = 3.0f;

    ArticulationJointConfig joint2Config;
    joint2Config.type = PxArticulationJointType::ePRISMATIC;
    joint2Config.motionZ = PxArticulationMotion::eLIMITED;
    joint2Config.enableLimits = true;
    joint2Config.limitLow = -1.0f;
    joint2Config.limitHigh = 1.0f;
    joint2Config.enableDrive = true;
    joint2Config.driveStiffness = 5000.0f;
    joint2Config.driveDamping = 500.0f;
    joint2Config.driveTarget = 0.5f;  // Extend 0.5m
    joint2Config.parentPose = PxTransform(PxVec3(0, 0.5f, 0));
    joint2Config.childPose = PxTransform(PxVec3(0, -0.2f, 0));

    PxArticulationLink* link2 = artMgr.createLink(
        articulation, link1, link2Config, joint2Config,
        physics.getDefaultMaterial()
    );

    if (link2) {
        std::cout << "  Created link 2 with PRISMATIC joint" << std::endl;
    }

    // Create link 3 with SPHERICAL joint (ball-and-socket)
    ArticulationLinkConfig link3Config;
    PxSphereGeometry sphere3(0.3f);
    link3Config.geometry = &sphere3;
    link3Config.globalPose = PxTransform(PxVec3(20, 8, 0));
    link3Config.density = 2.0f;

    ArticulationJointConfig joint3Config;
    joint3Config.type = PxArticulationJointType::eSPHERICAL;
    joint3Config.motionSwing1 = PxArticulationMotion::eLIMITED;
    joint3Config.motionSwing2 = PxArticulationMotion::eLIMITED;
    joint3Config.motionTwist = PxArticulationMotion::eFREE;
    joint3Config.enableLimits = true;
    joint3Config.limitLow = -PxPi / 4.0f;
    joint3Config.limitHigh = PxPi / 4.0f;
    joint3Config.parentPose = PxTransform(PxVec3(0, 0.2f, 0));
    joint3Config.childPose = PxTransform(PxVec3(0, -0.3f, 0));

    PxArticulationLink* link3 = artMgr.createLink(
        articulation, link2, link3Config, joint3Config,
        physics.getDefaultMaterial()
    );

    if (link3) {
        std::cout << "  Created link 3 with SPHERICAL joint" << std::endl;
        std::cout << "  Custom articulation complete with 4 links" << std::endl;
    }
}

/**
 * @brief Test 4: Joint State Monitoring
 */
void testJointStateMonitoring(ArticulationManager& artMgr) {
    std::cout << "\n=== TEST 4: Joint State Monitoring ===" << std::endl;
    std::cout << "Monitoring articulation states..." << std::endl;

    std::vector<PxArticulationReducedCoordinate*> articulations = artMgr.getArticulations();

    std::cout << "  Total articulations: " << articulations.size() << std::endl;

    for (size_t i = 0; i < articulations.size(); i++) {
        PxArticulationReducedCoordinate* art = articulations[i];
        std::cout << "\n  Articulation " << i << ":" << std::endl;
        std::cout << "    Links: " << artMgr.getLinkCount(art) << std::endl;
        std::cout << "    Sleeping: " << (artMgr.isSleeping(art) ? "YES" : "NO") << std::endl;

        std::vector<PxArticulationLink*> links = artMgr.getLinks(art);

        // Show state of first few joints
        for (size_t j = 1; j < std::min(links.size(), size_t(4)); j++) {
            PxArticulationLink* link = links[j];
            PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();

            if (joint) {
                PxReal position = artMgr.getJointPosition(link, PxArticulationAxis::eTWIST);
                PxReal velocity = artMgr.getJointVelocity(link, PxArticulationAxis::eTWIST);

                std::cout << "    Joint " << j << ": "
                          << "pos=" << std::fixed << std::setprecision(3) << position
                          << " rad (" << (position * 180.0f / PxPi) << "°), "
                          << "vel=" << velocity << " rad/s" << std::endl;
            }
        }
    }
}

/**
 * @brief Main simulation loop
 */
void runSimulation(PhysXCore& physics, PxScene* scene, ArticulationManager& artMgr) {
    std::cout << "\n=== Running Simulation ===" << std::endl;

    const PxReal timeStep = 1.0f / 60.0f;
    const int frameCount = 300;  // 5 seconds at 60fps

    for (int i = 0; i < frameCount; i++) {
        scene->simulate(timeStep);
        scene->fetchResults(true);

        // Monitor joint states every second
        if (i % 60 == 0) {
            std::cout << "\n--- Frame " << i << " (" << (i / 60.0f) << "s) ---" << std::endl;
            testJointStateMonitoring(artMgr);
        }

        // Animate drive targets at 2 seconds
        if (i == 120) {
            std::cout << "\n>>> Changing drive targets <<<" << std::endl;

            std::vector<PxArticulationReducedCoordinate*> articulations = artMgr.getArticulations();
            if (!articulations.empty()) {
                std::vector<PxArticulationLink*> links = artMgr.getLinks(articulations[0]);

                // Reverse drive targets
                for (size_t j = 1; j < links.size(); j++) {
                    PxReal currentTarget = links[j]->getInboundJoint()->getDriveTarget(PxArticulationAxis::eTWIST);
                    PxReal newTarget = -currentTarget;
                    artMgr.setDriveTarget(links[j], PxArticulationAxis::eTWIST, newTarget);
                }

                std::cout << "  Drive targets reversed" << std::endl;
            }
        }
    }

    std::cout << "\nSimulation complete!" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - Articulation System Example ===" << std::endl;
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

    // Initialize PhysX
    PhysXCore physics;
    std::cout << "Initializing PhysX..." << std::endl;
    if (!physics.initialize(config)) {
        std::cerr << "Failed to initialize PhysX: " << physics.getLastError() << std::endl;
        return 1;
    }
    std::cout << "PhysX initialized successfully!" << std::endl;

    // Get scene
    PxScene* scene = physics.getScene();
    if (!scene) {
        std::cerr << "Failed to get scene" << std::endl;
        return 1;
    }

    // Create ground plane
    PxRigidStatic* ground = PxCreatePlane(
        *physics.getPhysics(),
        PxPlane(0, 1, 0, 0),
        *physics.getDefaultMaterial()
    );
    scene->addActor(*ground);
    std::cout << "Created ground plane" << std::endl;

    // Initialize articulation manager
    ArticulationManager artMgr;
    if (!artMgr.initialize(physics.getPhysics())) {
        std::cerr << "Failed to initialize ArticulationManager: " << artMgr.getLastError() << std::endl;
        return 1;
    }
    std::cout << "ArticulationManager initialized" << std::endl;

    // Run tests
    testRobotArm(physics, scene, artMgr);
    testFlexibleChain(physics, scene, artMgr);
    testCustomArticulation(physics, scene, artMgr);

    // Run simulation
    runSimulation(physics, scene, artMgr);

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "\nThis example demonstrated:" << std::endl;
    std::cout << "  - Creating robot arms with revolute joints and drives" << std::endl;
    std::cout << "  - Building flexible chains (rope/snake simulation)" << std::endl;
    std::cout << "  - Custom articulations with mixed joint types" << std::endl;
    std::cout << "  - Joint limits and motion configuration" << std::endl;
    std::cout << "  - Drive control and target animation" << std::endl;
    std::cout << "  - Querying joint positions and velocities" << std::endl;
    std::cout << "  - Articulation state management" << std::endl;

    std::cout << "\nCleaning up..." << std::endl;
    artMgr.cleanup();

    return 0;
}
