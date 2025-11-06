/**
 * @file example_spatialtendon.cpp
 * @brief Spatial Tendon System Example
 *
 * This example demonstrates PhysX's spatial tendon functionality:
 * - Tendons routed through 3D space with attachment points
 * - Pulley systems and cable routing
 * - Multiple attachment points on articulation links
 * - Cable length constraints in 3D
 * - Complex tendon paths
 *
 * Based on PhysX Snippet: SnippetSpatialTendon
 *
 * Spatial Tendon Mechanics:
 * - Tendons attach at specific 3D points on links
 * - Cable routes through multiple attachment points
 * - Total cable length = sum of segment lengths
 * - Constraint: total_length ≤ rest_length
 * - Can simulate pulleys, guides, routing points
 *
 * Differences from Fixed Tendon:
 * - Fixed: Couples joint angles (1D constraint)
 * - Spatial: Routes through 3D space (geometric constraint)
 * - Spatial allows complex cable paths
 * - Spatial models realistic cable routing
 *
 * Applications:
 * - Cable-driven robots (CableRobots)
 * - Tendon-driven manipulators
 * - Pulley systems
 * - Crane cables
 * - Biomechanical muscle routing
 * - Bowden cable systems
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

using namespace PhysXWrapper;

/**
 * @brief Helper class for spatial tendon systems
 */
class SpatialTendonSystem {
private:
    PhysXCore& core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    struct SpatialTendonMechanism {
        PxArticulationReducedCoordinate* articulation;
        std::vector<PxArticulationLink*> links;
        std::vector<PxArticulationJointReducedCoordinate*> joints;
        PxArticulationSpatialTendon* spatialTendon;
        std::vector<PxArticulationAttachment*> attachments;
        std::string description;
    };

    std::vector<SpatialTendonMechanism> mechanisms;

public:
    SpatialTendonSystem(PhysXCore& coreRef, PxPhysics* phys, PxScene* scn, PxMaterial* mat)
        : core(coreRef), physics(phys), scene(scn), material(mat)
    {}

    /**
     * Create a simple pulley system with spatial tendon
     */
    void createPulleySystem(const PxVec3& basePos, const std::string& description) {
        std::cout << "\nCreating: " << description << std::endl;

        PxArticulationReducedCoordinate* articulation = physics->createArticulationReducedCoordinate();
        articulation->setSolverIterationCounts(32, 1);

        // Create base (fixed)
        PxTransform basePose(basePos);
        PxArticulationLink* base = articulation->createLink(nullptr, basePose);
        PxBoxGeometry baseGeom(1.0f, 0.2f, 0.2f);
        PxRigidActorExt::createExclusiveShape(*base, baseGeom, *material);
        PxRigidBodyExt::updateMassAndInertia(*base, 10.0f);
        base->setLinearDamping(0.5f);
        base->setAngularDamping(0.5f);

        std::vector<PxArticulationLink*> links;
        std::vector<PxArticulationJointReducedCoordinate*> joints;
        links.push_back(base);

        // Create two hanging masses connected by cable over pulley
        PxVec3 mass1Pos = basePos + PxVec3(-0.8f, -2.0f, 0);
        PxVec3 mass2Pos = basePos + PxVec3(0.8f, -1.0f, 0);

        for (int i = 0; i < 2; i++) {
            PxVec3 pos = (i == 0) ? mass1Pos : mass2Pos;
            PxArticulationLink* link = articulation->createLink(base, PxTransform(pos));

            PxBoxGeometry massGeom(0.3f, 0.3f, 0.3f);
            PxRigidActorExt::createExclusiveShape(*link, massGeom, *material);
            PxRigidBodyExt::updateMassAndInertia(*link, (i == 0) ? 5.0f : 3.0f);  // Different masses
            link->setLinearDamping(0.2f);

            PxArticulationJointReducedCoordinate* joint =
                static_cast<PxArticulationJointReducedCoordinate*>(link->getInboundJoint());

            PxReal xOffset = (i == 0) ? -0.8f : 0.8f;
            joint->setParentPose(PxTransform(PxVec3(xOffset, -0.2f, 0)));
            joint->setChildPose(PxTransform(PxVec3(0, 0.3f, 0)));

            // Prismatic joint (slides vertically)
            joint->setJointType(PxArticulationJointType::ePRISMATIC);
            joint->setMotion(PxArticulationAxis::eY, PxArticulationMotion::eFREE);

            // Set limits
            PxArticulationLimit limit(-3.0f, 0.5f);
            joint->setLimit(PxArticulationAxis::eY, limit);

            links.push_back(link);
            joints.push_back(joint);
        }

        // Create spatial tendon (cable)
        PxArticulationSpatialTendon* tendon = articulation->createSpatialTendon();

        // Attach cable points:
        // Start at mass 1 bottom -> over pulley (top of base) -> down to mass 2 bottom
        std::vector<PxArticulationAttachment*> attachments;

        // Attachment 1: Bottom of mass 1
        PxArticulationAttachment* attach1 = tendon->createAttachment(
            nullptr,                           // Parent attachment (none for first)
            1.0f,                              // Coefficient
            PxVec3(0, -0.3f, 0),              // Relative position on link
            links[1]                           // Link (mass 1)
        );
        attachments.push_back(attach1);

        // Attachment 2: Pulley point (left side of base)
        PxArticulationAttachment* attach2 = tendon->createAttachment(
            attach1,                           // Parent (forms cable segment)
            1.0f,
            PxVec3(-0.8f, 0.2f, 0),           // Left pulley point
            links[0]                           // Base link
        );
        attachments.push_back(attach2);

        // Attachment 3: Pulley point (right side of base)
        PxArticulationAttachment* attach3 = tendon->createAttachment(
            attach2,
            1.0f,
            PxVec3(0.8f, 0.2f, 0),            // Right pulley point
            links[0]
        );
        attachments.push_back(attach3);

        // Attachment 4: Bottom of mass 2
        PxArticulationAttachment* attach4 = tendon->createAttachment(
            attach3,
            1.0f,
            PxVec3(0, -0.3f, 0),              // Bottom of mass 2
            links[2]
        );
        attachments.push_back(attach4);

        // Configure tendon properties
        tendon->setStiffness(1000.0f);
        tendon->setDamping(10.0f);

        // Calculate rest length from initial configuration
        PxReal restLength = 5.0f;  // Total cable length
        tendon->setOffset(-restLength, true);  // Negative for inequality constraint (length <= rest)

        tendon->setLimitStiffness(1000.0f);

        std::cout << "  Pulley system created:" << std::endl;
        std::cout << "    Mass 1: 5 kg (left side)" << std::endl;
        std::cout << "    Mass 2: 3 kg (right side)" << std::endl;
        std::cout << "    Cable length: " << restLength << " m" << std::endl;
        std::cout << "    Attachment points: " << attachments.size() << std::endl;

        scene->addArticulation(*articulation);

        SpatialTendonMechanism mech;
        mech.articulation = articulation;
        mech.links = links;
        mech.joints = joints;
        mech.spatialTendon = tendon;
        mech.attachments = attachments;
        mech.description = description;
        mechanisms.push_back(mech);
    }

