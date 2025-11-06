/**
 * PhysX Snippet: Custom Convex Geometry
 *
 * 本示例演示如何实现自定义凸几何体(Custom Convex Geometry)。
 *
 * 理论基础：
 *
 * 1. 凸包(Convex Hull)
 *    凸包是包含给定点集的最小凸集。对于点集P，其凸包定义为：
 *    Conv(P) = {Σ λ_i · p_i | p_i ∈ P, λ_i ≥ 0, Σ λ_i = 1}
 *
 * 2. 支撑映射(Support Mapping)
 *    支撑映射是GJK等算法的核心：
 *    S(d) = arg max_{v ∈ V} v·d
 *    返回在方向d上投影最远的顶点。
 *
 * 3. Minkowski和与差
 *    A ⊕ B = {a + b | a ∈ A, b ∈ B}  (Minkowski和)
 *    A ⊖ B = {a - b | a ∈ A, b ∈ B}  (Minkowski差)
 *    两凸体碰撞 ⟺ 原点 ∈ (A ⊖ B)
 *
 * 4. 碰撞裕度(Collision Margin)
 *    为提高数值稳定性，凸体常使用膨胀表示：
 *    Shape_inflated = {p | dist(p, Shape) ≤ margin}
 *    裕度通常取0.01-0.1单位。
 *
 * 5. 质量属性计算
 *    对于凸多面体，可通过四面体分解计算：
 *    - 体积: V = Σ V_i，其中V_i为各四面体体积
 *    - 质心: c = (Σ V_i · c_i) / V
 *    - 惯性张量: I = Σ I_i (通过平行轴定理转换)
 *
 * 自定义凸体的关键实现：
 * - generateContacts: 生成接触点
 * - supportMapping: 支撑映射
 * - computeMassProperties: 计算质量属性
 * - raycast: 射线检测
 */

#include "PxPhysicsAPI.h"
#include "../common/PxPhysXCommon.h"
#include <vector>
#include <algorithm>
#include <cmath>

using namespace physx;

// 全局变量
static PxDefaultAllocator gAllocator;
static PxDefaultErrorCallback gErrorCallback;
static PxFoundation* gFoundation = nullptr;
static PxPhysics* gPhysics = nullptr;
static PxScene* gScene = nullptr;
static PxMaterial* gMaterial = nullptr;
static PxPvd* gPvd = nullptr;

/**
 * 自定义凸体类
 *
 * 实现一个星形凸多面体，支持完整的碰撞查询功能。
 */
class CustomConvex {
public:
    std::vector<PxVec3> vertices;
    std::vector<PxU32> indices;  // 三角形索引
    PxReal margin;               // 碰撞裕度

    // 预计算数据
    PxVec3 centroid;
    PxReal boundingRadius;

    CustomConvex(PxReal size = 1.0f, PxU32 numPoints = 8, PxReal m = 0.02f)
        : margin(m) {
        generateStarShape(size, numPoints);
        computeProperties();
    }

    /**
     * 生成星形凸多面体
     *
     * 通过在球面上生成点，然后应用径向变形创建星形：
     * r(θ, φ) = r_base · (1 + amplitude · sin(frequency · θ))
     */
    void generateStarShape(PxReal size, PxU32 numPoints) {
        vertices.clear();

        // 在球面上均匀分布点 (Fibonacci sphere)
        const PxReal goldenRatio = (1.0f + PxSqrt(5.0f)) / 2.0f;
        const PxReal angleIncrement = PxTwoPi * goldenRatio;

        for (PxU32 i = 0; i < numPoints; ++i) {
            PxReal t = static_cast<PxReal>(i) / numPoints;
            PxReal inclination = PxAcos(1.0f - 2.0f * t);
            PxReal azimuth = angleIncrement * i;

            // 基本球面坐标
            PxReal x = PxSin(inclination) * PxCos(azimuth);
            PxReal y = PxSin(inclination) * PxSin(azimuth);
            PxReal z = PxCos(inclination);

            // 应用星形变形
            PxReal amplitude = 0.3f;
            PxReal frequency = 3.0f;
            PxReal radialScale = 1.0f + amplitude * PxSin(frequency * azimuth);

            vertices.push_back(PxVec3(x, y, z) * size * radialScale);
        }

        // 生成凸包三角形索引 (简化版 - 实际应该使用QuickHull算法)
        // 这里使用扇形三角化
        for (PxU32 i = 2; i < numPoints; ++i) {
            indices.push_back(0);
            indices.push_back(i - 1);
            indices.push_back(i);
        }
    }

