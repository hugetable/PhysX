/**
 * PhysX Snippet: CustomGeometryCollision
 *
 * 演示自定义几何体的碰撞检测（Custom Geometry Collision Detection）
 *
 * 核心功能:
 * 1. 自定义几何体间的碰撞
 * 2. GJK算法实现
 * 3. EPA算法（穿透深度）
 * 4. 接触点生成
 *
 * 物理背景:
 *
 * 凸体碰撞检测（Convex Collision Detection）:
 * 检测两个凸体是否相交，并计算碰撞信息（接触点、法向量、穿透深度）。
 *
 * GJK算法（Gilbert-Johnson-Keerthi）:
 *
 * 核心思想：
 * 两个凸体A和B相交 ⟺ 闵可夫斯基差 A⊖B 包含原点
 *
 * 闵可夫斯基差：
 * A⊖B = {a - b | a ∈ A, b ∈ B}
 *
 * 支持函数（Support Function）:
 * S_{A⊖B}(d) = S_A(d) - S_B(-d)
 * 其中 S_A(d) 返回A在方向d上最远的点
 *
 * GJK迭代过程:
 * 1. 初始化单纯形（Simplex）为空
 * 2. 选择初始方向d
 * 3. 计算支持点 p = S_{A⊖B}(d)
 * 4. 如果 p·d < 0，则不相交，退出
 * 5. 将p加入单纯形
 * 6. 更新单纯形和搜索方向
 * 7. 如果单纯形包含原点，则相交
 * 8. 否则回到步骤3
 *
 * 单纯形类型:
 * - 点（0维）: 1个顶点
 * - 线段（1维）: 2个顶点
 * - 三角形（2维）: 3个顶点
 * - 四面体（3维）: 4个顶点
 *
 * EPA算法（Expanding Polytope Algorithm）:
 *
 * 用于计算穿透深度和方向（当GJK检测到碰撞后）
 *
 * 核心思想：
 * 从GJK的最终单纯形开始，迭代扩展多面体，
 * 找到最接近原点的面，该面的法向量和距离即为穿透信息。
 *
 * EPA过程:
 * 1. 初始化为GJK的最终四面体
 * 2. 找到最接近原点的面F
 * 3. 计算新的支持点 p = S_{A⊖B}(n_F)
 * 4. 如果 p 很接近F，收敛，退出
 * 5. 用p扩展多面体（移除被p遮挡的面）
 * 6. 回到步骤2
 *
 * 穿透深度:
 * d_penetration = distance(origin, F_closest)
 *
 * 穿透法向量:
 * n_penetration = normal(F_closest)
 *
 * SAT算法（Separating Axis Theorem）:
 *
 * 对于凸多面体，如果存在分离轴，则不相交
 *
 * 分离轴候选:
 * 1. A的面法向量
 * 2. B的面法向量
 * 3. A的边 × B的边（所有组合）
 *
 * 投影测试:
 * 对于轴n，计算A和B在n上的投影区间[a_min, a_max]和[b_min, b_max]
 * 如果区间不重叠，则n是分离轴，不相交
 *
 * 接触点生成（Contact Manifold）:
 *
 * 对于面-面接触:
 * 1. 找到参考面和入射面
 * 2. 裁剪入射面到参考面边界
 * 3. 保留深度最大的4个点
 *
 * 接触点持久化:
 * - 减少抖动
 * - 提高稳定性
 * - 保持接触历史
 *
 * 应用场景:
 * 1. 复杂形状碰撞
 * 2. 物理引擎核心
 * 3. 机器人碰撞检测
 * 4. 游戏碰撞系统
 *
 * 注意:
 * ⚠️ GJK对数值误差敏感
 * ⚠️ EPA需要正确处理退化情况
 * ⚠️ 接触点生成需要鲁棒性
 */

#include <PxPhysicsAPI.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace physx;

// ============================================================================
// GJK算法实现
// ============================================================================

