/**
 * @file example_gearjoint.cpp
 * @brief Gear Joint Constraint Example
 *
 * This example demonstrates PhysX's gear joint functionality:
 * - Coupling two revolute joints with a gear ratio
 * - Maintaining angular relationship between joints
 * - Simulating mechanical gear systems
 * - Gear trains and compound gears
 * - Positive and negative gear ratios (same/opposite direction)
 * - Error correction and constraint stability
 *
 * Based on PhysX Snippet: SnippetGearJoint
 *
 * Gear Joint Mechanics:
 * - Links two revolute (hinge) joints
 * - Maintains relationship: angle1 * gearRatio + angle2 = constant
 * - Positive ratio: gears rotate in same direction
 * - Negative ratio: gears rotate in opposite directions
 * - Gear ratio magnitude determines speed ratio
 *
 * Applications:
 * - Mechanical transmissions
 * - Clock mechanisms
 * - Robotic joint coupling
 * - Pulley systems
 * - Differential drives
 *
 * Physics:
 * The gear constraint enforces: θ₁ * r = θ₂
 * where r is the gear ratio (radius1/radius2 for physical gears)
 * Angular velocities: ω₁ * r = ω₂
 * Torques: τ₁ = τ₂ * r (energy conservation)
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

using namespace PhysXWrapper;

/**
 * @brief Helper class for creating gear joint systems
 */
class GearJointSystem {
private:
    PhysXCore& core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    struct GearPair {
        PxRigidDynamic* actor1;
        PxRigidDynamic* actor2;
        PxRevoluteJoint* hinge1;
        PxRevoluteJoint* hinge2;
        PxGearJoint* gearJoint;
        PxReal gearRatio;
        std::string description;
    };

    std::vector<GearPair> gearPairs;

public:
    GearJointSystem(PhysXCore& coreRef, PxPhysics* phys, PxScene* scn, PxMaterial* mat)
        : core(coreRef)
        , physics(phys)
        , scene(scn)
        , material(mat)
    {}

    /**
     * Create a revolute joint (hinge) attached to a base
     */
    PxRevoluteJoint* createHinge(PxRigidActor* base, const PxVec3& basePos,
                                  PxRigidDynamic* wheel, const PxVec3& wheelPos,
                                  const PxVec3& axis) {
        // Local frames for the joint
        PxVec3 localPosBase = basePos;
        PxVec3 localPosWheel = wheelPos;

        PxTransform localFrame0(localPosBase);
        PxTransform localFrame1(localPosWheel);

        // Set the axis of rotation
        // PhysX revolute joint rotates around X-axis by default
        // We need to create a rotation that aligns X with our desired axis
        PxQuat axisRotation = PxShortestRotation(PxVec3(1, 0, 0), axis.getNormalized());
        localFrame0.q = axisRotation;
        localFrame1.q = axisRotation;

        PxRevoluteJoint* joint = PxRevoluteJointCreate(*physics, base, localFrame0,
                                                       wheel, localFrame1);

        if (joint) {
            // Enable visualization
            joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

            // Optional: Set drive for testing
            // joint->setDriveVelocity(1.0f);
            // joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
        }

        return joint;
    }

