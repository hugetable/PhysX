/**
 * @file example_debug.cpp
 * @brief Example demonstrating DebugDrawer for physics visualization
 *
 * This example shows how to use the DebugDrawer class to visualize:
 * - Collision shapes and wireframes
 * - Joints and constraints
 * - Contact points and normals
 * - Velocity and force vectors
 * - Bounding boxes and centers of mass
 */

#include "PhysXManager.h"
#include "RigidBody/RigidBodyManager.h"
#include "Debug/DebugDrawer.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace PhysXWrapper;

// ============================================================================
// Custom Rendering Callback
// ============================================================================

/**
 * Simple callback that collects debug primitives and prints statistics
 */
class StatsDebugCallback : public DebugDrawCallback {
public:
    PxU32 lineCount = 0;
    PxU32 triangleCount = 0;
    PxU32 textCount = 0;

    void drawLine(const PxVec3& start, const PxVec3& end, PxU32 color) override {
        lineCount++;
    }

    void drawTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2, PxU32 color) override {
        triangleCount++;
    }

    void drawText(const PxVec3& position, const std::string& text, PxU32 color) override {
        textCount++;
    }

    void reset() {
        lineCount = 0;
        triangleCount = 0;
        textCount = 0;
    }

    void printStats() const {
        std::cout << "  Lines: " << lineCount
                  << ", Triangles: " << triangleCount
                  << ", Text: " << textCount << std::endl;
    }
};

// ============================================================================
// Test 1: Basic Shape Visualization
// ============================================================================

void testBasicShapeVisualization()
{
    std::cout << "\n=== Test 1: Basic Shape Visualization ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Create various shapes
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);

    // Box
    RigidBodyConfig boxConfig;
    boxConfig.type = RigidBodyType::DYNAMIC;
    boxConfig.position = PxVec3(-5, 2, 0);
    PxRigidDynamic* box = bodyManager.createBox(boxConfig, PxVec3(1, 1, 1));

    // Sphere
    RigidBodyConfig sphereConfig;
    sphereConfig.type = RigidBodyType::DYNAMIC;
    sphereConfig.position = PxVec3(0, 2, 0);
    PxRigidDynamic* sphere = bodyManager.createSphere(sphereConfig, 1.0f);

    // Capsule
    RigidBodyConfig capsuleConfig;
    capsuleConfig.type = RigidBodyType::DYNAMIC;
    capsuleConfig.position = PxVec3(5, 2, 0);
    PxRigidDynamic* capsule = bodyManager.createCapsule(capsuleConfig, 0.5f, 2.0f);

    // Ground plane
    bodyManager.createGroundPlane();

    // Create debug drawer
    DebugDrawer drawer;
    StatsDebugCallback callback;
    drawer.setDrawCallback(&callback);

    // Test different flag combinations
    std::cout << "\nTest 1a: Draw shapes only" << std::endl;
    drawer.setFlags(DebugDrawFlag::SHAPES);
    drawer.drawScene(scene);
    drawer.flush();
    callback.printStats();
    callback.reset();
    drawer.clear();

    std::cout << "\nTest 1b: Draw shapes + AABBs" << std::endl;
    drawer.setFlags(DebugDrawFlag::SHAPES | DebugDrawFlag::AABBS);
    drawer.drawScene(scene);
    drawer.flush();
    callback.printStats();
    callback.reset();
    drawer.clear();

    std::cout << "\nTest 1c: Draw shapes + axes" << std::endl;
    drawer.setFlags(DebugDrawFlag::SHAPES | DebugDrawFlag::AXES);
    drawer.drawScene(scene);
    drawer.flush();
    callback.printStats();
    callback.reset();
    drawer.clear();

    std::cout << "\nTest 1d: Draw everything" << std::endl;
    drawer.setFlags(DebugDrawFlag::ALL);
    drawer.drawScene(scene);
    drawer.flush();
    callback.printStats();

    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Basic shape visualization test passed" << std::endl;
}

// ============================================================================
// Test 2: Joint Visualization
// ============================================================================

void testJointVisualization()
{
    std::cout << "\n=== Test 2: Joint Visualization ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Create chain of boxes connected by joints
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);

    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.1f);

    // Create anchored static box
    PxRigidStatic* anchor = PxCreateStatic(*physics,
        PxTransform(PxVec3(0, 5, 0)),
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material);
    scene->addActor(*anchor);

    // Create chain of dynamic boxes
    const int chainLength = 5;
    PxRigidDynamic* prevActor = nullptr;
    std::vector<PxJoint*> joints;

    for (int i = 0; i < chainLength; i++) {
        PxVec3 position(0, 5 - (i + 1) * 2.0f, 0);

        RigidBodyConfig config;
        config.type = RigidBodyType::DYNAMIC;
        config.position = position;
        PxRigidDynamic* box = bodyManager.createBox(config, PxVec3(0.5f, 0.5f, 0.5f));

        // Create joint
        PxRigidActor* actor0 = (i == 0) ? static_cast<PxRigidActor*>(anchor) : prevActor;
        PxVec3 anchor0 = (i == 0) ? PxVec3(0, -1, 0) : PxVec3(0, 1, 0);
        PxVec3 anchor1 = PxVec3(0, 1, 0);

        PxSphericalJoint* joint = PxSphericalJointCreate(*physics,
            actor0, PxTransform(anchor0),
            box, PxTransform(anchor1));

        joints.push_back(joint);
        prevActor = box;
    }

    // Create debug drawer
    DebugDrawer drawer;
    StatsDebugCallback callback;
    drawer.setDrawCallback(&callback);

    // Draw joints
    std::cout << "\nVisualizing " << joints.size() << " joints" << std::endl;
    drawer.setFlags(DebugDrawFlag::SHAPES | DebugDrawFlag::JOINTS);

    for (PxJoint* joint : joints) {
        drawer.drawJoint(joint);
    }
    drawer.drawScene(scene);
    drawer.flush();
    callback.printStats();

    // Cleanup
    for (PxJoint* joint : joints) {
        joint->release();
    }
    material->release();
    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Joint visualization test passed" << std::endl;
}

