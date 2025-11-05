/**
 * @file PerformanceProfiler.h
 * @brief Performance profiler for physics simulation analysis
 *
 * This class provides comprehensive performance monitoring and profiling
 * for physics simulations, including frame timing, memory tracking,
 * scene statistics, and bottleneck detection.
 *
 * Useful for optimization and debugging
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <map>

using namespace physx;

namespace PhysXWrapper {

/**
 * @class PerformanceProfiler
 * @brief Comprehensive performance profiler for physics
 *
 * PerformanceProfiler provides detailed performance monitoring including:
 * - Frame timing (min, max, avg, percentiles)
 * - Memory usage tracking
 * - Scene object counts (actors, shapes, contacts)
 * - Broad-phase and narrow-phase timing
 * - Performance history and trending
 * - Bottleneck detection
 * - CSV export for analysis
 *
 * Features:
 * - Real-time monitoring
 * - Statistical analysis
 * - Performance warnings
 * - Custom timing sections
 * - Memory profiling
 * - Detailed reports
 *
 * Usage:
 * @code
 * PerformanceProfiler profiler;
 * profiler.initialize(physics, scene);
 *
 * // Main loop
 * while (running) {
 *     profiler.beginFrame();
 *
 *     scene->simulate(dt);
 *     scene->fetchResults(true);
 *
 *     profiler.endFrame();
 *
 *     // Print every 60 frames
 *     if (profiler.getFrameCount() % 60 == 0) {
 *         profiler.printSummary();
 *     }
 * }
 *
 * // Final report
 * profiler.printDetailedReport();
 * profiler.exportToCSV("performance.csv");
 * @endcode
 */
class PerformanceProfiler {
public:
    /**
     * @brief Frame timing data
     */
    struct FrameTiming {
        PxReal frameTime = 0.0f;        ///< Total frame time (ms)
        PxReal simulateTime = 0.0f;     ///< Simulation time (ms)
        PxReal fetchTime = 0.0f;        ///< Fetch results time (ms)
        PxReal customTime = 0.0f;       ///< Custom timing (ms)
        PxU64 timestamp = 0;            ///< Frame timestamp

        FrameTiming() = default;
    };

    /**
     * @brief Scene statistics
     */
    struct SceneStats {
        PxU32 staticActors = 0;         ///< Number of static actors
        PxU32 dynamicActors = 0;        ///< Number of dynamic actors
        PxU32 kinematicActors = 0;      ///< Number of kinematic actors
        PxU32 totalActors = 0;          ///< Total actors
        PxU32 shapes = 0;               ///< Total shapes
        PxU32 constraints = 0;          ///< Number of constraints
        PxU32 articulations = 0;        ///< Number of articulations
        PxU32 aggregates = 0;           ///< Number of aggregates

        SceneStats() = default;
        void print() const;
    };

    /**
     * @brief Contact statistics
     */
    struct ContactStats {
        PxU32 touchingPairs = 0;        ///< Number of touching pairs
        PxU32 contactPoints = 0;        ///< Total contact points
        PxU32 triggerPairs = 0;         ///< Number of trigger pairs

        ContactStats() = default;
        void print() const;
    };

    /**
     * @brief Memory statistics (estimated)
     */
    struct MemoryStats {
        size_t totalBytes = 0;          ///< Total memory usage (bytes)
        size_t actorBytes = 0;          ///< Actor memory
        size_t shapeBytes = 0;          ///< Shape memory
        size_t constraintBytes = 0;     ///< Constraint memory

        MemoryStats() = default;
        void print() const;
        PxReal getTotalMB() const { return totalBytes / (1024.0f * 1024.0f); }
    };

    /**
     * @brief Performance statistics
     */
    struct PerformanceStats {
        // Frame timing
        PxU32 frameCount = 0;
        PxReal avgFPS = 0.0f;
        PxReal minFPS = 0.0f;
        PxReal maxFPS = 0.0f;
        PxReal currentFPS = 0.0f;

        // Frame time (milliseconds)
        PxReal avgFrameTime = 0.0f;
        PxReal minFrameTime = 0.0f;
        PxReal maxFrameTime = 0.0f;
        PxReal stdDevFrameTime = 0.0f;

        // Percentiles
        PxReal frameTime50th = 0.0f;    ///< Median
        PxReal frameTime95th = 0.0f;    ///< 95th percentile
        PxReal frameTime99th = 0.0f;    ///< 99th percentile

        // Scene stats
        SceneStats sceneStats;
        ContactStats contactStats;
        MemoryStats memoryStats;

        // Total elapsed time
        PxReal totalElapsedTime = 0.0f;

        PerformanceStats() = default;
        void print() const;
        void reset();
    };

    /**
     * @brief Performance warning
     */
    struct PerformanceWarning {
        enum class Type {
            HIGH_FRAME_TIME,    ///< Frame time exceeded threshold
            LOW_FPS,            ///< FPS dropped below threshold
            HIGH_ACTOR_COUNT,   ///< Too many actors
            HIGH_CONTACT_COUNT, ///< Too many contacts
            MEMORY_USAGE        ///< High memory usage
        };

        Type type;
        std::string message;
        PxReal value;
        PxU64 frameNumber;

        PerformanceWarning(Type t, const std::string& msg, PxReal v, PxU64 frame)
            : type(t), message(msg), value(v), frameNumber(frame) {}
    };

    /**
     * @brief Profiler configuration
     */
    struct Config {
        bool enableHistory = true;              ///< Keep frame history
        PxU32 maxHistoryFrames = 300;           ///< Max frames to keep
        PxReal warningFrameTimeMs = 33.3f;      ///< Warning threshold (ms)
        PxReal warningFPS = 30.0f;              ///< FPS warning threshold
        PxU32 warningActorCount = 1000;         ///< Actor count warning
        PxU32 warningContactCount = 10000;      ///< Contact count warning
        bool autoDetectBottlenecks = true;      ///< Auto detect bottlenecks

