// FlowWrapper - A Modern C++ Wrapper for NVIDIA Flow
// Copyright (c) 2025. All rights reserved.

#ifndef FLOWWRAPPER_FLOWCONTEXT_H
#define FLOWWRAPPER_FLOWCONTEXT_H

#include <memory>
#include <string>
#include <vector>

namespace FlowWrapper {

/**
 * @brief Configuration for Flow context initialization
 */
struct FlowContextConfig {
    // API type (Vulkan, D3D12, etc.)
    std::string apiType = "Vulkan";

    // Enable debug mode
    bool enableDebug = false;

    // Device index
    int deviceIndex = 0;

    // Memory budget in MB
    size_t memoryBudgetMB = 512;

    FlowContextConfig() = default;

    static const FlowContextConfig& defaultConfig() {
        static const FlowContextConfig config;
        return config;
    }
};

/**
 * @brief Main Flow context wrapper class
 *
 * This class provides a simplified C++ interface to NVIDIA Flow
 * for fluid simulation and rendering.
 */
class FlowContext {
public:
    /**
     * @brief Create a Flow context
     * @param config Configuration parameters
     * @return true if successful, false otherwise
     */
    static std::unique_ptr<FlowContext> create(
        const FlowContextConfig& config = FlowContextConfig::defaultConfig()
    );

    /**
     * @brief Destructor
     */
    ~FlowContext();

    /**
     * @brief Initialize the Flow context
     * @return true if successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Shutdown the Flow context
     */
    void shutdown();

    /**
     * @brief Check if context is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    std::string getLastError() const;

    /**
     * @brief Update the Flow simulation
     * @param deltaTime Time step in seconds
     */
    void update(float deltaTime);

    // Prevent copying
    FlowContext(const FlowContext&) = delete;
    FlowContext& operator=(const FlowContext&) = delete;

private:
    /**
     * @brief Private constructor
     * @param config Configuration parameters
     */
    explicit FlowContext(const FlowContextConfig& config);

    // Private implementation (PIMPL idiom)
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Get Flow library version
 * @return Version string
 */
std::string getFlowVersion();

/**
 * @brief Check if Flow is available
 * @return true if Flow libraries are available, false otherwise
 */
bool isFlowAvailable();

} // namespace FlowWrapper

#endif // FLOWWRAPPER_FLOWCONTEXT_H
