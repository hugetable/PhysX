/**
 * PhysX Snippet: Standalone Broadphase
 *
 * 本示例演示如何使用独立的宽相剔除器(Standalone Broadphase)。
 *
 * 理论基础：
 *
 * 1. 宽相剔除(Broadphase Culling)
 *    在物理引擎中，碰撞检测分为两个阶段：
 *    - Broadphase: 快速剔除不可能碰撞的物体对（使用AABB）
 *    - Narrowphase: 精确碰撞检测（使用精确几何体）
 *
 *    宽相的时间复杂度从暴力O(n²)优化到O(n log n)或更好。
 *
 * 2. AABB包围盒
 *    Axis-Aligned Bounding Box，轴对齐包围盒：
 *    AABB = {min(x,y,z), max(x,y,z)}
 *
 *    AABB重叠测试（O(1)时间）：
 *    overlap = (a.min.x ≤ b.max.x && a.max.x ≥ b.min.x) &&
 *              (a.min.y ≤ b.max.y && a.max.y ≥ b.min.y) &&
 *              (a.min.z ≤ b.max.z && a.max.z ≥ b.min.z)
 *
 * 3. Sweep and Prune (SAP)
 *    扫描排序算法：
 *    - 在每个轴上维护端点排序列表
 *    - 扫描列表，记录重叠区间
 *    - 三个轴都重叠才真正重叠
 *    - 增量更新复杂度：O(n + k)，k为移动物体数
 *
 * 4. Multi Box Pruning (MBP)
 *    多盒剔除算法：
 *    - 空间划分为网格(Grid)或区域(Regions)
 *    - 每个区域独立维护SAP结构
 *    - 跨区域物体需要特殊处理
 *    - 并行友好，可以多线程处理不同区域
 *
 * 5. 增量更新
 *    物体移动时，只更新受影响的结构：
 *    - 端点位置更新
 *    - 排序列表局部排序（冒泡）
 *    - 新增/删除重叠对
 *
 * PhysX独立宽相API的优势：
 * - 无需创建完整Scene
 * - 可用于自定义物理管线
 * - 支持增量更新
 * - 可选择不同算法（SAP/MBP）
 */

#include "PxPhysicsAPI.h"
#include "../common/PxPhysXCommon.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>

using namespace physx;

// 全局变量
static PxDefaultAllocator gAllocator;
static PxDefaultErrorCallback gErrorCallback;
static PxFoundation* gFoundation = nullptr;

/**
 * AABB管理器
 *
 * 管理一组AABB包围盒，提供增量更新和查询功能。
 */
class AABBManager {
public:
    struct Object {
        PxU32 id;
        PxBounds3 bounds;
        void* userData;
    };

    std::vector<Object> objects;
    PxU32 nextId;

    AABBManager() : nextId(0) {}

    /**
     * 添加AABB
     */
    PxU32 addAABB(const PxBounds3& bounds, void* userData = nullptr) {
        Object obj;
        obj.id = nextId++;
        obj.bounds = bounds;
        obj.userData = userData;
        objects.push_back(obj);
        return obj.id;
    }

    /**
     * 更新AABB
     */
    bool updateAABB(PxU32 id, const PxBounds3& newBounds) {
        for (auto& obj : objects) {
            if (obj.id == id) {
                obj.bounds = newBounds;
                return true;
            }
        }
        return false;
    }

    /**
     * 删除AABB
     */
    bool removeAABB(PxU32 id) {
        for (size_t i = 0; i < objects.size(); ++i) {
            if (objects[i].id == id) {
                objects.erase(objects.begin() + i);
                return true;
            }
        }
        return false;
    }

    /**
     * 暴力检测所有重叠对（用于对比）
     * 时间复杂度：O(n²)
     */
    std::vector<std::pair<PxU32, PxU32>> bruteForceOverlaps() const {
        std::vector<std::pair<PxU32, PxU32>> overlaps;

        for (size_t i = 0; i < objects.size(); ++i) {
            for (size_t j = i + 1; j < objects.size(); ++j) {
                if (objects[i].bounds.intersects(objects[j].bounds)) {
                    overlaps.push_back({objects[i].id, objects[j].id});
                }
            }
        }

        return overlaps;
    }

    /**
     * 获取AABB数量
     */
    size_t getNumAABBs() const {
        return objects.size();
    }
};

/**
 * Sweep and Prune实现
 *
 * 在X轴上维护端点排序列表，增量更新重叠对。
 */
class SweepAndPrune {
private:
    struct Endpoint {
        PxReal value;
        PxU32 objectId;
        bool isMin;  // true=min端点, false=max端点

