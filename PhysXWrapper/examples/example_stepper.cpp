/**
 * @file example_stepper.cpp
 * @brief Advanced Time Stepper with Continuation Tasks and Substepping
 *
 * This example demonstrates PhysX's advanced time stepping with:
 * - Continuation tasks using PxLightCpuTask
 * - Chained substep completion tasks
 * - Kinematic actor updates between substeps
 * - Thread-safe task management
 * - Atomic operations for synchronization
 *
 * Based on PhysX Snippet: SnippetStepper
 *
 * Implementation Details:
 * - Uses PxLightCpuTask for lightweight task management
 * - Each substep has a completion task that triggers the next
 * - Kinematic platform updated before each substep
 * - Reference counting ensures proper task sequencing
 * - Synchronization primitives for thread safety
 *
 * Architecture:
 * Main Thread -> startSubstep(0) -> simulate() -> CompletionTask.run()
 *             -> startSubstep(1) -> simulate() -> CompletionTask.run()
 *             ...
 *             -> all substeps done -> signal completion
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <atomic>
#include <mutex>
#include <condition_variable>

using namespace PhysXWrapper;

// Forward declarations
class StepperExample;
void startNextSubstep();

// ============================================================================
// Synchronization primitives
// ============================================================================

class Sync {
private:
    std::mutex mutex;
    std::condition_variable cv;
    bool signaled;

public:
    Sync() : signaled(false) {}

    void reset() {
        std::lock_guard<std::mutex> lock(mutex);
        signaled = false;
    }

    void signal() {
        std::lock_guard<std::mutex> lock(mutex);
        signaled = true;
        cv.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return signaled; });
    }
};

// ============================================================================
// Global stepper context
// ============================================================================

struct StepContext {
    PxScene* scene;
    PxRigidDynamic* kinematicPlatform;

    // Substepping state
    static constexpr int NUM_SUBSTEPS = 2;
    static constexpr PxReal SUBSTEP_LENGTH = 1.0f / 120.0f;  // 120Hz substeps

    std::atomic<int> substepsFinished;
    std::atomic<int> tasksDestroyed;
    Sync* completionSync;

    // Platform motion parameters
    PxReal totalTime;
    static constexpr PxReal PERIOD = 4.0f;
    static constexpr PxReal AMPLITUDE = 5.0f;

    // Task pool
    class SubstepCompletionTask* taskPool;

    StepContext()
        : scene(nullptr)
        , kinematicPlatform(nullptr)
        , substepsFinished(0)
        , tasksDestroyed(0)
        , completionSync(nullptr)
        , totalTime(0.0f)
        , taskPool(nullptr)
    {}
};

static StepContext g_stepContext;

// ============================================================================
// Substep Completion Task
// ============================================================================

/**
 * @brief Completion task that runs after each substep
 *
 * This task is submitted to PhysX as a continuation task. It:
 * 1. Calls fetchResults() to get simulation results
 * 2. Checks if more substeps are needed
 * 3. Triggers the next substep if needed
 * 4. Signals completion when all substeps are done
 *
 * Thread Safety:
 * - Uses atomic counters for substeps and tasks
 * - Reference counting prevents premature execution
 * - Release() may run concurrently with other tasks
 */
class SubstepCompletionTask : public PxLightCpuTask {
private:
    int substepIndex;

public:
    SubstepCompletionTask(int index = 0)
        : PxLightCpuTask()
        , substepIndex(index)
    {
        // Get task manager from scene
        if (g_stepContext.scene) {
            mTm = g_stepContext.scene->getTaskManager();
        }
    }

    virtual void run() override {
        // Fetch results from the substep that just completed
        g_stepContext.scene->fetchResults(true);

        // Increment finished counter
        int finished = g_stepContext.substepsFinished.fetch_add(1) + 1;

        // If more substeps needed, start the next one
        if (finished < StepContext::NUM_SUBSTEPS) {
            startNextSubstep();
        }
    }

