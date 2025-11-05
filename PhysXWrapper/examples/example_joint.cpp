/**
 * @file example_joint.cpp
 * @brief Joint system example
 *
 * This example demonstrates:
 * - Spherical joints (ball-and-socket) with cone limits
 * - Fixed joints (welded)
 * - Breakable joints
 * - Revolute joints (hinges) with angle limits and motors
 * - Prismatic joints (sliders) with distance limits
 * - Distance joints (springs/ropes)
 * - D6 joints (six degree-of-freedom) with custom configurations
 * - Joint chains (rope simulation)
 *
 * Based on SnippetJoint from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include "Joint/JointManager.h"
#include <iostream>
#include <iomanip>

using namespace PhysXWrapper;
using namespace physx;

/**
 * @brief Create a box actor
 */
PxRigidDynamic* createBox(PxPhysics* physics, PxScene* scene, const PxVec3& pos,
                          const PxVec3& halfExtents, PxReal density, PxMaterial* material)
{
    PxRigidDynamic* box = physics->createRigidDynamic(PxTransform(pos));
    PxShape* shape = physics->createShape(PxBoxGeometry(halfExtents), *material);
    box->attachShape(*shape);
    shape->release();
    PxRigidBodyExt::updateMassAndInertia(*box, density);
    scene->addActor(*box);
    return box;
}

/**
 * @brief Create a sphere actor
 */
PxRigidDynamic* createSphere(PxPhysics* physics, PxScene* scene, const PxVec3& pos,
                             PxReal radius, PxReal density, PxMaterial* material)
{
    PxRigidDynamic* sphere = physics->createRigidDynamic(PxTransform(pos));
    PxShape* shape = physics->createShape(PxSphereGeometry(radius), *material);
    sphere->attachShape(*shape);
    shape->release();
    PxRigidBodyExt::updateMassAndInertia(*sphere, density);
    scene->addActor(*sphere);
    return sphere;
}

/**
 * @brief Test 1: Spherical Joint with Cone Limit
 */
void testSphericalJoint(PhysXCore& physics, PxScene* scene, JointManager& jointMgr) {
    std::cout << "\n=== TEST 1: Spherical Joint (Ball-and-Socket) ===" << std::endl;
    std::cout << "Creating pendulum with spherical joint and cone limit..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create anchor (static)
    PxRigidStatic* anchor = physics.getPhysics()->createRigidStatic(
        PxTransform(PxVec3(0, 20, 0))
    );
    scene->addActor(*anchor);

    // Create pendulum bob
    PxRigidDynamic* bob = createSphere(physics.getPhysics(), scene,
                                        PxVec3(0, 15, 0), 1.0f, 10.0f, material);

    // Create spherical joint with cone limit
    SphericalJointConfig config;
    config.enableLimit = true;
    config.yAngleLimit = PxPi / 4.0f;  // 45 degrees
    config.zAngleLimit = PxPi / 4.0f;  // 45 degrees
    config.limitStiffness = 100.0f;
    config.limitDamping = 10.0f;

    PxSphericalJoint* joint = jointMgr.createSphericalJoint(
        anchor, PxTransform(PxVec3(0, 0, 0)),
        bob, PxTransform(PxVec3(0, 0, 0)),
        config
    );

    if (joint) {
        std::cout << "  Created spherical joint with 45° cone limit" << std::endl;

        // Apply initial impulse
        bob->addForce(PxVec3(500, 0, 500), PxForceMode::eIMPULSE);
        std::cout << "  Applied impulse to pendulum" << std::endl;
    } else {
        std::cout << "  ERROR: " << jointMgr.getLastError() << std::endl;
    }
}

/**
 * @brief Test 2: Fixed Joint (Welded)
 */
void testFixedJoint(PhysXCore& physics, PxScene* scene, JointManager& jointMgr) {
    std::cout << "\n=== TEST 2: Fixed Joint (Welded) ===" << std::endl;
    std::cout << "Creating two boxes welded together..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create two boxes
    PxRigidDynamic* box1 = createBox(physics.getPhysics(), scene,
                                      PxVec3(10, 15, 0), PxVec3(1, 1, 1), 10.0f, material);
    PxRigidDynamic* box2 = createBox(physics.getPhysics(), scene,
                                      PxVec3(13, 15, 0), PxVec3(1, 1, 1), 10.0f, material);

    // Weld them together
    FixedJointConfig config;
    PxFixedJoint* joint = jointMgr.createFixedJoint(
        box1, PxTransform(PxVec3(1.5f, 0, 0)),
        box2, PxTransform(PxVec3(-1.5f, 0, 0)),
        config
    );

    if (joint) {
        std::cout << "  Created fixed joint - boxes are now welded" << std::endl;

        // Apply torque to first box
        box1->addTorque(PxVec3(0, 0, 100), PxForceMode::eIMPULSE);
        std::cout << "  Applied torque - both boxes should rotate together" << std::endl;
    } else {
        std::cout << "  ERROR: " << jointMgr.getLastError() << std::endl;
    }
}

