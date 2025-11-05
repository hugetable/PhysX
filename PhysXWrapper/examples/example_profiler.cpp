/**
 * @file example_profiler.cpp
 * @brief Example demonstrating PerformanceProfiler usage
 *
 * This example shows how to use the PerformanceProfiler class for
 * comprehensive performance monitoring and analysis of physics simulations.
 */

#include "PhysXCore.h"
#include "Utility/PerformanceProfiler.h"
#include <iostream>

using namespace PhysXWrapper;

// ============================================================================
// Helper Functions
// ============================================================================

void printSeparator(const std::string& title)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================\n" << std::endl;
}

void createGroundAndWalls(PxPhysics* physics, PxScene* scene)
{
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.5f);

    // Ground
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Walls
    PxRigidStatic* wall1 = PxCreateStatic(*physics,
                                           PxTransform(PxVec3(10, 5, 0)),
                                           PxBoxGeometry(0.5f, 5.0f, 10.0f),
                                           *material);
    scene->addActor(*wall1);

    PxRigidStatic* wall2 = PxCreateStatic(*physics,
                                           PxTransform(PxVec3(-10, 5, 0)),
                                           PxBoxGeometry(0.5f, 5.0f, 10.0f),
                                           *material);
    scene->addActor(*wall2);
}

void createFallingBoxes(PxPhysics* physics, PxScene* scene, int count)
{
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    for (int i = 0; i < count; i++) {
        PxReal x = (i % 10) * 1.5f - 5.0f;
        PxReal y = 10.0f + (i / 10) * 1.5f;
        PxReal z = 0.0f;

        PxRigidDynamic* box = PxCreateDynamic(*physics,
                                               PxTransform(PxVec3(x, y, z)),
                                               PxBoxGeometry(0.5f, 0.5f, 0.5f),
                                               *material, 10.0f);
        scene->addActor(*box);
    }
}

// ============================================================================
// Test 1: Basic Profiling
// ============================================================================

void test_BasicProfiling()
{
    printSeparator("Test 1: Basic Profiling");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGroundAndWalls(physics, scene);
    createFallingBoxes(physics, scene, 50);

    // Initialize profiler
    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    std::cout << "Running simulation with profiling..." << std::endl;

    PxReal deltaTime = 0.016f; // 60 FPS

    // Simulate 300 frames
    for (int i = 0; i < 300; i++) {
        profiler.beginFrame();

        profiler.beginSimulation();
        scene->simulate(deltaTime);
        profiler.endSimulation();

        profiler.beginFetch();
        scene->fetchResults(true);
        profiler.endFetch();

        profiler.endFrame();

        // Print summary every 60 frames
        if ((i + 1) % 60 == 0) {
            profiler.printSummary();
        }
    }

    // Final report
    std::cout << "\n";
    profiler.printDetailedReport();

    core.cleanup();
}

// ============================================================================
// Test 2: Performance Statistics
// ============================================================================

void test_PerformanceStatistics()
{
    printSeparator("Test 2: Performance Statistics");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGroundAndWalls(physics, scene);
    createFallingBoxes(physics, scene, 100);

    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    std::cout << "Simulating 200 frames..." << std::endl;

    for (int i = 0; i < 200; i++) {
        profiler.beginFrame();

        profiler.beginSimulation();
        scene->simulate(0.016f);
        profiler.endSimulation();

        profiler.beginFetch();
        scene->fetchResults(true);
        profiler.endFetch();

        profiler.endFrame();
    }

    // Get statistics
    PerformanceProfiler::PerformanceStats stats = profiler.getStats();

    std::cout << "\nPerformance Statistics:" << std::endl;
    stats.print();

    std::cout << "\nScene Statistics:" << std::endl;
    PerformanceProfiler::SceneStats sceneStats = profiler.getSceneStats();
    sceneStats.print();

    std::cout << "\nMemory Statistics:" << std::endl;
    PerformanceProfiler::MemoryStats memStats = profiler.getMemoryStats();
    memStats.print();

    core.cleanup();
}

// ============================================================================
// Test 3: Custom Timing Sections
// ============================================================================

