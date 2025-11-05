/**
 * @file test_integration_performance.cpp
 * @brief Performance integration tests for PhysXWrapper
 *
 * Tests performance characteristics and scalability
 */

#include <gtest/gtest.h>
#include "PhysXCore.h"
#include "Utility/PerformanceProfiler.h"
#include "Query/BVHBuilder.h"
#include "Geometry/GeometryQuery.h"
#include "RigidBody/AggregateManager.h"
#include <chrono>

using namespace PhysXWrapper;

// ============================================================================
// Test Fixture
// ============================================================================

class IntegrationPerformanceTest : public ::testing::Test {
protected:
    PhysXCore* core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    void SetUp() override {
        core = new PhysXCore();
        PhysXCore::Config config;
        config.numThreads = 4;  // Use multiple threads for performance
        core->initialize(config);
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

    PxRigidDynamic* createBox(const PxVec3& position) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(position),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            10.0f
        );
        scene->addActor(*box);
        return box;
    }

    double simulateAndMeasure(int numFrames, PxReal dt = 1.0f / 60.0f) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < numFrames; i++) {
            scene->simulate(dt);
            scene->fetchResults(true);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        return duration.count();
    }
};

// ============================================================================
// Actor Count Performance Tests
// ============================================================================

TEST_F(IntegrationPerformanceTest, FewActors_Performance) {
    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Create 10 actors
    for (int i = 0; i < 10; i++) {
        createBox(PxVec3(i * 1.5f, 10, 0));
    }

    // Measure performance
    for (int i = 0; i < 120; i++) {
        profiler.beginFrame();
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
        profiler.endFrame();
    }

    PerformanceProfiler::PerformanceStats stats = profiler.getStats();

    // With few actors, should run fast
    EXPECT_GT(stats.avgFPS, 100.0f);  // Should be well above 100 FPS
    EXPECT_LT(stats.avgFrameTime, 10.0f);  // Should be under 10ms per frame

    ground->release();
}

TEST_F(IntegrationPerformanceTest, ManyActors_Performance) {
    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Create 500 actors in a grid
    for (int x = 0; x < 25; x++) {
        for (int z = 0; z < 20; z++) {
            createBox(PxVec3(x * 1.5f, 10 + (x + z) * 0.1f, z * 1.5f));
        }
    }

    // Measure performance
    for (int i = 0; i < 120; i++) {
        profiler.beginFrame();
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
        profiler.endFrame();
    }

    PerformanceProfiler::PerformanceStats stats = profiler.getStats();

    // With many actors, still should maintain real-time performance
    EXPECT_GT(stats.avgFPS, 30.0f);  // Should maintain at least 30 FPS
    EXPECT_LT(stats.avgFrameTime, 33.3f);  // Should be under 33.3ms (30 FPS)

    ground->release();
}

TEST_F(IntegrationPerformanceTest, MassiveActorCount_ScalabilityTest) {
    // Test with very large number of actors (stress test)
    // Create ground
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Create 1000 actors
    std::vector<PxRigidDynamic*> actors;
    for (int i = 0; i < 1000; i++) {
        PxVec3 pos(
            (i % 50) * 1.0f,
            10 + (i / 50) * 2.0f,
            (i / 50) * 1.0f
        );
        actors.push_back(createBox(pos));
    }

    // Measure time for 60 frames
    double duration = simulateAndMeasure(60);

    // Calculate FPS
    double avgFrameTime = duration / 60.0;
    double fps = 1000.0 / avgFrameTime;

    std::cout << "1000 actors: " << fps << " FPS, " << avgFrameTime << " ms/frame" << std::endl;

    // Should still be able to simulate (even if not real-time)
    EXPECT_GT(fps, 5.0);  // At least 5 FPS with 1000 actors

    ground->release();
    for (auto* actor : actors) actor->release();
}

// ============================================================================
// Aggregate Performance Tests
// ============================================================================

