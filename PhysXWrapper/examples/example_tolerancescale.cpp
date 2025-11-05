/**
 * @file example_tolerancescale.cpp
 * @brief PhysX Tolerance Scale Configuration Example
 *
 * This example demonstrates the use of PxTolerancesScale for working with
 * different unit systems (meters vs centimeters, kilograms vs grams, etc.).
 *
 * Key Concepts:
 * - PxTolerancesScale allows using different physical units
 * - Proper scaling ensures numerically stable simulation regardless of units
 * - Length, mass, and speed scale parameters affect entire simulation
 * - Gravity, distances, and masses must be scaled consistently
 *
 * Based on PhysX Snippet: SnippetToleranceScale
 *
 * The example creates two identical scenes with different unit systems:
 * 1. Default units: meters (length=1), kilograms (mass=1)
 * 2. Scaled units: centimeters (length=100), grams (mass=0.001)
 *
 * Both simulations should produce identical results (just scaled numbers).
 */

#include "PhysXCore.h"
#include "Geometry/TriangleMeshBuilder.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace PhysXWrapper;

class ToleranceScaleExample {
private:
    struct SimulationResult {
        PxReal finalPosition;
        PxReal finalVelocity;
        int frameCount;
        std::string unitSystem;
    };

    std::vector<SimulationResult> results;

public:
    void createStack(PxScene* scene, PxPhysics* physics, PxMaterial* material,
                    const PxTransform& baseTransform, PxU32 size,
                    PxReal halfExtent, PxReal mass) {
        PxShape* shape = physics->createShape(
            PxBoxGeometry(halfExtent, halfExtent, halfExtent),
            *material
        );

        for (PxU32 i = 0; i < size; i++) {
            for (PxU32 j = 0; j < size - i; j++) {
                PxVec3 localPos(
                    PxReal(j * 2) - PxReal(size - i),
                    PxReal(i * 2 + 1),
                    0
                );
                localPos *= halfExtent;

                PxTransform localTm(localPos);
                PxRigidDynamic* body = physics->createRigidDynamic(
                    baseTransform.transform(localTm)
                );

                body->attachShape(*shape);
                PxRigidBodyExt::setMassAndUpdateInertia(*body, mass);
                scene->addActor(*body);
            }
        }

        shape->release();
    }

    SimulationResult runSimulation(const PxTolerancesScale& scale,
                                   PxReal scaleMass,
                                   const std::string& unitName) {
        std::cout << "\n======================================" << std::endl;
        std::cout << "Running simulation with " << unitName << std::endl;
        std::cout << "Length scale: " << scale.length << std::endl;
        std::cout << "Speed scale: " << scale.speed << std::endl;
        std::cout << "Mass scale: " << scaleMass << std::endl;
        std::cout << "======================================" << std::endl;

        // Create PhysX core with scaled tolerance
        PhysXCore core;
        PhysXCore::Config config;

        // Scale gravity based on length
        config.gravity = PxVec3(0.0f, -9.81f, 0.0f) * scale.length;
        config.numThreads = 2;
        config.toleranceScale = scale;

        if (!core.initialize(config)) {
            std::cerr << "Failed to initialize PhysX with scaled tolerance" << std::endl;
            return SimulationResult{0, 0, 0, unitName};
        }

        PxPhysics* physics = core.getPhysics();
        PxScene* scene = core.getScene();
        PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.6f);

        // Create ground plane
        PxRigidStatic* groundPlane = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
        scene->addActor(*groundPlane);

        // Create stacks at different positions
        PxReal stackZ = 10.0f * scale.length;
        for (PxU32 i = 0; i < 3; i++) {
            PxTransform stackTransform(PxVec3(0, 0, stackZ));
            createStack(scene, physics, material, stackTransform,
                       5,  // 5x5 pyramid
                       2.0f * scale.length,  // Scaled box size
                       1.0f * scaleMass);    // Scaled mass
            stackZ -= 15.0f * scale.length;
        }

        // Create a large sphere projectile
        PxTransform sphereTransform(PxVec3(0, 40, 100) * scale.length);
        PxRigidDynamic* sphere = PxCreateDynamic(
            *physics,
            sphereTransform,
            PxSphereGeometry(10.0f * scale.length),
            *material,
            100.0f * scaleMass
        );

        sphere->setLinearVelocity(PxVec3(0, -50, -100) * scale.length);
        scene->addActor(*sphere);

        // Simulate
        const PxReal dt = 1.0f / 60.0f;
        const int totalFrames = 150;  // 2.5 seconds

        std::cout << "Simulating " << totalFrames << " frames..." << std::endl;

