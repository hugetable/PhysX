/**
 * @file example_pbdfluid.cpp
 * @brief Example demonstrating PBD fluid simulation
 *
 * This example shows how to use PBDFluidManager to create particle-based fluids:
 * 1. Basic water simulation with container
 * 2. Multi-material fluids (water, oil, honey)
 * 3. Diffuse particles (foam, spray, bubbles)
 * 4. Fluid-rigid body interaction
 *
 * IMPORTANT: This example requires GPU/CUDA support.
 * Ensure you have:
 * - NVIDIA GPU with CUDA support
 * - CUDA Toolkit installed
 * - GPU-enabled PhysX build
 */

#include <iostream>
#include <Core/PhysXCore.h>
#include <Particle/PBDFluidManager.h>
#include <PxPhysicsAPI.h>

using namespace PhysXWrapper;
using namespace physx;

// ============================================================================
// Helper Functions
// ============================================================================

void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================" << std::endl;
}

void createContainer(PxPhysics* physics, PxScene* scene, PxMaterial* material,
                     const PxVec3& center, const PxVec3& size) {
    // Ground
    PxRigidStatic* ground = PxCreatePlane(*physics,
        PxPlane(0.f, 1.f, 0.f, center.y), *material);
    scene->addActor(*ground);

    // Walls
    PxRigidStatic* wall1 = PxCreatePlane(*physics,
        PxPlane(1.f, 0.f, 0.f, -center.x + size.x), *material);
    scene->addActor(*wall1);

    PxRigidStatic* wall2 = PxCreatePlane(*physics,
        PxPlane(-1.f, 0.f, 0.f, center.x + size.x), *material);
    scene->addActor(*wall2);

    PxRigidStatic* wall3 = PxCreatePlane(*physics,
        PxPlane(0.f, 0.f, 1.f, -center.z + size.z), *material);
    scene->addActor(*wall3);

    PxRigidStatic* wall4 = PxCreatePlane(*physics,
        PxPlane(0.f, 0.f, -1.f, center.z + size.z), *material);
    scene->addActor(*wall4);
}

void addFloatingBox(PxPhysics* physics, PxScene* scene, PxMaterial* material,
                    const PxVec3& position, PxReal size, PxReal density) {
    PxShape* shape = physics->createShape(
        PxBoxGeometry(size * 0.5f, size * 0.5f, size * 0.5f), *material);

    PxRigidDynamic* body = physics->createRigidDynamic(PxTransform(position));
    body->attachShape(*shape);

    PxReal mass = size * size * size * density;
    PxRigidBodyExt::updateMassAndInertia(*body, mass);

    scene->addActor(*body);
    shape->release();
}

// ============================================================================
// Test 1: Basic Water Simulation
// ============================================================================