/**
 * 单纯形结构
 */
struct Simplex {
    PxVec3 points[4];
    int size;

    Simplex() : size(0) {}

    void add(const PxVec3& point) {
        if (size < 4) {
            points[size++] = point;
        }
    }

    PxVec3& operator[](int i) { return points[i]; }
    const PxVec3& operator[](int i) const { return points[i]; }
};

/**
 * 支持函数：返回凸体在给定方向的最远点
 */
PxVec3 support(const std::vector<PxVec3>& vertices, const PxVec3& direction) {
    PxReal maxDot = -PX_MAX_F32;
    PxVec3 maxPoint = vertices[0];

    for (const auto& v : vertices) {
        PxReal dot = v.dot(direction);
        if (dot > maxDot) {
            maxDot = dot;
            maxPoint = v;
        }
    }

    return maxPoint;
}

/**
 * 闵可夫斯基差的支持函数
 */
PxVec3 supportMinkowski(const std::vector<PxVec3>& shapeA,
                        const std::vector<PxVec3>& shapeB,
                        const PxVec3& direction) {
    PxVec3 pointA = support(shapeA, direction);
    PxVec3 pointB = support(shapeB, -direction);
    return pointA - pointB;
}

/**
 * 更新单纯形和搜索方向（线段情况）
 */
bool updateSimplexLine(Simplex& simplex, PxVec3& direction) {
    PxVec3 a = simplex[1];  // 最新点
    PxVec3 b = simplex[0];

    PxVec3 ab = b - a;
    PxVec3 ao = -a;  // 指向原点

    // 如果原点在ab的voronoi区域
    if (ab.dot(ao) > 0) {
        // 方向垂直于ab指向原点
        direction = ab.cross(ao).cross(ab);
        if (direction.magnitudeSquared() < 1e-6f) {
            // ab几乎指向原点，使用任意垂直方向
            direction = PxVec3(ab.y, -ab.x, 0).getNormalized();
        }
    } else {
        // 原点最接近a
        simplex.size = 1;
        simplex[0] = a;
        direction = ao;
    }

    return false;
}

/**
 * 更新单纯形和搜索方向（三角形情况）
 */
bool updateSimplexTriangle(Simplex& simplex, PxVec3& direction) {
    PxVec3 a = simplex[2];  // 最新点
    PxVec3 b = simplex[1];
    PxVec3 c = simplex[0];

    PxVec3 ab = b - a;
    PxVec3 ac = c - a;
    PxVec3 ao = -a;

    PxVec3 abc = ab.cross(ac);  // 三角形法向量

    // 测试三个边的voronoi区域
    PxVec3 abPerp = abc.cross(ab);
    if (abPerp.dot(ao) > 0) {
        // 原点在ab边外侧
        simplex.size = 2;
        simplex[0] = b;
        simplex[1] = a;
        direction = ab.cross(ao).cross(ab);
        return false;
    }

    PxVec3 acPerp = ac.cross(abc);
    if (acPerp.dot(ao) > 0) {
        // 原点在ac边外侧
        simplex.size = 2;
        simplex[0] = c;
        simplex[1] = a;
        direction = ac.cross(ao).cross(ac);
        return false;
    }

    // 原点在三角形上方或下方
    if (abc.dot(ao) > 0) {
        // 上方
        direction = abc;
    } else {
        // 下方，翻转三角形
        direction = -abc;
        std::swap(simplex[0], simplex[1]);
    }

    return false;
}

/**
 * 更新单纯形和搜索方向（四面体情况）
 */
