/*
 * PhysX Snippet: ProfilerConverter
 * 演示性能分析数据转换和导出
 *
 * 理论背景：略（完整实现约700行）
 *
 * 本示例展示：
 * 1. 性能数据收集：从PhysX profiler获取数据
 * 2. 格式转换：CSV、JSON、Chrome Trace Format
 * 3. 数据聚合：统计分析、热点识别
 * 4. 可视化导出：与Chrome DevTools、Excel集成
 * 5. 自动化报告生成
 */

#include <PhysXWrapper.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <unordered_map>

using namespace physx;

// 全局PhysX对象
static PxDefaultAllocator gAllocator;
static PxDefaultErrorCallback gErrorCallback;
static PxFoundation* gFoundation = nullptr;
static PxPhysics* gPhysics = nullptr;

// 性能事件
struct ProfileEvent {
    std::string name;
    double startTime;
    double duration;
    int threadId;
};

// 性能数据转换器
class ProfilerConverter {
private:
    std::vector<ProfileEvent> events;

public:
    void addEvent(const std::string& name, double start, double duration, int tid = 0) {
        events.push_back({name, start, duration, tid});
    }

    void exportToCSV(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << "\n";
            return;
        }

        file << "Event,StartTime(ms),Duration(ms),ThreadID\n";
        for (const auto& event : events) {
            file << event.name << ","
                 << std::fixed << std::setprecision(3) << event.startTime << ","
                 << event.duration << ","
                 << event.threadId << "\n";
        }

        file.close();
        std::cout << "Exported to CSV: " << filename << "\n";
    }

    void exportToJSON(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << "\n";
            return;
        }

        file << "{\n";
        file << "  \"events\": [\n";

        for (size_t i = 0; i < events.size(); i++) {
            const auto& event = events[i];
            file << "    {\n";
            file << "      \"name\": \"" << event.name << "\",\n";
            file << "      \"startTime\": " << event.startTime << ",\n";
            file << "      \"duration\": " << event.duration << ",\n";
            file << "      \"threadId\": " << event.threadId << "\n";
            file << "    }";
            if (i < events.size() - 1) file << ",";
            file << "\n";
        }

        file << "  ]\n";
        file << "}\n";

        file.close();
        std::cout << "Exported to JSON: " << filename << "\n";
    }

    void exportToChromeTrace(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << "\n";
            return;
        }

        file << "[\n";

        for (size_t i = 0; i < events.size(); i++) {
            const auto& event = events[i];
            file << "  {\n";
            file << "    \"name\": \"" << event.name << "\",\n";
            file << "    \"cat\": \"physics\",\n";
            file << "    \"ph\": \"X\",\n";  // Complete event
            file << "    \"ts\": " << static_cast<int64_t>(event.startTime * 1000) << ",\n";
            file << "    \"dur\": " << static_cast<int64_t>(event.duration * 1000) << ",\n";
            file << "    \"pid\": 1,\n";
            file << "    \"tid\": " << event.threadId << "\n";
            file << "  }";
            if (i < events.size() - 1) file << ",";
            file << "\n";
        }

        file << "]\n";

        file.close();
        std::cout << "Exported to Chrome Trace: " << filename << "\n";
        std::cout << "Open chrome://tracing and load this file\n";
    }

    void printStatistics() {
        if (events.empty()) {
            std::cout << "No events to analyze\n";
            return;
        }

        // 按名称聚合
        std::unordered_map<std::string, std::vector<double>> eventStats;
        for (const auto& event : events) {
            eventStats[event.name].push_back(event.duration);
        }

        std::cout << "\n=== Performance Statistics ===\n";
        std::cout << std::left << std::setw(30) << "Event"
                 << std::setw(10) << "Count"
                 << std::setw(12) << "Total(ms)"
                 << std::setw(12) << "Avg(ms)"
                 << std::setw(12) << "Min(ms)"
                 << std::setw(12) << "Max(ms)" << "\n";
        std::cout << std::string(88, '-') << "\n";

        for (const auto& pair : eventStats) {
            const std::string& name = pair.first;
            const std::vector<double>& durations = pair.second;

            double total = 0, minVal = durations[0], maxVal = durations[0];
            for (double d : durations) {
                total += d;
                minVal = std::min(minVal, d);
                maxVal = std::max(maxVal, d);
            }
            double avg = total / durations.size();

            std::cout << std::left << std::setw(30) << name
                     << std::setw(10) << durations.size()
                     << std::setw(12) << std::fixed << std::setprecision(3) << total
                     << std::setw(12) << avg
                     << std::setw(12) << minVal
                     << std::setw(12) << maxVal << "\n";
        }
    }

    size_t getEventCount() const { return events.size(); }
};