void test1_BasicWaterSimulation() {
    printSeparator("Test 1: Basic Water Simulation");

    std::cout << "Creating basic water simulation with single fluid volume..." << std::endl;

    // Initialize PhysX with GPU support
    PhysXCore physx;
    physx.initialize();

    // Create GPU-enabled scene
    PhysXCore::SceneConfig sceneConfig;
    sceneConfig.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneConfig.enableGPU = true;  // Important!
    physx.createScene(sceneConfig);

    PxPhysics* physics = physx.getPhysics();
    PxScene* scene = physx.getScene();

    // Check GPU availability
    if (!PBDFluidManager::isGPUAvailable(physics)) {
        std::cerr << "ERROR: GPU/CUDA not available!" << std::endl;
        std::cerr << "PBD fluid simulation requires GPU support." << std::endl;
        std::cerr << "Please ensure:" << std::endl;
        std::cerr << "  - NVIDIA GPU with CUDA support" << std::endl;
        std::cerr << "  - CUDA Toolkit installed" << std::endl;
        std::cerr << "  - GPU-enabled PhysX build" << std::endl;
        return;
    }

    std::cout << "✓ GPU/CUDA available" << std::endl;

    // Create material
    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);

    // Create container
    createContainer(physics, scene, material, PxVec3(0, 0, 0), PxVec3(5, 10, 5));
    std::cout << "✓ Created container (10x10x10 meters)" << std::endl;

    // Initialize fluid manager
    PBDFluidManager fluidMgr;
    if (!fluidMgr.initialize(physics, scene)) {
        std::cerr << "ERROR: " << fluidMgr.getLastError() << std::endl;
        return;
    }
    std::cout << "✓ Initialized PBD fluid manager" << std::endl;

    // Configure water material
    PBDMaterialConfig waterMat = PBDMaterialPresets::Water();
    std::cout << "Water properties:" << std::endl;
    std::cout << "  Viscosity: " << waterMat.viscosity << std::endl;
    std::cout << "  Surface tension: " << waterMat.surfaceTension << std::endl;
    std::cout << "  Cohesion: " << waterMat.cohesion << std::endl;

    // Create particle system
    ParticleSystemConfig sysConfig;
    sysConfig.particleSpacing = 0.1f;
    sysConfig.fluidDensity = 1000.0f;  // Water density
    sysConfig.maxNeighborsPerParticle = 96;

    PxPBDParticleSystem* system = fluidMgr.createParticleSystem(
        sysConfig, waterMat);

    if (!system) {
        std::cerr << "ERROR: " << fluidMgr.getLastError() << std::endl;
        return;
    }
    std::cout << "✓ Created particle system" << std::endl;

    // Add water volume (20x30x20 particles = 12,000 particles)
    FluidVolumeConfig volume;
    volume.numX = 20;
    volume.numY = 30;
    volume.numZ = 20;
    volume.position = PxVec3(-1.0f, 3.0f, -1.0f);

    ParticleBufferHandle* buffer = fluidMgr.addFluidVolume(
        system, volume, sysConfig);

    if (!buffer) {
        std::cerr << "ERROR: " << fluidMgr.getLastError() << std::endl;
        return;
    }

    std::cout << "✓ Added water volume: " << buffer->maxParticles
              << " particles" << std::endl;

    // Add floating box
    addFloatingBox(physics, scene, material, PxVec3(0, 5, 0), 1.0f, 500.0f);
    std::cout << "✓ Added floating box (density: 500 kg/m³)" << std::endl;

    // Simulate
    std::cout << "\nSimulating water falling into container..." << std::endl;
    const PxReal dt = 1.0f / 60.0f;

    for (int i = 0; i < 300; ++i) {
        scene->simulate(dt);
        scene->fetchResults(true);
        scene->fetchResultsParticleSystem();  // Important for particles!

        if (i % 60 == 0) {
            ParticleData data = fluidMgr.getParticleData(system);
            std::cout << "Frame " << i << ": " << data.activeCount
                      << " active particles" << std::endl;

            // Sample: Print position of first particle
            if (!data.positions.empty()) {
                const PxVec4& pos = data.positions[0];
                std::cout << "  Particle 0: (" << pos.x << ", " << pos.y
                          << ", " << pos.z << ")" << std::endl;
            }
        }
    }

    std::cout << "\n✓ Test 1 completed successfully!" << std::endl;
}

// ============================================================================
// Test 2: Multi-Material Fluids
// ============================================================================

