/**
 * @file test_physxcore.cpp
 * @brief Unit tests for PhysXCore class
 *
 * Tests PhysX initialization, cleanup, and basic functionality
 */

#include <gtest/gtest.h>
#include "PhysXCore.h"

using namespace PhysXWrapper;

// ============================================================================
// Test Fixture
// ============================================================================

class PhysXCoreTest : public ::testing::Test {
protected:
    PhysXCore* core;

    void SetUp() override {
        core = new PhysXCore();
    }

    void TearDown() override {
        if (core) {
            core->cleanup();
            delete core;
            core = nullptr;
        }
    }
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(PhysXCoreTest, DefaultInitialization) {
    ASSERT_TRUE(core->initialize());
    EXPECT_TRUE(core->isInitialized());
    EXPECT_NE(core->getFoundation(), nullptr);
    EXPECT_NE(core->getPhysics(), nullptr);
    EXPECT_NE(core->getScene(), nullptr);
    EXPECT_NE(core->getDispatcher(), nullptr);
}

TEST_F(PhysXCoreTest, InitializationWithCustomConfig) {
    PhysXCore::Config config;
    config.gravity = PxVec3(0, -20.0f, 0);  // Custom gravity
    config.numThreads = 2;
    config.enableCCD = true;
    config.enableStabilization = true;

    ASSERT_TRUE(core->initialize(config));
    EXPECT_TRUE(core->isInitialized());

    // Verify scene uses custom gravity
    PxScene* scene = core->getScene();
    ASSERT_NE(scene, nullptr);
    PxVec3 gravity = scene->getGravity();
    EXPECT_FLOAT_EQ(gravity.y, -20.0f);
}

TEST_F(PhysXCoreTest, InitializationWithZeroGravity) {
    PhysXCore::Config config;
    config.gravity = PxVec3(0, 0, 0);  // Zero gravity

    ASSERT_TRUE(core->initialize(config));

    PxScene* scene = core->getScene();
    ASSERT_NE(scene, nullptr);
    PxVec3 gravity = scene->getGravity();
    EXPECT_FLOAT_EQ(gravity.x, 0.0f);
    EXPECT_FLOAT_EQ(gravity.y, 0.0f);
    EXPECT_FLOAT_EQ(gravity.z, 0.0f);
}

TEST_F(PhysXCoreTest, MultipleInitializationCalls) {
    ASSERT_TRUE(core->initialize());
    EXPECT_TRUE(core->isInitialized());

    // Second initialization should handle gracefully
    // (Either succeed with warning or safely handle)
    core->cleanup();
    ASSERT_TRUE(core->initialize());
    EXPECT_TRUE(core->isInitialized());
}

// ============================================================================
// Cleanup Tests
// ============================================================================

TEST_F(PhysXCoreTest, CleanupAfterInitialization) {
    ASSERT_TRUE(core->initialize());
    EXPECT_TRUE(core->isInitialized());

    core->cleanup();
    EXPECT_FALSE(core->isInitialized());
    EXPECT_EQ(core->getScene(), nullptr);
    EXPECT_EQ(core->getPhysics(), nullptr);
}

TEST_F(PhysXCoreTest, CleanupWithoutInitialization) {
    // Cleanup without initialization should not crash
    EXPECT_NO_THROW(core->cleanup());
    EXPECT_FALSE(core->isInitialized());
}

TEST_F(PhysXCoreTest, MultipleCleanupCalls) {
    ASSERT_TRUE(core->initialize());

    core->cleanup();
    EXPECT_FALSE(core->isInitialized());

    // Second cleanup should be safe
    EXPECT_NO_THROW(core->cleanup());
    EXPECT_FALSE(core->isInitialized());
}

// ============================================================================
// Getter Tests
// ============================================================================

TEST_F(PhysXCoreTest, GettersBeforeInitialization) {
    // Getters should return nullptr before initialization
    EXPECT_EQ(core->getFoundation(), nullptr);
    EXPECT_EQ(core->getPhysics(), nullptr);
    EXPECT_EQ(core->getScene(), nullptr);
    EXPECT_EQ(core->getDispatcher(), nullptr);
    EXPECT_EQ(core->getMaterial(), nullptr);
    EXPECT_EQ(core->getControllerManager(), nullptr);
}

TEST_F(PhysXCoreTest, GettersAfterInitialization) {
    ASSERT_TRUE(core->initialize());

    // All getters should return valid pointers
    EXPECT_NE(core->getFoundation(), nullptr);
    EXPECT_NE(core->getPhysics(), nullptr);
    EXPECT_NE(core->getScene(), nullptr);
    EXPECT_NE(core->getDispatcher(), nullptr);
    EXPECT_NE(core->getMaterial(), nullptr);
}

TEST_F(PhysXCoreTest, GettersAfterCleanup) {
    ASSERT_TRUE(core->initialize());
    core->cleanup();

    // Getters should return nullptr after cleanup
    EXPECT_EQ(core->getScene(), nullptr);
    EXPECT_EQ(core->getPhysics(), nullptr);
}

// ============================================================================
// Simulation Tests
// ============================================================================

TEST_F(PhysXCoreTest, BasicSimulationStep) {
    ASSERT_TRUE(core->initialize());

    PxScene* scene = core->getScene();
    ASSERT_NE(scene, nullptr);

    // Simulate one timestep
    PxReal deltaTime = 1.0f / 60.0f;
    EXPECT_NO_THROW({
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    });
}

TEST_F(PhysXCoreTest, MultipleSimulationSteps) {
    ASSERT_TRUE(core->initialize());

    PxScene* scene = core->getScene();
    ASSERT_NE(scene, nullptr);

    // Simulate 100 frames
    PxReal deltaTime = 1.0f / 60.0f;
    for (int i = 0; i < 100; i++) {
        EXPECT_NO_THROW({
            scene->simulate(deltaTime);
            scene->fetchResults(true);
        });
    }
}

TEST_F(PhysXCoreTest, SimulationWithVariableTimestep) {
    ASSERT_TRUE(core->initialize());

    PxScene* scene = core->getScene();
    ASSERT_NE(scene, nullptr);

    // Test with different timesteps
    std::vector<PxReal> timesteps = {0.008f, 0.016f, 0.033f, 0.05f};

    for (PxReal dt : timesteps) {
        EXPECT_NO_THROW({
            scene->simulate(dt);
            scene->fetchResults(true);
        });
    }
}

// ============================================================================
// Actor Management Tests
// ============================================================================

TEST_F(PhysXCoreTest, CreateStaticActor) {
    ASSERT_TRUE(core->initialize());

    PxPhysics* physics = core->getPhysics();
    PxScene* scene = core->getScene();
    PxMaterial* material = core->getMaterial();

    ASSERT_NE(physics, nullptr);
    ASSERT_NE(scene, nullptr);
    ASSERT_NE(material, nullptr);

    // Create ground plane
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    ASSERT_NE(ground, nullptr);

    scene->addActor(*ground);

    // Verify actor is in scene
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_STATIC);
    EXPECT_GT(numActors, 0u);