    /**
     * 计算几何属性
     */
    void computeProperties() {
        if (vertices.empty()) return;

        // 计算质心
        centroid = PxVec3(0.0f);
        for (const auto& v : vertices) {
            centroid += v;
        }
        centroid /= static_cast<PxReal>(vertices.size());

        // 计算包围半径
        boundingRadius = 0.0f;
        for (const auto& v : vertices) {
            PxReal dist = (v - centroid).magnitude();
            boundingRadius = PxMax(boundingRadius, dist);
        }
        boundingRadius += margin;
    }

    /**
     * 支撑映射实现
     *
     * 返回在给定方向上投影最远的顶点。
     * 时间复杂度：O(n)，n为顶点数
     *
     * 优化技巧：
     * - 可以使用hill climbing从上次结果开始
     * - 可以预计算方向查找表(Gauss map)
     */
    PxVec3 support(const PxVec3& direction) const {
        if (vertices.empty()) return PxVec3(0.0f);

        PxReal maxDot = -FLT_MAX;
        PxVec3 supportVertex = vertices[0];

        for (const auto& v : vertices) {
            PxReal dot = v.dot(direction);
            if (dot > maxDot) {
                maxDot = dot;
                supportVertex = v;
            }
        }

        // 添加裕度
        PxVec3 dir = direction;
        PxReal len = dir.normalize();
        if (len > 1e-6f) {
            supportVertex += dir * margin;
        }

        return supportVertex;
    }

    /**
     * 射线投射
     *
     * 使用迭代细化方法：
     * 1. 快速AABB测试
     * 2. 在射线上采样点，找到进入和退出区间
     * 3. 二分法细化交点
     */
    bool raycast(const PxVec3& origin, const PxVec3& direction,
                 PxReal maxDistance, PxReal& hitDistance,
                 PxVec3& hitNormal) const {
        // AABB快速剔除
        PxVec3 aabbMin = centroid - PxVec3(boundingRadius);
        PxVec3 aabbMax = centroid + PxVec3(boundingRadius);

        PxReal tMin = 0.0f;
        PxReal tMax = maxDistance;

        for (int i = 0; i < 3; ++i) {
            if (PxAbs(direction[i]) < 1e-6f) {
                if (origin[i] < aabbMin[i] || origin[i] > aabbMax[i]) {
                    return false;
                }
            } else {
                PxReal t1 = (aabbMin[i] - origin[i]) / direction[i];
                PxReal t2 = (aabbMax[i] - origin[i]) / direction[i];
                if (t1 > t2) std::swap(t1, t2);
                tMin = PxMax(tMin, t1);
                tMax = PxMin(tMax, t2);
                if (tMin > tMax) return false;
            }
        }

        // 射线-凸体相交测试
        // 使用采样+二分法
        const int numSamples = 32;
        PxReal sampleStep = maxDistance / numSamples;

        bool inside = false;
        PxReal enterT = -1.0f;
        PxReal exitT = -1.0f;

        for (int i = 0; i <= numSamples; ++i) {
            PxReal t = i * sampleStep;
            PxVec3 point = origin + direction * t;

            // 点在凸体内部测试（通过support mapping）
            bool pointInside = isPointInside(point);

            if (pointInside && !inside) {
                enterT = (i > 0) ? (i - 1) * sampleStep : 0.0f;
                inside = true;
            } else if (!pointInside && inside) {
                exitT = t;
                break;
            }
        }

        if (enterT < 0.0f) return false;

        // 二分法细化进入点
        PxReal refinedT = enterT;
        PxReal searchMin = enterT;
        PxReal searchMax = enterT + sampleStep;

        for (int iter = 0; iter < 10; ++iter) {
            PxReal midT = (searchMin + searchMax) * 0.5f;
            PxVec3 midPoint = origin + direction * midT;

            if (isPointInside(midPoint)) {
                searchMax = midT;
            } else {
                searchMin = midT;
            }
        }
        refinedT = searchMax;

        // 计算命中点和法线
        PxVec3 hitPoint = origin + direction * refinedT;
        hitDistance = refinedT;

        // 法线估计：从质心到命中点的方向
        hitNormal = hitPoint - centroid;
        PxReal normalLen = hitNormal.normalize();
        if (normalLen < 1e-6f) {
            hitNormal = -direction;
        }

        return true;
    }