void test2_MultiMaterialFluids() {
    printSeparator("Test 2: Multi-Material Fluids");

    std::cout << "Creating simulation with water, oil, and honey..." << std::endl;

    // Initialize PhysX with GPU support
    PhysXCore physx;
    physx.initialize();

    PhysXCore::SceneConfig sceneConfig;
    sceneConfig.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneConfig.enableGPU = true;
    physx.createScene(sceneConfig);

    PxPhysics* physics = physx.getPhysics();
    PxScene* scene = physx.getScene();

    if (!PBDFluidManager::isGPUAvailable(physics)) {
        std::cerr << "ERROR: GPU/CUDA not available!" << std::endl;
        return;
    }

    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);
    createContainer(physics, scene, material, PxVec3(0, 0, 0), PxVec3(6, 10, 6));

    // Initialize fluid manager
    PBDFluidManager fluidMgr;
    if (!fluidMgr.initialize(physics, scene)) {
        std::cerr << "ERROR: " << fluidMgr.getLastError() << std::endl;
        return;
    }

    // Create particle system with default material
    ParticleSystemConfig sysConfig;
    sysConfig.particleSpacing = 0.1f;
    sysConfig.fluidDensity = 1000.0f;

    PxPBDParticleSystem* system = fluidMgr.createParticleSystem(sysConfig);

    // Define three fluid volumes
    std::vector<FluidVolumeConfig> volumes(3);

    // Water (left)
    volumes[0].numX = 20;
    volumes[0].numY = 40;
    volumes[0].numZ = 20;
    volumes[0].position = PxVec3(-3.0f, 3.0f, -1.0f);

    // Oil (center)
    volumes[1].numX = 20;
    volumes[1].numY = 40;
    volumes[1].numZ = 20;
    volumes[1].position = PxVec3(-1.0f, 3.0f, -1.0f);

    // Honey (right)
    volumes[2].numX = 20;
    volumes[2].numY = 40;
    volumes[2].numZ = 20;
    volumes[2].position = PxVec3(1.0f, 3.0f, -1.0f);

    // Define three materials
    std::vector<PBDMaterialConfig> materials = {
        PBDMaterialPresets::Water(),
        PBDMaterialPresets::Oil(),
        PBDMaterialPresets::Honey()
    };

    std::cout << "Material properties:" << std::endl;
    std::cout << "  Water: viscosity=" << materials[0].viscosity << std::endl;
    std::cout << "  Oil:   viscosity=" << materials[1].viscosity << std::endl;
    std::cout << "  Honey: viscosity=" << materials[2].viscosity << std::endl;

    // Add multi-material volumes
    ParticleBufferHandle* buffer = fluidMgr.addMultiMaterialFluidVolumes(
        system, volumes, materials, sysConfig);

    if (!buffer) {
        std::cerr << "ERROR: " << fluidMgr.getLastError() << std::endl;
        return;
    }

    std::cout << "✓ Created multi-material fluid: " << buffer->maxParticles
              << " particles" << std::endl;

    // Simulate
    std::cout << "\nSimulating multi-material interaction..." << std::endl;
    const PxReal dt = 1.0f / 60.0f;

    for (int i = 0; i < 300; ++i) {
        scene->simulate(dt);
        scene->fetchResults(true);
        scene->fetchResultsParticleSystem();

        if (i % 60 == 0) {
            ParticleData data = fluidMgr.getParticleData(system);
            std::cout << "Frame " << i << ": " << data.activeCount
                      << " active particles" << std::endl;
        }
    }

    std::cout << "\n✓ Test 2 completed successfully!" << std::endl;
}

// ============================================================================
// Test 3: Diffuse Particles (Foam, Spray, Bubbles)
// ============================================================================