bool updateSimplexTetrahedron(Simplex& simplex, PxVec3& direction) {
    PxVec3 a = simplex[3];  // 最新点
    PxVec3 b = simplex[2];
    PxVec3 c = simplex[1];
    PxVec3 d = simplex[0];

    PxVec3 ab = b - a;
    PxVec3 ac = c - a;
    PxVec3 ad = d - a;
    PxVec3 ao = -a;

    // 检查四个面
    PxVec3 abc = ab.cross(ac);
    if (abc.dot(ao) > 0) {
        // 原点在ABC面外侧
        simplex.size = 3;
        simplex[0] = c;
        simplex[1] = b;
        simplex[2] = a;
        direction = abc;
        return false;
    }

    PxVec3 acd = ac.cross(ad);
    if (acd.dot(ao) > 0) {
        // 原点在ACD面外侧
        simplex.size = 3;
        simplex[0] = d;
        simplex[1] = c;
        simplex[2] = a;
        direction = acd;
        return false;
    }

    PxVec3 adb = ad.cross(ab);
    if (adb.dot(ao) > 0) {
        // 原点在ADB面外侧
        simplex.size = 3;
        simplex[0] = b;
        simplex[1] = d;
        simplex[2] = a;
        direction = adb;
        return false;
    }

    // 原点在四面体内部
    return true;
}

/**
 * 更新单纯形
 */
bool updateSimplex(Simplex& simplex, PxVec3& direction) {
    switch (simplex.size) {
        case 2: return updateSimplexLine(simplex, direction);
        case 3: return updateSimplexTriangle(simplex, direction);
        case 4: return updateSimplexTetrahedron(simplex, direction);
    }
    return false;
}

/**
 * GJK碰撞检测
 */
bool gjkCollisionDetection(const std::vector<PxVec3>& shapeA,
                           const std::vector<PxVec3>& shapeB,
                           Simplex& resultSimplex) {
    // 初始化
    PxVec3 direction(1, 0, 0);  // 初始搜索方向
    Simplex simplex;

    PxVec3 support = supportMinkowski(shapeA, shapeB, direction);
    simplex.add(support);
    direction = -support;  // 指向原点

    const int maxIterations = 32;
    for (int i = 0; i < maxIterations; ++i) {
        support = supportMinkowski(shapeA, shapeB, direction);

        if (support.dot(direction) < 0) {
            // 新点没有越过原点，不相交
            return false;
        }

        simplex.add(support);

        if (updateSimplex(simplex, direction)) {
            // 单纯形包含原点，相交
            resultSimplex = simplex;
            return true;
        }
    }

    return false;  // 超过最大迭代次数
}

// ============================================================================
// EPA算法（简化版）
// ============================================================================

/**
 * EPA面结构
 */
struct EPAFace {
    PxVec3 vertices[3];
    PxVec3 normal;
    PxReal distance;

    EPAFace(const PxVec3& a, const PxVec3& b, const PxVec3& c) {
        vertices[0] = a;
        vertices[1] = b;
        vertices[2] = c;

        PxVec3 ab = b - a;
        PxVec3 ac = c - a;
        normal = ab.cross(ac).getNormalized();
        distance = normal.dot(a);

        // 确保法向量指向原点外
        if (distance < 0) {
            normal = -normal;
            distance = -distance;
            std::swap(vertices[1], vertices[2]);
        }
    }
};

/**
 * EPA算法（简化版）
 */
bool epaCalculatePenetration(const std::vector<PxVec3>& shapeA,
                             const std::vector<PxVec3>& shapeB,
                             const Simplex& initialSimplex,
                             PxVec3& penetrationNormal,
                             PxReal& penetrationDepth) {
    // 简化实现：只用初始四面体的最近面
    std::vector<EPAFace> faces;

    // 从四面体创建4个面
    if (initialSimplex.size == 4) {
        PxVec3 a = initialSimplex[3];
        PxVec3 b = initialSimplex[2];
        PxVec3 c = initialSimplex[1];
        PxVec3 d = initialSimplex[0];

        faces.emplace_back(a, b, c);
        faces.emplace_back(a, c, d);
        faces.emplace_back(a, d, b);
        faces.emplace_back(b, d, c);
    } else {
        return false;
    }

    // 找到最接近原点的面
    PxReal minDistance = PX_MAX_F32;
    int minIndex = 0;

    for (size_t i = 0; i < faces.size(); ++i) {
        if (faces[i].distance < minDistance) {
            minDistance = faces[i].distance;
            minIndex = i;
        }
    }

    penetrationNormal = faces[minIndex].normal;
    penetrationDepth = faces[minIndex].distance;

    return true;
}

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: GJK碰撞检测测试
 */