    /**
     * Create a simple gear pair: two wheels connected by a gear joint
     */
    void createSimpleGearPair(const PxVec3& basePos, PxReal gearRatio,
                              const std::string& description) {
        // Create base (static anchor)
        PxRigidStatic* base = physics->createRigidStatic(PxTransform(basePos));
        PxBoxGeometry baseGeom(0.2f, 0.2f, 0.2f);
        PxRigidActorExt::createExclusiveShape(*base, baseGeom, *material);
        scene->addActor(*base);

        // Create first wheel (gear 1)
        PxVec3 wheel1Pos = basePos + PxVec3(0, -1.5f, 0);
        PxCylinderGeometry wheel1Geom(0.5f, 0.2f);  // radius 0.5m
        PxRigidDynamic* wheel1 = PxCreateDynamic(*physics, PxTransform(wheel1Pos),
                                                  wheel1Geom, *material, 10.0f);

        // Rotate cylinder to align with Z-axis (cylinder is Y-axis by default)
        PxQuat rot90(PxHalfPi, PxVec3(0, 0, 1));
        wheel1->setGlobalPose(PxTransform(wheel1Pos, rot90));

        scene->addActor(*wheel1);

        // Create second wheel (gear 2)
        PxReal wheel2Radius = 0.5f / std::abs(gearRatio);  // Scale by gear ratio
        PxVec3 wheel2Pos = basePos + PxVec3(0, -1.5f - 0.5f - wheel2Radius - 0.1f, 0);
        PxCylinderGeometry wheel2Geom(wheel2Radius, 0.2f);
        PxRigidDynamic* wheel2 = PxCreateDynamic(*physics, PxTransform(wheel2Pos),
                                                  wheel2Geom, *material, 10.0f);
        wheel2->setGlobalPose(PxTransform(wheel2Pos, rot90));
        scene->addActor(*wheel2);

        // Create revolute joints (hinges)
        PxVec3 hingeAxis(0, 0, 1);  // Rotate around Z-axis
        PxRevoluteJoint* hinge1 = createHinge(base, PxVec3(0, -1.5f, 0),
                                               wheel1, PxVec3(0, 0, 0), hingeAxis);
        PxRevoluteJoint* hinge2 = createHinge(base, wheel2Pos - basePos,
                                               wheel2, PxVec3(0, 0, 0), hingeAxis);

        if (!hinge1 || !hinge2) {
            std::cerr << "Failed to create hinges" << std::endl;
            return;
        }

        // Create gear joint connecting the two hinges
        PxGearJoint* gearJoint = PxGearJointCreate(*physics, hinge1, PxTransform(PxIdentity),
                                                    hinge2, PxTransform(PxIdentity));

        if (gearJoint) {
            // Set gear ratio
            gearJoint->setGearRatio(gearRatio);

            // Enable visualization
            gearJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

            std::cout << "Created gear pair: ratio=" << gearRatio
                      << " (" << description << ")" << std::endl;
        } else {
            std::cerr << "Failed to create gear joint" << std::endl;
            return;
        }

        // Apply initial torque to first wheel to start rotation
        wheel1->addTorque(PxVec3(0, 0, 50.0f));

        // Store the gear pair
        GearPair pair;
        pair.actor1 = wheel1;
        pair.actor2 = wheel2;
        pair.hinge1 = hinge1;
        pair.hinge2 = hinge2;
        pair.gearJoint = gearJoint;
        pair.gearRatio = gearRatio;
        pair.description = description;
        gearPairs.push_back(pair);
    }

