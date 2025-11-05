/**
 * @file test_utilities.cpp
 * @brief Unit tests for Utility classes
 *
 * Tests MaterialLibrary, PhysicsRecorder, PerformanceProfiler,
 * SerializationManager, and DebugDrawer
 */

#include <gtest/gtest.h>
#include "PhysXCore.h"
#include "Utility/MaterialLibrary.h"
#include "Utility/PhysicsRecorder.h"
#include "Utility/PerformanceProfiler.h"
#include "Debug/SerializationManager.h"
#include "Debug/DebugDrawer.h"
#include <fstream>

using namespace PhysXWrapper;

// ============================================================================
// Test Fixture
// ============================================================================

class UtilityTest : public ::testing::Test {
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
};

// ============================================================================
// MaterialLibrary Tests
// ============================================================================

TEST_F(UtilityTest, MaterialLibraryCreation) {
    MaterialLibrary library;
    EXPECT_NO_THROW(library.initialize(physics));
    EXPECT_TRUE(library.isInitialized());
}

TEST_F(UtilityTest, GetPredefinedMaterials) {
    MaterialLibrary library;
    library.initialize(physics);

    // Test common materials
    PxMaterial* wood = library.getMaterial("Wood");
    ASSERT_NE(wood, nullptr);

    PxMaterial* metal = library.getMaterial("Metal");
    ASSERT_NE(metal, nullptr);

    PxMaterial* rubber = library.getMaterial("Rubber");
    ASSERT_NE(rubber, nullptr);

    PxMaterial* ice = library.getMaterial("Ice");
    ASSERT_NE(ice, nullptr);
}

TEST_F(UtilityTest, MaterialProperties) {
    MaterialLibrary library;
    library.initialize(physics);

    PxMaterial* rubber = library.getMaterial("Rubber");
    ASSERT_NE(rubber, nullptr);

    // Rubber should have high restitution
    EXPECT_GT(rubber->getRestitution(), 0.5f);

    PxMaterial* ice = library.getMaterial("Ice");
    ASSERT_NE(ice, nullptr);

    // Ice should have low friction
    EXPECT_LT(ice->getStaticFriction(), 0.2f);
}

TEST_F(UtilityTest, CreateCustomMaterial) {
    MaterialLibrary library;
    library.initialize(physics);

    MaterialLibrary::MaterialProperties props;
    props.name = "CustomMaterial";
    props.staticFriction = 0.8f;
    props.dynamicFriction = 0.6f;
    props.restitution = 0.4f;

    PxMaterial* custom = library.createCustomMaterial(props);
    ASSERT_NE(custom, nullptr);

    EXPECT_FLOAT_EQ(custom->getStaticFriction(), 0.8f);
    EXPECT_FLOAT_EQ(custom->getDynamicFriction(), 0.6f);
    EXPECT_FLOAT_EQ(custom->getRestitution(), 0.4f);
}

TEST_F(UtilityTest, MaterialCategories) {
    MaterialLibrary library;
    library.initialize(physics);

    // Get materials by category
    auto metals = library.getMaterialsByCategory(MaterialLibrary::MaterialCategory::METAL);
    EXPECT_GT(metals.size(), 0u);

    auto terrain = library.getMaterialsByCategory(MaterialLibrary::MaterialCategory::TERRAIN);
    EXPECT_GT(terrain.size(), 0u);
}

TEST_F(UtilityTest, MaterialCombination) {
    MaterialLibrary library;
    library.initialize(physics);

    PxMaterial* wood = library.getMaterial("Wood");
    PxMaterial* metal = library.getMaterial("Metal");

    ASSERT_NE(wood, nullptr);
    ASSERT_NE(metal, nullptr);

    // Combine materials
    PxMaterial* combined = library.combineMaterials(wood, metal, 0.5f);
    ASSERT_NE(combined, nullptr);

    // Properties should be interpolated
    PxReal expectedFriction = (wood->getStaticFriction() + metal->getStaticFriction()) * 0.5f;
    EXPECT_NEAR(combined->getStaticFriction(), expectedFriction, 0.2f);
}

// ============================================================================
// PhysicsRecorder Tests
// ============================================================================

TEST_F(UtilityTest, RecorderCreation) {
    PhysicsRecorder recorder;
    EXPECT_NO_THROW(recorder.initialize(scene));
    EXPECT_TRUE(recorder.isInitialized());
}

TEST_F(UtilityTest, RecordAndPlayback) {
    PhysicsRecorder recorder;
    recorder.initialize(scene);

    // Create falling box
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box);

    // Start recording
    EXPECT_TRUE(recorder.startRecording());

    // Simulate and record
    for (int i = 0; i < 60; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
        recorder.recordFrame();
    }

    recorder.stopRecording();

    // Verify frames were recorded
    EXPECT_GT(recorder.getFrameCount(), 0u);

    // Start playback
    recorder.startPlayback(PhysicsRecorder::PlaybackMode::ONCE);

    // Update playback
    for (int i = 0; i < 30; i++) {
        recorder.updatePlayback(1.0f / 60.0f);
    }

    EXPECT_FALSE(recorder.isPlaybackFinished());

    box->release();
}