    /**
     * Create a cable-driven arm
     */
    void createCableDrivenArm(const PxVec3& basePos) {
        std::cout << "\nCreating cable-driven arm..." << std::endl;

        PxArticulationReducedCoordinate* articulation = physics->createArticulationReducedCoordinate();
        articulation->setSolverIterationCounts(32, 1);

        // Create base
        PxTransform basePose(basePos);
        PxArticulationLink* base = articulation->createLink(nullptr, basePose);
        PxBoxGeometry baseGeom(0.3f, 0.3f, 0.3f);
        PxRigidActorExt::createExclusiveShape(*base, baseGeom, *material);
        PxRigidBodyExt::updateMassAndInertia(*base, 5.0f);

        std::vector<PxArticulationLink*> links;
        std::vector<PxArticulationJointReducedCoordinate*> joints;
        links.push_back(base);

        // Create 2-segment arm
        PxVec3 linkSize(0.15f, 0.8f, 0.15f);
        PxReal linkLength = 0.8f;
        PxVec3 currentPos = basePos + PxVec3(0, -linkLength / 2, 0);

        for (int i = 0; i < 2; i++) {
            PxArticulationLink* link = articulation->createLink(
                (i == 0) ? base : links[i],
                PxTransform(currentPos)
            );

            PxBoxGeometry linkGeom(linkSize.x, linkSize.y / 2, linkSize.z);
            PxRigidActorExt::createExclusiveShape(*link, linkGeom, *material);
            PxRigidBodyExt::updateMassAndInertia(*link, 2.0f);
            link->setLinearDamping(0.5f);
            link->setAngularDamping(0.5f);

            PxArticulationJointReducedCoordinate* joint =
                static_cast<PxArticulationJointReducedCoordinate*>(link->getInboundJoint());

            PxTransform parentFrame, childFrame;
            if (i == 0) {
                parentFrame = PxTransform(PxVec3(0, -0.3f, 0));
                childFrame = PxTransform(PxVec3(0, linkSize.y / 2, 0));
            } else {
                parentFrame = PxTransform(PxVec3(0, -linkSize.y / 2, 0));
                childFrame = PxTransform(PxVec3(0, linkSize.y / 2, 0));
            }

            joint->setParentPose(parentFrame);
            joint->setChildPose(childFrame);
            joint->setJointType(PxArticulationJointType::eREVOLUTE);
            joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);

            PxArticulationLimit limit(-PxPi / 2, PxPi / 2);
            joint->setLimit(PxArticulationAxis::eTWIST, limit);

            links.push_back(link);
            joints.push_back(joint);

            currentPos.y -= linkLength;
        }

        // Create antagonistic cable pair (flexor and extensor)
        PxArticulationSpatialTendon* flexor = articulation->createSpatialTendon();
        PxArticulationSpatialTendon* extensor = articulation->createSpatialTendon();

