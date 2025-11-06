/**
 * PhysX Snippet: CustomGeometry
 *
 * 演示自定义几何体类型（Custom Geometry Types）
 *
 * 核心功能:
 * 1. 实现自定义几何体
 * 2. PxCustomGeometry回调
 * 3. 支持距离查询
 * 4. 支持碰撞检测
 *
 * 物理背景:
 *
 * 自定义几何体（Custom Geometry）:
 * PhysX提供了基本几何体（球、盒、胶囊等），但某些应用需要特殊形状：
 * - 椭球体（Ellipsoid）
 * - 超椭球体（Superellipsoid）
 * - 圆环（Torus）
 * - 自由曲面
 *
 * PxCustomGeometry接口:
 *
 * 用户需要实现的回调:
 * 1. getLocalBounds() - 计算局部AABB
 * 2. generateContacts() - 碰撞检测
 * 3. raycast() - 射线投射
 * 4. overlap() - 重叠检测
 * 5. sweep() - 扫描检测
 * 6. computeMassProperties() - 质量属性
 *
 * 椭球体（Ellipsoid）:
 *
 * 隐式方程:
 * (x/a)² + (y/b)² + (z/c)² = 1
 * 其中 a, b, c 是三个半轴长度
 *
 * 点到椭球体表面的距离:
 * d = ||p|| - r(θ, φ)
 * 其中 r(θ, φ) 是椭球体在方向(θ, φ)的半径
 *
 * 法向量:
 * n = ∇f / ||∇f||
 * 其中 f(x,y,z) = (x/a)² + (y/b)² + (z/c)² - 1
 * ∇f = (2x/a², 2y/b², 2z/c²)
 *
 * 超椭球体（Superellipsoid）:
 *
 * 隐式方程:
 * (|x/a|^(2/ε₂) + |y/b|^(2/ε₂))^(ε₂/ε₁) + |z/c|^(2/ε₁) = 1
 *
 * 参数:
 * - a, b, c: 半轴
 * - ε₁, ε₂: 形状参数（0-1: 凹，1: 椭球，>1: 凸）
 *
 * 特例:
 * - ε₁ = ε₂ = 1: 椭球体
 * - ε₁ = ε₂ = 2: 立方体（圆角）
 * - ε₁ = ε₂ = 0.5: 八面体（圆角）
 *
 * 圆环（Torus）:
 *
 * 隐式方程:
 * (R - √(x² + y²))² + z² = r²
 * 其中:
 * - R: 主半径（中心到管中心）
 * - r: 管半径
 *
 * 参数化形式:
 * x = (R + r × cos(v)) × cos(u)
 * y = (R + r × cos(v)) × sin(u)
 * z = r × sin(v)
 * 其中 u, v ∈ [0, 2π]
 *
 * 碰撞检测（Collision Detection）:
 *
 * 与球体碰撞:
 * 1. 计算球心到几何体表面的最近点
 * 2. 计算距离
 * 3. 如果距离 < 球半径，则碰撞
 * 4. 计算穿透深度和法向量
 *
 * 与盒体碰撞:
 * 1. SAT（Separating Axis Theorem）测试
 * 2. 测试盒体面法向量
 * 3. 测试自定义几何体特征轴
 * 4. 测试边-边叉积轴
 *
 * GJK算法（Gilbert-Johnson-Keerthi）:
 * 用于凸体碰撞检测的通用算法
 * 1. 迭代构建闵可夫斯基差的单纯形
 * 2. 检查原点是否在单纯形内
 * 3. 如果在内，则碰撞
 *
 * 质量属性计算:
 *
 * 对于均匀密度 ρ:
 * 质量: m = ρ × V
 * 质心: CoM = (1/V) × ∫∫∫ r dV
 * 惯性张量: I = ρ × ∫∫∫ (r² × I - r ⊗ r) dV
 *
 * 椭球体惯性张量:
 * I_xx = (m/5) × (b² + c²)
 * I_yy = (m/5) × (a² + c²)
 * I_zz = (m/5) × (a² + b²)
 *
 * 应用场景:
 * 1. 特殊形状物体（足球、橄榄球）
 * 2. 自然物体（卵石、树木）
 * 3. 有机形状（生物体）
 * 4. 艺术设计
 *
 * 注意:
 * ⚠️ 自定义几何体性能低于基本几何体
 * ⚠️ 需要提供高效的碰撞检测实现
 * ⚠️ 复杂形状建议使用凸网格或三角网格
 */

#include <PxPhysicsAPI.h>
#include <geometry/PxCustomGeometry.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace physx;