    /**
     * Create a gear train (multiple gears in series)
     */
    void createGearTrain(const PxVec3& startPos) {
        std::cout << "\nCreating gear train (3 gears in series)..." << std::endl;

        // Create base
        PxRigidStatic* base = physics->createRigidStatic(PxTransform(startPos));
        scene->addActor(*base);

        PxQuat rot90(PxHalfPi, PxVec3(0, 0, 1));
        PxVec3 hingeAxis(0, 0, 1);

        // Gear 1 (driver)
        PxVec3 gear1Pos = startPos + PxVec3(0, 0, 0);
        PxCylinderGeometry gear1Geom(0.5f, 0.15f);
        PxRigidDynamic* gear1 = PxCreateDynamic(*physics, PxTransform(gear1Pos, rot90),
                                                 gear1Geom, *material, 10.0f);
        scene->addActor(*gear1);
        PxRevoluteJoint* hinge1 = createHinge(base, gear1Pos - startPos,
                                               gear1, PxVec3(0, 0, 0), hingeAxis);

        // Gear 2 (intermediate) - ratio 2:1 (half speed)
        PxVec3 gear2Pos = startPos + PxVec3(1.2f, 0, 0);
        PxCylinderGeometry gear2Geom(1.0f, 0.15f);  // Twice the radius
        PxRigidDynamic* gear2 = PxCreateDynamic(*physics, PxTransform(gear2Pos, rot90),
                                                 gear2Geom, *material, 10.0f);
        scene->addActor(*gear2);
        PxRevoluteJoint* hinge2 = createHinge(base, gear2Pos - startPos,
                                               gear2, PxVec3(0, 0, 0), hingeAxis);

        // Gear 3 (output) - ratio 3:1 overall (1/3 speed of gear 1)
        PxVec3 gear3Pos = startPos + PxVec3(3.7f, 0, 0);
        PxCylinderGeometry gear3Geom(1.5f, 0.15f);
        PxRigidDynamic* gear3 = PxCreateDynamic(*physics, PxTransform(gear3Pos, rot90),
                                                 gear3Geom, *material, 10.0f);
        scene->addActor(*gear3);
        PxRevoluteJoint* hinge3 = createHinge(base, gear3Pos - startPos,
                                               gear3, PxVec3(0, 0, 0), hingeAxis);

        // Gear joint 1-2 (ratio -2, negative for opposite rotation)
        PxGearJoint* gearJoint12 = PxGearJointCreate(*physics, hinge1, PxTransform(PxIdentity),
                                                      hinge2, PxTransform(PxIdentity));
        gearJoint12->setGearRatio(-2.0f);
        gearJoint12->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Gear joint 2-3 (ratio -1.5)
        PxGearJoint* gearJoint23 = PxGearJointCreate(*physics, hinge2, PxTransform(PxIdentity),
                                                      hinge3, PxTransform(PxIdentity));
        gearJoint23->setGearRatio(-1.5f);
        gearJoint23->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Apply torque to first gear
        gear1->addTorque(PxVec3(0, 0, 100.0f));

        std::cout << "Gear train created:" << std::endl;
        std::cout << "  Gear 1 (driver): radius=0.5m" << std::endl;
        std::cout << "  Gear 2: radius=1.0m, ratio=-2:1 (half speed, opposite)" << std::endl;
        std::cout << "  Gear 3 (output): radius=1.5m, ratio=-1.5:1" << std::endl;
        std::cout << "  Overall ratio: 1:3 (output rotates 1/3 speed of input)" << std::endl;

        // Store references
        GearPair pair12, pair23;
        pair12.actor1 = gear1;
        pair12.actor2 = gear2;
        pair12.hinge1 = hinge1;
        pair12.hinge2 = hinge2;
        pair12.gearJoint = gearJoint12;
        pair12.gearRatio = -2.0f;
        pair12.description = "Gear 1-2 (train)";

        pair23.actor1 = gear2;
        pair23.actor2 = gear3;
        pair23.hinge1 = hinge2;
        pair23.hinge2 = hinge3;
        pair23.gearJoint = gearJoint23;
        pair23.gearRatio = -1.5f;
        pair23.description = "Gear 2-3 (train)";

        gearPairs.push_back(pair12);
        gearPairs.push_back(pair23);
    }

    /**
     * Print status of all gear pairs
     */
    void printStatus() {
        std::cout << "\n=== Gear System Status ===" << std::endl;
        std::cout << std::fixed << std::setprecision(3);

        for (size_t i = 0; i < gearPairs.size(); i++) {
            const GearPair& pair = gearPairs[i];

            // Get angular velocities
            PxVec3 angVel1 = pair.actor1->getAngularVelocity();
            PxVec3 angVel2 = pair.actor2->getAngularVelocity();

            // Get angles (from revolute joints)
            PxReal angle1 = pair.hinge1->getAngle();
            PxReal angle2 = pair.hinge2->getAngle();

            // Angular velocity magnitudes (around Z-axis)
            PxReal omega1 = angVel1.z;
            PxReal omega2 = angVel2.z;

            // Compute actual ratio
            PxReal actualRatio = (omega2 != 0.0f) ? (omega1 / omega2) : 0.0f;

            std::cout << "\n" << pair.description << ":" << std::endl;
            std::cout << "  Gear Ratio: " << pair.gearRatio << std::endl;
            std::cout << "  Angle 1: " << (angle1 * 180.0f / PxPi) << "°  ";
            std::cout << "Angle 2: " << (angle2 * 180.0f / PxPi) << "°" << std::endl;
            std::cout << "  AngVel 1: " << omega1 << " rad/s  ";
            std::cout << "AngVel 2: " << omega2 << " rad/s" << std::endl;
            std::cout << "  Actual Ratio: " << actualRatio << " (expected: " << pair.gearRatio << ")" << std::endl;

            // Check constraint violation
            PxReal error = std::abs(actualRatio - pair.gearRatio);
            if (std::abs(omega1) > 0.01f && std::abs(omega2) > 0.01f) {
                if (error > 0.1f) {
                    std::cout << "  ⚠️  WARNING: Ratio error = " << error << std::endl;
                } else {
                    std::cout << "  ✓ Constraint satisfied (error = " << error << ")" << std::endl;
                }
            }
        }
    }

