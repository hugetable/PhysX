/**
 * @file example_fixedtendon.cpp
 * @brief Fixed Tendon Constraint Example
 *
 * This example demonstrates PhysX's fixed tendon functionality:
 * - Coupling multiple articulation joints with tendon constraints
 * - Maintaining linear relationships between joint angles
 * - Simulating cable-driven mechanisms
 * - Robotic tendon systems
 * - Finger mechanisms with coupled joints
 *
 * Based on PhysX Snippet: SnippetFixedTendon
 *
 * Fixed Tendon Mechanics:
 * - Applies to articulation joints (not regular joints)
 * - Constraint: Σ(coefficient_i × joint_position_i) = rest_length
 * - Can couple multiple joints in complex ways
 * - Simulates inextensible cables/tendons
 * - Used in underactuated mechanisms
 *
 * Applications:
 * - Robotic hands with tendon actuation
 * - Cable-driven parallel robots
 * - Finger mechanisms (joints move together)
 * - Pulley systems
 * - Biomechanical simulations
 *
 * Physics:
 * The tendon constraint enforces: c₁θ₁ + c₂θ₂ + ... + cₙθₙ = L
 * where cᵢ are coefficients, θᵢ are joint angles, L is rest length
 * This creates mechanical coupling between joints
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

using namespace PhysXWrapper;

/**
 * @brief Helper class for creating articulated mechanisms with tendons
 */
class TendonSystem {
private:
    PhysXCore& core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    struct ArticulationWithTendon {
        PxArticulationReducedCoordinate* articulation;
        std::vector<PxArticulationLink*> links;
        std::vector<PxArticulationJointReducedCoordinate*> joints;
        PxArticulationTendonJoint* tendonJoints[8];  // Support up to 8 joints in tendon
        PxArticulationFixedTendon* fixedTendon;
        int numTendonJoints;
        std::string description;
    };

    std::vector<ArticulationWithTendon> mechanisms;

public:
    TendonSystem(PhysXCore& coreRef, PxPhysics* phys, PxScene* scn, PxMaterial* mat)
        : core(coreRef), physics(phys), scene(scn), material(mat)
    {}

    /**
     * Create a simple 3-link finger with tendon coupling
     */
    void createTendonFinger(const PxVec3& basePos, const std::string& description) {
        std::cout << "\nCreating: " << description << std::endl;

        // Create articulation
        PxArticulationReducedCoordinate* articulation = physics->createArticulationReducedCoordinate();

        // Configure articulation
        articulation->setSolverIterationCounts(32, 1);
        articulation->setMaxLinearVelocity(100.0f);

        // Create base link (palm)
        PxTransform basePose(basePos);
        PxArticulationLink* base = articulation->createLink(nullptr, basePose);
        PxBoxGeometry palmGeom(0.5f, 0.2f, 0.3f);
        PxRigidActorExt::createExclusiveShape(*base, palmGeom, *material);
        PxRigidBodyExt::updateMassAndInertia(*base, 1.0f);
        base->setLinearDamping(0.5f);
        base->setAngularDamping(0.5f);

        // Create 3 finger links (phalanges)
        std::vector<PxArticulationLink*> links;
        std::vector<PxArticulationJointReducedCoordinate*> joints;
        links.push_back(base);

        PxVec3 linkSize(0.15f, 0.6f, 0.15f);
        PxReal linkLength = 0.6f;
        PxVec3 currentPos = basePos + PxVec3(0, -0.2f - linkLength / 2, 0);

        for (int i = 0; i < 3; i++) {
            // Create link
            PxTransform linkPose(currentPos);
            PxArticulationLink* link = articulation->createLink(links[i], linkPose);

            PxBoxGeometry linkGeom(linkSize.x, linkSize.y / 2, linkSize.z);
            PxRigidActorExt::createExclusiveShape(*link, linkGeom, *material);
            PxRigidBodyExt::updateMassAndInertia(*link, 0.5f);
            link->setLinearDamping(0.5f);
            link->setAngularDamping(0.5f);

            // Get and configure joint
            PxArticulationJointReducedCoordinate* joint = static_cast<PxArticulationJointReducedCoordinate*>(link->getInboundJoint());

            // Set joint frames (revolute around Z-axis)
            PxTransform parentFrame, childFrame;
            if (i == 0) {
                // First joint connects to palm
                parentFrame = PxTransform(PxVec3(0, -0.2f, 0));
                childFrame = PxTransform(PxVec3(0, linkSize.y / 2, 0));
            } else {
                // Subsequent joints connect to previous link
                parentFrame = PxTransform(PxVec3(0, -linkSize.y / 2, 0));
                childFrame = PxTransform(PxVec3(0, linkSize.y / 2, 0));
            }

            joint->setParentPose(parentFrame);
            joint->setChildPose(childFrame);

            // Configure as revolute joint (1 DOF rotation around Z)
            joint->setJointType(PxArticulationJointType::eREVOLUTE);
            joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);

            // Set joint limits (0 to 90 degrees flexion)
            PxArticulationLimit limit(-0.1f, PxPi / 2.0f);  // 0 to 90 degrees
            joint->setLimit(PxArticulationAxis::eTWIST, limit);

            // Set joint drive (stiffness and damping)
            PxArticulationDrive drive;
            drive.stiffness = 100.0f;
            drive.damping = 10.0f;
            drive.maxForce = PX_MAX_F32;
            drive.driveType = PxArticulationDriveType::eFORCE;
            joint->setDrive(PxArticulationAxis::eTWIST, drive);

            links.push_back(link);
            joints.push_back(joint);

            // Update position for next link
            currentPos.y -= linkLength;
        }

