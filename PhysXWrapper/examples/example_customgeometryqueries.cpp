/**
 * PhysX Snippet: CustomGeometryQueries
 *
 * 演示自定义几何体的查询操作（Custom Geometry Queries）
 *
 * 核心功能:
 * 1. 射线投射（Raycast）
 * 2. 扫描检测（Sweep）
 * 3. 重叠检测（Overlap）
 * 4. 最近点查询（Closest Point）
 *
 * 物理背景:
 *
 * 几何查询（Geometry Queries）:
 * 在物理仿真、游戏、机器人等领域中，需要快速查询几何信息：
 * - 射线是否击中物体？
 * - 物体沿路径运动会碰撞吗？
 * - 两个物体是否重叠？
 * - 最近的物体是什么？
 *
 * 射线投射（Raycast）:
 *
 * 问题：给定射线 R(t) = origin + t × direction，
 * 求射线与凸体的第一个交点。
 *
 * 对于椭球体:
 * 将射线方程代入椭球方程:
 * ((o_x + t×d_x)/a)² + ((o_y + t×d_y)/b)² + ((o_z + t×d_z)/c)² = 1
 *
 * 整理成二次方程 At² + Bt + C = 0:
 * A = (d_x/a)² + (d_y/b)² + (d_z/c)²
 * B = 2[(o_x×d_x)/a² + (o_y×d_y)/b² + (o_z×d_z)/c²]
 * C = (o_x/a)² + (o_y/b)² + (o_z/c)² - 1
 *
 * 判别式: Δ = B² - 4AC
 * - Δ < 0: 无交点
 * - Δ = 0: 相切，一个交点
 * - Δ > 0: 两个交点，取较小的t
 *
 * t = (-B - √Δ) / (2A)  (进入点)
 * t = (-B + √Δ) / (2A)  (离开点)
 *
 * 扫描检测（Sweep Test）:
 *
 * 问题：物体A沿方向d移动，是否与静止物体B碰撞？
 * 求第一个碰撞时间t和碰撞信息。
 *
 * 连续碰撞检测（Continuous Collision Detection, CCD）:
 * - 保守推进法（Conservative Advancement）
 * - 时间of影响法（Time of Impact）
 * - 根查找法（Root Finding）
 *
 * 保守推进:
 * 1. 初始化 t = 0
 * 2. 计算当前配置下的最小距离 d
 * 3. 安全推进 Δt = d / ||v_rel||
 * 4. t += Δt
 * 5. 如果 t >= t_max，无碰撞
 * 6. 如果 d < ε，碰撞，返回t
 * 7. 回到步骤2
 *
 * 重叠检测（Overlap Test）:
 *
 * 快速检测两个物体是否重叠：
 * 1. AABB测试（粗检测）
 * 2. GJK/SAT（精确检测）
 *
 * AABB重叠:
 * [a_min, a_max] ∩ [b_min, b_max] ≠ ∅
 * ⟺ (a_min ≤ b_max) ∧ (b_min ≤ a_max)
 *
 * 最近点查询（Closest Point Query）:
 *
 * 问题：给定点p和凸体C，求C上距离p最近的点。
 *
 * 对于椭球体，使用数值优化:
 * minimize ||q - p||²
 * subject to (q_x/a)² + (q_y/b)² + (q_z/c)² = 1
 *
 * 拉格朗日乘子法:
 * ∇f = λ∇g
 * 其中 f(q) = ||q - p||², g(q) = (q_x/a)² + (q_y/b)² + (q_z/c)² - 1
 *
 * 迭代求解:
 * q^(k+1) = q^(k) - α × [∇f(q^(k)) - λ∇g(q^(k))]
 *
 * 距离场（Distance Field）:
 *
 * 有向距离场 SDF(p):
 * - SDF(p) > 0: p在物体外，值为到表面的距离
 * - SDF(p) = 0: p在表面上
 * - SDF(p) < 0: p在物体内，值为到表面的距离（负）
 *
 * 椭球体SDF:
 * SDF(p) = ||p|| × (1 - 1/f(p))
 * 其中 f(p) = √[(p_x/a)² + (p_y/b)² + (p_z/c)²]
 *
 * 应用场景:
 * 1. 碰撞检测
 * 2. 路径规划
 * 3. 视线检测
 * 4. AI感知系统
 *
 * 注意:
 * ⚠️ 精度vs性能权衡
 * ⚠️ 退化情况处理
 * ⚠️ 数值稳定性
 */

