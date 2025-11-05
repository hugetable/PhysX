/**
 * @file PBDFluidManager.cpp
 * @brief Implementation of PBDFluidManager class
 */

#include "Particle/PBDFluidManager.h"
#include "extensions/PxCudaHelpersExt.h"
#include <algorithm>
#include <map>

namespace PhysXWrapper {

/**
 * @brief Private implementation
 */
class PBDFluidManager::Impl {
public:
    Impl()
        : m_physics(nullptr)
        , m_scene(nullptr)
        , m_cudaContextManager(nullptr)
        , m_initialized(false)
    {}

    PxPhysics* m_physics;
    PxScene* m_scene;
    PxCudaContextManager* m_cudaContextManager;
    bool m_initialized;
    std::string m_lastError;

    // Tracked systems and buffers
    std::vector<PxPBDParticleSystem*> m_systems;
    std::map<PxPBDParticleSystem*, std::vector<ParticleBufferHandle*>> m_buffers;
    std::vector<PxPBDMaterial*> m_materials;

    // Default phase tracking
    std::map<PxPBDParticleSystem*, PxU32> m_defaultPhases;

    void setError(const std::string& error) {
        m_lastError = error;
    }

    void clearError() {
        m_lastError.clear();
    }

    void trackSystem(PxPBDParticleSystem* system) {
        if (std::find(m_systems.begin(), m_systems.end(), system) == m_systems.end()) {
            m_systems.push_back(system);
        }
    }

    void untrackSystem(PxPBDParticleSystem* system) {
        m_systems.erase(std::remove(m_systems.begin(), m_systems.end(), system), m_systems.end());
        m_buffers.erase(system);
        m_defaultPhases.erase(system);
    }

    void trackBuffer(PxPBDParticleSystem* system, ParticleBufferHandle* buffer) {
        m_buffers[system].push_back(buffer);
    }

    void untrackBuffer(PxPBDParticleSystem* system, ParticleBufferHandle* buffer) {
        auto& buffers = m_buffers[system];
        buffers.erase(std::remove(buffers.begin(), buffers.end(), buffer), buffers.end());
    }