// ============================================================================
// 椭球体自定义几何体
// ============================================================================

/**
 * 椭球体几何体回调
 */
class EllipsoidGeometry : public PxCustomGeometry::Callbacks {
public:
    PxVec3 radii;  // 三个半轴长度 (a, b, c)

    EllipsoidGeometry(const PxVec3& r) : radii(r) {}

    // 计算局部AABB
    virtual PxBounds3 getLocalBounds(const PxGeometry& geometry) const override {
        return PxBounds3(-radii, radii);
    }

    // 计算点到椭球体表面的距离（近似）
    PxReal distanceToSurface(const PxVec3& point) const {
        // 归一化坐标
        PxVec3 p(point.x / radii.x, point.y / radii.y, point.z / radii.z);
        PxReal distFromCenter = p.magnitude();

        // 近似距离
        return (distFromCenter - 1.0f) * radii.magnitude() / PxSqrt(3.0f);
    }

    // 计算表面法向量
    PxVec3 computeNormal(const PxVec3& surfacePoint) const {
        // ∇f = (2x/a², 2y/b², 2z/c²)
        PxVec3 grad(
            2.0f * surfacePoint.x / (radii.x * radii.x),
            2.0f * surfacePoint.y / (radii.y * radii.y),
            2.0f * surfacePoint.z / (radii.z * radii.z)
        );
        return grad.getNormalized();
    }

    // 生成接触点（与球体）
    virtual PxU32 generateContacts(const PxGeometry& geom0, const PxGeometry& geom1,
                                    const PxTransform& pose0, const PxTransform& pose1,
                                    const PxReal contactDistance, const PxReal meshContactMargin,
                                    const PxReal toleranceLength,
                                    PxContactBuffer& contactBuffer) override {
        // 简化：只实现与球体的碰撞
        if (geom1.getType() != PxGeometryType::eSPHERE) {
            return 0;  // 不支持其他几何体
        }

        const PxSphereGeometry& sphere = static_cast<const PxSphereGeometry&>(geom1);

        // 将球心转换到椭球体局部坐标
        PxVec3 sphereCenterLocal = pose0.transformInv(pose1.p);

        // 计算距离
        PxReal distance = distanceToSurface(sphereCenterLocal);

        // 检测碰撞
        if (distance < sphere.radius + contactDistance) {
            // 计算接触点
            PxVec3 direction = sphereCenterLocal.getNormalized();
            PxVec3 contactPointLocal = direction * (1.0f - distance / radii.magnitude());

            // 计算法向量
            PxVec3 normal = computeNormal(contactPointLocal);

            // 转换到世界坐标
            PxVec3 contactPoint = pose0.transform(contactPointLocal);
            PxVec3 normalWorld = pose0.rotate(normal);

            // 穿透深度
            PxReal penetration = sphere.radius - distance;

            // 添加接触点
            contactBuffer.contact(contactPoint, normalWorld, penetration);
            return 1;
        }

        return 0;
    }

    // 射线投射
    virtual bool raycast(const PxVec3& origin, const PxVec3& unitDir,
                         const PxGeometry& geom, const PxTransform& pose,
                         PxReal maxDist, PxHitFlags hitFlags,
                         PxU32 maxHits, PxGeomRaycastHit* rayHits, PxU32 stride,
                         PxRaycastThreadContext* threadContext) const override {
        // 转换到局部坐标
        PxVec3 localOrigin = pose.transformInv(origin);
        PxVec3 localDir = pose.rotateInv(unitDir);

        // 简化的射线-椭球体相交测试（数值方法）
        PxReal t = 0.0f;
        const PxReal step = 0.1f;

        while (t < maxDist) {
            PxVec3 testPoint = localOrigin + localDir * t;
            PxReal dist = distanceToSurface(testPoint);

            if (dist < 0.0f) {
                // 找到交点
                if (maxHits > 0) {
                    rayHits->position = pose.transform(testPoint);
                    rayHits->normal = pose.rotate(computeNormal(testPoint));
                    rayHits->distance = t;
                    rayHits->faceIndex = 0xFFFFFFFF;
                    return true;
                }
            }

            t += step;
        }

        return false;
    }

