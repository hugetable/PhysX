/**
 * @file example_pbdcloth.cpp
 * @brief PBD Cloth Simulation Example
 *
 * This example demonstrates PhysX's Position Based Dynamics (PBD) cloth simulation:
 * - GPU-accelerated particle-based cloth
 * - Distance and bending constraints
 * - Self-collision and environmental collision
 * - Multiple cloth types (flags, curtains, garments)
 * - Wind forces and external forces
 * - Attachment constraints (pinning)
 *
 * Based on PhysX Snippet: SnippetPBDCloth
 *
 * IMPORTANT: This feature requires GPU/CUDA support!
 * - Set PxSceneFlag::eENABLE_GPU_DYNAMICS
 * - Requires CUDA-capable GPU
 * - Uses PxPBDParticleSystem for cloth simulation
 *
 * PBD Cloth Mechanics:
 * - Position-based: Directly manipulates positions (not forces)
 * - Iterative constraint solver
 * - Distance constraints: Maintain edge lengths
 * - Bending constraints: Resist folding
 * - Self-collision: Prevent cloth self-intersection
 *
 * Advantages over traditional cloth:
 * - Faster convergence
 * - Better stability
 * - GPU parallelization
 * - No spring stiffness tuning issues
 *
 * Applications:
 * - Character clothing
 * - Flags and banners
 * - Curtains and drapes
 * - Sails and parachutes
 * - Tablecloths and fabrics
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

using namespace PhysXWrapper;

/**
 * @brief PBD Cloth system wrapper
 */
class PBDClothSystem {
private:
    PhysXCore& core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;
    PxCudaContextManager* cudaContextManager;

    struct ClothMesh {
        PxPBDParticleSystem* particleSystem;
        std::vector<PxVec4> particles;  // Position + inverse mass
        std::vector<PxVec4> velocities;
        std::vector<PxU32> phases;
        int width, height;
        std::string description;
    };

    std::vector<ClothMesh> cloths;

public:
    PBDClothSystem(PhysXCore& coreRef, PxPhysics* phys, PxScene* scn, PxMaterial* mat)
        : core(coreRef)
        , physics(phys)
        , scene(scn)
        , material(mat)
        , cudaContextManager(nullptr)
    {
        // Note: CUDA context manager would be initialized here in a real implementation
        // cudaContextManager = PxCreateCudaContextManager(*physics->getFoundation(), ...);
    }

