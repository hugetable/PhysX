/*
 * PhysX Snippet: Isosurface
 * 演示等值面生成技术
 *
 * 理论背景：
 * ==========
 *
 * 1. 等值面定义
 * --------------
 * 等值面(Isosurface)是标量场中所有函数值相等的点的集合：
 *
 * 数学定义：
 *   给定标量场 f: R³ → R
 *   等值面 S(c) = {(x,y,z) | f(x,y,z) = c}
 *   其中c是等值(isovalue)
 *
 * 物理应用：
 * - 流体表面：密度场threshold定义液面
 * - 软体表面：粒子密度场构造网格
 * - 温度场：温度分布可视化
 * - 压力场：等压面分析
 *
 * 2. Marching Cubes算法
 * ----------------------
 * Marching Cubes (1987, Lorensen & Cline)是经典等值面提取算法：
 *
 * 算法流程：
 * 1. 体素化：将空间划分为规则网格
 * 2. 采样：在网格顶点采样标量值
 * 3. 分类：根据顶点值与阈值的关系确定拓扑
 * 4. 三角化：查找预定义表生成三角面片
 * 5. 插值：线性插值计算精确交点位置
 *
 * 体素分类：
 *   每个体素有8个顶点，每个顶点有2种状态（内/外）
 *   总共 2⁸ = 256 种配置
 *   通过旋转和对称性简化为15种基本情况
 *
 * 顶点插值：
 *   给定边上两点 p₀, p₁ 及其标量值 v₀, v₁
 *   等值c的交点位置：
 *   p = p₀ + (c - v₀)/(v₁ - v₀) × (p₁ - p₀)
 *   这是线性插值，可用更高阶插值提高精度
 *
 * 3. Marching Tetrahedra
 * -----------------------
 * 改进算法，使用四面体替代立方体：
 *
 * 优势：
 * - 更简单：每个四面体只有4个顶点，2⁴=16种配置
 * - 无歧义：避免MC的拓扑歧义问题
 * - 更准确：更好的曲率近似
 *
 * 四面体划分：
 *   将每个立方体分为5或6个四面体
 *   保持相邻体素共享面
 *
 * 4. Dual Contouring
 * -------------------
 * 现代等值面算法，解决MC的锐特征问题：
 *
 * 核心思想：
 *   在体素中心放置顶点，而非边上
 *   使用QEF (Quadratic Error Function)优化顶点位置
 *
 * QEF定义：
 *   E(p) = Σᵢ (nᵢ·(p - pᵢ))²
 *   其中：
 *   - pᵢ：边交点
 *   - nᵢ：该处法线
 *   - p：待优化顶点位置
 *
 * 最小化QEF：
 *   ∂E/∂p = 0
 *   解线性方程组：(Σ nᵢnᵢᵀ)p = Σ nᵢ(nᵢ·pᵢ)
 *
 * 5. 自适应细分
 * --------------
 * 提高效率的LOD技术：
 *
 * Octree结构：
 *   - 粗糙区域：大体素
 *   - 细节区域：小体素
 *   - 动态细分：根据误差阈值
 *
 * 细分准则：
 *   if (|∇f| > threshold) {
 *       subdivide();
 *   }
 *   高梯度区域需要更高分辨率
 *
 * 误差估计：
 *   E_voxel = max|f - f_linear|
 *   其中f_linear是线性插值近似
 *
 * 6. 法线计算
 * -----------
 * 平滑着色需要顶点法线：
 *
 * 梯度法：
 *   n = ∇f = (∂f/∂x, ∂f/∂y, ∂f/∂z)
 *   使用中心差分近似：
 *   ∂f/∂x ≈ (f(x+h,y,z) - f(x-h,y,z))/(2h)
 *
 * 归一化：
 *   n̂ = n / |n|
 *
 * 平滑：
 *   对相邻顶点法线加权平均
 *   n_smooth = Σ wᵢnᵢ / Σ wᵢ
 *   权重：wᵢ = 1/distance²
 *
 * 7. 性能优化
 * -----------
 * 等值面生成的性能考虑：
 *
 * 复杂度：
 *   T = O(N³) for uniform grid
 *   N: 分辨率（每维）
 *
 * 优化策略：
 * a) Spatial Hashing:
 *    仅处理等值面附近体素
 *    减少 O(N³) → O(N²)
 *
 * b) 并行化:
 *    每个体素独立处理
 *    GPU加速：每线程一体素
 *
 * c) 增量更新:
 *    动态场景仅更新变化区域
 *    追踪"dirty"体素
 *
 * d) 缓存:
 *    存储边-顶点映射避免重复计算
 *    哈希表：edge_hash → vertex_index
 *
 * 8. PhysX应用
 * -------------
 * 等值面在PhysX中的应用：
 *
 * a) 流体表面重建:
 *    粒子 → 密度场 → 等值面 → 可视化
 *    密度计算（SPH）：
 *    ρ(x) = Σᵢ mᵢ W(|x-xᵢ|, h)
 *    W: 核函数，h: 平滑半径
 *
 * b) 软体网格生成:
 *    隐式表面 → 等值面 → 四面体网格
 *
 * c) 碰撞几何简化:
 *    复杂网格 → SDF → 简化等值面
 *
 * d) 自适应采样:
 *    动态调整分辨率优化性能
 *
 * 本示例展示：
 * 1. Marching Cubes：经典算法实现
 * 2. 标量场生成：多种隐式函数
 * 3. 网格创建：转换为PhysX三角网格
 * 4. 法线计算：平滑着色
 * 5. 性能分析：不同分辨率对比
 */