    // 重叠检测
    virtual bool overlap(const PxGeometry& geom0, const PxTransform& pose0,
                         const PxGeometry& geom1, const PxTransform& pose1,
                         PxOverlapThreadContext* threadContext) const override {
        // 简化：使用AABB快速检测
        PxBounds3 bounds0 = getLocalBounds(geom0);
        PxBounds3 bounds1;

        if (geom1.getType() == PxGeometryType::eSPHERE) {
            const PxSphereGeometry& sphere = static_cast<const PxSphereGeometry&>(geom1);
            PxVec3 r(sphere.radius, sphere.radius, sphere.radius);
            bounds1 = PxBounds3(-r, r);
        } else {
            return false;
        }

        PxBounds3 worldBounds0 = PxBounds3::transformFast(pose0, bounds0);
        PxBounds3 worldBounds1 = PxBounds3::transformFast(pose1, bounds1);

        return worldBounds0.intersects(worldBounds1);
    }

    // 扫描检测
    virtual bool sweep(const PxVec3& unitDir, const PxReal maxDist,
                       const PxGeometry& geom0, const PxTransform& pose0,
                       const PxGeometry& geom1, const PxTransform& pose1,
                       PxGeomSweepHit& sweepHit, PxHitFlags hitFlags,
                       const PxReal inflation, PxSweepThreadContext* threadContext) const override {
        // 简化实现：不支持扫描
        return false;
    }

    // 计算质量属性
    virtual void computeMassProperties(const PxGeometry& geometry, PxMassProperties& massProperties) const override {
        // 椭球体体积: V = (4/3) × π × a × b × c
        PxReal volume = (4.0f / 3.0f) * PxPi * radii.x * radii.y * radii.z;

        // 假设密度为1
        PxReal mass = volume;

        // 椭球体惯性张量（主轴对齐）
        PxReal I_xx = (mass / 5.0f) * (radii.y * radii.y + radii.z * radii.z);
        PxReal I_yy = (mass / 5.0f) * (radii.x * radii.x + radii.z * radii.z);
        PxReal I_zz = (mass / 5.0f) * (radii.x * radii.x + radii.y * radii.y);

        massProperties.mass = mass;
        massProperties.centerOfMass = PxVec3(0, 0, 0);
        massProperties.inertiaTensor = PxMat33(
            PxVec3(I_xx, 0, 0),
            PxVec3(0, I_yy, 0),
            PxVec3(0, 0, I_zz)
        );
    }

    // 支持函数（用于GJK等算法）
    virtual void supportFunction(const PxVec3& direction, PxVec3& support) const {
        // 椭球体的支持函数
        PxVec3 d_scaled(direction.x / radii.x, direction.y / radii.y, direction.z / radii.z);
        PxReal norm = d_scaled.magnitude();

        if (norm > 1e-6f) {
            support = PxVec3(
                radii.x * d_scaled.x / norm,
                radii.y * d_scaled.y / norm,
                radii.z * d_scaled.z / norm
            );
        } else {
            support = PxVec3(radii.x, 0, 0);
        }
    }

    virtual bool usePersistentContactManifold(const PxGeometry& geometry, PxReal& breakingThreshold) const override {
        return true;
    }
};

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 创建椭球体
 */