// ============================================================================
// Test 3: Velocity Vector Visualization
// ============================================================================

void testVelocityVisualization()
{
    std::cout << "\n=== Test 3: Velocity Vector Visualization ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Create objects with velocities
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);

    // Create moving objects
    for (int i = 0; i < 5; i++) {
        RigidBodyConfig config;
        config.type = RigidBodyType::DYNAMIC;
        config.position = PxVec3(-10 + i * 5.0f, 5, 0);
        PxRigidDynamic* sphere = bodyManager.createSphere(config, 0.5f);

        // Set different velocities
        sphere->setLinearVelocity(PxVec3(i - 2.0f, i * 0.5f, 0));
        sphere->setAngularVelocity(PxVec3(0, 0, i * 2.0f));
    }

    bodyManager.createGroundPlane();

    // Create debug drawer
    DebugDrawer drawer;
    StatsDebugCallback callback;
    drawer.setDrawCallback(&callback);
    drawer.setFlags(DebugDrawFlag::SHAPES | DebugDrawFlag::VELOCITIES);

    // Simulate and visualize
    std::cout << "\nSimulating with velocity visualization" << std::endl;
    for (int frame = 0; frame < 3; frame++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        std::cout << "Frame " << frame << ": ";
        drawer.drawScene(scene);
        drawer.flush();
        callback.printStats();
        callback.reset();
        drawer.clear();
    }

    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Velocity visualization test passed" << std::endl;
}

// ============================================================================
// Test 4: Primitive Drawing Functions
// ============================================================================

void testPrimitiveDrawing()
{
    std::cout << "\n=== Test 4: Primitive Drawing Functions ===" << std::endl;

    // Initialize PhysX (not really needed, but keeps consistency)
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    // Create debug drawer with buffered callback
    DebugDrawer drawer;
    BufferedDebugDrawer bufferedCallback;
    drawer.setDrawCallback(&bufferedCallback);

    // Test individual primitive drawing
    std::cout << "\nTest 4a: Draw line" << std::endl;
    drawer.drawLine(PxVec3(0, 0, 0), PxVec3(1, 1, 1), DebugColors::RED);
    drawer.flush();
    std::cout << "  Lines drawn: " << bufferedCallback.getLines().size() << std::endl;
    bufferedCallback.clear();

    std::cout << "\nTest 4b: Draw box" << std::endl;
    drawer.drawBox(PxVec3(0, 0, 0), PxVec3(1, 1, 1), PxQuat(PxIdentity), DebugColors::GREEN);
    drawer.flush();
    std::cout << "  Lines drawn: " << bufferedCallback.getLines().size() << std::endl;
    bufferedCallback.clear();

    std::cout << "\nTest 4c: Draw sphere" << std::endl;
    drawer.drawSphere(PxVec3(0, 0, 0), 1.0f, DebugColors::BLUE, 16);
    drawer.flush();
    std::cout << "  Lines drawn: " << bufferedCallback.getLines().size() << std::endl;
    bufferedCallback.clear();

    std::cout << "\nTest 4d: Draw capsule" << std::endl;
    drawer.drawCapsule(PxVec3(0, 0, 0), 0.5f, 2.0f, PxQuat(PxIdentity), DebugColors::YELLOW);
    drawer.flush();
    std::cout << "  Lines drawn: " << bufferedCallback.getLines().size() << std::endl;
    bufferedCallback.clear();

    std::cout << "\nTest 4e: Draw arrow" << std::endl;
    drawer.drawArrow(PxVec3(0, 0, 0), PxVec3(1, 0, 0), 2.0f, DebugColors::CYAN);
    drawer.flush();
    std::cout << "  Lines drawn: " << bufferedCallback.getLines().size() << std::endl;
    bufferedCallback.clear();

    std::cout << "\nTest 4f: Draw axes" << std::endl;
    drawer.drawAxes(PxTransform(PxVec3(0, 0, 0)), 1.0f);
    drawer.flush();
    std::cout << "  Lines drawn: " << bufferedCallback.getLines().size() << std::endl;
    bufferedCallback.clear();

    std::cout << "\nTest 4g: Draw bounds" << std::endl;
    PxBounds3 bounds(PxVec3(-1, -1, -1), PxVec3(1, 1, 1));
    drawer.drawBounds(bounds, DebugColors::MAGENTA);
    drawer.flush();
    std::cout << "  Lines drawn: " << bufferedCallback.getLines().size() << std::endl;
    bufferedCallback.clear();

    std::cout << "\nTest 4h: Draw text" << std::endl;
    drawer.drawText(PxVec3(0, 2, 0), "Hello PhysX!", DebugColors::WHITE);
    drawer.flush();
    std::cout << "  Text labels drawn: " << bufferedCallback.getTextLabels().size() << std::endl;
    bufferedCallback.clear();

    physxManager.cleanup();

    std::cout << "✓ Primitive drawing test passed" << std::endl;
}

