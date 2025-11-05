/**
 * @file test_integration_basic.cpp
 * @brief Basic integration tests for PhysXWrapper
 *
 * Tests complex scenarios involving multiple components
 */

#include <gtest/gtest.h>
#include "PhysXCore.h"
#include "RigidBody/RigidBodyContactHandler.h"
#include "RigidBody/RigidBodyTrigger.h"
#include "Geometry/ConvexMeshBuilder.h"
#include "Geometry/TriangleMeshBuilder.h"
#include "Joint/JointManager.h"
#include "Articulation/ArticulationManager.h"
#include "Utility/MaterialLibrary.h"
#include "Utility/PerformanceProfiler.h"
#include "Debug/DebugDrawer.h"

using namespace PhysXWrapper;

// ============================================================================
// Test Fixture
// ============================================================================

class IntegrationBasicTest : public ::testing::Test {
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

    void simulateFrames(int numFrames, PxReal dt = 1.0f / 60.0f) {
        for (int i = 0; i < numFrames; i++) {
            scene->simulate(dt);
            scene->fetchResults(true);
        }
    }
};

// ============================================================================
// Basic Integration Tests
// ============================================================================

TEST_F(IntegrationBasicTest, CompleteSceneSetup) {
    MaterialLibrary materials;
    TriangleMeshBuilder meshBuilder;
    PerformanceProfiler profiler;

    materials.initialize(physics);
    meshBuilder.initialize(physics);
    profiler.initialize(physics, scene);

    // Create ground with terrain
    std::vector<std::vector<PxReal>> heightMap(20, std::vector<PxReal>(20, 0.0f));
    PxTriangleMesh* terrain = meshBuilder.createTerrainMesh(heightMap, 1.0f, 1.0f);
    PxRigidStatic* ground = meshBuilder.createTriangleMeshActor(
        terrain, PxTransform(PxVec3(0, 0, 0)), material
    );
    scene->addActor(*ground);

    // Create falling objects with different materials
    PxMaterial* wood = materials.getMaterial("Wood");
    PxMaterial* metal = materials.getMaterial("Metal");

    PxRigidDynamic* woodBox = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *wood,
        10.0f
    );
    scene->addActor(*woodBox);

    PxRigidDynamic* metalBox = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(2, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *metal,
        10.0f
    );
    scene->addActor(*metalBox);

    // Simulate with profiling
    for (int i = 0; i < 120; i++) {
        profiler.beginFrame();
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
        profiler.endFrame();
    }

    // Verify simulation ran
    EXPECT_GT(profiler.getFrameCount(), 0u);

    // Verify objects fell
    EXPECT_LT(woodBox->getGlobalPose().p.y, 5.0f);
    EXPECT_LT(metalBox->getGlobalPose().p.y, 5.0f);

    woodBox->release();
    metalBox->release();
    ground->release();
    terrain->release();
}

TEST_F(IntegrationBasicTest, ContactHandlingWithJoints) {
    RigidBodyContactHandler contacts;
    JointManager joints;

    contacts.initialize(scene);
    joints.initialize(physics, scene);

    int contactBeginCount = 0;
    contacts.setOnContactBegin([&contactBeginCount](PxActor*, PxActor*) {
        contactBeginCount++;
    });

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Create jointed chain
    PxRigidDynamic* anchor = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    anchor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    scene->addActor(*anchor);

    PxRigidDynamic* box1 = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 8, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box1);

    PxRigidDynamic* box2 = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 6, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box2);

    joints.createSphericalJoint(anchor, PxTransform(PxVec3(0, -1, 0)), box1, PxTransform(PxVec3(0, 1, 0)));
    joints.createSphericalJoint(box1, PxTransform(PxVec3(0, -1, 0)), box2, PxTransform(PxVec3(0, 1, 0)));

    simulateFrames(180);

    // Should have detected contacts
    EXPECT_GT(contactBeginCount, 0);

    anchor->release();
    box1->release();
    box2->release();
    ground->release();
}

TEST_F(IntegrationBasicTest, TriggerWithArticulation) {
    RigidBodyTrigger triggers;
    ArticulationManager articulation;

    triggers.initialize(physics, scene);
    articulation.initialize(physics, scene);

    int triggerEnterCount = 0;
    triggers.setOnEnter([&triggerEnterCount](PxActor*, PxActor*) {
        triggerEnterCount++;
    });

    // Create trigger zone
    PxRigidStatic* triggerZone = triggers.createBoxTrigger(
        PxVec3(0, 5, 0),
        PxVec3(3, 3, 3)
    );

    // Create articulated ragdoll
    ArticulationManager::RagdollDesc ragdollDesc;
    ragdollDesc.position = PxVec3(0, 15, 0);
    ragdollDesc.totalMass = 70.0f;

    PxArticulationReducedCoordinate* ragdoll = articulation.createRagdoll(ragdollDesc);
    scene->addArticulation(*ragdoll);

    simulateFrames(180);

    // Ragdoll should have triggered the zone
    EXPECT_GT(triggerEnterCount, 0);

    triggerZone->release();
    ragdoll->release();
}