#include <PxPhysicsAPI.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace physx;

// ============================================================================
// 椭球体查询实现
// ============================================================================

/**
 * 椭球体类
 */
class Ellipsoid {
public:
    PxVec3 center;
    PxVec3 radii;  // (a, b, c)

    Ellipsoid(const PxVec3& c, const PxVec3& r) : center(c), radii(r) {}

    /**
     * 射线投射
     */
    bool raycast(const PxVec3& origin, const PxVec3& direction, PxReal maxDistance,
                 PxReal& hitDistance, PxVec3& hitNormal) const {
        // 转换到椭球体局部坐标
        PxVec3 localOrigin = origin - center;

        // 归一化坐标（缩放到单位球空间）
        PxVec3 o(localOrigin.x / radii.x, localOrigin.y / radii.y, localOrigin.z / radii.z);
        PxVec3 d(direction.x / radii.x, direction.y / radii.y, direction.z / radii.z);

        // 二次方程系数
        PxReal A = d.dot(d);
        PxReal B = 2.0f * o.dot(d);
        PxReal C = o.dot(o) - 1.0f;

        // 判别式
        PxReal discriminant = B * B - 4.0f * A * C;

        if (discriminant < 0) {
            return false;  // 无交点
        }

        // 计算t
        PxReal sqrtDisc = PxSqrt(discriminant);
        PxReal t1 = (-B - sqrtDisc) / (2.0f * A);
        PxReal t2 = (-B + sqrtDisc) / (2.0f * A);

        // 选择第一个正t
        PxReal t = t1 > 0 ? t1 : t2;

        if (t < 0 || t > maxDistance) {
            return false;
        }

        // 计算交点
        PxVec3 hitPoint = origin + direction * t;
        PxVec3 localHit = hitPoint - center;

        // 计算法向量
        PxVec3 grad(2.0f * localHit.x / (radii.x * radii.x),
                    2.0f * localHit.y / (radii.y * radii.y),
                    2.0f * localHit.z / (radii.z * radii.z));

        hitDistance = t;
        hitNormal = grad.getNormalized();

        return true;
    }

    /**
     * 最近点查询
     */
    PxVec3 closestPoint(const PxVec3& point) const {
        PxVec3 localPoint = point - center;

        // 归一化坐标
        PxVec3 p(localPoint.x / radii.x, localPoint.y / radii.y, localPoint.z / radii.z);

        // 如果点在椭球内，投影到表面
        PxReal dist = p.magnitude();

        if (dist < 1e-6f) {
            // 点在中心，返回任意表面点
            return center + PxVec3(radii.x, 0, 0);
        }

        // 沿径向投影到表面
        PxVec3 direction = p / dist;
        PxVec3 surfacePoint(direction.x * radii.x, direction.y * radii.y, direction.z * radii.z);

        return center + surfacePoint;
    }

    /**
     * 重叠检测（与球体）
     */
    bool overlapSphere(const PxVec3& sphereCenter, PxReal sphereRadius) const {
        PxVec3 closest = closestPoint(sphereCenter);
        PxReal distance = (closest - sphereCenter).magnitude();
        return distance < sphereRadius;
    }

    /**
     * 距离查询
     */
    PxReal distanceToPoint(const PxVec3& point) const {
        PxVec3 closest = closestPoint(point);
        PxReal distance = (point - closest).magnitude();

        // 判断点是否在内部
        PxVec3 localPoint = point - center;
        PxVec3 normalized(localPoint.x / radii.x, localPoint.y / radii.y, localPoint.z / radii.z);

        if (normalized.magnitude() < 1.0f) {
            return -distance;  // 内部为负
        }

        return distance;
    }

    /**
     * AABB计算
     */
    void getAABB(PxVec3& min, PxVec3& max) const {
        min = center - radii;
        max = center + radii;
    }
};