        Config() = default;
    };

public:
    /**
     * @brief Constructor
     */
    PerformanceProfiler();

    /**
     * @brief Destructor
     */
    ~PerformanceProfiler();

    // Disable copy
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;

    /**
     * @brief Initialize profiler
     * @param physics PhysX physics instance
     * @param scene PhysX scene instance
     * @return true if successful
     */
    bool initialize(PxPhysics* physics, PxScene* scene);

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     * @return true if initialized
     */
    bool isInitialized() const;

    // ========================================================================
    // Frame Profiling
    // ========================================================================

    /**
     * @brief Begin frame profiling
     *
     * Call this at the start of your simulation step
     */
    void beginFrame();

    /**
     * @brief End frame profiling
     *
     * Call this after simulation completes
     */
    void endFrame();

    /**
     * @brief Record simulation start
     *
     * Call before scene->simulate()
     */
    void beginSimulation();

    /**
     * @brief Record simulation end
     *
     * Call after scene->simulate(), before fetchResults()
     */
    void endSimulation();

    /**
     * @brief Record fetch start
     *
     * Call before scene->fetchResults()
     */
    void beginFetch();

    /**
     * @brief Record fetch end
     *
     * Call after scene->fetchResults()
     */
    void endFetch();

    // ========================================================================
    // Custom Timing Sections
    // ========================================================================

    /**
     * @brief Begin custom timing section
     * @param name Section name
     */
    void beginSection(const std::string& name);

    /**
     * @brief End custom timing section
     * @param name Section name
     */
    void endSection(const std::string& name);

    /**
     * @brief Get custom section time
     * @param name Section name
     * @return Time in milliseconds
     */
    PxReal getSectionTime(const std::string& name) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Get performance statistics
     * @return Performance stats
     */
    PerformanceStats getStats() const;

    /**
     * @brief Get scene statistics
     * @return Scene stats
     */
    SceneStats getSceneStats() const;

    /**
     * @brief Get contact statistics
     * @return Contact stats
     */
    ContactStats getContactStats() const;

    /**
     * @brief Get memory statistics (estimated)
     * @return Memory stats
     */
    MemoryStats getMemoryStats() const;

    /**
     * @brief Reset all statistics
     */
    void resetStats();

    // ========================================================================
    // Current Frame Data
    // ========================================================================

    /**
     * @brief Get frame count
     * @return Number of frames profiled
     */
    PxU64 getFrameCount() const;

    /**
     * @brief Get last frame time
     * @return Last frame time in milliseconds
     */
    PxReal getLastFrameTime() const;

    /**
     * @brief Get current FPS
     * @return Frames per second
     */
    PxReal getCurrentFPS() const;

    /**
     * @brief Get average FPS
     * @return Average FPS over all frames
     */
    PxReal getAverageFPS() const;

    /**
     * @brief Get total elapsed time
     * @return Total time in seconds
     */
    PxReal getTotalElapsedTime() const;

    // ========================================================================
    // Frame History
    // ========================================================================

    /**
     * @brief Get frame history
     * @return Vector of frame timings
     */
    std::vector<FrameTiming> getFrameHistory() const;

    /**
     * @brief Clear frame history
     */
    void clearHistory();

    /**
     * @brief Get history size
     * @return Number of frames in history
     */
    PxU32 getHistorySize() const;

    // ========================================================================
    // Warnings and Bottlenecks
    // ========================================================================

    /**
     * @brief Get performance warnings
     * @return Vector of warnings
     */
    std::vector<PerformanceWarning> getWarnings() const;

    /**
     * @brief Clear warnings
     */
    void clearWarnings();

    /**
     * @brief Check for bottlenecks
     * @return true if bottlenecks detected
     */
    bool hasBottlenecks() const;

    /**
     * @brief Get bottleneck description
     * @return String describing bottlenecks
     */
    std::string getBottleneckDescription() const;

    // ========================================================================
    // Reporting
    // ========================================================================

    /**
     * @brief Print quick summary
     */
    void printSummary() const;

    /**
     * @brief Print detailed report
     */
    void printDetailedReport() const;

    /**
     * @brief Print frame history
     * @param numFrames Number of recent frames to print (0 = all)
     */
    void printFrameHistory(PxU32 numFrames = 10) const;

    /**
     * @brief Print warnings
     */
    void printWarnings() const;

    /**
     * @brief Export to CSV file
     * @param filename Output filename
     * @return true if successful
     */
    bool exportToCSV(const std::string& filename) const;

    /**
     * @brief Export summary to JSON
     * @param filename Output filename
     * @return true if successful
     */
    bool exportToJSON(const std::string& filename) const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set profiler configuration
     * @param config Configuration
     */
    void setConfig(const Config& config);

    /**
     * @brief Get profiler configuration
     * @return Configuration
     */
    const Config& getConfig() const;

    /**
     * @brief Enable/disable profiling
     * @param enabled Enable flag
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if profiling enabled
     * @return true if enabled
     */
    bool isEnabled() const;

    /**
     * @brief Get PhysX physics instance
     * @return Physics instance
     */
    PxPhysics* getPhysics() const;

    /**
     * @brief Get PhysX scene instance
     * @return Scene instance
     */
    PxScene* getScene() const;

private:
    /**
     * @brief Update statistics
     */
    void updateStatistics();

    /**
     * @brief Check for warnings
     */
    void checkWarnings();

    /**
     * @brief Calculate percentiles
     */
    void calculatePercentiles();

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace PhysXWrapper