TEST_F(IntegrationBasicTest, ConvexMeshOnTriangleMesh) {
    ConvexMeshBuilder convexBuilder;
    TriangleMeshBuilder triangleBuilder;

    convexBuilder.initialize(physics);
    triangleBuilder.initialize(physics);

    // Create triangle mesh ground plane
    std::vector<PxVec3> groundVerts = {
        PxVec3(-10, 0, -10), PxVec3(10, 0, -10),
        PxVec3(10, 0, 10), PxVec3(-10, 0, 10)
    };
    std::vector<PxU32> groundIndices = {0, 1, 2, 0, 2, 3};

    PxTriangleMesh* groundMesh = triangleBuilder.createTriangleMesh(groundVerts, groundIndices);
    PxRigidStatic* ground = triangleBuilder.createTriangleMeshActor(
        groundMesh, PxTransform(PxVec3(0, 0, 0)), material
    );
    scene->addActor(*ground);

    // Create convex pyramid
    PxConvexMesh* pyramidMesh = convexBuilder.createPyramidMesh(2.0f, 3.0f);
    PxRigidDynamic* pyramid = convexBuilder.createConvexActor(
        pyramidMesh, PxTransform(PxVec3(0, 10, 0)), material, 20.0f
    );
    scene->addActor(*pyramid);

    simulateFrames(180);

    // Pyramid should have fallen and settled on ground
    PxVec3 pyramidPos = pyramid->getGlobalPose().p;
    EXPECT_LT(pyramidPos.y, 5.0f);
    EXPECT_GT(pyramidPos.y, -1.0f);

    ground->release();
    groundMesh->release();
    pyramid->release();
    pyramidMesh->release();
}

TEST_F(IntegrationBasicTest, ComplexJointedStructure) {
    JointManager joints;
    MaterialLibrary materials;

    joints.initialize(physics, scene);
    materials.initialize(physics);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Create bridge structure
    const int bridgeLength = 10;
    std::vector<PxRigidDynamic*> planks;

    PxMaterial* wood = materials.getMaterial("Wood");

    for (int i = 0; i < bridgeLength; i++) {
        PxRigidDynamic* plank = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(i * 2.0f, 5, 0)),
            PxBoxGeometry(1.0f, 0.2f, 1.0f),
            *wood,
            10.0f
        );
        scene->addActor(*plank);
        planks.push_back(plank);
    }

    // Connect planks with revolute joints
    for (size_t i = 0; i < planks.size() - 1; i++) {
        joints.createRevoluteJoint(
            planks[i], PxTransform(PxVec3(1.0f, 0, 0)),
            planks[i + 1], PxTransform(PxVec3(-1.0f, 0, 0))
        );
    }

    // Anchor ends to static posts
    PxRigidStatic* post1 = PxCreateStatic(
        *physics,
        PxTransform(PxVec3(-1, 5, 0)),
        PxBoxGeometry(0.5f, 5.0f, 0.5f),
        *material
    );
    scene->addActor(*post1);

    PxRigidStatic* post2 = PxCreateStatic(
        *physics,
        PxTransform(PxVec3((bridgeLength - 1) * 2.0f + 1, 5, 0)),
        PxBoxGeometry(0.5f, 5.0f, 0.5f),
        *material
    );
    scene->addActor(*post2);

    joints.createRevoluteJoint(post1, PxTransform(PxVec3(0.5f, 0, 0)), planks[0], PxTransform(PxVec3(-1.0f, 0, 0)));
    joints.createRevoluteJoint(planks.back(), PxTransform(PxVec3(1.0f, 0, 0)), post2, PxTransform(PxVec3(-0.5f, 0, 0)));

    simulateFrames(120);

    // Bridge should still be connected and sagging
    for (auto* plank : planks) {
        PxVec3 pos = plank->getGlobalPose().p;
        EXPECT_LT(pos.y, 5.5f);  // Should have sagged
        EXPECT_GT(pos.y, 0.0f);  // But not fallen through ground
    }

    ground->release();
    post1->release();
    post2->release();
    for (auto* plank : planks) plank->release();
}

TEST_F(IntegrationBasicTest, MultipleArticulationsInteracting) {
    ArticulationManager articulation;
    articulation.initialize(physics, scene);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Create multiple ragdolls
    std::vector<PxArticulationReducedCoordinate*> ragdolls;

    for (int i = 0; i < 3; i++) {
        ArticulationManager::RagdollDesc desc;
        desc.position = PxVec3(i * 3.0f, 10, 0);
        desc.totalMass = 70.0f;

        PxArticulationReducedCoordinate* ragdoll = articulation.createRagdoll(desc);
        scene->addArticulation(*ragdoll);
        ragdolls.push_back(ragdoll);
    }

    simulateFrames(200);

    // All ragdolls should have fallen
    for (auto* ragdoll : ragdolls) {
        PxArticulationLink* root = ragdoll->getLinks()[0];
        EXPECT_LT(root->getGlobalPose().p.y, 10.0f);
    }

    ground->release();
    for (auto* ragdoll : ragdolls) ragdoll->release();
}

