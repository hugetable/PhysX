/**
 * @file PBDFluidManager.h
 * @brief Position-Based Dynamics fluid simulation system for PhysX
 *
 * This class provides utilities for particle-based fluid simulation:
 * - Position-Based Dynamics (PBD) fluid solver
 * - GPU-accelerated particle simulation (CUDA required)
 * - Diffuse particles (foam, spray, bubbles)
 * - Multi-material fluid support
 * - Fluid-rigid body interaction
 * - Particle buffer management
 *
 * PBD fluids are useful for:
 * - Realistic water simulation
 * - Multiple fluid types (water, oil, honey, etc.)
 * - Visual effects (splashes, waves, foam)
 * - Interactive fluid dynamics
 *
 * IMPORTANT: This feature requires GPU/CUDA support and GPU-enabled scene.
 *
 * Based on SnippetPBF and SnippetPBFMultiMat from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include "extensions/PxParticleExt.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace PhysXWrapper {

using namespace physx;
using namespace ExtGpu;

/**
 * @brief PBD material configuration
 */
struct PBDMaterialConfig {
    // Basic properties
    PxReal friction = 0.05f;                  ///< Friction coefficient (0-1)
    PxReal damping = 0.0f;                    ///< Damping coefficient (0-1)
    PxReal adhesion = 0.0f;                   ///< Adhesion to surfaces (0-inf)

    // Fluid properties
    PxReal viscosity = 0.001f;                ///< Viscosity (0-inf, water ~0.001)
    PxReal surfaceTension = 0.0074f;          ///< Surface tension (N/m, water ~0.0728)
    PxReal cohesion = 0.0f;                   ///< Inter-particle cohesion (0-inf)

    // Advanced properties
    PxReal vorticityConfinement = 0.0f;       ///< Vorticity boost (0-inf)
    PxReal particleFrictionScale = 0.5f;      ///< Particle-particle friction (0-1)
    PxReal particleAdhesionScale = 0.005f;    ///< Particle-particle adhesion (0-inf)

    // Solver properties
    PxReal lift = 0.0f;                       ///< Lift force (0-inf)
    PxReal drag = 0.01f;                      ///< Drag force (0-inf)
    PxReal cflCoefficient = 1.0f;             ///< CFL condition coefficient (0.1-5)
    PxReal gravityScale = 1.0f;               ///< Gravity multiplier (0-inf)
};

/**
 * @brief Diffuse particle configuration
 */
struct DiffuseParticleConfig {
    bool enable = false;                      ///< Enable diffuse particles
    PxU32 maxDiffuseParticles = 100000;       ///< Maximum diffuse particles

    PxReal threshold = 300.0f;                ///< Generation threshold
    PxReal lifetime = 2.0f;                   ///< Particle lifetime (seconds)

    // Bubble properties
    PxReal bubbleDrag = 0.9f;                 ///< Bubble drag (0-1)
    PxReal buoyancy = 0.9f;                   ///< Bubble buoyancy (0-1)

    // Foam/Spray properties
    PxReal airDrag = 0.0f;                    ///< Air drag for spray (0-1)

    // Generation weights
    PxReal kineticEnergyWeight = 0.01f;       ///< Kinetic energy contribution
    PxReal pressureWeight = 1.0f;             ///< Pressure contribution
    PxReal divergenceWeight = 10.0f;          ///< Divergence contribution

    bool useAccurateVelocity = false;         ///< Use accurate velocity calculation
};

/**
 * @brief Particle system configuration
 */
struct ParticleSystemConfig {
    // Solver settings
    PxU32 maxNeighborsPerParticle = 96;       ///< Max neighbors for solver
    PxU32 solverIterationCount = 4;           ///< Solver iterations

    // Particle spacing
    PxReal particleSpacing = 0.1f;            ///< Rest distance between particles
    PxReal particleContactOffset = 0.0f;      ///< Contact offset (0 = auto)

    // Collision
    PxReal solidRestOffset = 0.0f;            ///< Solid rest offset (0 = auto)
    PxReal fluidRestOffset = 0.0f;            ///< Fluid rest offset (0 = auto)
    PxReal contactOffset = 0.0f;              ///< Contact offset (0 = auto)

    // Performance
    PxReal maxVelocity = 0.0f;                ///< Maximum velocity (0 = auto)
    bool enableCCD = false;                   ///< Enable speculative CCD