        // Create fixed tendon
        // The tendon couples all 3 joints so they flex together
        PxArticulationFixedTendon* tendon = articulation->createFixedTendon();

        std::cout << "  Creating tendon with 3 joints:" << std::endl;

        // Attach tendon to each joint
        // Coefficients determine how each joint contributes
        // Equal coefficients = joints move by same amount
        PxReal coefficients[3] = {1.0f, 1.0f, 1.0f};  // All joints equally coupled
        PxArticulationTendonJoint* tendonJoints[3];

        for (int i = 0; i < 3; i++) {
            tendonJoints[i] = tendon->createTendonJoint(nullptr, PxArticulationAxis::eTWIST,
                                                         coefficients[i], 0.0f, links[i + 1]);
            std::cout << "    Joint " << i << ": coefficient=" << coefficients[i] << std::endl;
        }

        // Set tendon stiffness and damping
        tendon->setStiffness(1000.0f);
        tendon->setDamping(10.0f);

        // Set rest length (offset) - the tendon maintains this total
        // Sum of (coefficient * joint_angle) = rest_length
        tendon->setOffset(0.0f);  // Zero offset = straight configuration is rest

        // Limit tendon force
        tendon->setLimitStiffness(100.0f);

        std::cout << "  Tendon configured:" << std::endl;
        std::cout << "    Stiffness: " << tendon->getStiffness() << std::endl;
        std::cout << "    Damping: " << tendon->getDamping() << std::endl;
        std::cout << "    Offset: " << tendon->getOffset() << std::endl;

        // Add articulation to scene
        scene->addArticulation(*articulation);

        // Apply force to first joint to test tendon
        joints[0]->setDriveTarget(PxArticulationAxis::eTWIST, PxPi / 4.0f);  // 45 degrees