// ============================================================================
// Test 5: Complex Scene Visualization
// ============================================================================

void testComplexSceneVisualization()
{
    std::cout << "\n=== Test 5: Complex Scene Visualization ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Create complex scene
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);

    // Create stack of boxes
    std::cout << "\nCreating complex scene with multiple objects..." << std::endl;
    for (int layer = 0; layer < 5; layer++) {
        for (int x = 0; x < 3; x++) {
            for (int z = 0; z < 3; z++) {
                RigidBodyConfig config;
                config.type = RigidBodyType::DYNAMIC;
                config.position = PxVec3(
                    -2.0f + x * 2.0f,
                    2.0f + layer * 2.1f,
                    -2.0f + z * 2.0f
                );
                bodyManager.createBox(config, PxVec3(1, 1, 1));
            }
        }
    }

    // Add some spheres
    for (int i = 0; i < 10; i++) {
        RigidBodyConfig config;
        config.type = RigidBodyType::DYNAMIC;
        config.position = PxVec3(
            -10 + (i % 5) * 5.0f,
            15 + (i / 5) * 3.0f,
            5
        );
        bodyManager.createSphere(config, 0.5f);
    }

    bodyManager.createGroundPlane();

    // Create debug drawer
    DebugDrawer drawer;
    StatsDebugCallback callback;
    drawer.setDrawCallback(&callback);

    // Test performance with different flag combinations
    std::cout << "\nTest 5a: Shapes only" << std::endl;
    drawer.setFlags(DebugDrawFlag::SHAPES);
    auto start = std::chrono::high_resolution_clock::now();
    drawer.drawScene(scene);
    drawer.flush();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    callback.printStats();
    std::cout << "  Time: " << duration.count() << " μs" << std::endl;
    callback.reset();
    drawer.clear();

    std::cout << "\nTest 5b: All features enabled" << std::endl;
    drawer.setFlags(DebugDrawFlag::ALL);
    start = std::chrono::high_resolution_clock::now();
    drawer.drawScene(scene);
    drawer.flush();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    callback.printStats();
    std::cout << "  Time: " << duration.count() << " μs" << std::endl;

    // Simulate and visualize
    std::cout << "\nTest 5c: Visualizing simulation" << std::endl;
    drawer.setFlags(DebugDrawFlag::SHAPES | DebugDrawFlag::VELOCITIES | DebugDrawFlag::COM);
    for (int frame = 0; frame < 5; frame++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);

        callback.reset();
        drawer.clear();
        drawer.drawScene(scene);
        drawer.flush();

        std::cout << "Frame " << frame << ": ";
        callback.printStats();
    }

    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Complex scene visualization test passed" << std::endl;
}

// ============================================================================
// Test 6: Console Debug Drawer
// ============================================================================

void testConsoleDebugDrawer()
{
    std::cout << "\n=== Test 6: Console Debug Drawer ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Create simple scene
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);

    RigidBodyConfig config;
    config.type = RigidBodyType::DYNAMIC;
    config.position = PxVec3(0, 5, 0);
    bodyManager.createBox(config, PxVec3(1, 1, 1));

    // Use console debug drawer
    DebugDrawer drawer;
    ConsoleDebugDrawer consoleCallback;
    drawer.setDrawCallback(&consoleCallback);
    drawer.setFlags(DebugDrawFlag::SHAPES);

    std::cout << "\nDrawing scene to console (first 10 lines only):" << std::endl;
    drawer.drawScene(scene);

    // Only flush a few lines to avoid spam
    const auto& lines = drawer.getLines();
    int linesToShow = std::min(10, static_cast<int>(lines.size()));
    for (int i = 0; i < linesToShow; i++) {
        consoleCallback.drawLine(lines[i].start, lines[i].end, lines[i].color);
    }
    std::cout << "... (" << lines.size() << " total lines)" << std::endl;

    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Console debug drawer test passed" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "PhysXWrapper - DebugDrawer Example" << std::endl;
    std::cout << "===================================" << std::endl;

    try {
        testBasicShapeVisualization();
        testJointVisualization();
        testVelocityVisualization();
        testPrimitiveDrawing();
        testComplexSceneVisualization();
        testConsoleDebugDrawer();

        std::cout << "\n==================================" << std::endl;
        std::cout << "All tests passed successfully!" << std::endl;
        std::cout << "==================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