TEST_F(IntegrationPerformanceTest, AggregateVsNoAggregate_Performance) {
    AggregateManager aggregateMgr;
    aggregateMgr.initialize(physics, scene);

    // Test without aggregate
    {
        PhysXCore testCore;
        testCore.initialize();
        PxScene* testScene = testCore.getScene();
        PxPhysics* testPhysics = testCore.getPhysics();

        // Create 200 actors
        for (int i = 0; i < 200; i++) {
            PxRigidDynamic* box = PxCreateDynamic(
                *testPhysics,
                PxTransform(PxVec3((i % 20) * 1.5f, 10, (i / 20) * 1.5f)),
                PxBoxGeometry(0.5f, 0.5f, 0.5f),
                *material,
                10.0f
            );
            testScene->addActor(*box);
        }

        double timeWithoutAggregate = 0;
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 60; i++) {
            testScene->simulate(1.0f / 60.0f);
            testScene->fetchResults(true);
        }
        auto end = std::chrono::high_resolution_clock::now();
        timeWithoutAggregate = std::chrono::duration<double, std::milli>(end - start).count();

        std::cout << "Without aggregate: " << timeWithoutAggregate << " ms" << std::endl;

        testCore.cleanup();
    }

    // Test with aggregate
    {
        PhysXCore testCore;
        testCore.initialize();
        PxScene* testScene = testCore.getScene();
        PxPhysics* testPhysics = testCore.getPhysics();

        AggregateManager testAggMgr;
        testAggMgr.initialize(testPhysics, testScene);

        // Create aggregate for 200 actors
        PxAggregate* aggregate = testAggMgr.createAggregate(200, false);

        // Add 200 actors to aggregate
        for (int i = 0; i < 200; i++) {
            PxRigidDynamic* box = PxCreateDynamic(
                *testPhysics,
                PxTransform(PxVec3((i % 20) * 1.5f, 10, (i / 20) * 1.5f)),
                PxBoxGeometry(0.5f, 0.5f, 0.5f),
                *material,
                10.0f
            );
            testAggMgr.addActorToAggregate(aggregate, box);
        }

        testAggMgr.addAggregateToScene(aggregate);

        double timeWithAggregate = 0;
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 60; i++) {
            testScene->simulate(1.0f / 60.0f);
            testScene->fetchResults(true);
        }
        auto end = std::chrono::high_resolution_clock::now();
        timeWithAggregate = std::chrono::duration<double, std::milli>(end - start).count();

        std::cout << "With aggregate: " << timeWithAggregate << " ms" << std::endl;

        // Aggregate should provide performance benefit (or at least not be slower)
        // Note: Benefit depends on scene configuration
        EXPECT_LT(timeWithAggregate, timeWithoutAggregate * 1.5);  // Allow some tolerance

        testCore.cleanup();
    }
}

// ============================================================================
// Query Performance Tests
// ============================================================================

TEST_F(IntegrationPerformanceTest, BVHQueryPerformance) {
    BVHBuilder bvhBuilder;
    GeometryQuery query;

    bvhBuilder.initialize(physics, scene);
    query.initialize(scene);

    // Create large grid of actors
    std::vector<PxRigidActor*> actors;
    for (int x = 0; x < 50; x++) {
        for (int z = 0; z < 50; z++) {
            actors.push_back(createBox(PxVec3(x * 2.0f, 0, z * 2.0f)));
        }
    }

    // Build BVH
    auto bvhStart = std::chrono::high_resolution_clock::now();
    PxBVH* bvh = bvhBuilder.buildBVHFromActors(actors);
    auto bvhEnd = std::chrono::high_resolution_clock::now();

    double bvhBuildTime = std::chrono::duration<double, std::milli>(bvhEnd - bvhStart).count();
    std::cout << "BVH build time for 2500 actors: " << bvhBuildTime << " ms" << std::endl;

    EXPECT_LT(bvhBuildTime, 1000.0);  // Should build in under 1 second

    // Test query performance with BVH
    auto queryStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        PxVec3 origin(25 + i * 0.5f, 10, 25);
        PxVec3 direction(0, -1, 0);
        std::vector<PxU32> hits = bvhBuilder.raycastBVH(bvh, origin, direction, 20.0f);
    }
    auto queryEnd = std::chrono::high_resolution_clock::now();

    double queryTime = std::chrono::duration<double, std::milli>(queryEnd - queryStart).count();
    std::cout << "100 BVH raycasts: " << queryTime << " ms" << std::endl;

    EXPECT_LT(queryTime, 100.0);  // Should complete in under 100ms

    bvh->release();
    for (auto* actor : actors) actor->release();
}