void test3_DiffuseParticles() {
    printSeparator("Test 3: Diffuse Particles");

    std::cout << "Creating water simulation with foam/spray/bubbles..." << std::endl;

    // Initialize PhysX with GPU support
    PhysXCore physx;
    physx.initialize();

    PhysXCore::SceneConfig sceneConfig;
    sceneConfig.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneConfig.enableGPU = true;
    physx.createScene(sceneConfig);

    PxPhysics* physics = physx.getPhysics();
    PxScene* scene = physx.getScene();

    if (!PBDFluidManager::isGPUAvailable(physics)) {
        std::cerr << "ERROR: GPU/CUDA not available!" << std::endl;
        return;
    }

    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.2f);
    createContainer(physics, scene, material, PxVec3(0, 0, 0), PxVec3(5, 10, 5));

    // Initialize fluid manager
    PBDFluidManager fluidMgr;
    if (!fluidMgr.initialize(physics, scene)) {
        std::cerr << "ERROR: " << fluidMgr.getLastError() << std::endl;
        return;
    }

    // Configure diffuse particles
    DiffuseParticleConfig diffuseConfig;
    diffuseConfig.enable = true;
    diffuseConfig.maxDiffuseParticles = 200000;
    diffuseConfig.threshold = 300.0f;
    diffuseConfig.lifetime = 2.0f;
    diffuseConfig.bubbleDrag = 0.9f;
    diffuseConfig.buoyancy = 0.9f;
    diffuseConfig.kineticEnergyWeight = 0.01f;
    diffuseConfig.pressureWeight = 1.0f;
    diffuseConfig.divergenceWeight = 10.0f;

    std::cout << "Diffuse particle configuration:" << std::endl;
    std::cout << "  Max diffuse particles: " << diffuseConfig.maxDiffuseParticles << std::endl;
    std::cout << "  Lifetime: " << diffuseConfig.lifetime << " seconds" << std::endl;
    std::cout << "  Bubble buoyancy: " << diffuseConfig.buoyancy << std::endl;

    // Create particle system with diffuse particles
    ParticleSystemConfig sysConfig;
    sysConfig.particleSpacing = 0.1f;
    sysConfig.fluidDensity = 1000.0f;

    PBDMaterialConfig waterMat = PBDMaterialPresets::Water();
    waterMat.vorticityConfinement = 10.0f;  // More turbulence

    PxPBDParticleSystem* system = fluidMgr.createParticleSystem(
        sysConfig, waterMat, diffuseConfig);

    // Add water volume falling from height (creates splash)
    FluidVolumeConfig volume;
    volume.numX = 25;
    volume.numY = 50;
    volume.numZ = 25;
    volume.position = PxVec3(-1.25f, 8.0f, -1.25f);  // Higher drop

    ParticleBufferHandle* buffer = fluidMgr.addFluidVolume(
        system, volume, sysConfig);

    std::cout << "✓ Created water volume: " << buffer->maxParticles
              << " particles" << std::endl;

    // Simulate
    std::cout << "\nSimulating with diffuse particles..." << std::endl;
    const PxReal dt = 1.0f / 60.0f;

    for (int i = 0; i < 400; ++i) {
        scene->simulate(dt);
        scene->fetchResults(true);
        scene->fetchResultsParticleSystem();

        if (i % 60 == 0) {
            ParticleData data = fluidMgr.getParticleData(system);

            // Get diffuse particle data
            std::vector<PxVec4> diffusePos;
            std::vector<PxVec4> diffuseVel;
            std::vector<PxReal> diffuseLife;

            PxU32 numDiffuse = fluidMgr.getDiffuseParticleData(
                buffer, diffusePos, diffuseVel, diffuseLife);

            std::cout << "Frame " << i << ":" << std::endl;
            std::cout << "  Fluid particles: " << data.activeCount << std::endl;
            std::cout << "  Diffuse particles: " << numDiffuse
                      << " (foam/spray/bubbles)" << std::endl;

            if (numDiffuse > 0) {
                std::cout << "  Sample diffuse particle:" << std::endl;
                std::cout << "    Position: (" << diffusePos[0].x << ", "
                          << diffusePos[0].y << ", " << diffusePos[0].z << ")" << std::endl;
                std::cout << "    Lifetime: " << diffuseLife[0] << " seconds" << std::endl;
            }
        }
    }

    std::cout << "\n✓ Test 3 completed successfully!" << std::endl;
}

// ============================================================================
// Test 4: Fluid-Rigid Body Interaction
// ============================================================================