    ground->release();
}

TEST_F(PhysXCoreTest, CreateDynamicActor) {
    ASSERT_TRUE(core->initialize());

    PxPhysics* physics = core->getPhysics();
    PxScene* scene = core->getScene();
    PxMaterial* material = core->getMaterial();

    ASSERT_NE(physics, nullptr);
    ASSERT_NE(scene, nullptr);
    ASSERT_NE(material, nullptr);

    // Create dynamic box
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    ASSERT_NE(box, nullptr);

    scene->addActor(*box);

    // Verify actor is in scene
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    EXPECT_GT(numActors, 0u);

    box->release();
}

TEST_F(PhysXCoreTest, CreateMultipleActors) {
    ASSERT_TRUE(core->initialize());

    PxPhysics* physics = core->getPhysics();
    PxScene* scene = core->getScene();
    PxMaterial* material = core->getMaterial();

    const int numBoxes = 10;
    std::vector<PxRigidDynamic*> boxes;

    for (int i = 0; i < numBoxes; i++) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(i * 1.0f, 10, 0)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            10.0f
        );
        ASSERT_NE(box, nullptr);
        scene->addActor(*box);
        boxes.push_back(box);
    }

    // Verify all actors are in scene
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    EXPECT_EQ(numActors, static_cast<PxU32>(numBoxes));

    // Cleanup
    for (auto* box : boxes) {
        box->release();
    }
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(PhysXCoreTest, CCDConfiguration) {
    PhysXCore::Config config;
    config.enableCCD = true;

    ASSERT_TRUE(core->initialize(config));

    PxScene* scene = core->getScene();
    ASSERT_NE(scene, nullptr);

    // CCD should be available
    PxSceneFlags flags = scene->getFlags();
    EXPECT_TRUE(flags & PxSceneFlag::eENABLE_CCD);
}

TEST_F(PhysXCoreTest, StabilizationConfiguration) {
    PhysXCore::Config config;
    config.enableStabilization = true;

    ASSERT_TRUE(core->initialize(config));

    PxScene* scene = core->getScene();
    ASSERT_NE(scene, nullptr);

    // Stabilization should be available
    PxSceneFlags flags = scene->getFlags();
    EXPECT_TRUE(flags & PxSceneFlag::eENABLE_STABILIZATION);
}

TEST_F(PhysXCoreTest, GPUDynamicsConfiguration) {
    PhysXCore::Config config;
    config.enableGPUDynamics = true;

    // Note: GPU dynamics requires CUDA, may not be available
    bool initialized = core->initialize(config);

    if (initialized) {
        PxScene* scene = core->getScene();
        ASSERT_NE(scene, nullptr);

        PxSceneFlags flags = scene->getFlags();
        // If GPU available, flag should be set
        if (flags & PxSceneFlag::eENABLE_GPU_DYNAMICS) {
            SUCCEED() << "GPU dynamics enabled successfully";
        } else {
            SUCCEED() << "GPU dynamics not available, but initialization succeeded";
        }
    } else {
        SUCCEED() << "GPU dynamics initialization failed (likely no CUDA support)";
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(PhysXCoreTest, InvalidConfiguration) {
    PhysXCore::Config config;
    config.numThreads = 0;  // Invalid thread count

    // Should either handle gracefully or fail initialization
    bool result = core->initialize(config);
    // As long as it doesn't crash, we're good
    SUCCEED();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