    void trackMaterial(PxPBDMaterial* material) {
        if (std::find(m_materials.begin(), m_materials.end(), material) == m_materials.end()) {
            m_materials.push_back(material);
        }
    }
};

// ============================================================================
// Construction / Destruction
// ============================================================================

PBDFluidManager::PBDFluidManager()
    : m_impl(std::make_unique<Impl>())
{
}

PBDFluidManager::~PBDFluidManager() {
    cleanup();
}

PBDFluidManager::PBDFluidManager(PBDFluidManager&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

PBDFluidManager& PBDFluidManager::operator=(PBDFluidManager&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

// ============================================================================
// Initialization
// ============================================================================

bool PBDFluidManager::initialize(PxPhysics* physics, PxScene* scene) {
    if (!physics || !scene) {
        m_impl->setError("Invalid physics or scene instance");
        return false;
    }

    if (m_impl->m_initialized) {
        m_impl->setError("Already initialized");
        return false;
    }

    // Get CUDA context manager
    PxCudaContextManager* cudaContextManager = scene->getCudaContextManager();
    if (!cudaContextManager) {
        m_impl->setError("Scene does not have CUDA context manager - GPU particles require GPU-enabled scene");
        return false;
    }

    if (!cudaContextManager->contextIsValid()) {
        m_impl->setError("CUDA context is not valid - check GPU/CUDA installation");
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_scene = scene;
    m_impl->m_cudaContextManager = cudaContextManager;
    m_impl->m_initialized = true;
    m_impl->clearError();

    return true;
}

void PBDFluidManager::cleanup() {
    if (!m_impl->m_initialized) return;

    // Release all buffers
    for (auto& systemBuffers : m_impl->m_buffers) {
        for (auto* buffer : systemBuffers.second) {
            if (buffer->diffuseBuffer) {
                buffer->diffuseBuffer->release();
            } else if (buffer->buffer) {
                buffer->buffer->release();
            }
            delete buffer;
        }
    }
    m_impl->m_buffers.clear();

    // Release all particle systems
    for (auto* system : m_impl->m_systems) {
        system->release();
    }
    m_impl->m_systems.clear();

    // Release all materials
    for (auto* material : m_impl->m_materials) {
        material->release();
    }
    m_impl->m_materials.clear();

    m_impl->m_defaultPhases.clear();

    m_impl->m_physics = nullptr;
    m_impl->m_scene = nullptr;
    m_impl->m_cudaContextManager = nullptr;
    m_impl->m_initialized = false;
    m_impl->clearError();
}

bool PBDFluidManager::isInitialized() const {
    return m_impl->m_initialized;
}

bool PBDFluidManager::isGPUAvailable(PxPhysics* physics) {
    if (!physics) return false;

    PxFoundation& foundation = physics->getFoundation();
    PxCudaContextManagerDesc cudaDesc;
    PxCudaContextManager* cudaManager = PxCreateCudaContextManager(
        foundation, cudaDesc, PxGetProfilerCallback());

    if (!cudaManager) return false;

    bool valid = cudaManager->contextIsValid();
    cudaManager->release();

    return valid;
}

// ============================================================================
// Particle System Creation
// ============================================================================

PxPBDParticleSystem* PBDFluidManager::createParticleSystem(
    const ParticleSystemConfig& systemConfig,
    const PBDMaterialConfig& materialConfig,
    const DiffuseParticleConfig& diffuseConfig)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("Not initialized");
        return nullptr;
    }

    // Create particle system
    PxPBDParticleSystem* system = m_impl->m_physics->createPBDParticleSystem(
        *m_impl->m_cudaContextManager,
        systemConfig.maxNeighborsPerParticle);

    if (!system) {
        m_impl->setError("Failed to create particle system");
        return nullptr;
    }

    // Apply system configuration
    applySystemConfig(system, systemConfig);

    // Create and apply default material
    PxPBDMaterial* material = createPBDMaterial(materialConfig);
    if (material) {
        PxU32 phaseID = createPhase(system, material, true);
        m_impl->m_defaultPhases[system] = phaseID;
    }

    // Add to scene
    m_impl->m_scene->addActor(*system);

    // Track system
    m_impl->trackSystem(system);
    m_impl->clearError();

    return system;
}

PxPBDMaterial* PBDFluidManager::createPBDMaterial(const PBDMaterialConfig& config) {
    if (!m_impl->m_initialized) {
        m_impl->setError("Not initialized");
        return nullptr;
    }

    // Create material with basic properties
    PxPBDMaterial* material = m_impl->m_physics->createPBDMaterial(
        config.friction,
        config.damping,
        config.adhesion,
        config.viscosity,
        config.particleFrictionScale,
        config.particleAdhesionScale,
        config.drag,
        config.lift,
        config.cflCoefficient);

    if (!material) {
        m_impl->setError("Failed to create PBD material");
        return nullptr;
    }

    // Apply additional properties
    applyMaterialConfig(material, config);

    // Track material
    m_impl->trackMaterial(material);
    m_impl->clearError();

    return material;
}

PxU32 PBDFluidManager::createPhase(PxPBDParticleSystem* system, PxPBDMaterial* material, bool selfCollide) {
    if (!system || !material) {
        m_impl->setError("Invalid system or material");
        return 0;
    }

    PxParticlePhaseFlags flags = PxParticlePhaseFlag::eParticlePhaseFluid;
    if (selfCollide) {
        flags |= PxParticlePhaseFlag::eParticlePhaseSelfCollide;
    }

    PxU32 phaseID = system->createPhase(material, flags);
    m_impl->clearError();

    return phaseID;
}

// ============================================================================
// Fluid Volume Creation
// ============================================================================

ParticleBufferHandle* PBDFluidManager::addFluidVolume(
    PxPBDParticleSystem* system,
    const FluidVolumeConfig& volumeConfig,
    const ParticleSystemConfig& systemConfig,
    PxU32 phaseID)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("Not initialized");
        return nullptr;
    }