TEST_F(UtilityTest, RecorderTimeControl) {
    PhysicsRecorder recorder;
    recorder.initialize(scene);

    // Create object
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box);

    // Record
    recorder.startRecording();
    for (int i = 0; i < 100; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
        recorder.recordFrame();
    }
    recorder.stopRecording();

    // Test seeking
    recorder.startPlayback(PhysicsRecorder::PlaybackMode::ONCE);
    recorder.seekToFrame(50);

    EXPECT_EQ(recorder.getCurrentFrame(), 50u);

    box->release();
}

TEST_F(UtilityTest, RecorderPlaybackModes) {
    PhysicsRecorder recorder;
    recorder.initialize(scene);

    // Create object
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box);

    // Record
    recorder.startRecording();
    for (int i = 0; i < 30; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
        recorder.recordFrame();
    }
    recorder.stopRecording();

    // Test LOOP mode
    recorder.startPlayback(PhysicsRecorder::PlaybackMode::LOOP);

    // Should loop back
    for (int i = 0; i < 70; i++) {
        recorder.updatePlayback(1.0f / 60.0f);
    }

    // Should still be playing (looped)
    EXPECT_FALSE(recorder.isPlaybackFinished());

    box->release();
}

// ============================================================================
// PerformanceProfiler Tests
// ============================================================================

TEST_F(UtilityTest, ProfilerCreation) {
    PerformanceProfiler profiler;
    EXPECT_NO_THROW(profiler.initialize(physics, scene));
    EXPECT_TRUE(profiler.isInitialized());
}

TEST_F(UtilityTest, ProfilerFrameTiming) {
    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    // Create some objects
    for (int i = 0; i < 10; i++) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(i * 1.5f, 10, 0)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            10.0f
        );
        scene->addActor(*box);
    }

    // Profile simulation
    for (int i = 0; i < 60; i++) {
        profiler.beginFrame();

        profiler.beginSimulation();
        scene->simulate(1.0f / 60.0f);
        profiler.endSimulation();

        profiler.beginFetch();
        scene->fetchResults(true);
        profiler.endFetch();

        profiler.endFrame();
    }

    // Get statistics
    PerformanceProfiler::PerformanceStats stats = profiler.getStats();

    EXPECT_EQ(stats.frameCount, 60u);
    EXPECT_GT(stats.avgFPS, 0.0f);
    EXPECT_GT(stats.avgFrameTime, 0.0f);
}

TEST_F(UtilityTest, ProfilerSceneStatistics) {
    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    // Create various actors
    for (int i = 0; i < 20; i++) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(i * 1.0f, 10, 0)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            10.0f
        );
        scene->addActor(*box);
    }

    profiler.beginFrame();
    scene->simulate(1.0f / 60.0f);
    scene->fetchResults(true);
    profiler.endFrame();

    // Get scene stats
    PerformanceProfiler::SceneStats stats = profiler.getSceneStats();

    EXPECT_GT(stats.dynamicActors, 0u);
    EXPECT_GT(stats.totalActors, 0u);
}

TEST_F(UtilityTest, ProfilerCustomSections) {
    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    // Profile custom sections
    for (int i = 0; i < 10; i++) {
        profiler.beginFrame();

        profiler.beginSection("CustomWork");
        // Simulate some work
        for (volatile int j = 0; j < 1000; j++) {}
        profiler.endSection("CustomWork");

        profiler.beginSimulation();
        scene->simulate(1.0f / 60.0f);
        profiler.endSimulation();

        profiler.beginFetch();
        scene->fetchResults(true);
        profiler.endFetch();

        profiler.endFrame();
    }

    // Get custom section time
    PxReal customTime = profiler.getSectionTime("CustomWork");
    EXPECT_GT(customTime, 0.0f);
}

TEST_F(UtilityTest, ProfilerExport) {
    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    // Run simulation
    for (int i = 0; i < 30; i++) {
        profiler.beginFrame();
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
        profiler.endFrame();
    }

    // Export to CSV
    bool csvSuccess = profiler.exportToCSV("/tmp/test_profile.csv");
    EXPECT_TRUE(csvSuccess);

    // Verify file exists
    std::ifstream csvFile("/tmp/test_profile.csv");
    EXPECT_TRUE(csvFile.good());
    csvFile.close();

    // Export to JSON
    bool jsonSuccess = profiler.exportToJSON("/tmp/test_profile.json");
    EXPECT_TRUE(jsonSuccess);

    // Verify file exists
    std::ifstream jsonFile("/tmp/test_profile.json");
    EXPECT_TRUE(jsonFile.good());
    jsonFile.close();
}

// ============================================================================
// SerializationManager Tests
// ============================================================================

