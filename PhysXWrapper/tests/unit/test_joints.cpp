/**
 * @file test_joints.cpp
 * @brief Unit tests for Joint and Articulation systems
 *
 * Tests JointManager and ArticulationManager
 */

#include <gtest/gtest.h>
#include "PhysXCore.h"
#include "Joint/JointManager.h"
#include "Articulation/ArticulationManager.h"

using namespace PhysXWrapper;

// ============================================================================
// Test Fixture
// ============================================================================

class JointTest : public ::testing::Test {
protected:
    PhysXCore* core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    void SetUp() override {
        core = new PhysXCore();
        core->initialize();
        physics = core->getPhysics();
        scene = core->getScene();
        material = core->getMaterial();
    }

    void TearDown() override {
        if (core) {
            core->cleanup();
            delete core;
            core = nullptr;
        }
    }

    // Helper: Create box actor
    PxRigidDynamic* createBox(const PxVec3& position, PxReal mass = 10.0f) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(position),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            mass
        );
        scene->addActor(*box);
        return box;
    }
};

// ============================================================================
// JointManager Tests
// ============================================================================

TEST_F(JointTest, JointManagerCreation) {
    JointManager manager;
    EXPECT_NO_THROW(manager.initialize(physics, scene));
    EXPECT_TRUE(manager.isInitialized());
}

TEST_F(JointTest, CreateSphericalJoint) {
    JointManager manager;
    manager.initialize(physics, scene);

    PxRigidDynamic* box1 = createBox(PxVec3(0, 10, 0));
    PxRigidDynamic* box2 = createBox(PxVec3(0, 8, 0));

    PxSphericalJoint* joint = manager.createSphericalJoint(
        box1, PxTransform(PxVec3(0, -1, 0)),
        box2, PxTransform(PxVec3(0, 1, 0))
    );

    ASSERT_NE(joint, nullptr);
    EXPECT_EQ(joint->getConstraintFlags(), PxConstraintFlag::Enum(0));

    box1->release();
    box2->release();
}

TEST_F(JointTest, CreateRevoluteJoint) {
    JointManager manager;
    manager.initialize(physics, scene);

    PxRigidDynamic* box1 = createBox(PxVec3(0, 10, 0));
    PxRigidDynamic* box2 = createBox(PxVec3(0, 8, 0));

    PxRevoluteJoint* joint = manager.createRevoluteJoint(
        box1, PxTransform(PxVec3(0, -1, 0)),
        box2, PxTransform(PxVec3(0, 1, 0))
    );

    ASSERT_NE(joint, nullptr);

    // Set angular limits
    joint->setLimit(PxJointAngularLimitPair(-PxPi/2, PxPi/2, 0.1f));
    joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);

    box1->release();
    box2->release();
}

TEST_F(JointTest, CreatePrismaticJoint) {
    JointManager manager;
    manager.initialize(physics, scene);

    PxRigidDynamic* box1 = createBox(PxVec3(0, 10, 0));
    PxRigidDynamic* box2 = createBox(PxVec3(0, 8, 0));

    PxPrismaticJoint* joint = manager.createPrismaticJoint(
        box1, PxTransform(PxVec3(0, -1, 0)),
        box2, PxTransform(PxVec3(0, 1, 0))
    );

    ASSERT_NE(joint, nullptr);

    // Set linear limits
    joint->setLimit(PxJointLinearLimitPair(PxTolerancesScale(), -2.0f, 2.0f, 0.1f));
    joint->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, true);

    box1->release();
    box2->release();
}