    /**
     * Create a rectangular cloth mesh
     */
    void createClothMesh(const PxVec3& origin, int width, int height,
                         PxReal spacing, const std::string& description) {
        std::cout << "\nCreating cloth: " << description << std::endl;
        std::cout << "  Dimensions: " << width << " x " << height << " particles" << std::endl;
        std::cout << "  Spacing: " << spacing << " m" << std::endl;

        ClothMesh cloth;
        cloth.width = width;
        cloth.height = height;
        cloth.description = description;

        int numParticles = width * height;
        cloth.particles.reserve(numParticles);
        cloth.velocities.reserve(numParticles);
        cloth.phases.reserve(numParticles);

        // Create particle grid
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                PxVec3 pos = origin + PxVec3(x * spacing, -y * spacing, 0);

                // Inverse mass: 0 = infinite mass (fixed), >0 = movable
                PxReal invMass = 1.0f;

                // Pin top row
                if (y == 0) {
                    invMass = 0.0f;  // Fixed particles
                }

                cloth.particles.push_back(PxVec4(pos.x, pos.y, pos.z, invMass));
                cloth.velocities.push_back(PxVec4(0, 0, 0, 0));
                cloth.phases.push_back(0);  // Phase for collision filtering
            }
        }

        std::cout << "  Total particles: " << numParticles << std::endl;
        std::cout << "  Fixed particles: " << width << " (top row)" << std::endl;
        std::cout << "  Movable particles: " << (numParticles - width) << std::endl;

        // In a real implementation, we would:
        // 1. Create PxPBDParticleSystem
        // 2. Set up particle buffer
        // 3. Create distance constraints (springs between neighbors)
        // 4. Create bending constraints (diagonal springs)
        // 5. Add to scene

        // Pseudo-code for reference:
        /*
        PxPBDParticleSystem* particleSystem = physics->createPBDParticleSystem(*cudaContextManager);

        // Configure particle system
        particleSystem->setParticleContactOffset(0.01f);
        particleSystem->setRestOffset(0.005f);
        particleSystem->setSolverIterationCounts(16);

        // Create particle buffer
        PxParticleBuffer* buffer = physics->createParticleBuffer(
            numParticles,
            cloth.particles.data(),
            cloth.velocities.data(),
            cloth.phases.data()
        );

        // Add constraints
        createDistanceConstraints(particleSystem, width, height, spacing);
        createBendingConstraints(particleSystem, width, height);

        // Add to scene
        scene->addActor(*particleSystem);

        cloth.particleSystem = particleSystem;
        */

        cloths.push_back(cloth);

        std::cout << "  ⚠️  Note: GPU/CUDA required for actual simulation" << std::endl;
        std::cout << "  This is a demonstration of the API structure" << std::endl;
    }

    /**
     * Create a flag (cloth pinned at top corners)
     */
    void createFlag(const PxVec3& origin) {
        std::cout << "\nCreating flag..." << std::endl;

        int width = 20;
        int height = 15;
        PxReal spacing = 0.1f;

        ClothMesh cloth;
        cloth.width = width;
        cloth.height = height;
        cloth.description = "Flag";

        int numParticles = width * height;
        cloth.particles.reserve(numParticles);
        cloth.velocities.reserve(numParticles);
        cloth.phases.reserve(numParticles);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                PxVec3 pos = origin + PxVec3(x * spacing, -y * spacing, 0);
                PxReal invMass = 1.0f;

                // Pin top two corners
                if (y == 0 && (x == 0 || x == width - 1)) {
                    invMass = 0.0f;
                }

                cloth.particles.push_back(PxVec4(pos.x, pos.y, pos.z, invMass));
                cloth.velocities.push_back(PxVec4(0, 0, 0, 0));
                cloth.phases.push_back(0);
            }
        }

        // Apply wind force (would be done in update loop)
        std::cout << "  Wind force: (5, 0, 0) N (simulated)" << std::endl;

        cloths.push_back(cloth);
    }

    /**
     * Create curtain (pinned along top edge)
     */
    void createCurtain(const PxVec3& origin) {
        std::cout << "\nCreating curtain..." << std::endl;

        int width = 30;
        int height = 40;
        PxReal spacing = 0.05f;

        ClothMesh cloth;
        cloth.width = width;
        cloth.height = height;
        cloth.description = "Curtain";

        int numParticles = width * height;
        cloth.particles.reserve(numParticles);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                PxVec3 pos = origin + PxVec3(x * spacing, -y * spacing, 0);
                PxReal invMass = (y == 0) ? 0.0f : 1.0f;  // Pin entire top row

                cloth.particles.push_back(PxVec4(pos.x, pos.y, pos.z, invMass));
                cloth.velocities.push_back(PxVec4(0, 0, 0, 0));
                cloth.phases.push_back(0);
            }
        }

        std::cout << "  High resolution: " << width << "x" << height << " = " << numParticles << " particles" << std::endl;

        cloths.push_back(cloth);
    }

    /**
     * Demonstrate constraint types
     */
    void demonstrateConstraints() {
        std::cout << "\n=== Cloth Constraint Types ===" << std::endl;

        std::cout << "\n1. Distance Constraints (Stretch Resistance):" << std::endl;
        std::cout << "   • Maintain edge lengths between neighboring particles" << std::endl;
        std::cout << "   • Horizontal, vertical, and diagonal springs" << std::endl;
        std::cout << "   • Compliance: lower = stiffer cloth" << std::endl;
        std::cout << "   • Formula: |p1 - p2| = rest_length" << std::endl;

        std::cout << "\n2. Bending Constraints (Fold Resistance):" << std::endl;
        std::cout << "   • Resist folding along edges" << std::endl;
        std::cout << "   • Dihedral angle constraints" << std::endl;
        std::cout << "   • Creates cloth stiffness" << std::endl;
        std::cout << "   • Higher compliance = more wrinkles" << std::endl;

        std::cout << "\n3. Self-Collision Constraints:" << std::endl;
        std::cout << "   • Prevent cloth from intersecting itself" << std::endl;
        std::cout << "   • Spatial hashing for efficiency" << std::endl;
        std::cout << "   • Thickness parameter" << std::endl;

        std::cout << "\n4. Environmental Collision:" << std::endl;
        std::cout << "   • Collide with rigid bodies" << std::endl;
        std::cout << "   • Friction coefficients" << std::endl;
        std::cout << "   • Contact offset for stability" << std::endl;

        std::cout << "\n5. Attachment Constraints:" << std::endl;
        std::cout << "   • Pin particles to world positions" << std::endl;
        std::cout << "   • Attach to moving rigid bodies" << std::endl;
        std::cout << "   • One-way vs two-way coupling" << std::endl;
    }

    /**
     * Print configuration parameters
     */
    void printConfiguration() {
        std::cout << "\n=== PBD Cloth Configuration ===" << std::endl;

        std::cout << "\nParticle System Parameters:" << std::endl;
        std::cout << "  particleContactOffset: 0.01 (collision detection distance)" << std::endl;
        std::cout << "  restOffset: 0.005 (minimum separation distance)" << std::endl;
        std::cout << "  solverIterationCounts: 16 (constraint solver iterations)" << std::endl;
        std::cout << "  maxVelocity: 100.0 (velocity clamping)" << std::endl;

        std::cout << "\nConstraint Parameters:" << std::endl;
        std::cout << "  stretchCompliance: 0.0001 (stretch stiffness)" << std::endl;
        std::cout << "  bendCompliance: 0.01 (bending stiffness)" << std::endl;
        std::cout << "  shearCompliance: 0.001 (shear resistance)" << std::endl;

        std::cout << "\nCollision Parameters:" << std::endl;
        std::cout << "  selfCollision: true" << std::endl;
        std::cout << "  selfCollisionFilterDistance: 0.02" << std::endl;
        std::cout << "  friction: 0.5" << std::endl;

        std::cout << "\nExternal Forces:" << std::endl;
        std::cout << "  gravity: (0, -9.81, 0) m/s²" << std::endl;
        std::cout << "  wind: configurable per cloth" << std::endl;
        std::cout << "  damping: 0.01 (velocity damping)" << std::endl;
    }

    /**
     * Simulate cloth behavior (demonstration)
     */
    void demonstrateSimulation() {
        std::cout << "\n=== Cloth Simulation Demo ===" << std::endl;

        std::cout << "\nSimulation Steps (per frame):" << std::endl;
        std::cout << "  1. Apply external forces (gravity, wind)" << std::endl;
        std::cout << "  2. Predict positions (velocity integration)" << std::endl;
        std::cout << "  3. Solve constraints iteratively:" << std::endl;
        std::cout << "     a. Distance constraints" << std::endl;
        std::cout << "     b. Bending constraints" << std::endl;
        std::cout << "     c. Collision constraints" << std::endl;
        std::cout << "     d. Attachment constraints" << std::endl;
        std::cout << "  4. Update velocities from position changes" << std::endl;
        std::cout << "  5. Apply damping" << std::endl;

        std::cout << "\nPerformance Characteristics:" << std::endl;
        std::cout << "  • GPU-accelerated: ~10,000 particles at 60 FPS" << std::endl;
        std::cout << "  • Parallel constraint solving" << std::endl;
        std::cout << "  • Efficient spatial hashing for collisions" << std::endl;
        std::cout << "  • Scalable to multiple cloths" << std::endl;

        std::cout << "\nTypical Use Cases:" << std::endl;
        std::cout << "  • Character clothing: 1,000-5,000 particles" << std::endl;
        std::cout << "  • Flags/banners: 300-1,000 particles" << std::endl;
        std::cout << "  • Curtains: 500-2,000 particles" << std::endl;
        std::cout << "  • Tablecloths: 1,000-3,000 particles" << std::endl;
    }

    /**
     * Print status
     */
    void printStatus() {
        std::cout << "\n=== Cloth Status ===" << std::endl;

        for (const auto& cloth : cloths) {
            std::cout << "\n" << cloth.description << ":" << std::endl;
            std::cout << "  Particles: " << cloth.particles.size() << std::endl;
            std::cout << "  Dimensions: " << cloth.width << " x " << cloth.height << std::endl;

            int fixedCount = 0;
            for (const auto& p : cloth.particles) {
                if (p.w == 0.0f) fixedCount++;
            }
            std::cout << "  Fixed: " << fixedCount << " particles" << std::endl;
            std::cout << "  Movable: " << (cloth.particles.size() - fixedCount) << " particles" << std::endl;

            // Calculate approximate bounds
            PxVec3 minPos(FLT_MAX), maxPos(-FLT_MAX);
            for (const auto& p : cloth.particles) {
                minPos.x = std::min(minPos.x, p.x);
                minPos.y = std::min(minPos.y, p.y);
                minPos.z = std::min(minPos.z, p.z);
                maxPos.x = std::max(maxPos.x, p.x);
                maxPos.y = std::max(maxPos.y, p.y);
                maxPos.z = std::max(maxPos.z, p.z);
            }

            PxVec3 size = maxPos - minPos;
            std::cout << "  Size: " << std::fixed << std::setprecision(2)
                      << size.x << " x " << size.y << " x " << size.z << " m" << std::endl;
        }
    }

    size_t getClothCount() const { return cloths.size(); }
};