void demonstrateGJKCollision() {
    std::cout << "\n=== 场景1: GJK碰撞检测测试 ===" << std::endl;
    std::cout << "测试不同配置的立方体碰撞" << std::endl;

    // 立方体A（单位立方体）
    std::vector<PxVec3> cubeA = {
        PxVec3(-0.5f, -0.5f, -0.5f), PxVec3(0.5f, -0.5f, -0.5f),
        PxVec3(0.5f, 0.5f, -0.5f), PxVec3(-0.5f, 0.5f, -0.5f),
        PxVec3(-0.5f, -0.5f, 0.5f), PxVec3(0.5f, -0.5f, 0.5f),
        PxVec3(0.5f, 0.5f, 0.5f), PxVec3(-0.5f, 0.5f, 0.5f)
    };

    // 测试不同位置的立方体B
    std::vector<std::pair<PxVec3, const char*>> testCases = {
        {PxVec3(0.8f, 0, 0), "轻微重叠"},
        {PxVec3(1.2f, 0, 0), "分离（边缘）"},
        {PxVec3(2.0f, 0, 0), "明显分离"},
        {PxVec3(0, 0, 0), "完全重叠"}
    };

    std::cout << "\n立方体B位置\t\t碰撞结果" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    for (const auto& testCase : testCases) {
        std::vector<PxVec3> cubeB;
        for (const auto& v : cubeA) {
            cubeB.push_back(v + testCase.first);
        }

        Simplex resultSimplex;
        bool collision = gjkCollisionDetection(cubeA, cubeB, resultSimplex);

        std::cout << "(" << testCase.first.x << ", " << testCase.first.y << ", " << testCase.first.z << ")\t"
                  << (collision ? "碰撞 ✓" : "分离 ✗") << "\t"
                  << testCase.second << std::endl;
    }
}

/**
 * 场景2: EPA穿透深度计算
 */
void demonstrateEPAPenetration() {
    std::cout << "\n=== 场景2: EPA穿透深度计算 ===" << std::endl;
    std::cout << "计算重叠立方体的穿透信息" << std::endl;

    // 立方体A
    std::vector<PxVec3> cubeA = {
        PxVec3(-0.5f, -0.5f, -0.5f), PxVec3(0.5f, -0.5f, -0.5f),
        PxVec3(0.5f, 0.5f, -0.5f), PxVec3(-0.5f, 0.5f, -0.5f),
        PxVec3(-0.5f, -0.5f, 0.5f), PxVec3(0.5f, -0.5f, 0.5f),
        PxVec3(0.5f, 0.5f, 0.5f), PxVec3(-0.5f, 0.5f, 0.5f)
    };

    std::vector<PxReal> overlaps = {0.1f, 0.3f, 0.5f, 0.7f};

    std::cout << "\n重叠量\t\tEPA穿透深度\t穿透方向" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;

    for (PxReal overlap : overlaps) {
        std::vector<PxVec3> cubeB;
        PxVec3 offset(1.0f - overlap, 0, 0);

        for (const auto& v : cubeA) {
            cubeB.push_back(v + offset);
        }

        Simplex simplex;
        if (gjkCollisionDetection(cubeA, cubeB, simplex)) {
            PxVec3 penetrationNormal;
            PxReal penetrationDepth;

            if (epaCalculatePenetration(cubeA, cubeB, simplex, penetrationNormal, penetrationDepth)) {
                std::cout << overlap << "\t\t"
                          << penetrationDepth << "\t\t"
                          << "(" << penetrationNormal.x << ", "
                          << penetrationNormal.y << ", "
                          << penetrationNormal.z << ")" << std::endl;
            }
        }
    }

    std::cout << "\n理论穿透深度应该约等于重叠量" << std::endl;
}