void test4_FluidRigidBodyInteraction() {
    printSeparator("Test 4: Fluid-Rigid Body Interaction");

    std::cout << "Creating water simulation with multiple floating objects..." << std::endl;

    // Initialize PhysX with GPU support
    PhysXCore physx;
    physx.initialize();

    PhysXCore::SceneConfig sceneConfig;
    sceneConfig.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneConfig.enableGPU = true;
    physx.createScene(sceneConfig);

    PxPhysics* physics = physx.getPhysics();
    PxScene* scene = physx.getScene();

    if (!PBDFluidManager::isGPUAvailable(physics)) {
        std::cerr << "ERROR: GPU/CUDA not available!" << std::endl;
        return;
    }

    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.3f);
    createContainer(physics, scene, material, PxVec3(0, 0, 0), PxVec3(6, 10, 6));

    // Initialize fluid manager
    PBDFluidManager fluidMgr;
    if (!fluidMgr.initialize(physics, scene)) {
        std::cerr << "ERROR: " << fluidMgr.getLastError() << std::endl;
        return;
    }

    // Create water
    ParticleSystemConfig sysConfig;
    sysConfig.particleSpacing = 0.1f;
    sysConfig.fluidDensity = 1000.0f;

    PBDMaterialConfig waterMat = PBDMaterialPresets::Water();

    PxPBDParticleSystem* system = fluidMgr.createParticleSystem(
        sysConfig, waterMat);

    // Large water volume
    FluidVolumeConfig volume;
    volume.numX = 40;
    volume.numY = 60;
    volume.numZ = 40;
    volume.position = PxVec3(-2.0f, 3.0f, -2.0f);

    fluidMgr.addFluidVolume(system, volume, sysConfig);
    std::cout << "✓ Created large water volume" << std::endl;

    // Add floating objects with different densities
    std::cout << "Adding objects with different densities:" << std::endl;

    // Light (wood) - floats high
    addFloatingBox(physics, scene, material, PxVec3(-2, 8, 0), 1.0f, 300.0f);
    std::cout << "  Wood box (300 kg/m³) - will float high" << std::endl;

    // Medium (plastic) - floats partially
    addFloatingBox(physics, scene, material, PxVec3(0, 8, 0), 1.0f, 800.0f);
    std::cout << "  Plastic box (800 kg/m³) - will float partially" << std::endl;

    // Heavy (stone) - sinks
    addFloatingBox(physics, scene, material, PxVec3(2, 8, 0), 1.0f, 2000.0f);
    std::cout << "  Stone box (2000 kg/m³) - will sink" << std::endl;

    // Simulate
    std::cout << "\nSimulating fluid-rigid body interaction..." << std::endl;
    const PxReal dt = 1.0f / 60.0f;

    for (int i = 0; i < 400; ++i) {
        scene->simulate(dt);
        scene->fetchResults(true);
        scene->fetchResultsParticleSystem();

        if (i % 60 == 0) {
            std::cout << "Frame " << i << std::endl;

            // Get statistics
            PxU32 totalParticles = fluidMgr.getTotalParticleCount();
            PxU32 numSystems = fluidMgr.getSystemCount();

            std::cout << "  Particle systems: " << numSystems << std::endl;
            std::cout << "  Total particles: " << totalParticles << std::endl;
        }
    }

    std::cout << "\n✓ Test 4 completed successfully!" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "PhysXWrapper - PBD Fluid Simulation Examples" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "\nIMPORTANT: These examples require GPU/CUDA support." << std::endl;
    std::cout << "If tests fail, please check:" << std::endl;
    std::cout << "  - NVIDIA GPU with CUDA capability" << std::endl;
    std::cout << "  - CUDA Toolkit installed" << std::endl;
    std::cout << "  - GPU-enabled PhysX build" << std::endl;
    std::cout << "\nStarting tests...\n" << std::endl;

    try {
        // Test 1: Basic water simulation
        test1_BasicWaterSimulation();

        // Test 2: Multi-material fluids
        test2_MultiMaterialFluids();

        // Test 3: Diffuse particles
        test3_DiffuseParticles();

        // Test 4: Fluid-rigid body interaction
        test4_FluidRigidBodyInteraction();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\nERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