/**
 * @brief Test 3: Breakable Fixed Joint
 */
void testBreakableJoint(PhysXCore& physics, PxScene* scene, JointManager& jointMgr) {
    std::cout << "\n=== TEST 3: Breakable Joint ===" << std::endl;
    std::cout << "Creating breakable connection..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create anchor
    PxRigidStatic* anchor = physics.getPhysics()->createRigidStatic(
        PxTransform(PxVec3(20, 20, 0))
    );
    scene->addActor(*anchor);

    // Create hanging box
    PxRigidDynamic* box = createBox(physics.getPhysics(), scene,
                                     PxVec3(20, 10, 0), PxVec3(2, 2, 2), 50.0f, material);

    // Create breakable joint
    FixedJointConfig config;
    JointBreakConfig breakConfig;
    breakConfig.enableBreak = true;
    breakConfig.breakForce = 1000.0f;   // Break at 1000N
    breakConfig.breakTorque = 100000.0f;

    PxFixedJoint* joint = jointMgr.createFixedJoint(
        anchor, PxTransform(PxVec3(0, 0, 0)),
        box, PxTransform(PxVec3(0, 0, 0)),
        config, breakConfig
    );

    if (joint) {
        std::cout << "  Created breakable joint (break force = 1000N)" << std::endl;
        std::cout << "  Heavy box should break joint and fall..." << std::endl;
    } else {
        std::cout << "  ERROR: " << jointMgr.getLastError() << std::endl;
    }

    // Set break callback
    jointMgr.setJointBreakCallback([](const JointBreakEvent& event) {
        std::cout << "  -> JOINT BROKE! Force: " << event.forceApplied
                  << "N, Torque: " << event.torqueApplied << "Nm" << std::endl;
    });
}

/**
 * @brief Test 4: Revolute Joint with Motor
 */
void testRevoluteJoint(PhysXCore& physics, PxScene* scene, JointManager& jointMgr) {
    std::cout << "\n=== TEST 4: Revolute Joint (Hinge with Motor) ===" << std::endl;
    std::cout << "Creating motorized hinge..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create anchor
    PxRigidStatic* anchor = physics.getPhysics()->createRigidStatic(
        PxTransform(PxVec3(30, 15, 0))
    );
    scene->addActor(*anchor);

    // Create door
    PxRigidDynamic* door = createBox(physics.getPhysics(), scene,
                                      PxVec3(32, 15, 0), PxVec3(2, 3, 0.2f), 10.0f, material);

    // Create revolute joint with motor
    RevoluteJointConfig config;
    config.enableLimit = true;
    config.lowerLimit = -PxPi / 2.0f;  // -90 degrees
    config.upperLimit = PxPi / 2.0f;   // +90 degrees
    config.enableDrive = true;
    config.driveVelocity = 2.0f;       // 2 rad/s
    config.driveForceLimit = 1000.0f;

    // Joint axis is Y (vertical)
    PxRevoluteJoint* joint = jointMgr.createRevoluteJoint(
        anchor, PxTransform(PxVec3(0, 0, 0)),
        door, PxTransform(PxVec3(-2, 0, 0)),
        config
    );

    if (joint) {
        std::cout << "  Created revolute joint with motor (±90° limit)" << std::endl;
        std::cout << "  Motor velocity: 2 rad/s" << std::endl;
    } else {
        std::cout << "  ERROR: " << jointMgr.getLastError() << std::endl;
    }
}

/**
 * @brief Test 5: Prismatic Joint (Slider)
 */
void testPrismaticJoint(PhysXCore& physics, PxScene* scene, JointManager& jointMgr) {
    std::cout << "\n=== TEST 5: Prismatic Joint (Slider) ===" << std::endl;
    std::cout << "Creating sliding mechanism..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create rail (static)
    PxRigidStatic* rail = physics.getPhysics()->createRigidStatic(
        PxTransform(PxVec3(40, 15, 0))
    );
    scene->addActor(*rail);

    // Create slider
    PxRigidDynamic* slider = createBox(physics.getPhysics(), scene,
                                        PxVec3(40, 15, 0), PxVec3(1, 1, 1), 10.0f, material);

    // Create prismatic joint with limits
    PrismaticJointConfig config;
    config.enableLimit = true;
    config.lowerLimit = -5.0f;   // Can slide 5 units left
    config.upperLimit = 5.0f;    // Can slide 5 units right

    // Slide along Z axis
    PxQuat rotation = PxQuat(PxPi / 2.0f, PxVec3(0, 1, 0));

    PxPrismaticJoint* joint = jointMgr.createPrismaticJoint(
        rail, PxTransform(PxVec3(0, 0, 0), rotation),
        slider, PxTransform(PxVec3(0, 0, 0), rotation),
        config
    );

    if (joint) {
        std::cout << "  Created prismatic joint (±5 unit limit)" << std::endl;

        // Apply force to make it slide
        slider->addForce(PxVec3(0, 0, 200), PxForceMode::eIMPULSE);
        std::cout << "  Applied force - slider should oscillate" << std::endl;
    } else {
        std::cout << "  ERROR: " << jointMgr.getLastError() << std::endl;
    }
}

