/**
 * @file PerformanceProfiler.cpp
 * @brief Implementation of PerformanceProfiler class
 */

#include "Utility/PerformanceProfiler.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>

namespace PhysXWrapper {

// ============================================================================
// Stats Implementation
// ============================================================================

void PerformanceProfiler::SceneStats::print() const
{
    std::cout << "Scene Statistics:" << std::endl;
    std::cout << "  Total Actors: " << totalActors << std::endl;
    std::cout << "    Static: " << staticActors << std::endl;
    std::cout << "    Dynamic: " << dynamicActors << std::endl;
    std::cout << "    Kinematic: " << kinematicActors << std::endl;
    std::cout << "  Shapes: " << shapes << std::endl;
    std::cout << "  Constraints: " << constraints << std::endl;
    std::cout << "  Articulations: " << articulations << std::endl;
    std::cout << "  Aggregates: " << aggregates << std::endl;
}

void PerformanceProfiler::ContactStats::print() const
{
    std::cout << "Contact Statistics:" << std::endl;
    std::cout << "  Touching Pairs: " << touchingPairs << std::endl;
    std::cout << "  Contact Points: " << contactPoints << std::endl;
    std::cout << "  Trigger Pairs: " << triggerPairs << std::endl;
}

void PerformanceProfiler::MemoryStats::print() const
{
    std::cout << "Memory Statistics (Estimated):" << std::endl;
    std::cout << "  Total: " << getTotalMB() << " MB" << std::endl;
    std::cout << "  Actors: " << (actorBytes / (1024.0f * 1024.0f)) << " MB" << std::endl;
    std::cout << "  Shapes: " << (shapeBytes / (1024.0f * 1024.0f)) << " MB" << std::endl;
    std::cout << "  Constraints: " << (constraintBytes / (1024.0f * 1024.0f)) << " MB" << std::endl;
}

void PerformanceProfiler::PerformanceStats::print() const
{
    std::cout << "Performance Statistics:" << std::endl;
    std::cout << "  Frame Count: " << frameCount << std::endl;
    std::cout << "  Total Time: " << totalElapsedTime << " seconds" << std::endl;
    std::cout << "\nFPS:" << std::endl;
    std::cout << "  Current: " << std::fixed << std::setprecision(1) << currentFPS << std::endl;
    std::cout << "  Average: " << avgFPS << std::endl;
    std::cout << "  Min: " << minFPS << std::endl;
    std::cout << "  Max: " << maxFPS << std::endl;
    std::cout << "\nFrame Time (ms):" << std::endl;
    std::cout << "  Average: " << std::setprecision(2) << avgFrameTime << std::endl;
    std::cout << "  Min: " << minFrameTime << std::endl;
    std::cout << "  Max: " << maxFrameTime << std::endl;
    std::cout << "  StdDev: " << stdDevFrameTime << std::endl;
    std::cout << "  50th percentile: " << frameTime50th << std::endl;
    std::cout << "  95th percentile: " << frameTime95th << std::endl;
    std::cout << "  99th percentile: " << frameTime99th << std::endl;
}

void PerformanceProfiler::PerformanceStats::reset()
{
    frameCount = 0;
    avgFPS = minFPS = maxFPS = currentFPS = 0.0f;
    avgFrameTime = minFrameTime = maxFrameTime = stdDevFrameTime = 0.0f;
    frameTime50th = frameTime95th = frameTime99th = 0.0f;
    totalElapsedTime = 0.0f;
}

// ============================================================================
// PerformanceProfiler::Impl
// ============================================================================

class PerformanceProfiler::Impl {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    PxPhysics* m_physics = nullptr;
    PxScene* m_scene = nullptr;

    Config m_config;
    bool m_enabled = true;
    bool m_initialized = false;

    // Frame timing
    TimePoint m_frameStartTime;
    TimePoint m_simulateStartTime;
    TimePoint m_fetchStartTime;

    FrameTiming m_currentFrameTiming;
    std::vector<FrameTiming> m_frameHistory;

    // Custom sections
    std::map<std::string, TimePoint> m_sectionStartTimes;
    std::map<std::string, PxReal> m_sectionTimes;