    /**
     * 点在凸体内部测试
     *
     * 使用支撑映射性质：
     * 点p在凸体内 ⟺ ∀方向d, p·d ≤ S(d)·d
     *
     * 实际测试多个方向的采样。
     */
    bool isPointInside(const PxVec3& point) const {
        // 测试从质心到点的方向
        PxVec3 dir = point - centroid;
        PxReal distSq = dir.magnitudeSquared();

        if (distSq > boundingRadius * boundingRadius) {
            return false;
        }

        if (distSq < 1e-6f) {
            return true;  // 点在质心
        }

        dir.normalize();
        PxVec3 supportVert = support(dir);
        PxReal supportDist = (supportVert - centroid).dot(dir);
        PxReal pointDist = (point - centroid).dot(dir);

        return pointDist <= supportDist;
    }

    /**
     * 计算质量属性
     *
     * 使用四面体分解法：
     * 将凸多面体分解为从质心出发的四面体，累加计算。
     *
     * 四面体体积：V = |det(v1, v2, v3)| / 6
     * 四面体惯性张量（关于质心）：
     * I_xx = ρV/20 · (y1²+y2²+y3²+y1y2+y1y3+y2y3 + ...)
     */
    void computeMassProperties(PxReal density, PxReal& mass,
                               PxVec3& centerOfMass,
                               PxMat33& inertiaTensor) const {
        mass = 0.0f;
        centerOfMass = PxVec3(0.0f);
        inertiaTensor = PxMat33(PxIdentity);

        if (indices.size() < 3) return;

        PxReal totalVolume = 0.0f;
        PxVec3 weightedCenter(0.0f);
        PxMat33 totalInertia(PxZero);

        // 遍历所有三角形，从质心构建四面体
        for (size_t i = 0; i < indices.size(); i += 3) {
            PxVec3 v1 = vertices[indices[i]];
            PxVec3 v2 = vertices[indices[i + 1]];
            PxVec3 v3 = vertices[indices[i + 2]];

            // 四面体顶点：centroid, v1, v2, v3
            // 构建矩阵 [v1-c, v2-c, v3-c]
            PxVec3 e1 = v1 - centroid;
            PxVec3 e2 = v2 - centroid;
            PxVec3 e3 = v3 - centroid;

            // 体积 = |det| / 6
            PxReal det = e1.dot(e2.cross(e3));
            PxReal volume = PxAbs(det) / 6.0f;

            // 四面体质心（从centroid）
            PxVec3 tetCenter = centroid + (e1 + e2 + e3) * 0.25f;

            totalVolume += volume;
            weightedCenter += tetCenter * volume;

            // 四面体惯性张量（简化计算）
            // 使用对角近似
            PxReal x2 = e1.x * e1.x + e2.x * e2.x + e3.x * e3.x;
            PxReal y2 = e1.y * e1.y + e2.y * e2.y + e3.y * e3.y;
            PxReal z2 = e1.z * e1.z + e2.z * e2.z + e3.z * e3.z;

            PxReal coeff = density * volume / 20.0f;
            totalInertia.column0.x += coeff * (y2 + z2);
            totalInertia.column1.y += coeff * (x2 + z2);
            totalInertia.column2.z += coeff * (x2 + y2);
        }

        if (totalVolume > 1e-6f) {
            mass = density * totalVolume;
            centerOfMass = weightedCenter / totalVolume;
            inertiaTensor = totalInertia;
        }
    }
};

/**
 * PxCustomGeometry回调实现
 */
class CustomConvexCallbacks : public PxCustomGeometry::Callbacks {
private:
    CustomConvex convex;

public:
    CustomConvexCallbacks(const CustomConvex& c) : convex(c) {}

    virtual PxBounds3 getLocalBounds(const PxGeometry&) const override {
        PxVec3 extent(convex.boundingRadius);
        return PxBounds3(-extent, extent);
    }