    // Density
    PxReal fluidDensity = 1000.0f;            ///< Fluid density (kg/m³, water = 1000)
};

/**
 * @brief Fluid volume configuration
 */
struct FluidVolumeConfig {
    // Grid dimensions
    PxU32 numX = 20;                          ///< X dimension particle count
    PxU32 numY = 20;                          ///< Y dimension particle count
    PxU32 numZ = 20;                          ///< Z dimension particle count

    // Position
    PxVec3 position = PxVec3(0, 5, 0);        ///< Starting position

    // Initial velocity
    PxVec3 initialVelocity = PxVec3(0, 0, 0); ///< Initial velocity

    // Material index for multi-material
    PxU32 materialIndex = 0;                  ///< Material/phase index
};

/**
 * @brief Particle buffer handle
 */
struct ParticleBufferHandle {
    PxParticleBuffer* buffer = nullptr;                      ///< Particle buffer
    PxParticleAndDiffuseBuffer* diffuseBuffer = nullptr;     ///< Diffuse buffer (if enabled)
    PxU32 maxParticles = 0;                                  ///< Maximum particles
    PxU32 activeParticles = 0;                               ///< Active particle count
    std::string name;                                         ///< Optional name
};

/**
 * @brief Particle data (for rendering/access)
 */
struct ParticleData {
    std::vector<PxVec4> positions;            ///< Positions (xyz) + invMass (w)
    std::vector<PxVec4> velocities;           ///< Velocities (xyz) + reserved (w)
    std::vector<PxU32> phases;                ///< Phase IDs
    PxU32 activeCount = 0;                    ///< Active particle count
};

/**
 * @brief Custom particle update callback
 */
using ParticleUpdateCallback = std::function<void(ParticleData& data)>;

/**
 * @brief PBD fluid manager class
 *
 * This class provides comprehensive PBD fluid simulation capabilities:
 * - GPU-accelerated particle-based fluid simulation
 * - Multiple fluid materials (water, oil, honey, etc.)
 * - Diffuse particle effects (foam, spray, bubbles)
 * - Fluid-rigid body interaction
 * - Custom particle manipulation
 * - Particle data access for rendering
 *
 * IMPORTANT: Requires GPU/CUDA support. The scene must be configured with:
 * - cudaContextManager set
 * - PxSceneFlag::eENABLE_GPU_DYNAMICS
 * - PxBroadPhaseType::eGPU
 *
 * @example
 * @code
 * // Check GPU support
 * if (!PBDFluidManager::isGPUAvailable(physics)) {
 *     std::cerr << "GPU required for PBD fluids" << std::endl;
 *     return;
 * }
 *
 * // Create fluid manager
 * PBDFluidManager fluidMgr;
 * fluidMgr.initialize(physics, scene);
 *
 * // Configure material (water)
 * PBDMaterialConfig waterMat;
 * waterMat.viscosity = 0.001f;
 * waterMat.surfaceTension = 0.0728f;
 * waterMat.cohesion = 0.01f;
 *
 * // Create particle system
 * ParticleSystemConfig sysConfig;
 * sysConfig.particleSpacing = 0.1f;
 * sysConfig.fluidDensity = 1000.0f;
 *
 * DiffuseParticleConfig diffuseConfig;
 * diffuseConfig.enable = true;
 * diffuseConfig.maxDiffuseParticles = 100000;
 *
 * PxPBDParticleSystem* system = fluidMgr.createParticleSystem(
 *     sysConfig, waterMat, diffuseConfig);
 *
 * // Add fluid volume
 * FluidVolumeConfig volume;
 * volume.numX = volume.numY = volume.numZ = 20;
 * volume.position = PxVec3(0, 10, 0);
 *
 * fluidMgr.addFluidVolume(system, volume, sysConfig);
 *
 * // Simulate
 * scene->simulate(dt);
 * scene->fetchResults(true);
 * scene->fetchResultsParticleSystem();  // Important!
 *
 * // Get particle data for rendering
 * ParticleData data = fluidMgr.getParticleData(system);
 * for (const auto& pos : data.positions) {
 *     renderParticle(pos.x, pos.y, pos.z);
 * }
 * @endcode
 */
class PBDFluidManager {
public:
    /**
     * @brief Constructor
     */
    PBDFluidManager();