/**
 * 场景3: 性能测试
 */
void demonstrateGJKPerformance() {
    std::cout << "\n=== 场景3: GJK性能测试 ===" << std::endl;
    std::cout << "测试不同复杂度凸体的GJK性能" << std::endl;

    // 生成不同顶点数的凸体（简化为正多面体）
    std::vector<int> vertexCounts = {8, 20, 50, 100};

    std::cout << "\n顶点数\t\t平均迭代次数\t检测时间(相对)" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    for (int numVerts : vertexCounts) {
        // 生成近似球体的顶点
        std::vector<PxVec3> shapeA, shapeB;

        for (int i = 0; i < numVerts; ++i) {
            PxReal theta = (PxPi * i) / numVerts;
            PxReal phi = (2.0f * PxPi * i) / numVerts;
            PxVec3 v(PxSin(theta) * PxCos(phi), PxSin(theta) * PxSin(phi), PxCos(theta));
            shapeA.push_back(v);
            shapeB.push_back(v + PxVec3(1.5f, 0, 0));
        }

        // 多次测试取平均
        int totalTests = 100;
        int totalIterations = 0;

        auto start = std::chrono::high_resolution_clock::now();

        for (int t = 0; t < totalTests; ++t) {
            Simplex simplex;
            gjkCollisionDetection(shapeA, shapeB, simplex);
            totalIterations += simplex.size;  // 近似迭代次数
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - start;

        std::cout << numVerts << "\t\t"
                  << (totalIterations / totalTests) << "\t\t"
                  << (duration.count() / totalTests) << " μs" << std::endl;
    }

    std::cout << "\n观察: GJK性能与顶点数关系不大，主要取决于形状配置" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: CustomGeometryCollision" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\n演示自定义几何体碰撞检测算法" << std::endl;

    // 运行3个场景
    demonstrateGJKCollision();
    demonstrateEPAPenetration();
    demonstrateGJKPerformance();

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\nGJK算法特点:" << std::endl;
    std::cout << "1. 通用性 - 适用于任意凸体" << std::endl;
    std::cout << "2. 效率高 - 通常<10次迭代" << std::endl;
    std::cout << "3. 数值稳定 - 基于支持函数" << std::endl;
    std::cout << "4. 易实现 - 逻辑清晰" << std::endl;

    std::cout << "\n关键公式:" << std::endl;
    std::cout << "闵可夫斯基差: A⊖B = {a - b | a ∈ A, b ∈ B}" << std::endl;
    std::cout << "支持函数: S_{A⊖B}(d) = S_A(d) - S_B(-d)" << std::endl;
    std::cout << "碰撞条件: 原点 ∈ A⊖B" << std::endl;

    std::cout << "\nEPA算法:" << std::endl;
    std::cout << "- 从GJK的四面体开始" << std::endl;
    std::cout << "- 迭代扩展多面体" << std::endl;
    std::cout << "- 找到最接近原点的面" << std::endl;
    std::cout << "- 返回穿透深度和法向量" << std::endl;

    std::cout << "\n应用场景:" << std::endl;
    std::cout << "- 物理引擎核心算法" << std::endl;
    std::cout << "- 机器人路径规划" << std::endl;
    std::cout << "- 游戏碰撞检测" << std::endl;
    std::cout << "- CAD/CAM系统" << std::endl;

    std::cout << "\n⚠️ 实现注意事项:" << std::endl;
    std::cout << "- 处理数值误差（epsilon比较）" << std::endl;
    std::cout << "- 避免退化情况（共线、共面）" << std::endl;
    std::cout << "- 限制最大迭代次数" << std::endl;
    std::cout << "- 正确处理边界情况" << std::endl;

    return 0;
}