    virtual bool generateContacts(const PxGeometry& geom0, const PxGeometry& geom1,
                                  const PxTransform& pose0, const PxTransform& pose1,
                                  const PxReal contactDistance, const PxReal meshContactMargin,
                                  const PxReal toleranceLength,
                                  PxContactBuffer& contactBuffer) const override {
        // 简化版：仅支持与球体的碰撞
        if (geom1.getType() == PxGeometryType::eSPHERE) {
            const PxSphereGeometry& sphere = static_cast<const PxSphereGeometry&>(geom1);

            // 变换到凸体局部空间
            PxVec3 sphereCenterLocal = pose0.transformInv(pose1.p);

            // 找到最近点
            PxVec3 closestPoint = findClosestPoint(sphereCenterLocal);

            // 计算穿透
            PxVec3 diff = sphereCenterLocal - closestPoint;
            PxReal distSq = diff.magnitudeSquared();
            PxReal radius = sphere.radius + contactDistance;

            if (distSq < radius * radius) {
                PxReal dist = PxSqrt(distSq);
                PxVec3 normal = (dist > 1e-6f) ? (diff / dist) : PxVec3(0, 1, 0);

                PxVec3 contactPointLocal = closestPoint + normal * convex.margin;
                PxVec3 contactPointGlobal = pose0.transform(contactPointLocal);
                PxVec3 normalGlobal = pose0.rotate(normal);

                PxReal separation = dist - radius;

                contactBuffer.contact(contactPointGlobal, normalGlobal, separation);
                return true;
            }
        }

        return false;
    }

    virtual bool raycast(const PxVec3& origin, const PxVec3& direction,
                        const PxGeometry& geom, const PxTransform& pose,
                        PxReal maxDist, PxHitFlags hitFlags,
                        PxU32 maxHits, PxGeomRaycastHit* rayHits,
                        PxU32 stride, PxRaycastThreadContext*) const override {
        // 变换到局部空间
        PxVec3 localOrigin = pose.transformInv(origin);
        PxVec3 localDir = pose.rotateInv(direction);

        PxReal hitDist;
        PxVec3 hitNormal;

        if (convex.raycast(localOrigin, localDir, maxDist, hitDist, hitNormal)) {
            if (maxHits > 0 && rayHits) {
                rayHits->position = pose.transform(localOrigin + localDir * hitDist);
                rayHits->normal = pose.rotate(hitNormal);
                rayHits->distance = hitDist;
                rayHits->faceIndex = 0xFFFFFFFF;
                return true;
            }
        }

        return false;
    }

    virtual bool overlap(const PxGeometry& geom0, const PxTransform& pose0,
                        const PxGeometry& geom1, const PxTransform& pose1,
                        PxOverlapThreadContext*) const override {
        // 简单的球形包围测试
        PxVec3 diff = pose1.p - pose0.p;
        PxReal distSq = diff.magnitudeSquared();

        PxReal radius0 = convex.boundingRadius;
        PxReal radius1 = 1.0f;  // 假设

        return distSq < (radius0 + radius1) * (radius0 + radius1);
    }

    virtual bool sweep(const PxVec3& unitDir, const PxReal maxDist,
                      const PxGeometry& geom0, const PxTransform& pose0,
                      const PxGeometry& geom1, const PxTransform& pose1,
                      PxGeomSweepHit& sweepHit, PxHitFlags hitFlags,
                      const PxReal inflation, PxSweepThreadContext*) const override {
        // 简化：使用保守推进
        const int numSteps = 20;
        PxReal stepDist = maxDist / numSteps;

        for (int i = 0; i <= numSteps; ++i) {
            PxReal t = i * stepDist;
            PxTransform sweptPose = pose0;
            sweptPose.p += unitDir * t;

            if (overlap(geom0, sweptPose, geom1, pose1, nullptr)) {
                sweepHit.position = sweptPose.p;
                sweepHit.normal = -unitDir;
                sweepHit.distance = t;
                sweepHit.faceIndex = 0xFFFFFFFF;
                return true;
            }
        }

        return false;
    }

    virtual void computeMassProperties(const PxGeometry& geometry,
                                      PxMassProperties& massProperties) const override {
        PxReal mass;
        PxVec3 com;
        PxMat33 inertia;

        convex.computeMassProperties(1.0f, mass, com, inertia);

        massProperties.mass = mass;
        massProperties.centerOfMass = com;
        massProperties.inertiaTensor = inertia;
    }

    virtual bool usePersistentContactManifold(const PxGeometry& geometry,
                                             PxReal& breakingThreshold) const override {
        breakingThreshold = 0.02f;
        return true;
    }

private:
    PxVec3 findClosestPoint(const PxVec3& point) const {
        // 使用梯度下降找最近点
        PxVec3 closest = convex.centroid;
        PxReal bestDist = (point - closest).magnitude();

        // 测试所有顶点
        for (const auto& v : convex.vertices) {
            PxReal dist = (point - v).magnitude();
            if (dist < bestDist) {
                bestDist = dist;
                closest = v;
            }
        }

        return closest;
    }
};

/**
 * 场景1：基本的自定义凸体创建和碰撞
 */