    if (!system) {
        m_impl->setError("Invalid particle system");
        return nullptr;
    }

    // Use default phase if not specified
    if (phaseID == 0) {
        auto it = m_impl->m_defaultPhases.find(system);
        if (it != m_impl->m_defaultPhases.end()) {
            phaseID = it->second;
        } else {
            m_impl->setError("No default phase for system and no phase specified");
            return nullptr;
        }
    }

    // Calculate particle properties
    const PxU32 numParticles = volumeConfig.numX * volumeConfig.numY * volumeConfig.numZ;
    const PxReal particleMass = calculateParticleMass(
        systemConfig.particleSpacing,
        systemConfig.fluidDensity);

    // Allocate particle data
    PxU32* phases = PX_EXT_PINNED_MEMORY_ALLOC(PxU32, *m_impl->m_cudaContextManager, numParticles);
    PxVec4* positions = PX_EXT_PINNED_MEMORY_ALLOC(PxVec4, *m_impl->m_cudaContextManager, numParticles);
    PxVec4* velocities = PX_EXT_PINNED_MEMORY_ALLOC(PxVec4, *m_impl->m_cudaContextManager, numParticles);

    // Generate particle grid
    PxReal x = volumeConfig.position.x;
    PxReal y = volumeConfig.position.y;
    PxReal z = volumeConfig.position.z;

    for (PxU32 i = 0; i < volumeConfig.numX; ++i) {
        for (PxU32 j = 0; j < volumeConfig.numY; ++j) {
            for (PxU32 k = 0; k < volumeConfig.numZ; ++k) {
                const PxU32 index = i * (volumeConfig.numY * volumeConfig.numZ) + j * volumeConfig.numZ + k;

                phases[index] = phaseID;
                positions[index] = PxVec4(x, y, z, 1.0f / particleMass);
                velocities[index] = PxVec4(
                    volumeConfig.initialVelocity.x,
                    volumeConfig.initialVelocity.y,
                    volumeConfig.initialVelocity.z,
                    0.0f);

                z += systemConfig.particleSpacing;
            }
            z = volumeConfig.position.z;
            y += systemConfig.particleSpacing;
        }
        y = volumeConfig.position.y;
        x += systemConfig.particleSpacing;
    }

    // Convert to vectors
    std::vector<PxVec4> posVec(positions, positions + numParticles);
    std::vector<PxVec4> velVec(velocities, velocities + numParticles);
    std::vector<PxU32> phaseVec(phases, phases + numParticles);

    // Free pinned memory
    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, positions);
    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, velocities);
    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, phases);

    // Create buffer (diffuse disabled for single volume)
    ParticleBufferHandle* handle = createParticleBuffer(system, posVec, velVec, phaseVec, nullptr);

    if (handle) {
        m_impl->trackBuffer(system, handle);
        m_impl->clearError();
    }

    return handle;
}