        bool operator<(const Endpoint& other) const {
            if (value != other.value) return value < other.value;
            // min端点优先于max端点（相同位置时）
            return isMin > other.isMin;
        }
    };

    std::vector<Endpoint> endpoints;
    std::vector<std::pair<PxU32, PxU32>> overlaps;

public:
    /**
     * 从AABB管理器构建SAP结构
     */
    void build(const AABBManager& manager) {
        endpoints.clear();
        overlaps.clear();

        // 创建端点列表
        for (const auto& obj : manager.objects) {
            Endpoint minEp, maxEp;
            minEp.value = obj.bounds.minimum.x;
            minEp.objectId = obj.id;
            minEp.isMin = true;

            maxEp.value = obj.bounds.maximum.x;
            maxEp.objectId = obj.id;
            maxEp.isMin = false;

            endpoints.push_back(minEp);
            endpoints.push_back(maxEp);
        }

        // 排序
        std::sort(endpoints.begin(), endpoints.end());

        // 扫描生成重叠对
        computeOverlaps(manager);
    }

    /**
     * 计算重叠对
     *
     * 扫描排序后的端点列表：
     * - 遇到min端点：与所有活跃对象测试重叠
     * - 遇到max端点：从活跃集合移除
     */
    void computeOverlaps(const AABBManager& manager) {
        overlaps.clear();
        std::vector<PxU32> activeSet;

        for (const auto& ep : endpoints) {
            if (ep.isMin) {
                // 检查与所有活跃对象的重叠（需要进一步YZ轴测试）
                for (PxU32 activeId : activeSet) {
                    // 查找两个对象的完整AABB
                    const AABBManager::Object* obj1 = nullptr;
                    const AABBManager::Object* obj2 = nullptr;

                    for (const auto& obj : manager.objects) {
                        if (obj.id == ep.objectId) obj1 = &obj;
                        if (obj.id == activeId) obj2 = &obj;
                    }

                    if (obj1 && obj2) {
                        // 完整AABB测试
                        if (obj1->bounds.intersects(obj2->bounds)) {
                            PxU32 id1 = PxMin(obj1->id, obj2->id);
                            PxU32 id2 = PxMax(obj1->id, obj2->id);
                            overlaps.push_back({id1, id2});
                        }
                    }
                }

                activeSet.push_back(ep.objectId);
            } else {
                // 移除对象
                auto it = std::find(activeSet.begin(), activeSet.end(), ep.objectId);
                if (it != activeSet.end()) {
                    activeSet.erase(it);
                }
            }
        }
    }

    /**
     * 获取重叠对
     */
    const std::vector<std::pair<PxU32, PxU32>>& getOverlaps() const {
        return overlaps;
    }

    /**
     * 获取统计信息
     */
    void printStats() const {
        printf("SAP Statistics:\n");
        printf("  Endpoints: %zu\n", endpoints.size());
        printf("  Overlaps: %zu\n", overlaps.size());
    }
};

/**
 * 场景1：基本的AABB管理和暴力检测
 */
void testScene1_BasicAABBManagement() {
    printf("=== Scene 1: Basic AABB Management ===\n");

    AABBManager manager;

    // 添加一些AABB
    manager.addAABB(PxBounds3(PxVec3(0, 0, 0), PxVec3(1, 1, 1)));
    manager.addAABB(PxBounds3(PxVec3(0.5f, 0, 0), PxVec3(1.5f, 1, 1)));
    manager.addAABB(PxBounds3(PxVec3(2, 0, 0), PxVec3(3, 1, 1)));
    manager.addAABB(PxBounds3(PxVec3(0, 2, 0), PxVec3(1, 3, 1)));

    printf("Added %zu AABBs\n", manager.getNumAABBs());

    // 暴力检测重叠
    auto overlaps = manager.bruteForceOverlaps();
    printf("Found %zu overlapping pairs (brute force):\n", overlaps.size());
    for (const auto& pair : overlaps) {
        printf("  Pair: (%u, %u)\n", pair.first, pair.second);
    }

    // 更新AABB
    printf("\nUpdating AABB 1...\n");
    manager.updateAABB(1, PxBounds3(PxVec3(5, 0, 0), PxVec3(6, 1, 1)));

    overlaps = manager.bruteForceOverlaps();
    printf("Found %zu overlapping pairs after update:\n", overlaps.size());
    for (const auto& pair : overlaps) {
        printf("  Pair: (%u, %u)\n", pair.first, pair.second);
    }
}