// ============================================================================
// 扫描检测实现
// ============================================================================

/**
 * 球体扫描（移动球体与静止椭球体）
 */
bool sphereSweepEllipsoid(const PxVec3& sphereStart, PxReal sphereRadius,
                          const PxVec3& sweepDirection, PxReal sweepDistance,
                          const Ellipsoid& ellipsoid,
                          PxReal& hitTime, PxVec3& hitNormal) {
    // 简化实现：保守推进法
    const int maxSteps = 32;
    const PxReal epsilon = 0.01f;

    PxReal t = 0.0f;
    PxReal step = sweepDistance / maxSteps;

    for (int i = 0; i < maxSteps; ++i) {
        PxVec3 currentPos = sphereStart + sweepDirection * t;

        // 检查当前位置是否重叠
        if (ellipsoid.overlapSphere(currentPos, sphereRadius)) {
            // 找到碰撞，计算精确时间和法向量
            hitTime = t;

            // 法向量：从椭球体中心指向球心
            PxVec3 closest = ellipsoid.closestPoint(currentPos);
            hitNormal = (currentPos - closest).getNormalized();

            return true;
        }

        t += step;

        if (t >= sweepDistance) {
            break;
        }
    }

    return false;
}

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 射线投射测试
 */
void demonstrateRaycastQueries() {
    std::cout << "\n=== 场景1: 射线投射测试 ===" << std::endl;
    std::cout << "测试不同角度的射线击中椭球体" << std::endl;

    Ellipsoid ellipsoid(PxVec3(0, 0, 0), PxVec3(2.0f, 1.5f, 1.0f));

    std::vector<std::pair<PxVec3, PxVec3>> rays = {
        {PxVec3(-5, 0, 0), PxVec3(1, 0, 0)},      // 沿X轴
        {PxVec3(0, -5, 0), PxVec3(0, 1, 0)},      // 沿Y轴
        {PxVec3(0, 0, -5), PxVec3(0, 0, 1)},      // 沿Z轴
        {PxVec3(-5, -5, 0), PxVec3(1, 1, 0).getNormalized()},  // 对角线
        {PxVec3(-5, 0, -5), PxVec3(1, 0, 1).getNormalized()}   // 另一对角线
    };

    std::cout << "\n射线起点\t\t方向\t\t\t击中距离\t法向量" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;

    for (const auto& ray : rays) {
        PxReal hitDistance;
        PxVec3 hitNormal;

        bool hit = ellipsoid.raycast(ray.first, ray.second, 10.0f, hitDistance, hitNormal);

        std::cout << "(" << ray.first.x << ", " << ray.first.y << ", " << ray.first.z << ")\t";
        std::cout << "(" << ray.second.x << ", " << ray.second.y << ", " << ray.second.z << ")\t";

        if (hit) {
            std::cout << hitDistance << "\t\t";
            std::cout << "(" << hitNormal.x << ", " << hitNormal.y << ", " << hitNormal.z << ")";
        } else {
            std::cout << "未击中";
        }
        std::cout << std::endl;
    }
}

/**
 * 场景2: 最近点查询
 */
void demonstrateClosestPointQueries() {
    std::cout << "\n=== 场景2: 最近点查询 ===" << std::endl;
    std::cout << "查询不同点到椭球体的最近点和距离" << std::endl;

    Ellipsoid ellipsoid(PxVec3(0, 0, 0), PxVec3(2.0f, 1.5f, 1.0f));

    std::vector<PxVec3> testPoints = {
        PxVec3(3, 0, 0),      // 外部-X轴
        PxVec3(0, 3, 0),      // 外部-Y轴
        PxVec3(0, 0, 3),      // 外部-Z轴
        PxVec3(0.5f, 0, 0),   // 内部
        PxVec3(3, 3, 0)       // 外部-对角
    };

    std::cout << "\n查询点\t\t\t最近点\t\t\t\t距离" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;

    for (const auto& point : testPoints) {
        PxVec3 closest = ellipsoid.closestPoint(point);
        PxReal distance = ellipsoid.distanceToPoint(point);

        std::cout << "(" << point.x << ", " << point.y << ", " << point.z << ")\t\t";
        std::cout << "(" << closest.x << ", " << closest.y << ", " << closest.z << ")\t\t";
        std::cout << distance << std::endl;
    }

    std::cout << "\n注意: 负距离表示点在椭球体内部" << std::endl;
}