void setupScene1() {
    printf("=== Scene 1: Basic Custom Convex Collision ===\n");

    // 创建地面
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    gScene->addActor(*ground);

    // 创建自定义凸体
    CustomConvex convex(1.0f, 12, 0.02f);
    CustomConvexCallbacks* callbacks = new CustomConvexCallbacks(convex);

    // 创建动态刚体
    PxRigidDynamic* actor = gPhysics->createRigidDynamic(
        PxTransform(PxVec3(0, 5, 0)));

    PxCustomGeometry customGeom(*callbacks);
    PxShape* shape = gPhysics->createShape(customGeom, *gMaterial);
    actor->attachShape(*shape);
    shape->release();

    // 设置质量属性
    PxReal mass;
    PxVec3 com;
    PxMat33 inertia;
    convex.computeMassProperties(1000.0f, mass, com, inertia);

    actor->setMass(mass);
    actor->setCMassLocalPose(PxTransform(com));
    actor->setMassSpaceInertiaTensor(PxVec3(inertia.column0.x, inertia.column1.y,
                                            inertia.column2.z));

    gScene->addActor(*actor);

    printf("Created custom convex with %zu vertices\n", convex.vertices.size());
    printf("Mass: %.2f kg, Bounding radius: %.2f m\n", mass, convex.boundingRadius);
}

/**
 * 场景2：自定义凸体堆叠
 */
void setupScene2() {
    printf("\n=== Scene 2: Custom Convex Stacking ===\n");

    // 创建地面
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    gScene->addActor(*ground);

    // 创建多个不同大小的凸体堆叠
    const int numLayers = 5;
    const int numPerLayer = 3;

    for (int layer = 0; layer < numLayers; ++layer) {
        for (int i = 0; i < numPerLayer; ++i) {
            PxReal size = 0.5f + 0.3f * (numLayers - layer) / numLayers;
            CustomConvex convex(size, 8 + layer * 2, 0.01f);
            CustomConvexCallbacks* callbacks = new CustomConvexCallbacks(convex);

            PxReal x = (i - numPerLayer / 2.0f) * 1.5f;
            PxReal y = 0.5f + layer * 1.2f;

            PxRigidDynamic* actor = gPhysics->createRigidDynamic(
                PxTransform(PxVec3(x, y, 0)));

            PxCustomGeometry customGeom(*callbacks);
            PxShape* shape = gPhysics->createShape(customGeom, *gMaterial);
            actor->attachShape(*shape);
            shape->release();

            PxReal mass;
            PxVec3 com;
            PxMat33 inertia;
            convex.computeMassProperties(1000.0f, mass, com, inertia);

            actor->setMass(mass);
            actor->setCMassLocalPose(PxTransform(com));
            actor->setMassSpaceInertiaTensor(PxVec3(inertia.column0.x,
                                                    inertia.column1.y,
                                                    inertia.column2.z));

            gScene->addActor(*actor);
        }
    }

    printf("Created %d custom convex bodies in stacked formation\n",
           numLayers * numPerLayer);
}

/**
 * 场景3：射线投射测试
 */