ParticleBufferHandle* PBDFluidManager::addMultiMaterialFluidVolumes(
    PxPBDParticleSystem* system,
    const std::vector<FluidVolumeConfig>& volumes,
    const std::vector<PBDMaterialConfig>& materials,
    const ParticleSystemConfig& systemConfig)
{
    if (!m_impl->m_initialized) {
        m_impl->setError("Not initialized");
        return nullptr;
    }

    if (!system) {
        m_impl->setError("Invalid particle system");
        return nullptr;
    }

    if (volumes.size() != materials.size()) {
        m_impl->setError("Number of volumes must match number of materials");
        return nullptr;
    }

    if (volumes.empty()) {
        m_impl->setError("No volumes specified");
        return nullptr;
    }

    // Create materials and phases
    std::vector<PxU32> phaseIDs;
    for (const auto& matConfig : materials) {
        PxPBDMaterial* material = createPBDMaterial(matConfig);
        if (!material) {
            m_impl->setError("Failed to create material");
            return nullptr;
        }

        PxU32 phaseID = createPhase(system, material, true);
        if (phaseID == 0) {
            m_impl->setError("Failed to create phase");
            return nullptr;
        }

        phaseIDs.push_back(phaseID);
    }

    // Calculate total particles
    PxU32 totalParticles = 0;
    for (const auto& volume : volumes) {
        totalParticles += volume.numX * volume.numY * volume.numZ;
    }

    const PxReal particleMass = calculateParticleMass(
        systemConfig.particleSpacing,
        systemConfig.fluidDensity);

    // Allocate combined particle data
    std::vector<PxVec4> positions;
    std::vector<PxVec4> velocities;
    std::vector<PxU32> phases;

    positions.reserve(totalParticles);
    velocities.reserve(totalParticles);
    phases.reserve(totalParticles);

    // Generate particles for each volume
    for (size_t volIdx = 0; volIdx < volumes.size(); ++volIdx) {
        const auto& volume = volumes[volIdx];
        const PxU32 phaseID = phaseIDs[volIdx];

        PxReal x = volume.position.x;
        PxReal y = volume.position.y;
        PxReal z = volume.position.z;

        for (PxU32 i = 0; i < volume.numX; ++i) {
            for (PxU32 j = 0; j < volume.numY; ++j) {
                for (PxU32 k = 0; k < volume.numZ; ++k) {
                    phases.push_back(phaseID);
                    positions.push_back(PxVec4(x, y, z, 1.0f / particleMass));
                    velocities.push_back(PxVec4(
                        volume.initialVelocity.x,
                        volume.initialVelocity.y,
                        volume.initialVelocity.z,
                        0.0f));

                    z += systemConfig.particleSpacing;
                }
                z = volume.position.z;
                y += systemConfig.particleSpacing;
            }
            y = volume.position.y;
            x += systemConfig.particleSpacing;
        }
    }

    // Create buffer
    ParticleBufferHandle* handle = createParticleBuffer(system, positions, velocities, phases, nullptr);

    if (handle) {
        m_impl->trackBuffer(system, handle);
        m_impl->clearError();
    }

    return handle;
}

void PBDFluidManager::removeFluidVolume(PxPBDParticleSystem* system, ParticleBufferHandle* buffer) {
    if (!system || !buffer) return;

    // Remove buffer from system
    if (buffer->diffuseBuffer) {
        system->removeParticleBuffer(buffer->diffuseBuffer);
        buffer->diffuseBuffer->release();
    } else if (buffer->buffer) {
        system->removeParticleBuffer(buffer->buffer);
        buffer->buffer->release();
    }

    // Untrack
    m_impl->untrackBuffer(system, buffer);
    delete buffer;
}

// ============================================================================
// Particle Data Access
// ============================================================================

ParticleData PBDFluidManager::getParticleData(PxPBDParticleSystem* system) const {
    ParticleData data;

    if (!system) {
        m_impl->setError("Invalid particle system");
        return data;
    }

    // Get all buffers for this system
    auto it = m_impl->m_buffers.find(system);
    if (it == m_impl->m_buffers.end() || it->second.empty()) {
        return data;
    }

    // Accumulate data from all buffers
    for (auto* bufferHandle : it->second) {
        PxParticleBuffer* buffer = bufferHandle->diffuseBuffer
            ? static_cast<PxParticleBuffer*>(bufferHandle->diffuseBuffer)
            : bufferHandle->buffer;

        if (!buffer) continue;

        // Get buffer data pointers
        PxVec4* positions = buffer->getPositionInvMasses();
        PxVec4* velocities = buffer->getVelocities();
        PxU32* phases = buffer->getPhases();

        PxU32 numParticles = buffer->getNbActiveParticles();

        if (positions && numParticles > 0) {
            data.positions.insert(data.positions.end(), positions, positions + numParticles);
        }

        if (velocities && numParticles > 0) {
            data.velocities.insert(data.velocities.end(), velocities, velocities + numParticles);
        }

        if (phases && numParticles > 0) {
            data.phases.insert(data.phases.end(), phases, phases + numParticles);
        }

        data.activeCount += numParticles;
    }

    m_impl->clearError();
    return data;
}