        // Store mechanism
        ArticulationWithTendon mechanism;
        mechanism.articulation = articulation;
        mechanism.links = links;
        mechanism.joints = joints;
        mechanism.fixedTendon = tendon;
        mechanism.numTendonJoints = 3;
        for (int i = 0; i < 3; i++) {
            mechanism.tendonJoints[i] = tendonJoints[i];
        }
        mechanism.description = description;
        mechanisms.push_back(mechanism);
    }

    /**
     * Create a 4-bar linkage with tendon coupling
     */
    void createFourBarLinkage(const PxVec3& basePos) {
        std::cout << "\nCreating 4-bar linkage with tendon..." << std::endl;

        PxArticulationReducedCoordinate* articulation = physics->createArticulationReducedCoordinate();
        articulation->setSolverIterationCounts(32, 1);

        // Create base (ground link)
        PxTransform basePose(basePos);
        PxArticulationLink* base = articulation->createLink(nullptr, basePose);
        PxBoxGeometry baseGeom(1.0f, 0.2f, 0.2f);
        PxRigidActorExt::createExclusiveShape(*base, baseGeom, *material);
        PxRigidBodyExt::updateMassAndInertia(*base, 10.0f);

        std::vector<PxArticulationLink*> links;
        std::vector<PxArticulationJointReducedCoordinate*> joints;
        links.push_back(base);

        // Create two moving links connected to base
        PxVec3 linkPositions[2] = {
            basePos + PxVec3(-0.8f, -0.5f, 0),
            basePos + PxVec3(0.8f, -0.5f, 0)
        };

        for (int i = 0; i < 2; i++) {
            PxArticulationLink* link = articulation->createLink(base, PxTransform(linkPositions[i]));
            PxBoxGeometry linkGeom(0.1f, 0.5f, 0.1f);
            PxRigidActorExt::createExclusiveShape(*link, linkGeom, *material);
            PxRigidBodyExt::updateMassAndInertia(*link, 2.0f);

            PxArticulationJointReducedCoordinate* joint =
                static_cast<PxArticulationJointReducedCoordinate*>(link->getInboundJoint());

            PxReal xOffset = (i == 0) ? -0.8f : 0.8f;
            joint->setParentPose(PxTransform(PxVec3(xOffset, -0.2f, 0)));
            joint->setChildPose(PxTransform(PxVec3(0, 0.5f, 0)));

            joint->setJointType(PxArticulationJointType::eREVOLUTE);
            joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);

            PxArticulationLimit limit(-PxPi / 2, PxPi / 2);
            joint->setLimit(PxArticulationAxis::eTWIST, limit);

            links.push_back(link);
            joints.push_back(joint);
        }

        // Create tendon coupling the two joints
        // Opposite coefficients = mirror motion
        PxArticulationFixedTendon* tendon = articulation->createFixedTendon();

        PxArticulationTendonJoint* tj0 = tendon->createTendonJoint(nullptr, PxArticulationAxis::eTWIST,
                                                                     1.0f, 0.0f, links[1]);
        PxArticulationTendonJoint* tj1 = tendon->createTendonJoint(nullptr, PxArticulationAxis::eTWIST,
                                                                     -1.0f, 0.0f, links[2]);

        tendon->setStiffness(500.0f);
        tendon->setDamping(50.0f);
        tendon->setOffset(0.0f);

        scene->addArticulation(*articulation);

        // Apply drive to first joint
        joints[0]->setDriveTarget(PxArticulationAxis::eTWIST, PxPi / 6.0f);

        std::cout << "  4-bar linkage created with mirror coupling" << std::endl;

        ArticulationWithTendon mechanism;
        mechanism.articulation = articulation;
        mechanism.links = links;
        mechanism.joints = joints;
        mechanism.fixedTendon = tendon;
        mechanism.tendonJoints[0] = tj0;
        mechanism.tendonJoints[1] = tj1;
        mechanism.numTendonJoints = 2;
        mechanism.description = "4-bar linkage";
        mechanisms.push_back(mechanism);
    }

    /**
     * Print status of all mechanisms
     */
    void printStatus() {
        std::cout << "\n=== Tendon System Status ===" << std::endl;
        std::cout << std::fixed << std::setprecision(3);

        for (const auto& mech : mechanisms) {
            std::cout << "\n" << mech.description << ":" << std::endl;

            // Print joint angles
            PxReal totalContribution = 0.0f;
            for (int i = 0; i < mech.numTendonJoints; i++) {
                PxArticulationTendonJoint* tj = mech.tendonJoints[i];
                PxReal coefficient = tj->coefficient;

                // Get joint angle from link
                PxArticulationLink* link = mech.links[i + 1];
                PxArticulationJointReducedCoordinate* joint =
                    static_cast<PxArticulationJointReducedCoordinate*>(link->getInboundJoint());

                PxReal angle = joint->getJointPosition(PxArticulationAxis::eTWIST);
                PxReal contribution = coefficient * angle;
                totalContribution += contribution;

                std::cout << "  Joint " << i << ": angle=" << (angle * 180.0f / PxPi) << "°  "
                          << "coeff=" << coefficient << "  "
                          << "contrib=" << contribution << std::endl;
            }

            PxReal restLength = mech.fixedTendon->getOffset();
            PxReal error = totalContribution - restLength;

            std::cout << "  Total contribution: " << totalContribution << std::endl;
            std::cout << "  Rest length: " << restLength << std::endl;
            std::cout << "  Constraint error: " << error;

            if (std::abs(error) < 0.01f) {
                std::cout << " ✓";
            }
            std::cout << std::endl;
        }
    }

    size_t getMechanismCount() const { return mechanisms.size(); }
};