/**
 * @brief Test 6: Distance Joint (Spring)
 */
void testDistanceJoint(PhysXCore& physics, PxScene* scene, JointManager& jointMgr) {
    std::cout << "\n=== TEST 6: Distance Joint (Spring) ===" << std::endl;
    std::cout << "Creating spring connection..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create two boxes
    PxRigidDynamic* box1 = createBox(physics.getPhysics(), scene,
                                      PxVec3(50, 20, 0), PxVec3(1, 1, 1), 10.0f, material);
    PxRigidDynamic* box2 = createBox(physics.getPhysics(), scene,
                                      PxVec3(50, 10, 0), PxVec3(1, 1, 1), 10.0f, material);

    // Lock first box (make it kinematic)
    box1->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

    // Create spring joint
    DistanceJointConfig config;
    config.minDistance = 5.0f;
    config.maxDistance = 15.0f;
    config.stiffness = 100.0f;   // Spring stiffness
    config.damping = 10.0f;      // Damping
    config.enableMinDistanceLimit = true;
    config.enableMaxDistanceLimit = true;

    PxDistanceJoint* joint = jointMgr.createDistanceJoint(
        box1, PxTransform(PxVec3(0, 0, 0)),
        box2, PxTransform(PxVec3(0, 0, 0)),
        config
    );

    if (joint) {
        std::cout << "  Created distance joint (spring: 5-15 units)" << std::endl;
        std::cout << "  Stiffness: 100, Damping: 10" << std::endl;

        // Apply force to stretch spring
        box2->addForce(PxVec3(0, -500, 0), PxForceMode::eIMPULSE);
        std::cout << "  Applied downward force - spring should oscillate" << std::endl;
    } else {
        std::cout << "  ERROR: " << jointMgr.getLastError() << std::endl;
    }
}

/**
 * @brief Test 7: D6 Joint (Configurable)
 */
void testD6Joint(PhysXCore& physics, PxScene* scene, JointManager& jointMgr) {
    std::cout << "\n=== TEST 7: D6 Joint (Six Degree-of-Freedom) ===" << std::endl;
    std::cout << "Creating configurable D6 joint..." << std::endl;

    PxMaterial* material = physics.getDefaultMaterial();

    // Create anchor
    PxRigidStatic* anchor = physics.getPhysics()->createRigidStatic(
        PxTransform(PxVec3(60, 15, 0))
    );
    scene->addActor(*anchor);

    // Create box
    PxRigidDynamic* box = createBox(physics.getPhysics(), scene,
                                     PxVec3(60, 10, 0), PxVec3(1, 1, 1), 10.0f, material);

    // Configure D6 joint: locked linear, limited swing, free twist
    D6JointConfig config;
    config.motionX = PxD6Motion::eLOCKED;
    config.motionY = PxD6Motion::eLOCKED;
    config.motionZ = PxD6Motion::eLOCKED;
    config.motionSwing1 = PxD6Motion::eLIMITED;
    config.motionSwing2 = PxD6Motion::eLIMITED;
    config.motionTwist = PxD6Motion::eFREE;

    config.enableSwingLimit = true;
    config.swingYAngle = PxPi / 6.0f;  // 30 degrees
    config.swingZAngle = PxPi / 6.0f;

    config.enableDrive = true;
    config.driveType = PxD6Drive::eSLERP;
    config.driveDamping = 1000.0f;
    config.driveStiffness = 0.0f;
    config.driveIsAcceleration = true;

    PxD6Joint* joint = jointMgr.createD6Joint(
        anchor, PxTransform(PxVec3(0, 0, 0)),
        box, PxTransform(PxVec3(0, 0, 0)),
        config
    );

    if (joint) {
        std::cout << "  Created D6 joint:" << std::endl;
        std::cout << "    - Linear: locked" << std::endl;
        std::cout << "    - Swing: limited to 30°" << std::endl;
        std::cout << "    - Twist: free rotation" << std::endl;
        std::cout << "    - Drive: damped rotation" << std::endl;

        // Apply torque
        box->addTorque(PxVec3(50, 50, 50), PxForceMode::eIMPULSE);
    } else {
        std::cout << "  ERROR: " << jointMgr.getLastError() << std::endl;
    }
}