PxU32 PBDFluidManager::getDiffuseParticleData(
    ParticleBufferHandle* buffer,
    std::vector<PxVec4>& outPositions,
    std::vector<PxVec4>& outVelocities,
    std::vector<PxReal>& outLifetimes) const
{
    if (!buffer || !buffer->diffuseBuffer) {
        m_impl->setError("Invalid buffer or diffuse particles not enabled");
        return 0;
    }

    PxParticleAndDiffuseBuffer* diffuseBuffer = buffer->diffuseBuffer;

    // Get diffuse particle data
    PxVec4* positions = diffuseBuffer->getDiffusePositionLifeTime();
    PxVec4* velocities = diffuseBuffer->getDiffuseVelocities();

    PxU32 numDiffuse = diffuseBuffer->getNbActiveDiffuseParticles();

    if (positions && numDiffuse > 0) {
        outPositions.assign(positions, positions + numDiffuse);
    }

    if (velocities && numDiffuse > 0) {
        outVelocities.assign(velocities, velocities + numDiffuse);
    }

    // Extract lifetimes from position.w
    outLifetimes.resize(numDiffuse);
    for (PxU32 i = 0; i < numDiffuse; ++i) {
        outLifetimes[i] = positions[i].w;
    }

    m_impl->clearError();
    return numDiffuse;
}

void PBDFluidManager::updateParticleData(PxPBDParticleSystem* system, ParticleUpdateCallback callback) {
    if (!system || !callback) {
        m_impl->setError("Invalid system or callback");
        return;
    }

    ParticleData data = getParticleData(system);
    callback(data);

    // Note: Updating particle data requires GPU synchronization
    // This is a read-only operation in the current implementation
    // For write operations, you would need to use GPU buffers directly
}

// ============================================================================
// Material Management
// ============================================================================

void PBDFluidManager::updateMaterialProperties(PxPBDMaterial* material, const PBDMaterialConfig& config) {
    if (!material) {
        m_impl->setError("Invalid material");
        return;
    }

    applyMaterialConfig(material, config);
    m_impl->clearError();
}

PBDMaterialConfig PBDFluidManager::getMaterialProperties(const PxPBDMaterial* material) const {
    PBDMaterialConfig config;

    if (!material) {
        m_impl->setError("Invalid material");
        return config;
    }

    config.friction = material->getFriction();
    config.damping = material->getDamping();
    config.adhesion = material->getAdhesion();
    config.viscosity = material->getViscosity();
    config.surfaceTension = material->getSurfaceTension();
    config.cohesion = material->getCohesion();
    config.vorticityConfinement = material->getVorticityConfinement();
    config.particleFrictionScale = material->getParticleFrictionScale();
    config.particleAdhesionScale = material->getParticleAdhesionScale();
    config.lift = material->getLift();
    config.drag = material->getDrag();
    config.cflCoefficient = material->getCFLCoefficient();

    m_impl->clearError();
    return config;
}

// ============================================================================
// System Management
// ============================================================================

std::vector<PxPBDParticleSystem*> PBDFluidManager::getParticleSystems() const {
    return m_impl->m_systems;
}

std::vector<ParticleBufferHandle*> PBDFluidManager::getBuffers(PxPBDParticleSystem* system) const {
    auto it = m_impl->m_buffers.find(system);
    if (it != m_impl->m_buffers.end()) {
        return it->second;
    }
    return {};
}

void PBDFluidManager::releaseParticleSystem(PxPBDParticleSystem* system) {
    if (!system) return;

    // Release all buffers
    auto it = m_impl->m_buffers.find(system);
    if (it != m_impl->m_buffers.end()) {
        for (auto* buffer : it->second) {
            if (buffer->diffuseBuffer) {
                buffer->diffuseBuffer->release();
            } else if (buffer->buffer) {
                buffer->buffer->release();
            }
            delete buffer;
        }
    }

    // Release system
    system->release();

    // Untrack
    m_impl->untrackSystem(system);
}