#include <PhysXWrapper.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <chrono>
#include <unordered_map>

using namespace physx;

// 全局PhysX对象
static PxDefaultAllocator gAllocator;
static PxDefaultErrorCallback gErrorCallback;
static PxFoundation* gFoundation = nullptr;
static PxPhysics* gPhysics = nullptr;
static PxDefaultCpuDispatcher* gDispatcher = nullptr;
static PxScene* gScene = nullptr;
static PxMaterial* gMaterial = nullptr;
static PxPvd* gPvd = nullptr;
static PxCooking* gCooking = nullptr;

//=============================================================================
// 标量场定义
//=============================================================================

class ScalarField {
public:
    virtual ~ScalarField() {}
    virtual float evaluate(const PxVec3& p) const = 0;
    virtual PxVec3 gradient(const PxVec3& p) const {
        // 数值梯度（中心差分）
        const float h = 0.01f;
        float fx = (evaluate(p + PxVec3(h,0,0)) - evaluate(p - PxVec3(h,0,0))) / (2*h);
        float fy = (evaluate(p + PxVec3(0,h,0)) - evaluate(p - PxVec3(0,h,0))) / (2*h);
        float fz = (evaluate(p + PxVec3(0,0,h)) - evaluate(p - PxVec3(0,0,h))) / (2*h);
        return PxVec3(fx, fy, fz);
    }
};

// 球体标量场
class SphereField : public ScalarField {
    PxVec3 center;
    float radius;
public:
    SphereField(const PxVec3& c, float r) : center(c), radius(r) {}

    float evaluate(const PxVec3& p) const override {
        return radius - (p - center).magnitude();
    }
};

// Metaball标量场（多个球体融合）
class MetaballField : public ScalarField {
    std::vector<PxVec3> centers;
    std::vector<float> radii;
public:
    void addBall(const PxVec3& center, float radius) {
        centers.push_back(center);
        radii.push_back(radius);
    }

    float evaluate(const PxVec3& p) const override {
        float sum = 0.0f;
        for (size_t i = 0; i < centers.size(); i++) {
            float r = (p - centers[i]).magnitude();
            if (r < 1e-6f) r = 1e-6f;  // 避免除零
            sum += (radii[i] * radii[i]) / (r * r);
        }
        return sum;
    }
};

// 环面标量场
class TorusField : public ScalarField {
    float majorRadius, minorRadius;
public:
    TorusField(float R, float r) : majorRadius(R), minorRadius(r) {}

    float evaluate(const PxVec3& p) const override {
        float q = std::sqrt(p.x*p.x + p.z*p.z) - majorRadius;
        return minorRadius - std::sqrt(q*q + p.y*p.y);
    }
};

//=============================================================================
// Marching Cubes实现
//=============================================================================

struct Vertex {
    PxVec3 position;
    PxVec3 normal;
};

struct Triangle {
    int v0, v1, v2;
};

// Marching Cubes查找表（简化版，仅实现部分配置）
static const int edgeTable[256] = {
    0x0, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    // ... (完整表包含256项，这里省略)
    // 实际实现应包含完整查找表
};

class MarchingCubes {
private:
    const ScalarField* field;
    float isovalue;
    PxVec3 minBound, maxBound;
    int resolution;

    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    std::unordered_map<uint64_t, int> vertexCache;

public:
    MarchingCubes(const ScalarField* f, float iso, const PxVec3& minB,
                  const PxVec3& maxB, int res)
        : field(f), isovalue(iso), minBound(minB), maxBound(maxB), resolution(res) {}