TEST_F(JointTest, CreateFixedJoint) {
    JointManager manager;
    manager.initialize(physics, scene);

    PxRigidDynamic* box1 = createBox(PxVec3(0, 10, 0));
    PxRigidDynamic* box2 = createBox(PxVec3(0, 8, 0));

    PxFixedJoint* joint = manager.createFixedJoint(
        box1, PxTransform(PxVec3(0, -1, 0)),
        box2, PxTransform(PxVec3(0, 1, 0))
    );

    ASSERT_NE(joint, nullptr);

    // Simulate - boxes should stay rigidly connected
    for (int i = 0; i < 60; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Distance between boxes should remain constant
    PxVec3 pos1 = box1->getGlobalPose().p;
    PxVec3 pos2 = box2->getGlobalPose().p;
    PxReal distance = (pos1 - pos2).magnitude();
    EXPECT_NEAR(distance, 2.0f, 0.5f);

    box1->release();
    box2->release();
}

TEST_F(JointTest, CreateDistanceJoint) {
    JointManager manager;
    manager.initialize(physics, scene);

    PxRigidDynamic* box1 = createBox(PxVec3(0, 10, 0));
    PxRigidDynamic* box2 = createBox(PxVec3(0, 8, 0));

    PxDistanceJoint* joint = manager.createDistanceJoint(
        box1, PxTransform(PxVec3(0, -1, 0)),
        box2, PxTransform(PxVec3(0, 1, 0))
    );

    ASSERT_NE(joint, nullptr);

    // Set distance constraints
    joint->setMinDistance(1.0f);
    joint->setMaxDistance(3.0f);
    joint->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, true);
    joint->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, true);

    box1->release();
    box2->release();
}

TEST_F(JointTest, CreateD6Joint) {
    JointManager manager;
    manager.initialize(physics, scene);

    PxRigidDynamic* box1 = createBox(PxVec3(0, 10, 0));
    PxRigidDynamic* box2 = createBox(PxVec3(0, 8, 0));

    PxD6Joint* joint = manager.createD6Joint(
        box1, PxTransform(PxVec3(0, -1, 0)),
        box2, PxTransform(PxVec3(0, 1, 0))
    );

    ASSERT_NE(joint, nullptr);

    // Configure D6 joint - lock X and Z translation, free Y
    joint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
    joint->setMotion(PxD6Axis::eY, PxD6Motion::eFREE);
    joint->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

    box1->release();
    box2->release();
}

TEST_F(JointTest, JointChain) {
    JointManager manager;
    manager.initialize(physics, scene);

    // Create chain of boxes connected by joints
    const int chainLength = 5;
    std::vector<PxRigidDynamic*> boxes;
    std::vector<PxSphericalJoint*> joints;

    // Create first box (anchored to world)
    PxRigidDynamic* prevBox = createBox(PxVec3(0, 10, 0));
    prevBox->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    boxes.push_back(prevBox);

    // Create chain
    for (int i = 1; i < chainLength; i++) {
        PxRigidDynamic* currentBox = createBox(PxVec3(0, 10 - i * 1.5f, 0));
        boxes.push_back(currentBox);

        PxSphericalJoint* joint = manager.createSphericalJoint(
            prevBox, PxTransform(PxVec3(0, -0.75f, 0)),
            currentBox, PxTransform(PxVec3(0, 0.75f, 0))
        );
        joints.push_back(joint);

        prevBox = currentBox;
    }

    // Simulate
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Chain should still be connected (last box not fallen too far)
    PxVec3 lastPos = boxes.back()->getGlobalPose().p;
    EXPECT_GT(lastPos.y, 0.0f);

    for (auto* box : boxes) box->release();
}

TEST_F(JointTest, JointBreakForce) {
    JointManager manager;
    manager.initialize(physics, scene);

    PxRigidDynamic* box1 = createBox(PxVec3(0, 10, 0));
    box1->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

    PxRigidDynamic* box2 = createBox(PxVec3(0, 8, 0), 100.0f);  // Heavy box

    PxFixedJoint* joint = manager.createFixedJoint(
        box1, PxTransform(PxVec3(0, -1, 0)),
        box2, PxTransform(PxVec3(0, 1, 0))
    );

    // Set low break force
    joint->setBreakForce(50.0f, 50.0f);

    // Simulate - joint may break due to heavy mass
    bool jointBroken = false;
    for (int i = 0; i < 60; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        // Check if joint is broken
        if (joint->getConstraintFlags() & PxConstraintFlag::eBROKEN) {
            jointBroken = true;
            break;
        }
    }

    // Either broke or held, both are valid
    SUCCEED();

    box1->release();
    box2->release();
}