TEST_F(IntegrationBasicTest, FullPhysicsSceneWithAllFeatures) {
    // Initialize all managers
    MaterialLibrary materials;
    ConvexMeshBuilder convexBuilder;
    TriangleMeshBuilder triangleBuilder;
    RigidBodyContactHandler contacts;
    RigidBodyTrigger triggers;
    JointManager joints;
    ArticulationManager articulation;
    PerformanceProfiler profiler;
    DebugDrawer debugDrawer;

    materials.initialize(physics);
    convexBuilder.initialize(physics);
    triangleBuilder.initialize(physics);
    contacts.initialize(scene);
    triggers.initialize(physics, scene);
    joints.initialize(physics, scene);
    articulation.initialize(physics, scene);
    profiler.initialize(physics, scene);
    debugDrawer.initialize(scene);

    // Create terrain
    std::vector<std::vector<PxReal>> heightMap(15, std::vector<PxReal>(15));
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            heightMap[i][j] = sin(i * 0.3f) * cos(j * 0.3f) * 2.0f;
        }
    }

    PxTriangleMesh* terrain = triangleBuilder.createTerrainMesh(heightMap, 2.0f, 2.0f);
    PxRigidStatic* ground = triangleBuilder.createTriangleMeshActor(
        terrain, PxTransform(PxVec3(0, 0, 0)), material
    );
    scene->addActor(*ground);

    // Create trigger zone
    PxRigidStatic* triggerZone = triggers.createBoxTrigger(
        PxVec3(10, 5, 10),
        PxVec3(5, 5, 5)
    );

    int triggerCount = 0;
    triggers.setOnEnter([&triggerCount](PxActor*, PxActor*) {
        triggerCount++;
    });

    // Create convex objects with various materials
    PxMaterial* rubber = materials.getMaterial("Rubber");
    PxMaterial* metal = materials.getMaterial("Metal");

    PxConvexMesh* pyramidMesh = convexBuilder.createPyramidMesh(1.5f, 2.0f);

    PxRigidDynamic* rubberPyramid = convexBuilder.createConvexActor(
        pyramidMesh, PxTransform(PxVec3(5, 15, 5)), rubber, 15.0f
    );
    scene->addActor(*rubberPyramid);

    PxRigidDynamic* metalPyramid = convexBuilder.createConvexActor(
        pyramidMesh, PxTransform(PxVec3(15, 15, 15)), metal, 30.0f
    );
    scene->addActor(*metalPyramid);

    // Create articulated robot arm
    ArticulationManager::RobotArmDesc armDesc;
    armDesc.basePosition = PxVec3(0, 10, 0);
    armDesc.numSegments = 5;

    PxArticulationReducedCoordinate* robotArm = articulation.createRobotArm(armDesc);
    scene->addArticulation(*robotArm);

    // Create jointed chain
    std::vector<PxRigidDynamic*> chain;
    for (int i = 0; i < 5; i++) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(20, 10 - i * 1.5f, 0)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            10.0f
        );
        scene->addActor(*box);
        chain.push_back(box);

        if (i > 0) {
            joints.createSphericalJoint(
                chain[i - 1], PxTransform(PxVec3(0, -0.75f, 0)),
                chain[i], PxTransform(PxVec3(0, 0.75f, 0))
            );
        }
    }

    // Simulate with profiling
    for (int i = 0; i < 240; i++) {
        profiler.beginFrame();

        profiler.beginSimulation();
        scene->simulate(1.0f / 60.0f);
        profiler.endSimulation();

        profiler.beginFetch();
        scene->fetchResults(true);
        profiler.endFetch();

        // Draw debug info
        if (i % 30 == 0) {
            debugDrawer.drawAllActors();
            debugDrawer.drawVelocities();
        }

        profiler.endFrame();
    }

    // Verify simulation
    EXPECT_GT(profiler.getFrameCount(), 0u);

    // Get stats
    PerformanceProfiler::PerformanceStats stats = profiler.getStats();
    EXPECT_GT(stats.sceneStats.dynamicActors, 0u);
    EXPECT_GT(stats.avgFPS, 0.0f);

    // Verify trigger was activated
    EXPECT_GT(triggerCount, 0);

    // Cleanup
    ground->release();
    terrain->release();
    triggerZone->release();
    rubberPyramid->release();
    metalPyramid->release();
    pyramidMesh->release();
    robotArm->release();
    for (auto* box : chain) box->release();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
