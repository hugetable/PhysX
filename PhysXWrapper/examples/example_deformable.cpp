/**
 * @file example_deformable.cpp
 * @brief Deformable volume (soft body) example
 *
 * This example demonstrates:
 * - GPU-accelerated soft body simulation
 * - Creating deformable volumes from meshes
 * - Material property configuration
 * - Mesh generation (cube, sphere, cylinder)
 * - Collision with rigid bodies
 *
 * REQUIREMENTS:
 * - NVIDIA GPU with CUDA support
 * - PhysX compiled with GPU support
 *
 * Based on SnippetDeformableVolume from PhysX SDK.
 */

#include "Core/PhysXCore.h"
#include "Deformable/DeformableVolumeManager.h"
#include <iostream>

using namespace PhysXWrapper;
using namespace physx;

int main(int argc, char** argv) {
    std::cout << "=== PhysXWrapper - Deformable Volume Example ===" << std::endl;
    std::cout << std::endl;

    // Configure PhysX with GPU support
    PhysXCoreConfig config;
    config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    config.numThreads = 2;
    config.enablePVD = false;
    config.enableGPU = true;  // CRITICAL: Enable GPU

    // Check for PVD flag
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--pvd") {
            config.enablePVD = true;
            std::cout << "PVD enabled" << std::endl;
        }
    }

    // Initialize PhysX
    PhysXCore physics;
    std::cout << "Initializing PhysX with GPU support..." << std::endl;
    if (!physics.initialize(config)) {
        std::cerr << "Failed to initialize PhysX: " << physics.getLastError() << std::endl;
        return 1;
    }

    // Check if GPU is available
    PxCudaContextManager* cudaContextManager = physics.getCudaContextManager();
    if (!cudaContextManager) {
        std::cerr << "ERROR: CUDA context manager not available!" << std::endl;
        std::cerr << "Deformable volumes require GPU/CUDA support." << std::endl;
        std::cerr << "Please ensure:" << std::endl;
        std::cerr << "  1. You have an NVIDIA GPU" << std::endl;
        std::cerr << "  2. CUDA is installed" << std::endl;
        std::cerr << "  3. PhysX is compiled with GPU support" << std::endl;
        return 1;
    }

    std::cout << "GPU/CUDA support detected!" << std::endl;

    // Get scene
    PxScene* scene = physics.getScene();
    if (!scene) {
        std::cerr << "Failed to get scene" << std::endl;
        return 1;
    }

    // Create ground plane
    PxRigidStatic* ground = PxCreatePlane(
        *physics.getPhysics(),
        PxPlane(0, 1, 0, 0),
        *physics.getDefaultMaterial()
    );
    scene->addActor(*ground);
    std::cout << "Created ground plane" << std::endl;

    // Initialize deformable volume manager
    DeformableVolumeManager defMgr;
    if (!defMgr.initialize(physics.getPhysics(), cudaContextManager)) {
        std::cerr << "Failed to initialize DeformableVolumeManager: "
                  << defMgr.getLastError() << std::endl;
        return 1;
    }
    std::cout << "DeformableVolumeManager initialized" << std::endl;

    // Create soft body cube
    std::cout << "\n=== Creating Soft Body Cube ===" << std::endl;
    SimpleMesh cubeMesh = defMgr.createCubeMesh(PxVec3(0, 5, 0), 2.0f, 0.5f);
    std::cout << "  Generated cube mesh: " << cubeMesh.vertices.size()
              << " vertices, " << (cubeMesh.indices.size() / 3) << " triangles" << std::endl;

    DeformableVolumeConfig cubeConfig;
    cubeConfig.density = 100.0f;
    cubeConfig.solverIterationCount = 30;
    cubeConfig.enableSelfCollision = false;

    DeformableVolumeMaterialConfig cubeMaterial;
    cubeMaterial.youngsModulus = 1.0e5f;  // Relatively soft
    cubeMaterial.poissonRatio = 0.4f;
    cubeMaterial.damping = 0.1f;

    DeformableVolumeHandle* cube = defMgr.createDeformableVolume(
        scene, cubeMesh, cubeConfig, cubeMaterial,
        PxTransform(PxIdentity), "SoftCube"
    );

    if (cube) {
        std::cout << "  Soft body cube created successfully!" << std::endl;
        std::cout << "  Vertices: " << defMgr.getVertexCount(cube) << std::endl;
    } else {
        std::cerr << "  Failed to create cube: " << defMgr.getLastError() << std::endl;
    }

    // Create soft body sphere
    std::cout << "\n=== Creating Soft Body Sphere ===" << std::endl;
    SimpleMesh sphereMesh = defMgr.createSphereMesh(PxVec3(0, 10, 0), 1.5f, 0.4f);
    std::cout << "  Generated sphere mesh: " << sphereMesh.vertices.size()
              << " vertices, " << (sphereMesh.indices.size() / 3) << " triangles" << std::endl;

    DeformableVolumeConfig sphereConfig;
    sphereConfig.density = 80.0f;
    sphereConfig.solverIterationCount = 30;

    DeformableVolumeMaterialConfig sphereMaterial;
    sphereMaterial.youngsModulus = 5.0e4f;  // Softer than cube
    sphereMaterial.poissonRatio = 0.45f;
    sphereMaterial.damping = 0.15f;

    DeformableVolumeHandle* sphere = defMgr.createDeformableVolume(
        scene, sphereMesh, sphereConfig, sphereMaterial,
        PxTransform(PxIdentity), "SoftSphere"
    );

    if (sphere) {
        std::cout << "  Soft body sphere created successfully!" << std::endl;
        std::cout << "  Vertices: " << defMgr.getVertexCount(sphere) << std::endl;
    } else {
        std::cerr << "  Failed to create sphere: " << defMgr.getLastError() << std::endl;
    }

    // Add some rigid bodies for interaction
    std::cout << "\n=== Adding Rigid Bodies ===" << std::endl;
    PxRigidDynamic* box1 = physics.getPhysics()->createRigidDynamic(
        PxTransform(PxVec3(-2, 8, 0))
    );
    PxShape* boxShape = PxRigidActorExt::createExclusiveShape(
        *box1, PxBoxGeometry(0.5f, 0.5f, 0.5f), *physics.getDefaultMaterial()
    );
    PxRigidBodyExt::updateMassAndInertia(*box1, 10.0f);
    scene->addActor(*box1);
    std::cout << "  Added rigid box" << std::endl;

    // Run simulation
    std::cout << "\n=== Running Simulation ===" << std::endl;
    const PxReal timeStep = 1.0f / 60.0f;
    const int frameCount = 300;  // 5 seconds

    for (int i = 0; i < frameCount; i++) {
        scene->simulate(timeStep);
        scene->fetchResults(true);

        // Update deformed meshes (copy from GPU)
        defMgr.updateDeformedMeshes();

        // Print progress
        if (i % 60 == 0) {
            std::cout << "  Frame " << i << " / " << frameCount
                      << " (" << (i / 60.0f) << "s)" << std::endl;

            // Print some vertex positions
            if (cube) {
                auto vertices = defMgr.getDeformedVertices(cube);
                if (!vertices.empty()) {
                    std::cout << "    Cube vertex[0]: ("
                              << vertices[0].x << ", "
                              << vertices[0].y << ", "
                              << vertices[0].z << ")" << std::endl;
                }
            }
        }
    }

    std::cout << "\nSimulation complete!" << std::endl;
    std::cout << "Total soft bodies created: " << defMgr.getVolumeCount() << std::endl;

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "This example demonstrated:" << std::endl;
    std::cout << "  - GPU-accelerated soft body simulation" << std::endl;
    std::cout << "  - Creating deformable volumes (cube, sphere)" << std::endl;
    std::cout << "  - Material configuration (stiffness, damping)" << std::endl;
    std::cout << "  - Collision with ground and rigid bodies" << std::endl;
    std::cout << "  - Querying deformed vertex positions" << std::endl;

    std::cout << "\nNote: This example requires GPU/CUDA support." << std::endl;
    std::cout << "If you see initialization errors, check your GPU setup." << std::endl;

    std::cout << "\nCleaning up..." << std::endl;
    defMgr.cleanup();

    return 0;
}