/**
 * 场景3: 重叠检测
 */
void demonstrateOverlapQueries() {
    std::cout << "\n=== 场景3: 重叠检测 ===" << std::endl;
    std::cout << "测试球体与椭球体的重叠" << std::endl;

    Ellipsoid ellipsoid(PxVec3(0, 0, 0), PxVec3(2.0f, 1.5f, 1.0f));

    std::vector<std::pair<PxVec3, PxReal>> spheres = {
        {PxVec3(0, 0, 0), 0.5f},      // 中心小球
        {PxVec3(2, 0, 0), 0.5f},      // 边缘重叠
        {PxVec3(3, 0, 0), 0.5f},      // 外部相切
        {PxVec3(4, 0, 0), 0.5f},      // 外部分离
        {PxVec3(1, 1, 0), 1.0f}       // 大球重叠
    };

    std::cout << "\n球体中心\t\t半径\t\t重叠结果" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;

    for (const auto& sphere : spheres) {
        bool overlaps = ellipsoid.overlapSphere(sphere.first, sphere.second);

        std::cout << "(" << sphere.first.x << ", " << sphere.first.y << ", " << sphere.first.z << ")\t\t";
        std::cout << sphere.second << "\t\t";
        std::cout << (overlaps ? "重叠 ✓" : "分离 ✗") << std::endl;
    }
}

/**
 * 场景4: 扫描检测
 */
void demonstrateSweepQueries() {
    std::cout << "\n=== 场景4: 扫描检测 ===" << std::endl;
    std::cout << "测试移动球体与静止椭球体的首次碰撞" << std::endl;

    Ellipsoid ellipsoid(PxVec3(0, 0, 0), PxVec3(2.0f, 1.5f, 1.0f));

    std::vector<std::tuple<PxVec3, PxVec3, PxReal>> sweepTests = {
        {PxVec3(-5, 0, 0), PxVec3(1, 0, 0), 0.5f},     // 沿X轴
        {PxVec3(-5, 2, 0), PxVec3(1, 0, 0), 0.3f},     // 上方掠过
        {PxVec3(-5, 0, 2), PxVec3(1, 0, 0), 0.5f},     // 侧方
        {PxVec3(-5, -5, 0), PxVec3(1, 1, 0).getNormalized(), 0.5f}  // 对角线
    };

    std::cout << "\n起点\t\t\t方向\t\t\t半径\t碰撞时间" << std::endl;
    std::cout << "------------------------------------------------------------------------" << std::endl;

    for (const auto& test : sweepTests) {
        PxVec3 start = std::get<0>(test);
        PxVec3 dir = std::get<1>(test);
        PxReal radius = std::get<2>(test);

        PxReal hitTime;
        PxVec3 hitNormal;

        bool hit = sphereSweepEllipsoid(start, radius, dir, 10.0f, ellipsoid, hitTime, hitNormal);

        std::cout << "(" << start.x << ", " << start.y << ", " << start.z << ")\t";
        std::cout << "(" << dir.x << ", " << dir.y << ", " << dir.z << ")\t";
        std::cout << radius << "\t";

        if (hit) {
            std::cout << hitTime;
        } else {
            std::cout << "未碰撞";
        }
        std::cout << std::endl;
    }
}

/**
 * 场景5: 批量查询性能测试
 */
