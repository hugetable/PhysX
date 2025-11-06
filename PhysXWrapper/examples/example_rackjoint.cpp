/**
 * @file example_rackjoint.cpp
 * @brief Rack and Pinion Joint Example
 *
 * This example demonstrates PhysX's rack and pinion joint:
 * - Converting rotational motion to linear motion
 * - Coupling revolute (hinge) and prismatic (slider) joints
 * - Maintaining geometric relationship between rotation and translation
 * - Simulating steering mechanisms, linear actuators, jack lifts
 * - Positive and negative ratios (direction control)
 *
 * Based on PhysX Snippet: SnippetRackJoint
 *
 * Rack and Pinion Mechanics:
 * - Pinion: A gear (revolute joint) that rotates
 * - Rack: A linear toothed bar (prismatic joint) that slides
 * - Relationship: distance = angle * radius
 * - Angular velocity to linear velocity: v = ω * r
 * - Force to torque conversion: F = τ / r
 *
 * Applications:
 * - Steering systems (car steering rack)
 * - Linear actuators
 * - Jack lifts and elevators
 * - CNC machine tool positioning
 * - Camera sliders
 *
 * Physics:
 * The rack joint enforces: d = θ * r
 * where d is linear displacement, θ is angular displacement, r is ratio
 * Linear velocity: v = ω * r
 * Force-torque relationship: τ = F * r
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

using namespace PhysXWrapper;

/**
 * @brief Helper class for rack and pinion systems
 */
class RackPinionSystem {
private:
    PhysXCore& core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    struct RackPinionPair {
        PxRigidDynamic* pinion;        // Rotating gear
        PxRigidDynamic* rack;          // Sliding bar
        PxRevoluteJoint* revoluteJoint;  // Pinion rotation
        PxPrismaticJoint* prismaticJoint; // Rack sliding
        PxRackAndPinionJoint* rackJoint;  // Coupling constraint
        PxReal ratio;
        std::string description;
    };

    std::vector<RackPinionPair> pairs;

public:
    RackPinionSystem(PhysXCore& coreRef, PxPhysics* phys, PxScene* scn, PxMaterial* mat)
        : core(coreRef)
        , physics(phys)
        , scene(scn)
        , material(mat)
    {}

    /**
     * Create a simple rack and pinion mechanism
     */
    void createRackPinion(const PxVec3& basePos, PxReal ratio,
                          const std::string& description) {
        std::cout << "\nCreating rack & pinion: " << description << std::endl;
        std::cout << "  Ratio: " << ratio << " (meters per radian)" << std::endl;

        // Create static base for mounting
        PxRigidStatic* base = physics->createRigidStatic(PxTransform(basePos));
        PxBoxGeometry baseGeom(0.3f, 0.3f, 0.3f);
        PxRigidActorExt::createExclusiveShape(*base, baseGeom, *material);
        scene->addActor(*base);

        // Create pinion (rotating gear) - cylinder
        PxVec3 pinionPos = basePos + PxVec3(0, -0.5f, 0);
        PxReal pinionRadius = std::abs(ratio);  // Ratio determines effective radius
        PxCylinderGeometry pinionGeom(pinionRadius, 0.2f);
        PxRigidDynamic* pinion = PxCreateDynamic(*physics, PxTransform(pinionPos),
                                                  pinionGeom, *material, 10.0f);

        // Rotate cylinder to horizontal (around Z-axis)
        PxQuat rot90(PxHalfPi, PxVec3(0, 0, 1));
        pinion->setGlobalPose(PxTransform(pinionPos, rot90));
        scene->addActor(*pinion);

        // Create revolute joint for pinion (rotates around Z-axis)
        PxVec3 revoluteAxis(0, 0, 1);
        PxTransform pinionLocalFrame(PxVec3(0, 0, 0));
        PxQuat axisRot = PxShortestRotation(PxVec3(1, 0, 0), revoluteAxis);
        pinionLocalFrame.q = axisRot;

        PxTransform baseLocalFrame = PxTransform(pinionPos - basePos);
        baseLocalFrame.q = axisRot;

        PxRevoluteJoint* revoluteJoint = PxRevoluteJointCreate(*physics,
                                                                base, baseLocalFrame,
                                                                pinion, pinionLocalFrame);

        if (!revoluteJoint) {
            std::cerr << "Failed to create revolute joint" << std::endl;
            return;
        }

        revoluteJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Create rack (sliding bar) - long box
        PxVec3 rackPos = basePos + PxVec3(0, -0.5f - pinionRadius - 0.3f, 0);
        PxBoxGeometry rackGeom(2.0f, 0.2f, 0.2f);  // Long bar
        PxRigidDynamic* rack = PxCreateDynamic(*physics, PxTransform(rackPos),
                                                rackGeom, *material, 10.0f);
        scene->addActor(*rack);

        // Create prismatic joint for rack (slides along Y-axis)
        PxVec3 prismaticAxis(1, 0, 0);  // Slide horizontally
        PxTransform rackLocalFrame(PxVec3(0, 0, 0));
        PxQuat prismaticRot = PxShortestRotation(PxVec3(1, 0, 0), prismaticAxis);
        rackLocalFrame.q = prismaticRot;

        PxTransform baseRackFrame = PxTransform(rackPos - basePos);
        baseRackFrame.q = prismaticRot;

        PxPrismaticJoint* prismaticJoint = PxPrismaticJointCreate(*physics,
                                                                   base, baseRackFrame,
                                                                   rack, rackLocalFrame);

        if (!prismaticJoint) {
            std::cerr << "Failed to create prismatic joint" << std::endl;
            return;
        }

        prismaticJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Optional: Set limits on rack travel
        prismaticJoint->setLimit(PxJointLinearLimitPair(-5.0f, 5.0f, 0.1f));
        prismaticJoint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, true);