TEST_F(IntegrationPerformanceTest, MassiveRaycastPerformance) {
    GeometryQuery query;
    query.initialize(scene);

    // Create obstacle field
    for (int x = 0; x < 20; x++) {
        for (int z = 0; z < 20; z++) {
            createBox(PxVec3(x * 3.0f, 1, z * 3.0f));
        }
    }

    // Perform many raycasts
    auto start = std::chrono::high_resolution_clock::now();

    int hitCount = 0;
    for (int i = 0; i < 1000; i++) {
        PxVec3 origin(
            (i % 100) * 0.6f,
            10,
            (i / 100) * 12.0f
        );
        PxVec3 direction(0, -1, 0);

        PxRaycastHit hit;
        if (query.raycast(origin, direction, 20.0f, hit)) {
            hitCount++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "1000 raycasts: " << duration << " ms, " << hitCount << " hits" << std::endl;

    EXPECT_LT(duration, 500.0);  // Should complete in under 500ms
    EXPECT_GT(hitCount, 0);
}

// ============================================================================
// Memory Performance Tests
// ============================================================================

TEST_F(IntegrationPerformanceTest, MemoryUsageTracking) {
    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    // Get initial memory
    profiler.beginFrame();
    scene->simulate(1.0f / 60.0f);
    scene->fetchResults(true);
    profiler.endFrame();

    PerformanceProfiler::MemoryStats initialMemory = profiler.getMemoryStats();

    // Create many actors
    for (int i = 0; i < 500; i++) {
        createBox(PxVec3((i % 25) * 2.0f, 10, (i / 25) * 2.0f));
    }

    // Measure memory after adding actors
    profiler.beginFrame();
    scene->simulate(1.0f / 60.0f);
    scene->fetchResults(true);
    profiler.endFrame();

    PerformanceProfiler::MemoryStats finalMemory = profiler.getMemoryStats();

    // Memory should have increased
    EXPECT_GT(finalMemory.totalBytes, initialMemory.totalBytes);

    std::cout << "Initial memory: " << initialMemory.getTotalMB() << " MB" << std::endl;
    std::cout << "Final memory: " << finalMemory.getTotalMB() << " MB" << std::endl;
    std::cout << "Memory increase: " << (finalMemory.getTotalMB() - initialMemory.getTotalMB()) << " MB" << std::endl;
}

// ============================================================================
// Simulation Timestep Performance Tests
// ============================================================================

TEST_F(IntegrationPerformanceTest, VariableTimestep_Performance) {
    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    // Create scene
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    for (int i = 0; i < 100; i++) {
        createBox(PxVec3((i % 10) * 2.0f, 10, (i / 10) * 2.0f));
    }

    // Test different timesteps
    std::vector<PxReal> timesteps = {1.0f / 30.0f, 1.0f / 60.0f, 1.0f / 120.0f};

    for (PxReal dt : timesteps) {
        profiler.resetStats();

        for (int i = 0; i < 60; i++) {
            profiler.beginFrame();
            scene->simulate(dt);
            scene->fetchResults(true);
            profiler.endFrame();
        }

        PerformanceProfiler::PerformanceStats stats = profiler.getStats();

        std::cout << "Timestep " << dt << ": " << stats.avgFrameTime << " ms/frame" << std::endl;

        // Smaller timestep should take roughly proportional time
        EXPECT_GT(stats.avgFrameTime, 0.0f);
    }

    ground->release();
}

// ============================================================================
// Profiler Overhead Tests
// ============================================================================

TEST_F(IntegrationPerformanceTest, ProfilerOverhead) {
    // Test without profiler
    double timeWithoutProfiler = 0;
    {
        PhysXCore testCore;
        testCore.initialize();
        PxScene* testScene = testCore.getScene();
        PxPhysics* testPhysics = testCore.getPhysics();

        // Create actors
        for (int i = 0; i < 100; i++) {
            PxRigidDynamic* box = PxCreateDynamic(
                *testPhysics,
                PxTransform(PxVec3((i % 10) * 2.0f, 10, (i / 10) * 2.0f)),
                PxBoxGeometry(0.5f, 0.5f, 0.5f),
                *material,
                10.0f
            );
            testScene->addActor(*box);
        }

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 120; i++) {
            testScene->simulate(1.0f / 60.0f);
            testScene->fetchResults(true);
        }
        auto end = std::chrono::high_resolution_clock::now();

        timeWithoutProfiler = std::chrono::duration<double, std::milli>(end - start).count();

        testCore.cleanup();
    }

    // Test with profiler
    double timeWithProfiler = 0;
    {
        PhysXCore testCore;
        testCore.initialize();
        PxScene* testScene = testCore.getScene();
        PxPhysics* testPhysics = testCore.getPhysics();

        PerformanceProfiler profiler;
        profiler.initialize(testPhysics, testScene);

        // Create actors
        for (int i = 0; i < 100; i++) {
            PxRigidDynamic* box = PxCreateDynamic(
                *testPhysics,
                PxTransform(PxVec3((i % 10) * 2.0f, 10, (i / 10) * 2.0f)),
                PxBoxGeometry(0.5f, 0.5f, 0.5f),
                *material,
                10.0f
            );
            testScene->addActor(*box);
        }

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 120; i++) {
            profiler.beginFrame();
            testScene->simulate(1.0f / 60.0f);
            testScene->fetchResults(true);
            profiler.endFrame();
        }
        auto end = std::chrono::high_resolution_clock::now();

        timeWithProfiler = std::chrono::duration<double, std::milli>(end - start).count();

        testCore.cleanup();
    }

    std::cout << "Without profiler: " << timeWithoutProfiler << " ms" << std::endl;
    std::cout << "With profiler: " << timeWithProfiler << " ms" << std::endl;
    double overhead = ((timeWithProfiler - timeWithoutProfiler) / timeWithoutProfiler) * 100.0;
    std::cout << "Profiler overhead: " << overhead << "%" << std::endl;

    // Profiler overhead should be minimal (< 10%)
    EXPECT_LT(overhead, 10.0);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