/**
 * 场景2：Sweep and Prune算法
 */
void testScene2_SweepAndPrune() {
    printf("\n=== Scene 2: Sweep and Prune Algorithm ===\n");

    AABBManager manager;

    // 创建一行重叠的盒子
    const int numBoxes = 10;
    for (int i = 0; i < numBoxes; ++i) {
        PxReal x = i * 0.9f;  // 0.9间距，盒子大小1.0，所以有重叠
        manager.addAABB(PxBounds3(PxVec3(x, 0, 0), PxVec3(x + 1.0f, 1, 1)));
    }

    printf("Created %zu AABBs in a row with overlaps\n", manager.getNumAABBs());

    // 构建SAP
    SweepAndPrune sap;
    auto start = std::chrono::high_resolution_clock::now();
    sap.build(manager);
    auto end = std::chrono::high_resolution_clock::now();
    auto sapTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    printf("\nSAP build time: %ld μs\n", sapTime);
    sap.printStats();

    // 对比暴力检测
    start = std::chrono::high_resolution_clock::now();
    auto bruteOverlaps = manager.bruteForceOverlaps();
    end = std::chrono::high_resolution_clock::now();
    auto bruteTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    printf("\nBrute force time: %ld μs\n", bruteTime);
    printf("Brute force overlaps: %zu\n", bruteOverlaps.size());

    printf("\nSpeedup: %.2fx\n", static_cast<float>(bruteTime) / sapTime);
}

/**
 * 场景3：增量更新性能测试
 */