    /**
     * @brief Destructor
     */
    ~PBDFluidManager();

    // No copy
    PBDFluidManager(const PBDFluidManager&) = delete;
    PBDFluidManager& operator=(const PBDFluidManager&) = delete;

    // Move allowed
    PBDFluidManager(PBDFluidManager&&) noexcept;
    PBDFluidManager& operator=(PBDFluidManager&&) noexcept;

    /**
     * @brief Initialize fluid manager
     * @param physics PhysX physics instance
     * @param scene Scene to use (must have GPU enabled)
     * @return True if successful
     */
    bool initialize(PxPhysics* physics, PxScene* scene);

    /**
     * @brief Cleanup and release resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const;

    /**
     * @brief Check if GPU/CUDA is available
     * @param physics PhysX physics instance
     * @return True if GPU is available
     */
    static bool isGPUAvailable(PxPhysics* physics);

    // ========================================================================
    // Particle System Creation
    // ========================================================================

    /**
     * @brief Create PBD particle system
     * @param systemConfig Particle system configuration
     * @param materialConfig Initial material configuration
     * @param diffuseConfig Diffuse particle configuration
     * @return Particle system (nullptr on failure)
     */
    PxPBDParticleSystem* createParticleSystem(
        const ParticleSystemConfig& systemConfig,
        const PBDMaterialConfig& materialConfig = PBDMaterialConfig(),
        const DiffuseParticleConfig& diffuseConfig = DiffuseParticleConfig());

    /**
     * @brief Create PBD material
     * @param config Material configuration
     * @return PBD material (nullptr on failure)
     */
    PxPBDMaterial* createPBDMaterial(const PBDMaterialConfig& config);

    /**
     * @brief Create phase for particle system
     * @param system Particle system
     * @param material Material for this phase
     * @param selfCollide Enable self-collision
     * @return Phase ID (0 on failure)
     */
    PxU32 createPhase(PxPBDParticleSystem* system, PxPBDMaterial* material, bool selfCollide = true);

    // ========================================================================
    // Fluid Volume Creation
    // ========================================================================

    /**
     * @brief Add fluid volume to particle system
     * @param system Particle system to add to
     * @param volumeConfig Volume configuration
     * @param systemConfig System configuration (for spacing/density)
     * @param phaseID Phase ID for particles (0 = default)
     * @return Particle buffer handle (nullptr on failure)
     */
    ParticleBufferHandle* addFluidVolume(
        PxPBDParticleSystem* system,
        const FluidVolumeConfig& volumeConfig,
        const ParticleSystemConfig& systemConfig,
        PxU32 phaseID = 0);

    /**
     * @brief Add multiple fluid volumes with different materials
     * @param system Particle system
     * @param volumes Volume configurations
     * @param materials Material configurations (one per volume)
     * @param systemConfig System configuration
     * @return Particle buffer handle (nullptr on failure)
     */
    ParticleBufferHandle* addMultiMaterialFluidVolumes(
        PxPBDParticleSystem* system,
        const std::vector<FluidVolumeConfig>& volumes,
        const std::vector<PBDMaterialConfig>& materials,
        const ParticleSystemConfig& systemConfig);

    /**
     * @brief Remove fluid volume
     * @param system Particle system
     * @param buffer Buffer handle to remove
     */
    void removeFluidVolume(PxPBDParticleSystem* system, ParticleBufferHandle* buffer);

    // ========================================================================
    // Particle Data Access
    // ========================================================================

    /**
     * @brief Get particle data for rendering/processing
     * @param system Particle system
     * @return Particle data structure
     */
    ParticleData getParticleData(PxPBDParticleSystem* system) const;

    /**
     * @brief Get diffuse particle data
     * @param buffer Buffer handle (must have diffuse enabled)
     * @param outPositions Output positions
     * @param outVelocities Output velocities
     * @param outLifetimes Output lifetimes
     * @return Number of active diffuse particles
     */
    PxU32 getDiffuseParticleData(
        ParticleBufferHandle* buffer,
        std::vector<PxVec4>& outPositions,
        std::vector<PxVec4>& outVelocities,
        std::vector<PxReal>& outLifetimes) const;

    /**
     * @brief Update particle positions/velocities
     * @param system Particle system
     * @param callback Update callback
     */
    void updateParticleData(PxPBDParticleSystem* system, ParticleUpdateCallback callback);