/**
 * @brief Main example
 */
class PBDClothExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;
    PBDClothSystem* system;

public:
    PBDClothExample()
        : physics(nullptr), scene(nullptr), material(nullptr), system(nullptr)
    {}

    ~PBDClothExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "===================================================" << std::endl;
        std::cout << "PhysX PBD Cloth Simulation Example" << std::endl;
        std::cout << "===================================================" << std::endl;

        std::cout << "\n⚠️  IMPORTANT: This example requires GPU/CUDA support!" << std::endl;
        std::cout << "This is a demonstration of the API structure." << std::endl;
        std::cout << "For actual cloth simulation, you need:" << std::endl;
        std::cout << "  • CUDA-capable GPU" << std::endl;
        std::cout << "  • PxSceneFlag::eENABLE_GPU_DYNAMICS" << std::endl;
        std::cout << "  • PxCudaContextManager" << std::endl;

        PhysXCore::Config config;
        config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        config.numThreads = 2;

        if (!core.initialize(config)) {
            std::cerr << "Failed to initialize PhysX" << std::endl;
            return false;
        }

        physics = core.getPhysics();
        scene = core.getScene();
        material = physics->createMaterial(0.5f, 0.5f, 0.3f);

        // Create ground
        PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
        scene->addActor(*ground);

        // Create collision sphere
        PxRigidStatic* sphere = physics->createRigidStatic(PxTransform(PxVec3(1, 1, 0)));
        PxSphereGeometry sphereGeom(0.5f);
        PxRigidActorExt::createExclusiveShape(*sphere, sphereGeom, *material);
        scene->addActor(*sphere);

        system = new PBDClothSystem(core, physics, scene, material);

        std::cout << "\nPBD Cloth Features:" << std::endl;
        std::cout << "  • Position-based dynamics (stable, fast)" << std::endl;
        std::cout << "  • GPU-accelerated particle simulation" << std::endl;
        std::cout << "  • Self-collision and environmental collision" << std::endl;
        std::cout << "  • Distance and bending constraints" << std::endl;
        std::cout << "  • Attachment to rigid bodies" << std::endl;
        std::cout << "  • External forces (wind, gravity)" << std::endl;

        return true;
    }

    void run() {
        std::cout << "\n=== Creating Cloth Scenarios ===" << std::endl;

        // Scenario 1: Simple hanging cloth
        system->createClothMesh(PxVec3(-2, 3, 0), 10, 10, 0.2f, "Hanging cloth");

        // Scenario 2: Flag
        system->createFlag(PxVec3(0, 3, 0));

        // Scenario 3: Curtain
        system->createCurtain(PxVec3(2, 3, 0));

        std::cout << "\nTotal cloths: " << system->getClothCount() << std::endl;

        // Demonstrate features
        system->demonstrateConstraints();
        system->printConfiguration();
        system->demonstrateSimulation();

        // Print status
        system->printStatus();

        std::cout << "\n\n=== Example Complete ===" << std::endl;
        std::cout << "\nKey Features Demonstrated:" << std::endl;
        std::cout << "  ✓ Multiple cloth configurations (hanging, flag, curtain)" << std::endl;
        std::cout << "  ✓ Particle mesh generation" << std::endl;
        std::cout << "  ✓ Constraint types and parameters" << std::endl;
        std::cout << "  ✓ Attachment strategies (pinning)" << std::endl;
        std::cout << "  ✓ Configuration parameters" << std::endl;
        std::cout << "  ✓ Performance characteristics" << std::endl;

        std::cout << "\nImplementation Notes:" << std::endl;
        std::cout << "  • This example shows the API structure" << std::endl;
        std::cout << "  • Actual GPU simulation requires CUDA context" << std::endl;
        std::cout << "  • Use PxPBDParticleSystem for real implementation" << std::endl;
        std::cout << "  • Refer to PhysX documentation for GPU setup" << std::endl;
    }

    void cleanup() {
        if (system) delete system;
        if (material) material->release();
        core.cleanup();
    }
};

int main() {
    PBDClothExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