// ============================================================================
// Statistics
// ============================================================================

PxU32 PBDFluidManager::getTotalParticleCount() const {
    PxU32 total = 0;

    for (auto* system : m_impl->m_systems) {
        total += getActiveParticleCount(system);
    }

    return total;
}

PxU32 PBDFluidManager::getActiveParticleCount(PxPBDParticleSystem* system) const {
    if (!system) return 0;

    PxU32 count = 0;

    auto it = m_impl->m_buffers.find(system);
    if (it != m_impl->m_buffers.end()) {
        for (auto* buffer : it->second) {
            count += buffer->activeParticles;
        }
    }

    return count;
}

PxU32 PBDFluidManager::getSystemCount() const {
    return static_cast<PxU32>(m_impl->m_systems.size());
}

std::string PBDFluidManager::getLastError() const {
    return m_impl->m_lastError;
}

// ============================================================================
// Helper Functions
// ============================================================================

PxReal PBDFluidManager::calculateParticleMass(PxReal particleSpacing, PxReal fluidDensity) {
    // Mass = density * volume
    // Volume of sphere: (4/3) * pi * r^3
    // Use particleSpacing as diameter: r = particleSpacing / 2
    const PxReal radius = particleSpacing * 0.5f;
    return fluidDensity * 1.333f * 3.14159f * radius * radius * radius;
}

PxReal PBDFluidManager::calculateRestOffset(PxReal particleSpacing) {
    return 0.5f * particleSpacing / 0.6f;
}