    // ========================================================================
    // Material Management
    // ========================================================================

    /**
     * @brief Update material properties at runtime
     * @param material Material to update
     * @param config New configuration
     */
    void updateMaterialProperties(PxPBDMaterial* material, const PBDMaterialConfig& config);

    /**
     * @brief Get material configuration
     * @param material Material to query
     * @return Material configuration
     */
    PBDMaterialConfig getMaterialProperties(const PxPBDMaterial* material) const;

    // ========================================================================
    // System Management
    // ========================================================================

    /**
     * @brief Get all tracked particle systems
     * @return Vector of particle systems
     */
    std::vector<PxPBDParticleSystem*> getParticleSystems() const;

    /**
     * @brief Get all particle buffers for a system
     * @param system Particle system
     * @return Vector of buffer handles
     */
    std::vector<ParticleBufferHandle*> getBuffers(PxPBDParticleSystem* system) const;

    /**
     * @brief Release particle system and all associated buffers
     * @param system Particle system to release
     */
    void releaseParticleSystem(PxPBDParticleSystem* system);

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Get total particle count across all systems
     * @return Total particle count
     */
    PxU32 getTotalParticleCount() const;

    /**
     * @brief Get active particle count for system
     * @param system Particle system
     * @return Active particle count
     */
    PxU32 getActiveParticleCount(PxPBDParticleSystem* system) const;

    /**
     * @brief Get number of tracked systems
     * @return System count
     */
    PxU32 getSystemCount() const;

    /**
     * @brief Get last error message
     * @return Error message or empty string
     */
    std::string getLastError() const;

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /**
     * @brief Calculate particle mass from spacing and density
     * @param particleSpacing Spacing between particles
     * @param fluidDensity Fluid density (kg/m³)
     * @return Particle mass
     */
    static PxReal calculateParticleMass(PxReal particleSpacing, PxReal fluidDensity);

    /**
     * @brief Calculate rest offset from spacing
     * @param particleSpacing Spacing between particles
     * @return Rest offset
     */
    static PxReal calculateRestOffset(PxReal particleSpacing);

    /**
     * @brief Calculate fluid rest offset from rest offset
     * @param restOffset Rest offset
     * @return Fluid rest offset
     */
    static PxReal calculateFluidRestOffset(PxReal restOffset);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    // Helper methods
    void applyMaterialConfig(PxPBDMaterial* material, const PBDMaterialConfig& config);
    void applySystemConfig(PxPBDParticleSystem* system, const ParticleSystemConfig& config);
    ParticleBufferHandle* createParticleBuffer(
        PxPBDParticleSystem* system,
        const std::vector<PxVec4>& positions,
        const std::vector<PxVec4>& velocities,
        const std::vector<PxU32>& phases,
        const DiffuseParticleConfig* diffuseConfig = nullptr);
};

/**
 * @brief Pre-defined material presets
 */
namespace PBDMaterialPresets {
    /**
     * @brief Water material preset
     */
    inline PBDMaterialConfig Water() {
        PBDMaterialConfig config;
        config.viscosity = 0.001f;
        config.surfaceTension = 0.0728f;
        config.cohesion = 0.01f;
        config.vorticityConfinement = 5.0f;
        return config;
    }

    /**
     * @brief Oil material preset
     */
    inline PBDMaterialConfig Oil() {
        PBDMaterialConfig config;
        config.viscosity = 0.1f;
        config.surfaceTension = 0.03f;
        config.cohesion = 0.05f;
        config.vorticityConfinement = 2.0f;
        return config;
    }

    /**
     * @brief Honey material preset
     */
    inline PBDMaterialConfig Honey() {
        PBDMaterialConfig config;
        config.viscosity = 10.0f;
        config.surfaceTension = 0.08f;
        config.cohesion = 0.2f;
        config.vorticityConfinement = 0.5f;
        return config;
    }

    /**
     * @brief Mercury material preset
     */
    inline PBDMaterialConfig Mercury() {
        PBDMaterialConfig config;
        config.viscosity = 0.0015f;
        config.surfaceTension = 0.4865f;
        config.cohesion = 0.3f;
        config.vorticityConfinement = 1.0f;
        return config;
    }
}

} // namespace PhysXWrapper