/**
 * @brief Main example application
 */
class FixedTendonExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;
    TendonSystem* system;

public:
    FixedTendonExample()
        : physics(nullptr), scene(nullptr), material(nullptr), system(nullptr)
    {}

    ~FixedTendonExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "===================================================" << std::endl;
        std::cout << "PhysX Fixed Tendon Example" << std::endl;
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
        material = physics->createMaterial(0.5f, 0.5f, 0.3f);

        // Create ground
        PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
        scene->addActor(*ground);

        scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);

        system = new TendonSystem(core, physics, scene, material);

        std::cout << "\nFixed Tendon Mechanics:" << std::endl;
        std::cout << "  • Couples multiple articulation joints" << std::endl;
        std::cout << "  • Constraint: Σ(cᵢ × θᵢ) = L" << std::endl;
        std::cout << "  • Simulates inextensible cables/tendons" << std::endl;
        std::cout << "  • Used in underactuated mechanisms" << std::endl;
        std::cout << "  • Applications: robotic hands, cable robots" << std::endl;

        return true;
    }

    void createScenarios() {
        std::cout << "\n=== Creating Tendon Scenarios ===" << std::endl;

        // Scenario 1: Tendon-driven finger (3 joints coupled)
        system->createTendonFinger(PxVec3(-5, 10, 0), "Tendon finger (equal coupling)");

        // Scenario 2: 4-bar linkage with mirror coupling
        system->createFourBarLinkage(PxVec3(5, 10, 0));

        std::cout << "\nTotal mechanisms: " << system->getMechanismCount() << std::endl;
    }

    void simulate(PxReal dt) {
        scene->simulate(dt);
        scene->fetchResults(true);
    }

    void run() {
        createScenarios();

        std::cout << "\n=== Starting Simulation ===" << std::endl;

        const PxReal dt = 1.0f / 60.0f;
        const int totalFrames = 600;

        for (int frame = 0; frame < totalFrames; frame++) {
            simulate(dt);

            if (frame % 120 == 0) {
                std::cout << "\n=== Frame " << frame << " (t=" << (frame * dt) << "s) ===";
                system->printStatus();
            }
        }

        std::cout << "\n\n=== Final Status ===";
        system->printStatus();

        std::cout << "\n\n=== Simulation Complete ===" << std::endl;
        std::cout << "\nKey Observations:" << std::endl;
        std::cout << "  • Tendon maintains constraint: Σ(cᵢθᵢ) = L" << std::endl;
        std::cout << "  • Coupled joints move together" << std::endl;
        std::cout << "  • Tendon forces distributed across joints" << std::endl;
        std::cout << "  • Enables underactuated control" << std::endl;
    }

    void cleanup() {
        if (system) delete system;
        if (material) material->release();
        core.cleanup();
    }
};

int main() {
    FixedTendonExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