        for (int frame = 0; frame < totalFrames; frame++) {
            scene->simulate(dt);
            scene->fetchResults(true);

            // Print progress every 30 frames (0.5 seconds)
            if (frame % 30 == 0) {
                PxVec3 spherePos = sphere->getGlobalPose().p;
                PxVec3 sphereVel = sphere->getLinearVelocity();

                std::cout << "Frame " << std::setw(3) << frame << ": ";
                std::cout << "Sphere pos=(" << std::fixed << std::setprecision(2)
                         << spherePos.x << ", " << spherePos.y << ", " << spherePos.z << ")  ";
                std::cout << "vel=(" << sphereVel.x << ", " << sphereVel.y << ", "
                         << sphereVel.z << ")" << std::endl;
            }
        }

        // Record final result
        SimulationResult result;
        result.finalPosition = sphere->getGlobalPose().p.y;
        result.finalVelocity = sphere->getLinearVelocity().magnitude();
        result.frameCount = totalFrames;
        result.unitSystem = unitName;

        std::cout << "Final sphere Y position: " << result.finalPosition << std::endl;
        std::cout << "Final velocity magnitude: " << result.finalVelocity << std::endl;

        // Cleanup
        sphere->release();
        groundPlane->release();
        material->release();
        core.cleanup();

        return result;
    }

    void compareResults() {
        if (results.size() < 2) {
            std::cout << "Not enough results to compare" << std::endl;
            return;
        }

        std::cout << "\n======================================" << std::endl;
        std::cout << "COMPARISON OF SIMULATION RESULTS" << std::endl;
        std::cout << "======================================" << std::endl;

        // The default simulation
        const SimulationResult& defaultSim = results[0];
        std::cout << "\nDefault Units (meters, kilograms):" << std::endl;
        std::cout << "  Final Y position: " << defaultSim.finalPosition << " m" << std::endl;
        std::cout << "  Final velocity: " << defaultSim.finalVelocity << " m/s" << std::endl;

        // The scaled simulation
        const SimulationResult& scaledSim = results[1];
        std::cout << "\nScaled Units (centimeters, grams):" << std::endl;
        std::cout << "  Final Y position: " << scaledSim.finalPosition << " cm" << std::endl;
        std::cout << "  Final velocity: " << scaledSim.finalVelocity << " cm/s" << std::endl;

        // Compare scaled values
        PxReal scaleFactor = 100.0f;  // cm to m conversion
        PxReal scaledToMeters = scaledSim.finalPosition / scaleFactor;
        PxReal scaledVelToMeters = scaledSim.finalVelocity / scaleFactor;

        std::cout << "\nScaled simulation converted to meters:" << std::endl;
        std::cout << "  Final Y position: " << scaledToMeters << " m" << std::endl;
        std::cout << "  Final velocity: " << scaledVelToMeters << " m/s" << std::endl;

        // Calculate differences
        PxReal posDiff = std::abs(defaultSim.finalPosition - scaledToMeters);
        PxReal velDiff = std::abs(defaultSim.finalVelocity - scaledVelToMeters);

        std::cout << "\nDifferences (should be near zero):" << std::endl;
        std::cout << "  Position difference: " << posDiff << " m ("
                 << (posDiff / defaultSim.finalPosition * 100) << "%)" << std::endl;
        std::cout << "  Velocity difference: " << velDiff << " m/s ("
                 << (velDiff / defaultSim.finalVelocity * 100) << "%)" << std::endl;

        if (posDiff < 0.1f && velDiff < 0.1f) {
            std::cout << "\n✓ Results match! Tolerance scaling working correctly." << std::endl;
        } else {
            std::cout << "\n✗ Results differ significantly!" << std::endl;
        }
    }

    void run() {
        std::cout << "PhysX Tolerance Scale Example" << std::endl;
        std::cout << "=============================" << std::endl;

        // Test 1: Default scale (meters, kilograms)
        PxTolerancesScale defaultScale;
        results.push_back(runSimulation(defaultScale, 1000.0f, "Default (m, kg)"));

        // Test 2: Centimeter scale
        PxTolerancesScale cmScale;
        cmScale.length = 100;              // 1 meter = 100 cm
        cmScale.speed *= cmScale.length;   // Scale speed accordingly
        results.push_back(runSimulation(cmScale, 1.0f, "Centimeter (cm, g)"));

        // Test 3: Comparison
        compareResults();

        // Additional test: Millimeter scale
        std::cout << "\nBonus test: Millimeter scale" << std::endl;
        PxTolerancesScale mmScale;
        mmScale.length = 1000;              // 1 meter = 1000 mm
        mmScale.speed *= mmScale.length;
        results.push_back(runSimulation(mmScale, 0.001f, "Millimeter (mm, mg)"));

        std::cout << "\n=================" << std::endl;
        std::cout << "Example complete!" << std::endl;
        std::cout << "=================" << std::endl;
    }
};

int main() {
    ToleranceScaleExample example;
    example.run();
    return 0;
}