    virtual void release() override {
        // Manually call destructor
        this->~SubstepCompletionTask();

        // Check if all tasks are destroyed
        // release() calls may run concurrently, so use atomic increment
        int destroyed = g_stepContext.tasksDestroyed.fetch_add(1) + 1;

        if (destroyed == StepContext::NUM_SUBSTEPS) {
            // All substeps complete, signal main thread
            g_stepContext.completionSync->signal();
        }
    }

    virtual const char* getName() const override {
        return "SubstepCompletionTask";
    }
};

// ============================================================================
// Stepper Implementation
// ============================================================================

void updateKinematicTarget(PxReal time) {
    if (!g_stepContext.kinematicPlatform) return;

    // Calculate sinusoidal platform position
    PxReal angularVelocity = PxTwoPi / StepContext::PERIOD;
    PxReal yPosition = std::sin(angularVelocity * time) * StepContext::AMPLITUDE;

    // Set kinematic target for smooth motion
    PxTransform targetPose(PxVec3(0.0f, yPosition, 0.0f));
    g_stepContext.kinematicPlatform->setKinematicTarget(targetPose);
}

void startNextSubstep() {
    // Get current substep index
    int substepIndex = g_stepContext.substepsFinished.load();

    // Calculate time for this substep
    PxReal substepTime = g_stepContext.totalTime + substepIndex * StepContext::SUBSTEP_LENGTH;

    // Update kinematic platform target BEFORE substep
    updateKinematicTarget(substepTime);

    // Create completion task for this substep
    // Use placement new to allocate from task pool
    SubstepCompletionTask* completionTask =
        new (&g_stepContext.taskPool[substepIndex]) SubstepCompletionTask(substepIndex);

    // Set reference count to 1
    // This prevents the task from running until we call removeReference()
    completionTask->addReference();

    // Submit simulation with completion task
    // PhysX will call completionTask->run() when simulation finishes
    g_stepContext.scene->simulate(StepContext::SUBSTEP_LENGTH, completionTask);

    // We can do parallel work here that must complete before task->run()
    // (nothing needed for this example)

    // Remove reference to allow task to run
    // Once simulation completes, completionTask->run() will execute
    completionTask->removeReference();
}

// ============================================================================
// Main Example Class
// ============================================================================

class StepperExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    PxRigidDynamic* kinematicPlatform;
    PxRigidDynamic* dynamicSphere;

    SubstepCompletionTask* taskPool;
    Sync completionSync;