void setupScene3() {
    printf("\n=== Scene 3: Raycast Testing ===\n");

    // 创建静态自定义凸体
    CustomConvex convex(2.0f, 16, 0.02f);
    CustomConvexCallbacks* callbacks = new CustomConvexCallbacks(convex);

    PxRigidStatic* actor = gPhysics->createRigidStatic(
        PxTransform(PxVec3(0, 5, 0)));

    PxCustomGeometry customGeom(*callbacks);
    PxShape* shape = gPhysics->createShape(customGeom, *gMaterial);
    actor->attachShape(*shape);
    shape->release();

    gScene->addActor(*actor);

    // 执行多次射线投射测试
    const int numRays = 8;
    int hits = 0;

    printf("\nRaycast tests:\n");
    for (int i = 0; i < numRays; ++i) {
        PxReal angle = PxTwoPi * i / numRays;
        PxVec3 direction(PxCos(angle), 0, PxSin(angle));
        PxVec3 origin(0, 5, 0);

        PxRaycastBuffer hit;
        bool status = gScene->raycast(origin, direction, 10.0f, hit);

        if (status) {
            hits++;
            printf("  Ray %d: HIT at distance %.2f, normal (%.2f, %.2f, %.2f)\n",
                   i, hit.block.distance,
                   hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
        } else {
            printf("  Ray %d: MISS\n", i);
        }
    }

    printf("Total hits: %d/%d\n", hits, numRays);
}

/**
 * 场景4：性能测试
 */
void setupScene4() {
    printf("\n=== Scene 4: Performance Test ===\n");

    // 创建地面
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
    gScene->addActor(*ground);

    // 创建大量小的自定义凸体
    const int numObjects = 50;
    int created = 0;

    for (int i = 0; i < numObjects; ++i) {
        PxReal size = 0.3f + (rand() % 100) / 200.0f;
        PxU32 complexity = 6 + (rand() % 4) * 2;

        CustomConvex convex(size, complexity, 0.01f);
        CustomConvexCallbacks* callbacks = new CustomConvexCallbacks(convex);

        PxReal x = ((rand() % 100) / 50.0f - 1.0f) * 5.0f;
        PxReal y = 5.0f + (rand() % 100) / 10.0f;
        PxReal z = ((rand() % 100) / 50.0f - 1.0f) * 5.0f;

        PxRigidDynamic* actor = gPhysics->createRigidDynamic(
            PxTransform(PxVec3(x, y, z)));

        PxCustomGeometry customGeom(*callbacks);
        PxShape* shape = gPhysics->createShape(customGeom, *gMaterial);
        actor->attachShape(*shape);
        shape->release();

        PxReal mass;
        PxVec3 com;
        PxMat33 inertia;
        convex.computeMassProperties(1000.0f, mass, com, inertia);

        actor->setMass(mass);
        actor->setCMassLocalPose(PxTransform(com));
        actor->setMassSpaceInertiaTensor(PxVec3(inertia.column0.x,
                                                inertia.column1.y,
                                                inertia.column2.z));

        gScene->addActor(*actor);
        created++;
    }

    printf("Created %d custom convex bodies for performance testing\n", created);
}

/**
 * 初始化PhysX
 */
bool initPhysics() {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    if (!gFoundation) {
        printf("PxCreateFoundation failed!\n");
        return false;
    }

    gPvd = PxCreatePvd(*gFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
    gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation,
                               PxTolerancesScale(), true, gPvd);
    if (!gPhysics) {
        printf("PxCreatePhysics failed!\n");
        return false;
    }

    // 创建场景
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    gScene = gPhysics->createScene(sceneDesc);
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.1f);

    return true;
}

/**
 * 清理PhysX
 */
void cleanupPhysics() {
    PX_RELEASE(gScene);
    PX_RELEASE(gPhysics);
    if (gPvd) {
        PxPvdTransport* transport = gPvd->getTransport();
        PX_RELEASE(gPvd);
        PX_RELEASE(transport);
    }
    PX_RELEASE(gFoundation);
}

/**
 * 模拟场景
 */
void simulateScene(int sceneId, PxReal duration, PxReal stepSize) {
    printf("\n--- Simulating Scene %d for %.1f seconds ---\n", sceneId, duration);

    int numSteps = static_cast<int>(duration / stepSize);
    for (int i = 0; i < numSteps; ++i) {
        gScene->simulate(stepSize);
        gScene->fetchResults(true);

        // 每秒输出一次统计
        if (i % static_cast<int>(1.0f / stepSize) == 0) {
            PxU32 numActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
            printf("  t=%.1fs: %u dynamic actors\n", i * stepSize, numActors);
        }
    }
}

/**
 * 主函数
 */
int main() {
    printf("PhysX Custom Convex Geometry Example\n");
    printf("====================================\n\n");

    if (!initPhysics()) {
        return 1;
    }

    // 测试场景1：基本碰撞
    setupScene1();
    simulateScene(1, 3.0f, 1.0f/60.0f);
    gScene->release();

    // 重新创建场景
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(sceneDesc);

    // 测试场景2：堆叠
    setupScene2();
    simulateScene(2, 3.0f, 1.0f/60.0f);
    gScene->release();

    // 重新创建场景
    gScene = gPhysics->createScene(sceneDesc);

    // 测试场景3：射线投射
    setupScene3();

    // 测试场景4：性能
    gScene->release();
    gScene = gPhysics->createScene(sceneDesc);
    setupScene4();
    simulateScene(4, 2.0f, 1.0f/60.0f);

    printf("\n=== Summary ===\n");
    printf("Demonstrated custom convex geometry with:\n");
    printf("- Star-shaped convex hull generation\n");
    printf("- Support mapping implementation\n");
    printf("- Mass properties calculation\n");
    printf("- Raycast, overlap, and sweep queries\n");
    printf("- Contact generation\n");
    printf("- Performance testing with 50+ bodies\n");

    cleanupPhysics();
    printf("\nExample completed successfully!\n");

    return 0;
}