        // Create rack and pinion joint (couples the two joints)
        PxRackAndPinionJoint* rackJoint = PxRackAndPinionJointCreate(*physics,
                                                                       revoluteJoint, PxTransform(PxIdentity),
                                                                       prismaticJoint, PxTransform(PxIdentity));

        if (!rackJoint) {
            std::cerr << "Failed to create rack and pinion joint" << std::endl;
            return;
        }

        // Set the ratio (distance per angle)
        rackJoint->setRatio(ratio);

        // Enable visualization
        rackJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        std::cout << "  Created successfully" << std::endl;

        // Apply initial torque to pinion to start motion
        pinion->addTorque(PxVec3(0, 0, 30.0f));

        // Store the pair
        RackPinionPair pair;
        pair.pinion = pinion;
        pair.rack = rack;
        pair.revoluteJoint = revoluteJoint;
        pair.prismaticJoint = prismaticJoint;
        pair.rackJoint = rackJoint;
        pair.ratio = ratio;
        pair.description = description;
        pairs.push_back(pair);
    }

    /**
     * Create a steering system simulation
     */
    void createSteeringSystem(const PxVec3& basePos) {
        std::cout << "\nCreating steering system..." << std::endl;

        // Create chassis (static)
        PxRigidStatic* chassis = physics->createRigidStatic(PxTransform(basePos));
        PxBoxGeometry chassisGeom(1.0f, 0.2f, 2.0f);
        PxRigidActorExt::createExclusiveShape(*chassis, chassisGeom, *material);
        scene->addActor(*chassis);

        // Create steering wheel (pinion)
        PxVec3 wheelPos = basePos + PxVec3(0, 1.0f, 0);
        PxCylinderGeometry wheelGeom(0.3f, 0.1f);
        PxRigidDynamic* wheel = PxCreateDynamic(*physics, PxTransform(wheelPos),
                                                 wheelGeom, *material, 5.0f);
        PxQuat rot90(PxHalfPi, PxVec3(1, 0, 0));
        wheel->setGlobalPose(PxTransform(wheelPos, rot90));
        scene->addActor(*wheel);

        // Revolute joint for steering wheel (rotates around Y-axis)
        PxTransform wheelFrame0(wheelPos - basePos, PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxTransform wheelFrame1(PxVec3(0, 0, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxRevoluteJoint* wheelJoint = PxRevoluteJointCreate(*physics, chassis, wheelFrame0,
                                                             wheel, wheelFrame1);
        wheelJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Set steering wheel limits (±2 turns = ±720°)
        PxReal maxAngle = 4.0f * PxPi;  // 720 degrees
        wheelJoint->setLimit(PxJointAngularLimitPair(-maxAngle, maxAngle, 0.1f));
        wheelJoint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);

        // Create steering rack (horizontal slider)
        PxVec3 rackPos = basePos + PxVec3(0, 0.5f, 0);
        PxBoxGeometry rackGeom(1.5f, 0.1f, 0.1f);
        PxRigidDynamic* rack = PxCreateDynamic(*physics, PxTransform(rackPos),
                                                rackGeom, *material, 3.0f);
        scene->addActor(*rack);

        // Prismatic joint for rack (slides along X-axis)
        PxTransform rackFrame0(rackPos - basePos);
        PxTransform rackFrame1(PxVec3(0, 0, 0));
        PxPrismaticJoint* rackJoint = PxPrismaticJointCreate(*physics, chassis, rackFrame0,
                                                              rack, rackFrame1);
        rackJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Set rack travel limits
        rackJoint->setLimit(PxJointLinearLimitPair(-1.0f, 1.0f, 0.1f));
        rackJoint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, true);

        // Connect wheel and rack with rack-and-pinion joint
        // Typical steering ratio: 15:1 (15 degrees of wheel = 1 inch of rack)
        // Convert to radians/meter: 15° = 0.262 rad, ratio ≈ 0.025 m/rad
        PxReal steeringRatio = 0.05f;  // 0.05 meters per radian

        PxRackAndPinionJoint* rpJoint = PxRackAndPinionJointCreate(*physics,
                                                                     wheelJoint, PxTransform(PxIdentity),
                                                                     rackJoint, PxTransform(PxIdentity));
        rpJoint->setRatio(steeringRatio);
        rpJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Apply torque to simulate steering input
        wheel->addTorque(PxVec3(0, 50.0f, 0));

        std::cout << "  Steering system created:" << std::endl;
        std::cout << "    Steering wheel: ±720° limit" << std::endl;
        std::cout << "    Rack travel: ±1.0m" << std::endl;
        std::cout << "    Ratio: " << steeringRatio << " m/rad" << std::endl;

        // Store
        RackPinionPair pair;
        pair.pinion = wheel;
        pair.rack = rack;
        pair.revoluteJoint = wheelJoint;
        pair.prismaticJoint = rackJoint;
        pair.rackJoint = rpJoint;
        pair.ratio = steeringRatio;
        pair.description = "Steering system";
        pairs.push_back(pair);
    }

    /**
     * Print status of all rack and pinion pairs
     */
    void printStatus() {
        std::cout << "\n=== Rack & Pinion Status ===" << std::endl;
        std::cout << std::fixed << std::setprecision(4);

        for (size_t i = 0; i < pairs.size(); i++) {
            const RackPinionPair& pair = pairs[i];

            // Get rotational angle and velocity
            PxReal angle = pair.revoluteJoint->getAngle();
            PxVec3 angVel = pair.pinion->getAngularVelocity();

            // Get linear position and velocity
            PxReal position = pair.prismaticJoint->getPosition();
            PxVec3 linVel = pair.rack->getLinearVelocity();

            // Compute expected position from angle
            PxReal expectedPosition = angle * pair.ratio;

            // Compute error
            PxReal error = std::abs(position - expectedPosition);

            std::cout << "\n" << pair.description << ":" << std::endl;
            std::cout << "  Ratio: " << pair.ratio << " m/rad" << std::endl;
            std::cout << "  Pinion angle: " << (angle * 180.0f / PxPi) << "° ("
                      << angle << " rad)" << std::endl;
            std::cout << "  Pinion angular velocity: " << angVel.magnitude() << " rad/s" << std::endl;
            std::cout << "  Rack position: " << position << " m" << std::endl;
            std::cout << "  Rack velocity: " << linVel.magnitude() << " m/s" << std::endl;
            std::cout << "  Expected position: " << expectedPosition << " m" << std::endl;
            std::cout << "  Position error: " << error << " m" << std::endl;

            if (error < 0.01f) {
                std::cout << "  ✓ Constraint satisfied" << std::endl;
            } else {
                std::cout << "  ⚠️  WARNING: Large constraint error" << std::endl;
            }

            // Velocity relationship check
            PxReal expectedLinVel = angVel.magnitude() * pair.ratio;
            PxReal velError = std::abs(linVel.magnitude() - expectedLinVel);
            std::cout << "  Expected velocity: " << expectedLinVel << " m/s  ";
            std::cout << "  Velocity error: " << velError << " m/s" << std::endl;
        }
    }

    size_t getPairCount() const { return pairs.size(); }
};

