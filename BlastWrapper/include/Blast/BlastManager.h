// BlastWrapper - A Modern C++ Wrapper for NVIDIA Blast
// Copyright (c) 2025. All rights reserved.

#ifndef BLASTWRAPPER_BLASTMANAGER_H
#define BLASTWRAPPER_BLASTMANAGER_H

#include <memory>
#include <string>
#include <vector>

namespace BlastWrapper {

/**
 * @brief Configuration for Blast manager initialization
 */
struct BlastConfig {
    // Enable profiling
    bool enableProfiling = false;

    // Maximum number of actors
    uint32_t maxActors = 1024;

    // Maximum number of chunks per actor
    uint32_t maxChunksPerActor = 256;

    // Enable debug rendering
    bool enableDebugRender = false;

    BlastConfig() = default;

    static const BlastConfig& defaultConfig() {
        static const BlastConfig config;
        return config;
    }
};

/**
 * @brief Fracture event data
 */
struct FractureEvent {
    uint32_t actorId;
    uint32_t chunkIndex;
    float damage;
    bool isSplit;
};

/**
 * @brief Main Blast manager wrapper class
 *
 * This class provides a simplified C++ interface to NVIDIA Blast
 * for destruction simulation.
 */
class BlastManager {
public:
    /**
     * @brief Create a Blast manager
     * @param config Configuration parameters
     * @return Unique pointer to BlastManager
     */
    static std::unique_ptr<BlastManager> create(
        const BlastConfig& config = BlastConfig::defaultConfig()
    );

    /**
     * @brief Destructor
     */
    ~BlastManager();

    /**
     * @brief Initialize the Blast manager
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Shutdown the Blast manager
     */
    void shutdown();

    /**
     * @brief Check if manager is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    std::string getLastError() const;

    /**
     * @brief Update the Blast simulation
     * @param deltaTime Time step in seconds
     */
    void update(float deltaTime);

    /**
     * @brief Apply damage to an actor
     * @param actorId Actor identifier
     * @param damage Damage amount
     * @param position Position of damage in world space
     * @param radius Damage radius
     * @return Number of fracture events generated
     */
    uint32_t applyDamage(uint32_t actorId, float damage,
                         const float position[3], float radius);

    /**
     * @brief Get fracture events from last update
     * @return Vector of fracture events
     */
    std::vector<FractureEvent> getFractureEvents() const;

    // Prevent copying
    BlastManager(const BlastManager&) = delete;
    BlastManager& operator=(const BlastManager&) = delete;

private:
    /**
     * @brief Private constructor
     * @param config Configuration parameters
     */
    explicit BlastManager(const BlastConfig& config);

    // Private implementation (PIMPL idiom)
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Get Blast library version
 * @return Version string
 */
std::string getBlastVersion();

/**
 * @brief Check if Blast is available
 * @return true if Blast libraries are available, false otherwise
 */
bool isBlastAvailable();

} // namespace BlastWrapper

#endif // BLASTWRAPPER_BLASTMANAGER_H