    // Statistics
    PxU64 m_frameCount = 0;
    PxReal m_totalFrameTime = 0.0f;
    PxReal m_minFrameTime = FLT_MAX;
    PxReal m_maxFrameTime = 0.0f;
    PxReal m_totalElapsedTime = 0.0f;

    // Warnings
    std::vector<PerformanceWarning> m_warnings;

    // Scene stats cache
    SceneStats m_lastSceneStats;
    ContactStats m_lastContactStats;
    MemoryStats m_lastMemoryStats;

    // Helper to convert time point to milliseconds
    PxReal getElapsedMs(const TimePoint& start, const TimePoint& end) const {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / 1000.0f;
    }
};

// ============================================================================
// Construction/Destruction
// ============================================================================

PerformanceProfiler::PerformanceProfiler()
    : m_impl(std::make_unique<Impl>())
{
}

PerformanceProfiler::~PerformanceProfiler()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool PerformanceProfiler::initialize(PxPhysics* physics, PxScene* scene)
{
    if (!physics || !scene) {
        std::cerr << "PerformanceProfiler::initialize: physics or scene is null" << std::endl;
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_scene = scene;
    m_impl->m_initialized = true;

    return true;
}

void PerformanceProfiler::cleanup()
{
    m_impl->m_frameHistory.clear();
    m_impl->m_sectionTimes.clear();
    m_impl->m_warnings.clear();
    m_impl->m_frameCount = 0;
    m_impl->m_initialized = false;
}

bool PerformanceProfiler::isInitialized() const
{
    return m_impl->m_initialized;
}

// ============================================================================
// Frame Profiling
// ============================================================================

void PerformanceProfiler::beginFrame()
{
    if (!m_impl->m_enabled) return;

    m_impl->m_frameStartTime = Impl::Clock::now();
    m_impl->m_currentFrameTiming = FrameTiming();
}

void PerformanceProfiler::endFrame()
{
    if (!m_impl->m_enabled) return;

    auto endTime = Impl::Clock::now();
    m_impl->m_currentFrameTiming.frameTime = m_impl->getElapsedMs(m_impl->m_frameStartTime, endTime);
    m_impl->m_currentFrameTiming.timestamp = m_impl->m_frameCount;

    // Update statistics
    updateStatistics();

    // Store in history
    if (m_impl->m_config.enableHistory) {
        m_impl->m_frameHistory.push_back(m_impl->m_currentFrameTiming);

        // Trim history if needed
        if (m_impl->m_frameHistory.size() > m_impl->m_config.maxHistoryFrames) {
            m_impl->m_frameHistory.erase(m_impl->m_frameHistory.begin());
        }
    }

    // Check for warnings
    if (m_impl->m_config.autoDetectBottlenecks) {
        checkWarnings();
    }

    m_impl->m_frameCount++;
}

void PerformanceProfiler::beginSimulation()
{
    if (!m_impl->m_enabled) return;
    m_impl->m_simulateStartTime = Impl::Clock::now();
}

void PerformanceProfiler::endSimulation()
{
    if (!m_impl->m_enabled) return;
    auto endTime = Impl::Clock::now();
    m_impl->m_currentFrameTiming.simulateTime = m_impl->getElapsedMs(m_impl->m_simulateStartTime, endTime);
}

void PerformanceProfiler::beginFetch()
{
    if (!m_impl->m_enabled) return;
    m_impl->m_fetchStartTime = Impl::Clock::now();
}

void PerformanceProfiler::endFetch()
{
    if (!m_impl->m_enabled) return;
    auto endTime = Impl::Clock::now();
    m_impl->m_currentFrameTiming.fetchTime = m_impl->getElapsedMs(m_impl->m_fetchStartTime, endTime);
}

// ============================================================================
// Custom Timing Sections
// ============================================================================

void PerformanceProfiler::beginSection(const std::string& name)
{
    if (!m_impl->m_enabled) return;
    m_impl->m_sectionStartTimes[name] = Impl::Clock::now();
}

void PerformanceProfiler::endSection(const std::string& name)
{
    if (!m_impl->m_enabled) return;

    auto it = m_impl->m_sectionStartTimes.find(name);
    if (it != m_impl->m_sectionStartTimes.end()) {
        auto endTime = Impl::Clock::now();
        PxReal elapsedMs = m_impl->getElapsedMs(it->second, endTime);
        m_impl->m_sectionTimes[name] = elapsedMs;
        m_impl->m_sectionStartTimes.erase(it);
    }
}

PxReal PerformanceProfiler::getSectionTime(const std::string& name) const
{
    auto it = m_impl->m_sectionTimes.find(name);
    if (it != m_impl->m_sectionTimes.end()) {
        return it->second;
    }
    return 0.0f;
}

// ========================================================================
// Statistics
// ========================================================================

PerformanceProfiler::PerformanceStats PerformanceProfiler::getStats() const
{
    PerformanceStats stats;

    stats.frameCount = static_cast<PxU32>(m_impl->m_frameCount);
    stats.totalElapsedTime = m_impl->m_totalElapsedTime;

    if (m_impl->m_frameCount > 0) {
        stats.avgFrameTime = m_impl->m_totalFrameTime / m_impl->m_frameCount;
        stats.minFrameTime = m_impl->m_minFrameTime;
        stats.maxFrameTime = m_impl->m_maxFrameTime;

        // Calculate FPS
        if (stats.avgFrameTime > 0.0f) {
            stats.avgFPS = 1000.0f / stats.avgFrameTime;
        }
        if (stats.minFrameTime > 0.0f) {
            stats.maxFPS = 1000.0f / stats.minFrameTime;
        }
        if (stats.maxFrameTime > 0.0f) {
            stats.minFPS = 1000.0f / stats.maxFrameTime;
        }
        if (m_impl->m_currentFrameTiming.frameTime > 0.0f) {
            stats.currentFPS = 1000.0f / m_impl->m_currentFrameTiming.frameTime;
        }

        // Calculate standard deviation
        if (!m_impl->m_frameHistory.empty()) {
            PxReal variance = 0.0f;
            for (const auto& frame : m_impl->m_frameHistory) {
                PxReal diff = frame.frameTime - stats.avgFrameTime;
                variance += diff * diff;
            }
            variance /= m_impl->m_frameHistory.size();
            stats.stdDevFrameTime = std::sqrt(variance);

            // Calculate percentiles
            std::vector<PxReal> frameTimes;
            frameTimes.reserve(m_impl->m_frameHistory.size());
            for (const auto& frame : m_impl->m_frameHistory) {
                frameTimes.push_back(frame.frameTime);
            }
            std::sort(frameTimes.begin(), frameTimes.end());

            if (!frameTimes.empty()) {
                size_t idx50 = frameTimes.size() / 2;
                size_t idx95 = static_cast<size_t>(frameTimes.size() * 0.95f);
                size_t idx99 = static_cast<size_t>(frameTimes.size() * 0.99f);

                stats.frameTime50th = frameTimes[idx50];
                stats.frameTime95th = frameTimes[idx95];
                stats.frameTime99th = frameTimes[idx99];
            }
        }
    }

    stats.sceneStats = getSceneStats();
    stats.contactStats = getContactStats();
    stats.memoryStats = getMemoryStats();

    return stats;
}

PerformanceProfiler::SceneStats PerformanceProfiler::getSceneStats() const
{
    SceneStats stats;

    if (!m_impl->m_scene) {
        return stats;
    }

    stats.staticActors = m_impl->m_scene->getNbActors(PxActorTypeFlag::eRIGID_STATIC);
    stats.dynamicActors = m_impl->m_scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    stats.totalActors = stats.staticActors + stats.dynamicActors;

    stats.constraints = m_impl->m_scene->getNbConstraints();
    stats.articulations = m_impl->m_scene->getNbArticulations();
    stats.aggregates = m_impl->m_scene->getNbAggregates();

    // Count shapes
    PxU32 numActors = stats.totalActors;
    if (numActors > 0) {
        std::vector<PxActor*> actors(numActors);
        m_impl->m_scene->getActors(PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC,
                                     actors.data(), numActors);

        for (PxActor* actor : actors) {
            PxRigidActor* rigidActor = actor->is<PxRigidActor>();
            if (rigidActor) {
                stats.shapes += rigidActor->getNbShapes();
            }
        }
    }

    return stats;
}

PerformanceProfiler::ContactStats PerformanceProfiler::getContactStats() const
{
    ContactStats stats;
    // Note: PhysX doesn't provide direct access to contact pairs count
    // This would require a contact callback to track accurately
    // For now we return estimated values
    return stats;
}

PerformanceProfiler::MemoryStats PerformanceProfiler::getMemoryStats() const
{
    MemoryStats stats;

    SceneStats sceneStats = getSceneStats();

    // Rough estimates (actual values depend on many factors)
    const size_t ACTOR_SIZE = 256;  // bytes per actor
    const size_t SHAPE_SIZE = 128;   // bytes per shape
    const size_t CONSTRAINT_SIZE = 64; // bytes per constraint

    stats.actorBytes = sceneStats.totalActors * ACTOR_SIZE;
    stats.shapeBytes = sceneStats.shapes * SHAPE_SIZE;
    stats.constraintBytes = sceneStats.constraints * CONSTRAINT_SIZE;
    stats.totalBytes = stats.actorBytes + stats.shapeBytes + stats.constraintBytes;

    return stats;
}

void PerformanceProfiler::resetStats()
{
    m_impl->m_frameCount = 0;
    m_impl->m_totalFrameTime = 0.0f;
    m_impl->m_minFrameTime = FLT_MAX;
    m_impl->m_maxFrameTime = 0.0f;
    m_impl->m_totalElapsedTime = 0.0f;
    m_impl->m_frameHistory.clear();
    m_impl->m_warnings.clear();
}

// ========================================================================
// Current Frame Data
// ========================================================================

PxU64 PerformanceProfiler::getFrameCount() const
{
    return m_impl->m_frameCount;
}

PxReal PerformanceProfiler::getLastFrameTime() const
{
    return m_impl->m_currentFrameTiming.frameTime;
}

PxReal PerformanceProfiler::getCurrentFPS() const
{
    if (m_impl->m_currentFrameTiming.frameTime > 0.0f) {
        return 1000.0f / m_impl->m_currentFrameTiming.frameTime;
    }
    return 0.0f;
}

PxReal PerformanceProfiler::getAverageFPS() const
{
    if (m_impl->m_frameCount > 0 && m_impl->m_totalFrameTime > 0.0f) {
        PxReal avgFrameTime = m_impl->m_totalFrameTime / m_impl->m_frameCount;
        return 1000.0f / avgFrameTime;
    }
    return 0.0f;
}

PxReal PerformanceProfiler::getTotalElapsedTime() const
{
    return m_impl->m_totalElapsedTime;
}

// ========================================================================
// Frame History
// ========================================================================

std::vector<PerformanceProfiler::FrameTiming> PerformanceProfiler::getFrameHistory() const
{
    return m_impl->m_frameHistory;
}

void PerformanceProfiler::clearHistory()
{
    m_impl->m_frameHistory.clear();
}

PxU32 PerformanceProfiler::getHistorySize() const
{
    return static_cast<PxU32>(m_impl->m_frameHistory.size());
}

// ========================================================================
// Warnings and Bottlenecks
// ========================================================================

std::vector<PerformanceProfiler::PerformanceWarning> PerformanceProfiler::getWarnings() const
{
    return m_impl->m_warnings;
}

void PerformanceProfiler::clearWarnings()
{
    m_impl->m_warnings.clear();
}

bool PerformanceProfiler::hasBottlenecks() const
{
    PerformanceStats stats = getStats();

    // Check various bottleneck conditions
    if (stats.avgFrameTime > m_impl->m_config.warningFrameTimeMs) return true;
    if (stats.avgFPS < m_impl->m_config.warningFPS) return true;
    if (stats.sceneStats.totalActors > m_impl->m_config.warningActorCount) return true;

    return false;
}

std::string PerformanceProfiler::getBottleneckDescription() const
{
    std::stringstream ss;
    PerformanceStats stats = getStats();

    if (stats.avgFrameTime > m_impl->m_config.warningFrameTimeMs) {
        ss << "High frame time: " << stats.avgFrameTime << "ms (threshold: "
           << m_impl->m_config.warningFrameTimeMs << "ms)\n";
    }

    if (stats.avgFPS < m_impl->m_config.warningFPS) {
        ss << "Low FPS: " << stats.avgFPS << " (threshold: "
           << m_impl->m_config.warningFPS << ")\n";
    }

    if (stats.sceneStats.totalActors > m_impl->m_config.warningActorCount) {
        ss << "High actor count: " << stats.sceneStats.totalActors << " (threshold: "
           << m_impl->m_config.warningActorCount << ")\n";
    }

    return ss.str();
}

// ========================================================================
// Reporting
// ========================================================================

void PerformanceProfiler::printSummary() const
{
    PerformanceStats stats = getStats();

    std::cout << "\n=== Performance Summary ===" << std::endl;
    std::cout << "Frame: " << m_impl->m_frameCount << std::endl;
    std::cout << "FPS: " << std::fixed << std::setprecision(1) << stats.currentFPS
              << " (avg: " << stats.avgFPS << ")" << std::endl;
    std::cout << "Frame Time: " << std::setprecision(2) << m_impl->m_currentFrameTiming.frameTime
              << " ms (avg: " << stats.avgFrameTime << " ms)" << std::endl;
    std::cout << "Actors: " << stats.sceneStats.totalActors << " (D:" << stats.sceneStats.dynamicActors
              << " S:" << stats.sceneStats.staticActors << ")" << std::endl;
    std::cout << "Shapes: " << stats.sceneStats.shapes << std::endl;
    std::cout << "==========================" << std::endl;
}

void PerformanceProfiler::printDetailedReport() const
{
    PerformanceStats stats = getStats();

    std::cout << "\n========================================" << std::endl;
    std::cout << "    Performance Detailed Report" << std::endl;
    std::cout << "========================================\n" << std::endl;

    stats.print();
    std::cout << std::endl;
    stats.sceneStats.print();
    std::cout << std::endl;
    stats.memoryStats.print();

    if (!m_impl->m_warnings.empty()) {
        std::cout << "\n";
        printWarnings();
    }

    if (hasBottlenecks()) {
        std::cout << "\nBottlenecks Detected:" << std::endl;
        std::cout << getBottleneckDescription() << std::endl;
    }
}

void PerformanceProfiler::printFrameHistory(PxU32 numFrames) const
{
    size_t count = m_impl->m_frameHistory.size();
    size_t startIdx = 0;

    if (numFrames > 0 && count > numFrames) {
        startIdx = count - numFrames;
    }

    std::cout << "\nFrame History (last " << (count - startIdx) << " frames):" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Frame | Time(ms) | Sim(ms) | Fetch(ms)" << std::endl;
    std::cout << "------|----------|---------|----------" << std::endl;

    for (size_t i = startIdx; i < count; i++) {
        const FrameTiming& frame = m_impl->m_frameHistory[i];
        std::cout << std::setw(5) << frame.timestamp << " | "
                  << std::setw(8) << frame.frameTime << " | "
                  << std::setw(7) << frame.simulateTime << " | "
                  << std::setw(8) << frame.fetchTime << std::endl;
    }
}

void PerformanceProfiler::printWarnings() const
{
    if (m_impl->m_warnings.empty()) {
        std::cout << "No performance warnings" << std::endl;
        return;
    }

    std::cout << "Performance Warnings (" << m_impl->m_warnings.size() << "):" << std::endl;
    for (const auto& warning : m_impl->m_warnings) {
        std::cout << "  [Frame " << warning.frameNumber << "] " << warning.message
                  << " (value: " << warning.value << ")" << std::endl;
    }
}

bool PerformanceProfiler::exportToCSV(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    // Write header
    file << "Frame,FrameTime(ms),SimulateTime(ms),FetchTime(ms),FPS\n";

    // Write frame data
    for (const auto& frame : m_impl->m_frameHistory) {
        PxReal fps = (frame.frameTime > 0.0f) ? (1000.0f / frame.frameTime) : 0.0f;
        file << frame.timestamp << ","
             << frame.frameTime << ","
             << frame.simulateTime << ","
             << frame.fetchTime << ","
             << fps << "\n";
    }

    file.close();
    std::cout << "Exported " << m_impl->m_frameHistory.size() << " frames to " << filename << std::endl;
    return true;
}

bool PerformanceProfiler::exportToJSON(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    PerformanceStats stats = getStats();

    file << "{\n";
    file << "  \"frameCount\": " << stats.frameCount << ",\n";
    file << "  \"totalTime\": " << stats.totalElapsedTime << ",\n";
    file << "  \"avgFPS\": " << stats.avgFPS << ",\n";
    file << "  \"minFPS\": " << stats.minFPS << ",\n";
    file << "  \"maxFPS\": " << stats.maxFPS << ",\n";
    file << "  \"avgFrameTime\": " << stats.avgFrameTime << ",\n";
    file << "  \"minFrameTime\": " << stats.minFrameTime << ",\n";
    file << "  \"maxFrameTime\": " << stats.maxFrameTime << ",\n";
    file << "  \"stdDevFrameTime\": " << stats.stdDevFrameTime << ",\n";
    file << "  \"actors\": " << stats.sceneStats.totalActors << ",\n";
    file << "  \"shapes\": " << stats.sceneStats.shapes << ",\n";
    file << "  \"constraints\": " << stats.sceneStats.constraints << "\n";
    file << "}\n";

    file.close();
    std::cout << "Exported summary to " << filename << std::endl;
    return true;
}

// ========================================================================
// Configuration
// ========================================================================

void PerformanceProfiler::setConfig(const Config& config)
{
    m_impl->m_config = config;
}

const PerformanceProfiler::Config& PerformanceProfiler::getConfig() const
{
    return m_impl->m_config;
}

void PerformanceProfiler::setEnabled(bool enabled)
{
    m_impl->m_enabled = enabled;
}

bool PerformanceProfiler::isEnabled() const
{
    return m_impl->m_enabled;
}

PxPhysics* PerformanceProfiler::getPhysics() const
{
    return m_impl->m_physics;
}

PxScene* PerformanceProfiler::getScene() const
{
    return m_impl->m_scene;
}

// ========================================================================
// Private Methods
// ========================================================================

void PerformanceProfiler::updateStatistics()
{
    PxReal frameTime = m_impl->m_currentFrameTiming.frameTime;

    m_impl->m_totalFrameTime += frameTime;
    m_impl->m_totalElapsedTime += frameTime / 1000.0f;

    if (frameTime < m_impl->m_minFrameTime) {
        m_impl->m_minFrameTime = frameTime;
    }
    if (frameTime > m_impl->m_maxFrameTime) {
        m_impl->m_maxFrameTime = frameTime;
    }
}

void PerformanceProfiler::checkWarnings()
{
    PxReal frameTime = m_impl->m_currentFrameTiming.frameTime;
    PxReal fps = getCurrentFPS();

    // Check frame time
    if (frameTime > m_impl->m_config.warningFrameTimeMs) {
        m_impl->m_warnings.emplace_back(
            PerformanceWarning::Type::HIGH_FRAME_TIME,
            "Frame time exceeded threshold",
            frameTime,
            m_impl->m_frameCount
        );
    }

    // Check FPS
    if (fps < m_impl->m_config.warningFPS) {
        m_impl->m_warnings.emplace_back(
            PerformanceWarning::Type::LOW_FPS,
            "FPS below threshold",
            fps,
            m_impl->m_frameCount
        );
    }

    // Check actor count
    SceneStats sceneStats = getSceneStats();
    if (sceneStats.totalActors > m_impl->m_config.warningActorCount) {
        m_impl->m_warnings.emplace_back(
            PerformanceWarning::Type::HIGH_ACTOR_COUNT,
            "Too many actors",
            static_cast<PxReal>(sceneStats.totalActors),
            m_impl->m_frameCount
        );
    }

    // Trim warnings if too many
    const size_t MAX_WARNINGS = 100;
    if (m_impl->m_warnings.size() > MAX_WARNINGS) {
        m_impl->m_warnings.erase(m_impl->m_warnings.begin(),
                                  m_impl->m_warnings.begin() + (m_impl->m_warnings.size() - MAX_WARNINGS));
    }
}

void PerformanceProfiler::calculatePercentiles()
{
    // This is called from getStats() where percentiles are calculated
}

} // namespace PhysXWrapper