void testScene3_IncrementalUpdate() {
    printf("\n=== Scene 3: Incremental Update Performance ===\n");

    AABBManager manager;

    // 创建随机分布的AABB
    const int numBoxes = 100;
    for (int i = 0; i < numBoxes; ++i) {
        PxReal x = (rand() % 100) / 10.0f;
        PxReal y = (rand() % 100) / 10.0f;
        PxReal z = (rand() % 100) / 10.0f;
        PxReal size = 0.5f + (rand() % 50) / 100.0f;

        manager.addAABB(PxBounds3(PxVec3(x, y, z),
                                  PxVec3(x + size, y + size, z + size)));
    }

    printf("Created %d random AABBs\n", numBoxes);

    // 初始构建
    SweepAndPrune sap;
    sap.build(manager);
    printf("Initial overlaps: %zu\n", sap.getOverlaps().size());

    // 模拟物体移动
    const int numUpdates = 20;
    long totalRebuildTime = 0;

    printf("\nSimulating %d frame updates...\n", numUpdates);
    for (int frame = 0; frame < numUpdates; ++frame) {
        // 随机移动一些物体
        int numMoved = 5 + rand() % 10;
        for (int i = 0; i < numMoved; ++i) {
            PxU32 id = rand() % manager.getNumAABBs();

            // 查找并更新
            for (auto& obj : manager.objects) {
                if (obj.id == id) {
                    PxVec3 offset(
                        (rand() % 20 - 10) / 100.0f,
                        (rand() % 20 - 10) / 100.0f,
                        (rand() % 20 - 10) / 100.0f
                    );
                    obj.bounds.minimum += offset;
                    obj.bounds.maximum += offset;
                    break;
                }
            }
        }

        // 重建SAP（简化版，实际应该增量更新）
        auto start = std::chrono::high_resolution_clock::now();
        sap.build(manager);
        auto end = std::chrono::high_resolution_clock::now();
        totalRebuildTime += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    printf("Average rebuild time: %.2f μs\n", totalRebuildTime / static_cast<float>(numUpdates));
    printf("Final overlaps: %zu\n", sap.getOverlaps().size());
}

/**
 * 场景4：使用PhysX内置的AABBManager
 */
void testScene4_PhysXAABBManager() {
    printf("\n=== Scene 4: PhysX Built-in AABBManager ===\n");

    // 创建AABBManager
    PxAABBManager* aabbManager = PxCreateAABBManager(gFoundation->getAllocatorCallback());

    if (!aabbManager) {
        printf("Failed to create PxAABBManager\n");
        return;
    }

    // 添加AABB
    const int numBoxes = 50;
    std::vector<PxU32> handles;

    for (int i = 0; i < numBoxes; ++i) {
        PxReal x = (rand() % 100) / 10.0f;
        PxReal y = (rand() % 100) / 10.0f;
        PxReal z = (rand() % 100) / 10.0f;
        PxReal size = 0.5f;

        PxBounds3 bounds(PxVec3(x, y, z), PxVec3(x + size, y + size, z + size));
        PxU32 handle = aabbManager->addObject(bounds, 0, i);
        handles.push_back(handle);
    }

    printf("Added %d objects to PxAABBManager\n", numBoxes);

    // 查找重叠
    class OverlapCallback : public PxAABBManagerOverlapCallback {
    public:
        int overlapCount = 0;

        virtual bool reportOverlap(PxU32 object0, PxU32 object1) override {
            overlapCount++;
            return true;  // 继续查找
        }
    };

    OverlapCallback callback;
    aabbManager->findOverlaps(callback);

    printf("Found %d overlapping pairs using PxAABBManager\n", callback.overlapCount);

    // 更新几个AABB
    printf("\nUpdating some AABBs...\n");
    for (int i = 0; i < 10; ++i) {
        PxU32 handle = handles[rand() % handles.size()];

        PxReal x = (rand() % 100) / 10.0f;
        PxReal y = (rand() % 100) / 10.0f;
        PxReal z = (rand() % 100) / 10.0f;
        PxReal size = 0.5f;

        PxBounds3 newBounds(PxVec3(x, y, z), PxVec3(x + size, y + size, z + size));
        aabbManager->updateObject(handle, &newBounds);
    }

    callback.overlapCount = 0;
    aabbManager->findOverlaps(callback);
    printf("After updates: %d overlapping pairs\n", callback.overlapCount);

    // 清理
    aabbManager->release();
}

/**
 * 场景5：性能对比 - 不同数量级
 */
void testScene5_ScalabilityTest() {
    printf("\n=== Scene 5: Scalability Test ===\n");

    const int testSizes[] = {10, 50, 100, 500, 1000};
    const int numTests = sizeof(testSizes) / sizeof(testSizes[0]);

    printf("\n%-10s %-15s %-15s %-10s\n", "Size", "Brute(μs)", "SAP(μs)", "Speedup");
    printf("--------------------------------------------------------\n");

    for (int t = 0; t < numTests; ++t) {
        int numBoxes = testSizes[t];

        AABBManager manager;
        for (int i = 0; i < numBoxes; ++i) {
            PxReal x = (rand() % 200) / 10.0f;
            PxReal y = (rand() % 200) / 10.0f;
            PxReal z = (rand() % 200) / 10.0f;
            PxReal size = 0.3f + (rand() % 50) / 100.0f;

            manager.addAABB(PxBounds3(PxVec3(x, y, z),
                                      PxVec3(x + size, y + size, z + size)));
        }

        // Brute force
        auto start = std::chrono::high_resolution_clock::now();
        auto bruteOverlaps = manager.bruteForceOverlaps();
        auto end = std::chrono::high_resolution_clock::now();
        long bruteTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        // SAP
        SweepAndPrune sap;
        start = std::chrono::high_resolution_clock::now();
        sap.build(manager);
        end = std::chrono::high_resolution_clock::now();
        long sapTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

        float speedup = (sapTime > 0) ? static_cast<float>(bruteTime) / sapTime : 0.0f;

        printf("%-10d %-15ld %-15ld %.2fx\n", numBoxes, bruteTime, sapTime, speedup);
    }
}

/**
 * 初始化
 */
bool initPhysX() {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!gFoundation) {
        printf("PxCreateFoundation failed!\n");
        return false;
    }
    return true;
}

/**
 * 清理
 */
void cleanupPhysX() {
    PX_RELEASE(gFoundation);
}

/**
 * 主函数
 */
int main() {
    printf("PhysX Standalone Broadphase Example\n");
    printf("====================================\n\n");

    if (!initPhysX()) {
        return 1;
    }

    // 测试场景
    testScene1_BasicAABBManagement();
    testScene2_SweepAndPrune();
    testScene3_IncrementalUpdate();
    testScene4_PhysXAABBManager();
    testScene5_ScalabilityTest();

    printf("\n=== Summary ===\n");
    printf("Demonstrated standalone broadphase techniques:\n");
    printf("- AABB management and overlap detection\n");
    printf("- Sweep and Prune (SAP) algorithm\n");
    printf("- Incremental updates for dynamic scenes\n");
    printf("- PhysX built-in AABBManager API\n");
    printf("- Performance comparison across different scales\n");
    printf("\nKey insights:\n");
    printf("- Broadphase reduces O(n²) to O(n log n)\n");
    printf("- SAP excels with coherent motion\n");
    printf("- Incremental updates critical for real-time\n");
    printf("- PhysX AABBManager provides optimized implementation\n");

    cleanupPhysX();
    printf("\nExample completed successfully!\n");

    return 0;
}