void test_CustomTimingSections()
{
    printSeparator("Test 3: Custom Timing Sections");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGroundAndWalls(physics, scene);

    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    std::cout << "Testing custom timing sections..." << std::endl;

    for (int i = 0; i < 100; i++) {
        profiler.beginFrame();

        // Custom section: Object creation
        profiler.beginSection("ObjectCreation");
        if (i % 10 == 0) {
            createFallingBoxes(physics, scene, 5);
        }
        profiler.endSection("ObjectCreation");

        // Custom section: Simulation
        profiler.beginSection("Simulation");
        profiler.beginSimulation();
        scene->simulate(0.016f);
        profiler.endSimulation();
        profiler.endSection("Simulation");

        // Custom section: Fetch
        profiler.beginSection("Fetch");
        profiler.beginFetch();
        scene->fetchResults(true);
        profiler.endFetch();
        profiler.endSection("Fetch");

        profiler.endFrame();
    }

    // Print custom section times
    std::cout << "\nCustom Section Times:" << std::endl;
    std::cout << "  ObjectCreation: " << profiler.getSectionTime("ObjectCreation") << " ms" << std::endl;
    std::cout << "  Simulation: " << profiler.getSectionTime("Simulation") << " ms" << std::endl;
    std::cout << "  Fetch: " << profiler.getSectionTime("Fetch") << " ms" << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 4: Performance Warnings
// ============================================================================

void test_PerformanceWarnings()
{
    printSeparator("Test 4: Performance Warnings");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGroundAndWalls(physics, scene);

    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    // Configure warnings
    PerformanceProfiler::Config config;
    config.warningFrameTimeMs = 20.0f;  // Low threshold to trigger warnings
    config.warningActorCount = 100;      // Low threshold
    config.autoDetectBottlenecks = true;
    profiler.setConfig(config);

    std::cout << "Creating heavy scene to trigger warnings..." << std::endl;

    for (int i = 0; i < 150; i++) {
        profiler.beginFrame();

        // Gradually add actors
        if (i % 5 == 0) {
            createFallingBoxes(physics, scene, 10);
        }

        profiler.beginSimulation();
        scene->simulate(0.016f);
        profiler.endSimulation();

        profiler.beginFetch();
        scene->fetchResults(true);
        profiler.endFetch();

        profiler.endFrame();
    }

    // Print warnings
    profiler.printWarnings();

    // Check for bottlenecks
    if (profiler.hasBottlenecks()) {
        std::cout << "\nBottlenecks Detected:" << std::endl;
        std::cout << profiler.getBottleneckDescription() << std::endl;
    }

    core.cleanup();
}

// ============================================================================
// Test 5: Export Functionality
// ============================================================================

void test_ExportFunctionality()
{
    printSeparator("Test 5: Export Functionality");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    createGroundAndWalls(physics, scene);
    createFallingBoxes(physics, scene, 75);

    PerformanceProfiler profiler;
    profiler.initialize(physics, scene);

    std::cout << "Simulating 150 frames..." << std::endl;

    for (int i = 0; i < 150; i++) {
        profiler.beginFrame();

        profiler.beginSimulation();
        scene->simulate(0.016f);
        profiler.endSimulation();

        profiler.beginFetch();
        scene->fetchResults(true);
        profiler.endFetch();

        profiler.endFrame();
    }

    // Export to CSV
    std::cout << "\nExporting performance data..." << std::endl;
    bool csvSuccess = profiler.exportToCSV("performance_data.csv");
    if (csvSuccess) {
        std::cout << "✓ CSV export successful: performance_data.csv" << std::endl;
    }

    // Export to JSON
    bool jsonSuccess = profiler.exportToJSON("performance_summary.json");
    if (jsonSuccess) {
        std::cout << "✓ JSON export successful: performance_summary.json" << std::endl;
    }

    core.cleanup();
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "PhysXWrapper - PerformanceProfiler Example" << std::endl;
    std::cout << "==========================================\n" << std::endl;

    try {
        test_BasicProfiling();
        test_PerformanceStatistics();
        test_CustomTimingSections();
        test_PerformanceWarnings();
        test_ExportFunctionality();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