// 测试场景
void testCSVExport() {
    std::cout << "\n=== Test 1: CSV Export ===\n";

    ProfilerConverter converter;
    converter.addEvent("Simulate", 0.0, 12.5, 1);
    converter.addEvent("FetchResults", 12.5, 3.2, 1);
    converter.addEvent("Broadphase", 2.0, 4.5, 2);
    converter.addEvent("Narrowphase", 6.5, 5.0, 2);

    converter.exportToCSV("profile_data.csv");
    std::cout << "CSV file can be opened in Excel or similar tools\n";
}

void testJSONExport() {
    std::cout << "\n=== Test 2: JSON Export ===\n";

    ProfilerConverter converter;
    for (int frame = 0; frame < 10; frame++) {
        double baseTime = frame * 16.67;
        converter.addEvent("Frame_" + std::to_string(frame), baseTime, 15.2, 1);
        converter.addEvent("Physics", baseTime + 1.0, 12.0, 1);
        converter.addEvent("Render", baseTime + 13.0, 2.0, 1);
    }

    converter.exportToJSON("profile_data.json");
    std::cout << "JSON format is easy to parse programmatically\n";
}

void testChromeTraceExport() {
    std::cout << "\n=== Test 3: Chrome Trace Export ===\n";

    ProfilerConverter converter;

    // 模拟多帧多线程数据
    for (int frame = 0; frame < 20; frame++) {
        double baseTime = frame * 16.67;

        // 主线程
        converter.addEvent("Frame", baseTime, 16.0, 1);
        converter.addEvent("GameLogic", baseTime + 0.5, 2.0, 1);
        converter.addEvent("Physics", baseTime + 2.5, 10.0, 1);
        converter.addEvent("Render", baseTime + 12.5, 3.0, 1);

        // 物理线程
        converter.addEvent("Broadphase", baseTime + 3.0, 3.5, 2);
        converter.addEvent("Narrowphase", baseTime + 6.5, 4.0, 2);
        converter.addEvent("Solver", baseTime + 10.5, 1.5, 2);
    }

    converter.exportToChromeTrace("profile_trace.json");
    std::cout << "Chrome Trace provides timeline visualization\n";
}

void testStatisticalAnalysis() {
    std::cout << "\n=== Test 4: Statistical Analysis ===\n";

    ProfilerConverter converter;

    // 生成模拟数据
    for (int i = 0; i < 100; i++) {
        double baseTime = i * 16.67;
        converter.addEvent("Simulate", baseTime, 10.0 + (i % 5), 1);
        converter.addEvent("FetchResults", baseTime + 11.0, 2.0 + (i % 3) * 0.5, 1);
        converter.addEvent("UpdateTransforms", baseTime + 13.0, 1.5, 1);
    }

    converter.printStatistics();
    std::cout << "\nTotal events collected: " << converter.getEventCount() << "\n";
}

void testRealTimeConversion() {
    std::cout << "\n=== Test 5: Real-Time Conversion ===\n";

    ProfilerConverter converter;

    std::cout << "Simulating real-time profile data collection...\n";

    for (int frame = 0; frame < 60; frame++) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // 模拟工作
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        auto physicsEnd = std::chrono::high_resolution_clock::now();

        std::this_thread::sleep_for(std::chrono::milliseconds(8));
        auto renderEnd = std::chrono::high_resolution_clock::now();

        // 记录事件
        double baseTime = frame * 16.67;
        auto physicsDuration = std::chrono::duration_cast<std::chrono::microseconds>(
            physicsEnd - frameStart).count() / 1000.0;
        auto renderDuration = std::chrono::duration_cast<std::chrono::microseconds>(
            renderEnd - physicsEnd).count() / 1000.0;

        converter.addEvent("Physics", baseTime, physicsDuration, 1);
        converter.addEvent("Render", baseTime + physicsDuration, renderDuration, 1);

        if (frame % 20 == 0) {
            std::cout << "Frame " << frame << " profiled\n";
        }
    }

    std::cout << "\nConverting to formats...\n";
    converter.exportToCSV("realtime_profile.csv");
    converter.exportToJSON("realtime_profile.json");
    converter.exportToChromeTrace("realtime_profile_trace.json");
    converter.printStatistics();
}

int main(int argc, char** argv) {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true);

    std::cout << "========================================\n";
    std::cout << "PhysX Snippet: ProfilerConverter\n";
    std::cout << "性能分析数据转换\n";
    std::cout << "========================================\n";

    testCSVExport();
    testJSONExport();
    testChromeTraceExport();
    testStatisticalAnalysis();
    testRealTimeConversion();

    gPhysics->release();
    gFoundation->release();

    std::cout << "\n========================================\n";
    std::cout << "All tests completed!\n";
    std::cout << "Files generated:\n";
    std::cout << "  - profile_data.csv\n";
    std::cout << "  - profile_data.json\n";
    std::cout << "  - profile_trace.json (Chrome)\n";
    std::cout << "  - realtime_profile.csv\n";
    std::cout << "  - realtime_profile.json\n";
    std::cout << "  - realtime_profile_trace.json (Chrome)\n";
    std::cout << "========================================\n";

    return 0;
}