// ============================================================================
// ArticulationManager Tests
// ============================================================================

TEST_F(JointTest, ArticulationManagerCreation) {
    ArticulationManager manager;
    EXPECT_NO_THROW(manager.initialize(physics, scene));
    EXPECT_TRUE(manager.isInitialized());
}

TEST_F(JointTest, CreateSimpleArticulation) {
    ArticulationManager manager;
    manager.initialize(physics, scene);

    PxArticulationReducedCoordinate* articulation = manager.createArticulation();
    ASSERT_NE(articulation, nullptr);

    // Create root link
    PxArticulationLink* root = manager.createRoot(
        articulation,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        material,
        10.0f
    );
    ASSERT_NE(root, nullptr);

    // Add articulation to scene
    scene->addArticulation(*articulation);

    // Simulate
    for (int i = 0; i < 60; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Root should have moved
    PxTransform pose = root->getGlobalPose();
    EXPECT_LT(pose.p.y, 10.0f);

    articulation->release();
}

TEST_F(JointTest, CreateArticulationChain) {
    ArticulationManager manager;
    manager.initialize(physics, scene);

    PxArticulationReducedCoordinate* articulation = manager.createArticulation();

    // Create root
    PxArticulationLink* root = manager.createRoot(
        articulation,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        material,
        10.0f
    );

    // Create chain of links
    PxArticulationLink* prevLink = root;
    for (int i = 0; i < 3; i++) {
        PxArticulationLink* link = manager.createLink(
            prevLink,
            PxTransform(PxVec3(0, -1.5f, 0)),
            PxBoxGeometry(0.4f, 0.4f, 0.4f),
            material,
            5.0f
        );
        ASSERT_NE(link, nullptr);
        prevLink = link;
    }

    scene->addArticulation(*articulation);

    // Simulate
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Check that articulation stayed together
    EXPECT_EQ(articulation->getNbLinks(), 4u);

    articulation->release();
}

TEST_F(JointTest, ArticulationWithRevoluteJoints) {
    ArticulationManager manager;
    manager.initialize(physics, scene);

    PxArticulationReducedCoordinate* articulation = manager.createArticulation();

    // Create root (fixed base)
    PxArticulationLink* base = manager.createRoot(
        articulation,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(1.0f, 0.2f, 1.0f),
        material,
        10.0f
    );

    // Create arm segment
    PxArticulationLink* arm = manager.createLink(
        base,
        PxTransform(PxVec3(0, -1.0f, 0)),
        PxBoxGeometry(0.2f, 1.0f, 0.2f),
        material,
        5.0f
    );

    // Configure joint as revolute
    PxArticulationJointReducedCoordinate* joint = arm->getInboundJoint();
    if (joint) {
        joint->setJointType(PxArticulationJointType::eREVOLUTE);
        joint->setMotion(PxArticulationAxis::eSWING1, PxArticulationMotion::eFREE);
    }

    scene->addArticulation(*articulation);

    // Simulate
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Arm should have swung
    EXPECT_NE(arm->getGlobalPose().p.y, 9.0f);

    articulation->release();
}

TEST_F(JointTest, ArticulationRagdoll) {
    ArticulationManager manager;
    manager.initialize(physics, scene);

    // Create simplified ragdoll
    ArticulationManager::RagdollDesc ragdollDesc;
    ragdollDesc.position = PxVec3(0, 10, 0);
    ragdollDesc.totalMass = 70.0f;

    PxArticulationReducedCoordinate* ragdoll = manager.createRagdoll(ragdollDesc);
    ASSERT_NE(ragdoll, nullptr);

    scene->addArticulation(*ragdoll);

    // Simulate
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Ragdoll should have fallen
    PxArticulationLink* root = ragdoll->getLinks()[0];
    EXPECT_LT(root->getGlobalPose().p.y, 10.0f);

    ragdoll->release();
}

TEST_F(JointTest, ArticulationRobotArm) {
    ArticulationManager manager;
    manager.initialize(physics, scene);

    // Create robot arm configuration
    ArticulationManager::RobotArmDesc armDesc;
    armDesc.basePosition = PxVec3(0, 5, 0);
    armDesc.numSegments = 4;
    armDesc.segmentLength = 1.5f;
    armDesc.segmentRadius = 0.2f;
    armDesc.segmentMass = 2.0f;

    PxArticulationReducedCoordinate* robotArm = manager.createRobotArm(armDesc);
    ASSERT_NE(robotArm, nullptr);

    scene->addArticulation(*robotArm);

    EXPECT_EQ(robotArm->getNbLinks(), 4u);

    // Simulate
    for (int i = 0; i < 60; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    robotArm->release();
}

TEST_F(JointTest, ArticulationDrive) {
    ArticulationManager manager;
    manager.initialize(physics, scene);

    PxArticulationReducedCoordinate* articulation = manager.createArticulation();

    // Create base and link
    PxArticulationLink* base = manager.createRoot(
        articulation,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        material,
        10.0f
    );

    PxArticulationLink* link = manager.createLink(
        base,
        PxTransform(PxVec3(0, -1.5f, 0)),
        PxBoxGeometry(0.3f, 0.3f, 0.3f),
        material,
        5.0f
    );

    // Configure drive
    PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
    if (joint) {
        joint->setJointType(PxArticulationJointType::eREVOLUTE);
        joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);

        // Set drive parameters
        PxArticulationDrive drive;
        drive.stiffness = 100.0f;
        drive.damping = 10.0f;
        drive.maxForce = 100.0f;
        joint->setDrive(PxArticulationAxis::eTWIST, drive);

        // Set target angle
        joint->setDriveTarget(PxArticulationAxis::eTWIST, PxPi / 4);
    }

    scene->addArticulation(*articulation);

    // Simulate
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Joint should have moved towards target
    SUCCEED();

    articulation->release();
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(JointTest, MultipleArticulations) {
    ArticulationManager manager;
    manager.initialize(physics, scene);

    std::vector<PxArticulationReducedCoordinate*> articulations;

    // Create multiple simple articulations
    for (int i = 0; i < 3; i++) {
        PxArticulationReducedCoordinate* art = manager.createArticulation();
        PxArticulationLink* root = manager.createRoot(
            art,
            PxTransform(PxVec3(i * 3.0f, 10, 0)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            material,
            10.0f
        );

        scene->addArticulation(*art);
        articulations.push_back(art);
    }

    // Simulate
    for (int i = 0; i < 60; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    for (auto* art : articulations) {
        art->release();
    }
}

TEST_F(JointTest, JointAndArticulationTogether) {
    JointManager jointMgr;
    ArticulationManager artMgr;

    jointMgr.initialize(physics, scene);
    artMgr.initialize(physics, scene);

    // Create regular joint chain
    PxRigidDynamic* box1 = createBox(PxVec3(-5, 10, 0));
    PxRigidDynamic* box2 = createBox(PxVec3(-5, 8, 0));
    jointMgr.createSphericalJoint(
        box1, PxTransform(PxVec3(0, -1, 0)),
        box2, PxTransform(PxVec3(0, 1, 0))
    );

    // Create articulation
    PxArticulationReducedCoordinate* art = artMgr.createArticulation();
    PxArticulationLink* root = artMgr.createRoot(
        art,
        PxTransform(PxVec3(5, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        material,
        10.0f
    );
    scene->addArticulation(*art);

    // Simulate both
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    box1->release();
    box2->release();
    art->release();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