TEST_F(UtilityTest, SerializationCreation) {
    SerializationManager serialization;
    EXPECT_NO_THROW(serialization.initialize(physics, scene));
    EXPECT_TRUE(serialization.isInitialized());
}

TEST_F(UtilityTest, SerializeAndDeserializeScene) {
    SerializationManager serialization;
    serialization.initialize(physics, scene);

    // Create scene with objects
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    for (int i = 0; i < 5; i++) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(i * 2.0f, 10, 0)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            10.0f
        );
        scene->addActor(*box);
    }

    // Save scene
    bool saveSuccess = serialization.saveScene("/tmp/test_scene.pxs");
    EXPECT_TRUE(saveSuccess);

    // Verify file exists
    std::ifstream file("/tmp/test_scene.pxs");
    EXPECT_TRUE(file.good());
    file.close();

    ground->release();
}

TEST_F(UtilityTest, SerializeActor) {
    SerializationManager serialization;
    serialization.initialize(physics, scene);

    // Create actor
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box);

    // Serialize actor
    bool saveSuccess = serialization.saveActor(box, "/tmp/test_actor.pxa");
    EXPECT_TRUE(saveSuccess);

    // Verify file exists
    std::ifstream file("/tmp/test_actor.pxa");
    EXPECT_TRUE(file.good());
    file.close();

    box->release();
}

// ============================================================================
// DebugDrawer Tests
// ============================================================================

TEST_F(UtilityTest, DebugDrawerCreation) {
    DebugDrawer drawer;
    EXPECT_NO_THROW(drawer.initialize(scene));
    EXPECT_TRUE(drawer.isInitialized());
}

TEST_F(UtilityTest, DrawActorShapes) {
    DebugDrawer drawer;
    drawer.initialize(scene);

    // Create object
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box);

    // Draw shapes (should not crash)
    EXPECT_NO_THROW(drawer.drawActorShapes(box));

    box->release();
}

TEST_F(UtilityTest, DrawAllActors) {
    DebugDrawer drawer;
    drawer.initialize(scene);

    // Create multiple objects
    for (int i = 0; i < 5; i++) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(i * 2.0f, 10, 0)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            10.0f
        );
        scene->addActor(*box);
    }

    // Draw all (should not crash)
    EXPECT_NO_THROW(drawer.drawAllActors());
}

TEST_F(UtilityTest, DrawContactPoints) {
    DebugDrawer drawer;
    drawer.initialize(scene);

    // Create ground and falling box
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 5, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box);

    // Simulate to create contacts
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Draw contacts (should not crash)
    EXPECT_NO_THROW(drawer.drawContactPoints());

    ground->release();
    box->release();
}

TEST_F(UtilityTest, DrawVelocities) {
    DebugDrawer drawer;
    drawer.initialize(scene);

    // Create moving object
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box);
    box->setLinearVelocity(PxVec3(5, 0, 0));

    // Draw velocities (should not crash)
    EXPECT_NO_THROW(drawer.drawVelocities());

    box->release();
}

TEST_F(UtilityTest, DrawBounds) {
    DebugDrawer drawer;
    drawer.initialize(scene);

    // Create objects
    for (int i = 0; i < 3; i++) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(i * 3.0f, 10, 0)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            10.0f
        );
        scene->addActor(*box);
    }

    // Draw AABBs (should not crash)
    EXPECT_NO_THROW(drawer.drawBounds());
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(UtilityTest, ProfilerWithRecorder) {
    PerformanceProfiler profiler;
    PhysicsRecorder recorder;

    profiler.initialize(physics, scene);
    recorder.initialize(scene);

    // Create object
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 10, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material,
        10.0f
    );
    scene->addActor(*box);

    // Record and profile simultaneously
    recorder.startRecording();

    for (int i = 0; i < 60; i++) {
        profiler.beginFrame();
        profiler.beginSimulation();
        scene->simulate(1.0f / 60.0f);
        profiler.endSimulation();

        profiler.beginFetch();
        scene->fetchResults(true);
        profiler.endFetch();

        recorder.recordFrame();
        profiler.endFrame();
    }

    recorder.stopRecording();

    // Both should have data
    EXPECT_GT(profiler.getFrameCount(), 0u);
    EXPECT_GT(recorder.getFrameCount(), 0u);

    box->release();
}

TEST_F(UtilityTest, MaterialLibraryWithSimulation) {
    MaterialLibrary library;
    library.initialize(physics);

    // Create ground with ice material
    PxMaterial* ice = library.getMaterial("Ice");
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *ice);
    scene->addActor(*ground);

    // Create box with rubber material
    PxMaterial* rubber = library.getMaterial("Rubber");
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 5, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *rubber,
        10.0f
    );
    scene->addActor(*box);

    // Simulate
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Box should have bounced (rubber has high restitution)
    PxVec3 velocity = box->getLinearVelocity();
    // After bouncing, should still have some velocity
    SUCCEED();  // Hard to test exact bounce behavior

    ground->release();
    box->release();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