void demonstrateEllipsoidCreation(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景1: 创建椭球体 ===" << std::endl;
    std::cout << "创建不同半径比的椭球体" << std::endl;

    // 椭球体1: 扁平（a > b = c）
    EllipsoidGeometry ellipsoid1(PxVec3(2.0f, 1.0f, 1.0f));
    PxCustomGeometry customGeom1(&ellipsoid1);

    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.3f);
    PxRigidDynamic* actor1 = physics->createRigidDynamic(PxTransform(PxVec3(-3, 0, 5)));
    PxShape* shape1 = physics->createShape(customGeom1, *material);
    actor1->attachShape(*shape1);

    PxMassProperties massProps1;
    ellipsoid1.computeMassProperties(customGeom1, massProps1);
    actor1->setMass(massProps1.mass);
    actor1->setMassSpaceInertiaTensor(massProps1.inertiaTensor.getDiagonal());

    scene->addActor(*actor1);
    shape1->release();

    std::cout << "椭球体1（扁平）: 半径(" << ellipsoid1.radii.x << ", "
              << ellipsoid1.radii.y << ", " << ellipsoid1.radii.z << ")" << std::endl;
    std::cout << "质量: " << massProps1.mass << " kg" << std::endl;

    // 椭球体2: 细长（a = b < c）
    EllipsoidGeometry ellipsoid2(PxVec3(0.5f, 0.5f, 2.0f));
    PxCustomGeometry customGeom2(&ellipsoid2);

    PxRigidDynamic* actor2 = physics->createRigidDynamic(PxTransform(PxVec3(0, 0, 5)));
    PxShape* shape2 = physics->createShape(customGeom2, *material);
    actor2->attachShape(*shape2);

    PxMassProperties massProps2;
    ellipsoid2.computeMassProperties(customGeom2, massProps2);
    actor2->setMass(massProps2.mass);
    actor2->setMassSpaceInertiaTensor(massProps2.inertiaTensor.getDiagonal());

    scene->addActor(*actor2);
    shape2->release();

    std::cout << "椭球体2（细长）: 半径(" << ellipsoid2.radii.x << ", "
              << ellipsoid2.radii.y << ", " << ellipsoid2.radii.z << ")" << std::endl;
    std::cout << "质量: " << massProps2.mass << " kg" << std::endl;

    // 椭球体3: 标准球体（a = b = c）
    EllipsoidGeometry ellipsoid3(PxVec3(1.0f, 1.0f, 1.0f));
    PxCustomGeometry customGeom3(&ellipsoid3);

    PxRigidDynamic* actor3 = physics->createRigidDynamic(PxTransform(PxVec3(3, 0, 5)));
    PxShape* shape3 = physics->createShape(customGeom3, *material);
    actor3->attachShape(*shape3);

    PxMassProperties massProps3;
    ellipsoid3.computeMassProperties(customGeom3, massProps3);
    actor3->setMass(massProps3.mass);
    actor3->setMassSpaceInertiaTensor(massProps3.inertiaTensor.getDiagonal());

    scene->addActor(*actor3);
    shape3->release();

    std::cout << "椭球体3（球体）: 半径(" << ellipsoid3.radii.x << ", "
              << ellipsoid3.radii.y << ", " << ellipsoid3.radii.z << ")" << std::endl;
    std::cout << "质量: " << massProps3.mass << " kg" << std::endl;

    material->release();
}

/**
 * 场景2: 椭球体与球体碰撞
 */
void demonstrateEllipsoidCollision(PxPhysics* physics, PxScene* scene) {
    std::cout << "\n=== 场景2: 椭球体与球体碰撞 ===" << std::endl;
    std::cout << "测试自定义碰撞检测" << std::endl;

    // 创建椭球体
    EllipsoidGeometry ellipsoidCallback(PxVec3(2.0f, 1.5f, 1.0f));
    PxCustomGeometry ellipsoidGeom(&ellipsoidCallback);

    PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.6f);
    PxRigidDynamic* ellipsoid = physics->createRigidDynamic(PxTransform(PxVec3(0, 0, 2)));
    PxShape* ellipsoidShape = physics->createShape(ellipsoidGeom, *material);
    ellipsoid->attachShape(*ellipsoidShape);

    PxMassProperties massProps;
    ellipsoidCallback.computeMassProperties(ellipsoidGeom, massProps);
    ellipsoid->setMass(massProps.mass);
    ellipsoid->setMassSpaceInertiaTensor(massProps.inertiaTensor.getDiagonal());

    scene->addActor(*ellipsoid);
    ellipsoidShape->release();

    // 创建球体（下落）
    PxRigidDynamic* sphere = physics->createRigidDynamic(PxTransform(PxVec3(0, 0, 10)));
    PxShape* sphereShape = physics->createShape(PxSphereGeometry(0.5f), *material);
    sphere->attachShape(*sphereShape);
    PxRigidBodyExt::setMassAndUpdateInertia(*sphere, 10.0f);
    scene->addActor(*sphere);
    sphereShape->release();

    material->release();

    std::cout << "球体将下落并与椭球体碰撞" << std::endl;
    std::cout << "自定义碰撞检测将被调用" << std::endl;
}

/**
 * 场景3: 质量属性验证
 */