PxReal PBDFluidManager::calculateFluidRestOffset(PxReal restOffset) {
    return restOffset * 0.6f;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void PBDFluidManager::applyMaterialConfig(PxPBDMaterial* material, const PBDMaterialConfig& config) {
    material->setFriction(config.friction);
    material->setDamping(config.damping);
    material->setAdhesion(config.adhesion);
    material->setViscosity(config.viscosity);
    material->setSurfaceTension(config.surfaceTension);
    material->setCohesion(config.cohesion);
    material->setVorticityConfinement(config.vorticityConfinement);
    material->setParticleFrictionScale(config.particleFrictionScale);
    material->setParticleAdhesionScale(config.particleAdhesionScale);
    material->setLift(config.lift);
    material->setDrag(config.drag);
    material->setCFLCoefficient(config.cflCoefficient);
    material->setGravityScale(config.gravityScale);
}

void PBDFluidManager::applySystemConfig(PxPBDParticleSystem* system, const ParticleSystemConfig& config) {
    // Calculate offsets if not provided
    PxReal restOffset = config.contactOffset;
    if (restOffset == 0.0f) {
        restOffset = calculateRestOffset(config.particleSpacing);
    }

    PxReal fluidRestOffset = config.fluidRestOffset;
    if (fluidRestOffset == 0.0f) {
        fluidRestOffset = calculateFluidRestOffset(restOffset);
    }

    PxReal solidRestOffset = config.solidRestOffset;
    if (solidRestOffset == 0.0f) {
        solidRestOffset = restOffset;
    }

    PxReal contactOffset = config.contactOffset;
    if (contactOffset == 0.0f) {
        contactOffset = restOffset + 0.01f;
    }

    PxReal particleContactOffset = config.particleContactOffset;
    if (particleContactOffset == 0.0f) {
        particleContactOffset = PxMax(solidRestOffset + 0.01f, fluidRestOffset / 0.6f);
    }

    // Apply configuration
    system->setRestOffset(restOffset);
    system->setContactOffset(contactOffset);
    system->setParticleContactOffset(particleContactOffset);
    system->setSolidRestOffset(solidRestOffset);
    system->setFluidRestOffset(fluidRestOffset);

    system->setParticleFlag(PxParticleFlag::eENABLE_SPECULATIVE_CCD, config.enableCCD);

    PxReal maxVelocity = config.maxVelocity;
    if (maxVelocity == 0.0f) {
        maxVelocity = solidRestOffset * 100.0f;
    }
    system->setMaxVelocity(maxVelocity);

    system->setSolverIterationCounts(config.solverIterationCount);
}

ParticleBufferHandle* PBDFluidManager::createParticleBuffer(
    PxPBDParticleSystem* system,
    const std::vector<PxVec4>& positions,
    const std::vector<PxVec4>& velocities,
    const std::vector<PxU32>& phases,
    const DiffuseParticleConfig* diffuseConfig)
{
    if (positions.size() != velocities.size() || positions.size() != phases.size()) {
        m_impl->setError("Particle data arrays must have same size");
        return nullptr;
    }

    const PxU32 numParticles = static_cast<PxU32>(positions.size());

    // Allocate pinned memory
    PxU32* phasesGpu = PX_EXT_PINNED_MEMORY_ALLOC(PxU32, *m_impl->m_cudaContextManager, numParticles);
    PxVec4* positionsGpu = PX_EXT_PINNED_MEMORY_ALLOC(PxVec4, *m_impl->m_cudaContextManager, numParticles);
    PxVec4* velocitiesGpu = PX_EXT_PINNED_MEMORY_ALLOC(PxVec4, *m_impl->m_cudaContextManager, numParticles);

    // Copy data
    std::copy(phases.begin(), phases.end(), phasesGpu);
    std::copy(positions.begin(), positions.end(), positionsGpu);
    std::copy(velocities.begin(), velocities.end(), velocitiesGpu);

    ParticleBufferHandle* handle = new ParticleBufferHandle();
    handle->maxParticles = numParticles;
    handle->activeParticles = numParticles;

    // Create buffer (with or without diffuse)
    if (diffuseConfig && diffuseConfig->enable) {
        // Create diffuse buffer
        PxDiffuseParticleParams dpParams;
        dpParams.threshold = diffuseConfig->threshold;
        dpParams.bubbleDrag = diffuseConfig->bubbleDrag;
        dpParams.buoyancy = diffuseConfig->buoyancy;
        dpParams.airDrag = diffuseConfig->airDrag;
        dpParams.kineticEnergyWeight = diffuseConfig->kineticEnergyWeight;
        dpParams.pressureWeight = diffuseConfig->pressureWeight;
        dpParams.divergenceWeight = diffuseConfig->divergenceWeight;
        dpParams.lifetime = diffuseConfig->lifetime;
        dpParams.useAccurateVelocity = diffuseConfig->useAccurateVelocity;

        ExtGpu::PxParticleAndDiffuseBufferDesc bufferDesc;
        bufferDesc.maxParticles = numParticles;
        bufferDesc.numActiveParticles = numParticles;
        bufferDesc.maxDiffuseParticles = diffuseConfig->maxDiffuseParticles;
        bufferDesc.maxActiveDiffuseParticles = diffuseConfig->maxDiffuseParticles;
        bufferDesc.diffuseParams = dpParams;
        bufferDesc.positions = positionsGpu;
        bufferDesc.velocities = velocitiesGpu;
        bufferDesc.phases = phasesGpu;

        handle->diffuseBuffer = ExtGpu::PxCreateAndPopulateParticleAndDiffuseBuffer(
            bufferDesc, m_impl->m_cudaContextManager);

        system->addParticleBuffer(handle->diffuseBuffer);
    } else {
        // Create regular buffer
        ExtGpu::PxParticleBufferDesc bufferDesc;
        bufferDesc.maxParticles = numParticles;
        bufferDesc.numActiveParticles = numParticles;
        bufferDesc.positions = positionsGpu;
        bufferDesc.velocities = velocitiesGpu;
        bufferDesc.phases = phasesGpu;

        handle->buffer = ExtGpu::PxCreateAndPopulateParticleBuffer(
            bufferDesc, m_impl->m_cudaContextManager);

        system->addParticleBuffer(handle->buffer);
    }

    // Free pinned memory
    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, positionsGpu);
    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, velocitiesGpu);
    PX_EXT_PINNED_MEMORY_FREE(*m_impl->m_cudaContextManager, phasesGpu);

    return handle;
}

} // namespace PhysXWrapper