/**
 * @brief Main example application
 */
class RackJointExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;
    RackPinionSystem* system;

public:
    RackJointExample()
        : physics(nullptr)
        , scene(nullptr)
        , material(nullptr)
        , system(nullptr)
    {}

    ~RackJointExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "===================================================" << std::endl;
        std::cout << "PhysX Rack and Pinion Joint Example" << std::endl;
        std::cout << "===================================================" << std::endl;

        PhysXCore::Config config;
        config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        config.numThreads = 2;

        if (!core.initialize(config)) {
            std::cerr << "Failed to initialize PhysX" << std::endl;
            return false;
        }

        physics = core.getPhysics();
        scene = core.getScene();
        material = physics->createMaterial(0.5f, 0.5f, 0.1f);

        // Enable visualization
        scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);

        system = new RackPinionSystem(core, physics, scene, material);

        std::cout << "\nRack and Pinion Mechanics:" << std::endl;
        std::cout << "  • Converts rotation to linear motion" << std::endl;
        std::cout << "  • Relationship: distance = angle × ratio" << std::endl;
        std::cout << "  • Velocity: v = ω × ratio" << std::endl;
        std::cout << "  • Force-torque: τ = F × ratio" << std::endl;
        std::cout << "  • Used in: steering, actuators, lifts" << std::endl;

        return true;
    }

    void createScenarios() {
        std::cout << "\n=== Creating Rack & Pinion Scenarios ===" << std::endl;

        // Scenario 1: Small ratio (slow linear motion)
        system->createRackPinion(PxVec3(-8, 5, 0), 0.1f,
                                 "Small ratio (0.1 m/rad)");

        // Scenario 2: Medium ratio
        system->createRackPinion(PxVec3(-4, 5, 0), 0.3f,
                                 "Medium ratio (0.3 m/rad)");

        // Scenario 3: Large ratio (fast linear motion)
        system->createRackPinion(PxVec3(0, 5, 0), 0.5f,
                                 "Large ratio (0.5 m/rad)");

        // Scenario 4: Realistic steering system
        system->createSteeringSystem(PxVec3(5, 5, 0));

        std::cout << "\nTotal mechanisms created: " << system->getPairCount() << std::endl;
    }

    void simulate(PxReal dt) {
        scene->simulate(dt);
        scene->fetchResults(true);
    }

    void run() {
        std::cout << "\n=== Setting Up Scenarios ===" << std::endl;
        createScenarios();

        std::cout << "\n=== Starting Simulation ===" << std::endl;

        const PxReal dt = 1.0f / 60.0f;
        const int totalFrames = 600;  // 10 seconds

        for (int frame = 0; frame < totalFrames; frame++) {
            simulate(dt);

            // Print status every 2 seconds
            if (frame % 120 == 0) {
                std::cout << "\n=== Frame " << frame << " (t=" << (frame * dt) << "s) ===";
                system->printStatus();
            }
        }

        std::cout << "\n\n=== Final Status ===";
        system->printStatus();

        std::cout << "\n\n=== Simulation Complete ===" << std::endl;
        std::cout << "\nKey Observations:" << std::endl;
        std::cout << "  • Linear position = angular position × ratio" << std::endl;
        std::cout << "  • Linear velocity = angular velocity × ratio" << std::endl;
        std::cout << "  • Larger ratio = faster linear motion" << std::endl;
        std::cout << "  • Constraint maintained throughout simulation" << std::endl;
        std::cout << "  • Energy conserved (torque-force relationship)" << std::endl;
    }

    void cleanup() {
        if (system) delete system;
        if (material) material->release();
        core.cleanup();
    }
};

int main() {
    RackJointExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