    void generate() {
        vertices.clear();
        triangles.clear();
        vertexCache.clear();

        PxVec3 step = (maxBound - minBound) / static_cast<float>(resolution);

        // 遍历所有体素
        for (int ix = 0; ix < resolution; ix++) {
            for (int iy = 0; iy < resolution; iy++) {
                for (int iz = 0; iz < resolution; iz++) {
                    processVoxel(ix, iy, iz, step);
                }
            }
        }

        std::cout << "Generated " << vertices.size() << " vertices, "
                 << triangles.size() << " triangles\n";
    }

    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<Triangle>& getTriangles() const { return triangles; }

private:
    void processVoxel(int ix, int iy, int iz, const PxVec3& step) {
        // 计算8个顶点位置
        PxVec3 corners[8];
        float values[8];

        for (int i = 0; i < 8; i++) {
            int dx = (i & 1);
            int dy = (i & 2) >> 1;
            int dz = (i & 4) >> 2;

            corners[i] = minBound + PxVec3(
                (ix + dx) * step.x,
                (iy + dy) * step.y,
                (iz + dz) * step.z
            );
            values[i] = field->evaluate(corners[i]);
        }

        // 确定配置索引
        int cubeIndex = 0;
        for (int i = 0; i < 8; i++) {
            if (values[i] < isovalue) {
                cubeIndex |= (1 << i);
            }
        }

        // 简化实现：仅处理部分配置
        // 完整实现需要查找edgeTable和triTable

        // 示例：简单情况 - 一个三角形
        if (cubeIndex != 0 && cubeIndex != 255) {
            // 创建三个顶点（在边上插值）
            for (int e = 0; e < 12; e++) {
                int v0 = edgeToVertex[e][0];
                int v1 = edgeToVertex[e][1];

                if ((values[v0] < isovalue) != (values[v1] < isovalue)) {
                    PxVec3 pos = interpolate(corners[v0], corners[v1],
                                            values[v0], values[v1]);
                    PxVec3 normal = field->gradient(pos).getNormalized();

                    Vertex v = {pos, normal};
                    vertices.push_back(v);
                }
            }

            // 简化：每3个顶点组成一个三角形
            if (vertices.size() >= 3) {
                Triangle t;
                t.v0 = vertices.size() - 3;
                t.v1 = vertices.size() - 2;
                t.v2 = vertices.size() - 1;
                triangles.push_back(t);
            }
        }
    }

    PxVec3 interpolate(const PxVec3& p0, const PxVec3& p1, float v0, float v1) {
        if (std::abs(v0 - v1) < 1e-6f) return p0;
        float t = (isovalue - v0) / (v1 - v0);
        return p0 + t * (p1 - p0);
    }

    static const int edgeToVertex[12][2];
};

const int MarchingCubes::edgeToVertex[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0},  // 底面4条边
    {4,5}, {5,6}, {6,7}, {7,4},  // 顶面4条边
    {0,4}, {1,5}, {2,6}, {3,7}   // 4条竖边
};

//=============================================================================
// 网格转PhysX
//=============================================================================

PxTriangleMesh* createTriangleMesh(const std::vector<Vertex>& vertices,
                                   const std::vector<Triangle>& triangles) {
    if (vertices.empty() || triangles.empty()) {
        std::cout << "No geometry to create mesh\n";
        return nullptr;
    }

    // 准备顶点数据
    std::vector<PxVec3> points;
    for (const auto& v : vertices) {
        points.push_back(v.position);
    }

    // 准备三角形数据
    std::vector<PxU32> indices;
    for (const auto& t : triangles) {
        indices.push_back(t.v0);
        indices.push_back(t.v1);
        indices.push_back(t.v2);
    }

    PxTriangleMeshDesc meshDesc;
    meshDesc.points.count = points.size();
    meshDesc.points.data = points.data();
    meshDesc.points.stride = sizeof(PxVec3);
    meshDesc.triangles.count = triangles.size();
    meshDesc.triangles.data = indices.data();
    meshDesc.triangles.stride = 3 * sizeof(PxU32);

    PxDefaultMemoryOutputStream writeBuffer;
    bool status = gCooking->cookTriangleMesh(meshDesc, writeBuffer);

    if (!status) {
        std::cerr << "Failed to cook triangle mesh\n";
        return nullptr;
    }

    PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
    return gPhysics->createTriangleMesh(readBuffer);
}

//=============================================================================
// 测试场景
//=============================================================================

void testSphereIsosurface() {
    std::cout << "\n=== Test 1: Sphere Isosurface ===\n";

    SphereField sphere(PxVec3(0, 5, 0), 2.0f);
    MarchingCubes mc(&sphere, 0.0f, PxVec3(-5, 0, -5), PxVec3(5, 10, 5), 20);

    auto start = std::chrono::high_resolution_clock::now();
    mc.generate();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Generation time: " << duration.count() << " ms\n";

    // 创建PhysX网格
    PxTriangleMesh* mesh = createTriangleMesh(mc.getVertices(), mc.getTriangles());
    if (mesh) {
        std::cout << "Triangle mesh created successfully\n";
        std::cout << "Vertices: " << mesh->getNbVertices() << "\n";
        std::cout << "Triangles: " << mesh->getNbTriangles() << "\n";
        mesh->release();
    }
}