        // Flexor cable (pulls arm to flex)
        PxArticulationAttachment* flexAttach1 = flexor->createAttachment(
            nullptr, 1.0f, PxVec3(0.2f, -0.2f, 0), links[0]  // Base front
        );
        PxArticulationAttachment* flexAttach2 = flexor->createAttachment(
            flexAttach1, 1.0f, PxVec3(0.15f, 0.3f, 0), links[1]  // Link 1 front
        );
        PxArticulationAttachment* flexAttach3 = flexor->createAttachment(
            flexAttach2, 1.0f, PxVec3(0.15f, -0.3f, 0), links[2]  // Link 2 front
        );

        flexor->setStiffness(500.0f);
        flexor->setDamping(50.0f);
        flexor->setOffset(-2.5f, true);

        // Extensor cable (pulls arm to extend)
        PxArticulationAttachment* extAttach1 = extensor->createAttachment(
            nullptr, 1.0f, PxVec3(-0.2f, -0.2f, 0), links[0]  // Base back
        );
        PxArticulationAttachment* extAttach2 = extensor->createAttachment(
            extAttach1, 1.0f, PxVec3(-0.15f, 0.3f, 0), links[1]  // Link 1 back
        );
        PxArticulationAttachment* extAttach3 = extensor->createAttachment(
            extAttach2, 1.0f, PxVec3(-0.15f, -0.3f, 0), links[2]  // Link 2 back
        );

        extensor->setStiffness(500.0f);
        extensor->setDamping(50.0f);
        extensor->setOffset(-2.5f, true);

        scene->addArticulation(*articulation);

        std::cout << "  Cable-driven arm created:" << std::endl;
        std::cout << "    2 segments" << std::endl;
        std::cout << "    Antagonistic cable pair (flexor/extensor)" << std::endl;
        std::cout << "    Biomimetic muscle-like actuation" << std::endl;

        SpatialTendonMechanism mech;
        mech.articulation = articulation;
        mech.links = links;
        mech.joints = joints;
        mech.spatialTendon = flexor;  // Store flexor as primary
        mech.description = "Cable-driven arm";
        mechanisms.push_back(mech);
    }

    /**
     * Print status
     */
    void printStatus() {
        std::cout << "\n=== Spatial Tendon Status ===" << std::endl;
        std::cout << std::fixed << std::setprecision(3);

        for (const auto& mech : mechanisms) {
            std::cout << "\n" << mech.description << ":" << std::endl;

            // Print link positions
            for (size_t i = 1; i < mech.links.size(); i++) {
                PxVec3 pos = mech.links[i]->getGlobalPose().p;
                std::cout << "  Link " << i << " position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
            }

            // Calculate current cable length
            PxReal totalLength = 0.0f;
            for (const auto& attachment : mech.attachments) {
                if (attachment->getParent()) {
                    // Get world positions
                    PxVec3 parentPos = attachment->getParent()->getLink()->getGlobalPose().transform(
                        attachment->getParent()->getRelativeOffset()
                    );
                    PxVec3 childPos = attachment->getLink()->getGlobalPose().transform(
                        attachment->getRelativeOffset()
                    );

                    PxReal segmentLength = (childPos - parentPos).magnitude();
                    totalLength += segmentLength;
                }
            }

            PxReal restLength = -mech.spatialTendon->getOffset();
            PxReal tension = (totalLength > restLength) ? (totalLength - restLength) : 0.0f;

            std::cout << "  Cable length: " << totalLength << " m" << std::endl;
            std::cout << "  Rest length: " << restLength << " m" << std::endl;
            std::cout << "  Tension: " << tension << " m";

            if (tension < 0.01f) {
                std::cout << " (slack)";
            } else {
                std::cout << " (taut)";
            }
            std::cout << std::endl;
        }
    }

    size_t getMechanismCount() const { return mechanisms.size(); }
};

/**
 * @brief Main example
 */
class SpatialTendonExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;
    SpatialTendonSystem* system;

public:
    SpatialTendonExample()
        : physics(nullptr), scene(nullptr), material(nullptr), system(nullptr)
    {}

    ~SpatialTendonExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "===================================================" << std::endl;
        std::cout << "PhysX Spatial Tendon Example" << std::endl;
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

        PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
        scene->addActor(*ground);

        scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);

        system = new SpatialTendonSystem(core, physics, scene, material);

        std::cout << "\nSpatial Tendon Mechanics:" << std::endl;
        std::cout << "  • Cables routed through 3D attachment points" << std::endl;
        std::cout << "  • Length constraint: total_length ≤ rest_length" << std::endl;
        std::cout << "  • Simulates pulleys, guides, cable routing" << std::endl;
        std::cout << "  • Realistic cable physics" << std::endl;
        std::cout << "  • Applications: cable robots, tendon-driven manipulators" << std::endl;

        return true;
    }

    void createScenarios() {
        std::cout << "\n=== Creating Spatial Tendon Scenarios ===" << std::endl;

        system->createPulleySystem(PxVec3(-5, 10, 0), "Pulley system (Atwood machine)");
        system->createCableDrivenArm(PxVec3(5, 10, 0));

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
    }

    void cleanup() {
        if (system) delete system;
        if (material) material->release();
        core.cleanup();
    }
};

int main() {
    SpatialTendonExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