public:
    StepperExample()
        : physics(nullptr)
        , scene(nullptr)
        , material(nullptr)
        , kinematicPlatform(nullptr)
        , dynamicSphere(nullptr)
        , taskPool(nullptr)
    {}

    ~StepperExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "==================================================" << std::endl;
        std::cout << "PhysX Advanced Stepper Example" << std::endl;
        std::cout << "==================================================" << std::endl;

        // Initialize PhysX
        PhysXCore::Config config;
        config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        config.numThreads = 2;

        if (!core.initialize(config)) {
            std::cerr << "Failed to initialize PhysX" << std::endl;
            return false;
        }

        physics = core.getPhysics();
        scene = core.getScene();
        material = physics->createMaterial(0.5f, 0.5f, 0.7f);

        // Setup global context
        g_stepContext.scene = scene;
        g_stepContext.completionSync = &completionSync;

        // Allocate task pool (one task per substep)
        taskPool = new SubstepCompletionTask[StepContext::NUM_SUBSTEPS];
        g_stepContext.taskPool = taskPool;

        // Create kinematic platform
        PxBoxGeometry platformGeom(5.0f, 0.5f, 5.0f);
        PxTransform platformPose(PxVec3(0.0f, 0.0f, 0.0f));
        kinematicPlatform = PxCreateDynamic(*physics, platformPose, platformGeom, *material, 1.0f);
        kinematicPlatform->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        scene->addActor(*kinematicPlatform);

        g_stepContext.kinematicPlatform = kinematicPlatform;

        // Create dynamic sphere
        PxSphereGeometry sphereGeom(1.0f);
        PxTransform spherePose(PxVec3(0.0f, 5.0f, 0.0f));
        dynamicSphere = PxCreateDynamic(*physics, spherePose, sphereGeom, *material, 1.0f);
        scene->addActor(*dynamicSphere);

        std::cout << "\nConfiguration:" << std::endl;
        std::cout << "  Substeps per frame: " << StepContext::NUM_SUBSTEPS << std::endl;
        std::cout << "  Substep length: " << (StepContext::SUBSTEP_LENGTH * 1000) << " ms" << std::endl;
        std::cout << "  Total frame time: "
                  << (StepContext::NUM_SUBSTEPS * StepContext::SUBSTEP_LENGTH * 1000) << " ms" << std::endl;
        std::cout << "  Platform period: " << StepContext::PERIOD << " seconds" << std::endl;
        std::cout << "  Platform amplitude: " << StepContext::AMPLITUDE << " meters" << std::endl;
        std::cout << "\nUsing continuation tasks with atomic synchronization" << std::endl;

        return true;
    }

    void runOneStep() {
        // Reset step context
        completionSync.reset();
        g_stepContext.substepsFinished = 0;
        g_stepContext.tasksDestroyed = 0;

        // Start first substep
        // This will trigger a chain: substep0 -> substep1 -> ... -> completion
        startNextSubstep();

        // Wait for all substeps to complete
        // This blocks until the last task signals completion
        completionSync.wait();

        // Update total time
        g_stepContext.totalTime += StepContext::NUM_SUBSTEPS * StepContext::SUBSTEP_LENGTH;
    }

    void printStatus(int frame) {
        PxTransform platformPose = kinematicPlatform->getGlobalPose();
        PxTransform spherePose = dynamicSphere->getGlobalPose();
        PxVec3 sphereVel = dynamicSphere->getLinearVelocity();

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Frame " << std::setw(4) << frame
                  << " @ t=" << std::setw(6) << g_stepContext.totalTime << "s  ";
        std::cout << "Platform Y=" << std::setw(7) << platformPose.p.y << "  ";
        std::cout << "Sphere Y=" << std::setw(7) << spherePose.p.y << "  ";
        std::cout << "Speed=" << std::setw(6) << sphereVel.magnitude() << " m/s" << std::endl;
    }

    void run() {
        std::cout << "\n=== Starting Simulation ===" << std::endl;
        std::cout << "The kinematic platform oscillates with a sine wave" << std::endl;
        std::cout << "The dynamic sphere bounces on the platform" << std::endl;
        std::cout << "Platform position updated before EACH substep" << std::endl;
        std::cout << "================================\n" << std::endl;

        const int totalFrames = 600;  // 10 seconds at 60fps

        for (int frame = 0; frame < totalFrames; frame++) {
            runOneStep();

            // Print status every 60 frames (1 second)
            if (frame % 60 == 0) {
                printStatus(frame);
            }
        }

        std::cout << "\n=== Simulation Complete ===" << std::endl;
        printStatus(totalFrames);

        std::cout << "\nKey Features Demonstrated:" << std::endl;
        std::cout << "  ✓ Continuation tasks for chained substeps" << std::endl;
        std::cout << "  ✓ Atomic operations for thread-safe counters" << std::endl;
        std::cout << "  ✓ Synchronization primitives (mutex, condition_variable)" << std::endl;
        std::cout << "  ✓ Kinematic updates between substeps" << std::endl;
        std::cout << "  ✓ Task pool memory management" << std::endl;
        std::cout << "  ✓ Reference counting for task sequencing" << std::endl;
    }

    void cleanup() {
        if (taskPool) {
            delete[] taskPool;
            taskPool = nullptr;
        }

        if (kinematicPlatform) kinematicPlatform->release();
        if (dynamicSphere) dynamicSphere->release();
        if (material) material->release();

        // Clear global context
        g_stepContext = StepContext();

        core.cleanup();
    }
};

// ============================================================================
// Main
// ============================================================================

int main() {
    StepperExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