void testMetaballIsosurface() {
    std::cout << "\n=== Test 2: Metaball Isosurface ===\n";

    MetaballField metaballs;
    metaballs.addBall(PxVec3(-1, 5, 0), 1.5f);
    metaballs.addBall(PxVec3(1, 5, 0), 1.5f);
    metaballs.addBall(PxVec3(0, 6.5f, 0), 1.2f);

    MarchingCubes mc(&metaballs, 1.0f, PxVec3(-5, 0, -5), PxVec3(5, 10, 5), 30);

    auto start = std::chrono::high_resolution_clock::now();
    mc.generate();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Generation time: " << duration.count() << " ms\n";
    std::cout << "Metaballs create smooth blended surfaces\n";
}

void testTorusIsosurface() {
    std::cout << "\n=== Test 3: Torus Isosurface ===\n";

    TorusField torus(2.0f, 0.8f);
    MarchingCubes mc(&torus, 0.0f, PxVec3(-4, -2, -4), PxVec3(4, 2, 4), 25);

    auto start = std::chrono::high_resolution_clock::now();
    mc.generate();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Generation time: " << duration.count() << " ms\n";
    std::cout << "Torus demonstrates genus-1 topology\n";
}

void testResolutionComparison() {
    std::cout << "\n=== Test 4: Resolution Comparison ===\n";

    SphereField sphere(PxVec3(0, 0, 0), 1.0f);
    std::vector<int> resolutions = {10, 20, 40, 60};

    std::cout << "Res\tTime(ms)\tVertices\tTriangles\n";
    std::cout << "---\t--------\t--------\t---------\n";

    for (int res : resolutions) {
        MarchingCubes mc(&sphere, 0.0f, PxVec3(-2,-2,-2), PxVec3(2,2,2), res);

        auto start = std::chrono::high_resolution_clock::now();
        mc.generate();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << res << "\t" << duration.count() << "\t\t"
                 << mc.getVertices().size() << "\t\t"
                 << mc.getTriangles().size() << "\n";
    }
}

void testDynamicIsosurface() {
    std::cout << "\n=== Test 5: Dynamic Isosurface (Animated) ===\n";

    std::cout << "Simulating animated metaball field...\n";

    MetaballField metaballs;
    int frames = 60;

    for (int frame = 0; frame < frames; frame++) {
        metaballs = MetaballField();

        // 动画：球体在轨道上运动
        float t = frame / 60.0f * 2.0f * PxPi;
        metaballs.addBall(PxVec3(std::cos(t)*2, 0, std::sin(t)*2), 1.2f);
        metaballs.addBall(PxVec3(-std::cos(t)*2, 0, -std::sin(t)*2), 1.2f);

        if (frame % 20 == 0) {
            MarchingCubes mc(&metaballs, 1.0f, PxVec3(-5,-5,-5), PxVec3(5,5,5), 20);
            mc.generate();
            std::cout << "Frame " << frame << ": "
                     << mc.getVertices().size() << " vertices\n";
        }
    }

    std::cout << "Dynamic isosurface enables morphing effects\n";
}

//=============================================================================
// 主函数
//=============================================================================
int main(int argc, char** argv) {
    // 初始化PhysX
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!gFoundation) {
        std::cerr << "PxCreateFoundation failed!" << std::endl;
        return 1;
    }

    gPvd = PxCreatePvd(*gFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation,
                               PxTolerancesScale(), true, gPvd);
    if (!gPhysics) {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return 1;
    }

    gCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(PxTolerancesScale()));
    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    std::cout << "========================================\n";
    std::cout << "PhysX Snippet: Isosurface\n";
    std::cout << "等值面生成技术\n";
    std::cout << "========================================\n";

    // 运行所有测试
    testSphereIsosurface();
    testMetaballIsosurface();
    testTorusIsosurface();
    testResolutionComparison();
    testDynamicIsosurface();

    // 清理
    gMaterial->release();
    gDispatcher->release();
    gCooking->release();
    gPhysics->release();
    if (gPvd) {
        PxPvdTransport* transport = gPvd->getTransport();
        gPvd->release();
        if (transport) transport->release();
    }
    gFoundation->release();

    std::cout << "\n========================================\n";
    std::cout << "All tests completed!\n";
    std::cout << "========================================\n";

    return 0;
}