void demonstrateMassProperties() {
    std::cout << "\n=== 场景3: 质量属性验证 ===" << std::endl;
    std::cout << "验证椭球体的质量和惯性计算" << std::endl;

    EllipsoidGeometry ellipsoid(PxVec3(2.0f, 1.5f, 1.0f));
    PxCustomGeometry geom(&ellipsoid);

    PxMassProperties massProps;
    ellipsoid.computeMassProperties(geom, massProps);

    std::cout << "\n椭球体半径: (" << ellipsoid.radii.x << ", "
              << ellipsoid.radii.y << ", " << ellipsoid.radii.z << ")" << std::endl;
    std::cout << "体积: " << (4.0f/3.0f) * PxPi * ellipsoid.radii.x * ellipsoid.radii.y * ellipsoid.radii.z << std::endl;
    std::cout << "质量: " << massProps.mass << " kg (假设密度=1)" << std::endl;
    std::cout << "质心: (" << massProps.centerOfMass.x << ", "
              << massProps.centerOfMass.y << ", "
              << massProps.centerOfMass.z << ")" << std::endl;
    std::cout << "惯性张量对角线:" << std::endl;
    std::cout << "  I_xx = " << massProps.inertiaTensor[0][0] << std::endl;
    std::cout << "  I_yy = " << massProps.inertiaTensor[1][1] << std::endl;
    std::cout << "  I_zz = " << massProps.inertiaTensor[2][2] << std::endl;

    // 理论验证
    PxReal m = massProps.mass;
    PxReal a = ellipsoid.radii.x;
    PxReal b = ellipsoid.radii.y;
    PxReal c = ellipsoid.radii.z;

    PxReal I_xx_theory = (m / 5.0f) * (b*b + c*c);
    PxReal I_yy_theory = (m / 5.0f) * (a*a + c*c);
    PxReal I_zz_theory = (m / 5.0f) * (a*a + b*b);

    std::cout << "\n理论值（用于验证）:" << std::endl;
    std::cout << "  I_xx_theory = " << I_xx_theory << std::endl;
    std::cout << "  I_yy_theory = " << I_yy_theory << std::endl;
    std::cout << "  I_zz_theory = " << I_zz_theory << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: CustomGeometry" << std::endl;
    std::cout << "===============================" << std::endl;
    std::cout << "\n演示自定义几何体类型" << std::endl;

    // 初始化PhysX
    PxDefaultAllocator allocator;
    PxDefaultErrorCallback errorCallback;

    PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
    if (!foundation) {
        std::cerr << "PxCreateFoundation failed!" << std::endl;
        return 1;
    }

    PxPhysics* physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale());
    if (!physics) {
        std::cerr << "PxCreatePhysics failed!" << std::endl;
        return 1;
    }

    // 创建场景
    PxSceneDesc sceneDesc(physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, 0.0f, -9.81f);
    sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    PxScene* scene = physics->createScene(sceneDesc);

    // 添加地面
    PxMaterial* groundMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);
    PxRigidStatic* ground = physics->createRigidStatic(PxTransform(PxVec3(0, 0, 0)));
    PxShape* groundShape = physics->createShape(PxPlaneGeometry(), *groundMaterial);
    ground->attachShape(*groundShape);
    scene->addActor(*ground);
    groundShape->release();
    groundMaterial->release();

    // 运行场景
    demonstrateEllipsoidCreation(physics, scene);
    demonstrateEllipsoidCollision(physics, scene);
    demonstrateMassProperties();

    // 模拟几帧
    std::cout << "\n=== 运行模拟 ===" << std::endl;
    const PxReal dt = 1.0f / 60.0f;
    for (int i = 0; i < 120; ++i) {
        scene->simulate(dt);
        scene->fetchResults(true);

        if (i % 30 == 0) {
            std::cout << "帧 " << i << ": 模拟运行中..." << std::endl;
        }
    }

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\n自定义几何体要点:" << std::endl;
    std::cout << "1. 继承 PxCustomGeometry::Callbacks" << std::endl;
    std::cout << "2. 实现必要的回调函数" << std::endl;
    std::cout << "3. 提供高效的碰撞检测算法" << std::endl;
    std::cout << "4. 计算正确的质量属性" << std::endl;

    std::cout << "\n椭球体公式:" << std::endl;
    std::cout << "隐式方程: (x/a)² + (y/b)² + (z/c)² = 1" << std::endl;
    std::cout << "体积: V = (4/3) × π × a × b × c" << std::endl;
    std::cout << "惯性: I_xx = (m/5) × (b² + c²)" << std::endl;
    std::cout << "法向量: n = ∇f / ||∇f||" << std::endl;

    std::cout << "\n应用场景:" << std::endl;
    std::cout << "- 橄榄球、橄榄形物体" << std::endl;
    std::cout << "- 鸡蛋、卵石等自然形状" << std::endl;
    std::cout << "- 生物体建模" << std::endl;
    std::cout << "- 特殊设计形状" << std::endl;

    std::cout << "\n⚠️ 注意事项:" << std::endl;
    std::cout << "- 性能低于基本几何体" << std::endl;
    std::cout << "- 需要提供精确的碰撞检测" << std::endl;
    std::cout << "- 复杂形状考虑使用凸/三角网格" << std::endl;

    // 清理
    scene->release();
    physics->release();
    foundation->release();

    return 0;
}