/**
 * @brief Test 8: Joint Chain (Rope)
 */
void testJointChain(PhysXCore& physics, PxScene* scene, JointManager& jointMgr) {
    std::cout << "\n=== TEST 8: Joint Chain (Rope Simulation) ===" << std::endl;
    std::cout << "Creating rope with spherical joints..." << std::endl;

    // Create anchor
    PxRigidStatic* anchor = physics.getPhysics()->createRigidStatic(
        PxTransform(PxVec3(70, 25, 0))
    );
    scene->addActor(*anchor);

    // Create rope chain
    JointChainConfig config;
    config.type = JointChainConfig::ChainType::SPHERICAL;
    config.linkCount = 10;
    config.linkLength = 1.0f;
    config.linkRadius = 0.2f;
    config.linkMass = 1.0f;
    config.breakable = false;

    std::vector<PxRigidDynamic*> chain = jointMgr.createJointChain(
        scene, anchor, PxVec3(0, -1, 0), config, physics.getDefaultMaterial()
    );

    if (!chain.empty()) {
        std::cout << "  Created rope with " << chain.size() << " links" << std::endl;

        // Attach weight to end
        if (!chain.empty()) {
            PxRigidDynamic* weight = createSphere(physics.getPhysics(), scene,
                                                   chain.back()->getGlobalPose().p + PxVec3(0, -1, 0),
                                                   0.5f, 20.0f, physics.getDefaultMaterial());

            // Connect weight to last link
            SphericalJointConfig jointConfig;
            jointMgr.createSphericalJoint(
                chain.back(), PxTransform(PxVec3(0, -0.5f, 0)),
                weight, PxTransform(PxVec3(0, 0.5f, 0)),
                jointConfig
            );

            std::cout << "  Attached weight to rope end" << std::endl;

            // Apply swing force
            weight->addForce(PxVec3(200, 0, 0), PxForceMode::eIMPULSE);
            std::cout << "  Applied swing force" << std::endl;
        }
    } else {
        std::cout << "  ERROR: " << jointMgr.getLastError() << std::endl;
    }
}

/**
 * @brief Main simulation loop
 */
void runSimulation(PhysXCore& physics, PxScene* scene, JointManager& jointMgr) {
    std::cout << "\n=== Running Simulation ===" << std::endl;
    std::cout << "Simulating 5 seconds..." << std::endl;

    const PxReal timeStep = 1.0f / 60.0f;
    const int frameCount = 300;  // 5 seconds at 60fps

    for (int i = 0; i < frameCount; i++) {
        scene->simulate(timeStep);
        scene->fetchResults(true);

        // Print progress every second
        if (i % 60 == 0) {
            std::cout << "  Frame " << i << " / " << frameCount
                      << " (" << (i / 60.0f) << "s)" << std::endl;
        }
    }

    std::cout << "Simulation complete!" << std::endl;
    std::cout << "Total joints created: " << jointMgr.getJointCount() << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - Joint System Example ===" << std::endl;
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

    // Create scene
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

    // Initialize joint manager
    JointManager jointMgr;
    if (!jointMgr.initialize(physics.getPhysics())) {
        std::cerr << "Failed to initialize JointManager: " << jointMgr.getLastError() << std::endl;
        return 1;
    }
    std::cout << "JointManager initialized" << std::endl;

    // Run tests
    testSphericalJoint(physics, scene, jointMgr);
    testFixedJoint(physics, scene, jointMgr);
    testBreakableJoint(physics, scene, jointMgr);
    testRevoluteJoint(physics, scene, jointMgr);
    testPrismaticJoint(physics, scene, jointMgr);
    testDistanceJoint(physics, scene, jointMgr);
    testD6Joint(physics, scene, jointMgr);
    testJointChain(physics, scene, jointMgr);

    // Run simulation
    runSimulation(physics, scene, jointMgr);

    std::cout << "\n=== All Tests Complete ===" << std::endl;
    std::cout << "\nThis example demonstrated:" << std::endl;
    std::cout << "  - Spherical joints (ball-and-socket) with cone limits" << std::endl;
    std::cout << "  - Fixed joints (welded connections)" << std::endl;
    std::cout << "  - Breakable joints with force/torque thresholds" << std::endl;
    std::cout << "  - Revolute joints (hinges) with angle limits and motors" << std::endl;
    std::cout << "  - Prismatic joints (sliders) with distance limits" << std::endl;
    std::cout << "  - Distance joints (springs) with stiffness/damping" << std::endl;
    std::cout << "  - D6 joints (six DOF) with custom configurations" << std::endl;
    std::cout << "  - Joint chains for rope simulation" << std::endl;

    std::cout << "\nCleaning up..." << std::endl;
    jointMgr.cleanup();

    return 0;
}
