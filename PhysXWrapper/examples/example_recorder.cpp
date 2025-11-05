/**
 * @file example_recorder.cpp
 * @brief Example demonstrating PhysicsRecorder usage
 *
 * This example shows how to use the PhysicsRecorder class for
 * recording and playing back physics simulations.
 */

#include "PhysXCore.h"
#include "Utility/PhysicsRecorder.h"
#include <iostream>

using namespace PhysXWrapper;

void printSeparator(const std::string& title)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================\n" << std::endl;
}

void test_RecordAndPlayback()
{
    printSeparator("Test: Record and Playback");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    // Create ground
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.5f);
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Create falling boxes
    for (int i = 0; i < 5; i++) {
        PxRigidDynamic* box = PxCreateDynamic(*physics,
                                               PxTransform(PxVec3(i * 2.0f, 10.0f, 0)),
                                               PxBoxGeometry(0.5f, 0.5f, 0.5f),
                                               *material, 10.0f);
        scene->addActor(*box);
    }

    // Initialize recorder
    PhysicsRecorder recorder;
    recorder.initialize(scene);

    // Start recording
    std::cout << "Starting recording..." << std::endl;
    recorder.startRecording();

    // Simulate and record
    for (int i = 0; i < 200; i++) {
        scene->simulate(0.016f);
        scene->fetchResults(true);
        recorder.recordFrame();
    }

    recorder.stopRecording();
    std::cout << "Recording stopped" << std::endl;

    // Print recording info
    recorder.printInfo();

    // Playback
    std::cout << "\nStarting playback..." << std::endl;
    recorder.startPlayback(PhysicsRecorder::PlaybackMode::ONCE);

    int frame = 0;
    while (!recorder.isPlaybackFinished()) {
        recorder.updatePlayback(0.016f);
        frame++;

        if (frame % 50 == 0) {
            std::cout << "Playback frame: " << recorder.getCurrentFrame()
                      << " / " << recorder.getFrameCount() << std::endl;
        }
    }

    std::cout << "Playback finished" << std::endl;

    core.cleanup();
}

int main()
{
    std::cout << "PhysXWrapper - PhysicsRecorder Example" << std::endl;
    std::cout << "======================================\n" << std::endl;

    try {
        test_RecordAndPlayback();

        std::cout << "\n========================================" << std::endl;
        std::cout << "Test completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