void demonstrateQueryPerformance() {
    std::cout << "\n=== 场景5: 批量查询性能测试 ===" << std::endl;
    std::cout << "测试不同查询类型的性能" << std::endl;

    Ellipsoid ellipsoid(PxVec3(0, 0, 0), PxVec3(2.0f, 1.5f, 1.0f));

    const int numTests = 10000;

    // 射线投射性能
    auto start = std::chrono::high_resolution_clock::now();
    int raycastHits = 0;

    for (int i = 0; i < numTests; ++i) {
        PxVec3 origin(-5 + (i % 10) * 0.1f, (i % 20) * 0.1f - 1.0f, 0);
        PxVec3 dir(1, 0, 0);
        PxReal hitDist;
        PxVec3 hitNormal;

        if (ellipsoid.raycast(origin, dir, 10.0f, hitDist, hitNormal)) {
            raycastHits++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> raycastTime = end - start;

    // 最近点查询性能
    start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numTests; ++i) {
        PxVec3 point((i % 100) * 0.05f - 2.5f, (i % 50) * 0.05f - 1.25f, 0);
        ellipsoid.closestPoint(point);
    }

    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> closestPointTime = end - start;

    // 重叠检测性能
    start = std::chrono::high_resolution_clock::now();
    int overlapCount = 0;

    for (int i = 0; i < numTests; ++i) {
        PxVec3 center((i % 100) * 0.05f - 2.5f, (i % 50) * 0.05f - 1.25f, 0);
        if (ellipsoid.overlapSphere(center, 0.5f)) {
            overlapCount++;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> overlapTime = end - start;

    std::cout << "\n查询类型\t\t测试次数\t总时间(ms)\t平均时间(μs)" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << "射线投射\t\t" << numTests << "\t\t" << raycastTime.count() << "\t\t"
              << (raycastTime.count() * 1000.0 / numTests) << std::endl;
    std::cout << "最近点查询\t\t" << numTests << "\t\t" << closestPointTime.count() << "\t\t"
              << (closestPointTime.count() * 1000.0 / numTests) << std::endl;
    std::cout << "重叠检测\t\t" << numTests << "\t\t" << overlapTime.count() << "\t\t"
              << (overlapTime.count() * 1000.0 / numTests) << std::endl;

    std::cout << "\n结果统计: " << raycastHits << "次射线击中, "
              << overlapCount << "次重叠" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: CustomGeometryQueries" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "\n演示自定义几何体的各种查询操作" << std::endl;

    // 运行5个场景
    demonstrateRaycastQueries();
    demonstrateClosestPointQueries();
    demonstrateOverlapQueries();
    demonstrateSweepQueries();
    demonstrateQueryPerformance();

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\n查询类型对比:" << std::endl;
    std::cout << "┌────────────────┬──────────┬──────────┬──────────┐" << std::endl;
    std::cout << "│ 查询类型       │ 复杂度   │ 精度     │ 用途     │" << std::endl;
    std::cout << "├────────────────┼──────────┼──────────┼──────────┤" << std::endl;
    std::cout << "│ 射线投射       │ O(1)     │ 高       │ 视线检测 │" << std::endl;
    std::cout << "│ 最近点查询     │ O(1)     │ 中       │ 距离计算 │" << std::endl;
    std::cout << "│ 重叠检测       │ O(1)     │ 高       │ 碰撞粗检 │" << std::endl;
    std::cout << "│ 扫描检测       │ O(n)     │ 中       │ CCD      │" << std::endl;
    std::cout << "└────────────────┴──────────┴──────────┴──────────┘" << std::endl;

    std::cout << "\n关键公式:" << std::endl;
    std::cout << "射线方程: R(t) = origin + t × direction" << std::endl;
    std::cout << "椭球方程: (x/a)² + (y/b)² + (z/c)² = 1" << std::endl;
    std::cout << "二次方程: At² + Bt + C = 0, t = (-B ± √Δ) / (2A)" << std::endl;
    std::cout << "距离场: SDF(p) = ||p|| × (1 - 1/f(p))" << std::endl;

    std::cout << "\n应用场景:" << std::endl;
    std::cout << "1. 游戏 - 武器射线、视线检测" << std::endl;
    std::cout << "2. 机器人 - 路径规划、碰撞避免" << std::endl;
    std::cout << "3. 物理引擎 - CCD、查询系统" << std::endl;
    std::cout << "4. AI - 感知系统、决策" << std::endl;

    std::cout << "\n优化建议:" << std::endl;
    std::cout << "- 使用空间划分（BVH、八叉树）" << std::endl;
    std::cout << "- AABB粗检测过滤" << std::endl;
    std::cout << "- 缓存计算结果" << std::endl;
    std::cout << "- 早期退出策略" << std::endl;

    return 0;
}