    size_t getGearPairCount() const { return gearPairs.size(); }
};

/**
 * @brief Main example application
 */
class GearJointExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;
    GearJointSystem* gearSystem;

public:
    GearJointExample()
        : physics(nullptr)
        , scene(nullptr)
        , material(nullptr)
        , gearSystem(nullptr)
    {}

    ~GearJointExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "===================================================" << std::endl;
        std::cout << "PhysX Gear Joint Example" << std::endl;
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
        material = physics->createMaterial(0.5f, 0.5f, 0.1f);  // Low restitution

        // Enable joint visualization
        scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);

        gearSystem = new GearJointSystem(core, physics, scene, material);

        std::cout << "\nGear Joint Mechanics:" << std::endl;
        std::cout << "  • Links two revolute joints with fixed ratio" << std::endl;
        std::cout << "  • Positive ratio: gears rotate same direction" << std::endl;
        std::cout << "  • Negative ratio: gears rotate opposite directions" << std::endl;
        std::cout << "  • Angular velocities: ω₁ * ratio = ω₂" << std::endl;
        std::cout << "  • Torques conserved: τ₁ = τ₂ * ratio" << std::endl;

        return true;
    }

    void createScenarios() {
        std::cout << "\n=== Creating Gear Scenarios ===" << std::endl;

        // Scenario 1: Simple 1:1 gear (same size, opposite rotation)
        gearSystem->createSimpleGearPair(PxVec3(-10, 5, 0), -1.0f,
                                         "1:1 opposite rotation");

        // Scenario 2: 2:1 reduction (output half speed)
        gearSystem->createSimpleGearPair(PxVec3(-5, 5, 0), 2.0f,
                                         "2:1 reduction (same direction)");

        // Scenario 3: 1:3 overdrive (output triple speed)
        gearSystem->createSimpleGearPair(PxVec3(0, 5, 0), -0.333f,
                                         "1:3 overdrive (opposite)");

        // Scenario 4: Gear train (multiple gears in series)
        gearSystem->createGearTrain(PxVec3(5, 5, 0));

        std::cout << "\nTotal gear pairs created: " << gearSystem->getGearPairCount() << std::endl;
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
                gearSystem->printStatus();
            }
        }

        std::cout << "\n\n=== Final Status ===";
        gearSystem->printStatus();

        std::cout << "\n\n=== Simulation Complete ===" << std::endl;
        std::cout << "\nKey Observations:" << std::endl;
        std::cout << "  • Gear ratios are maintained throughout simulation" << std::endl;
        std::cout << "  • Angular velocities scale according to ratio" << std::endl;
        std::cout << "  • Negative ratios produce opposite rotation" << std::endl;
        std::cout << "  • Gear trains multiply ratios (compound gearing)" << std::endl;
        std::cout << "  • Energy is conserved (ignoring friction)" << std::endl;
    }

    void cleanup() {
        if (gearSystem) delete gearSystem;
        if (material) material->release();
        core.cleanup();
    }
};

int main() {
    GearJointExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
