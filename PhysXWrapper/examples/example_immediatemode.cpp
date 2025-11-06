/**
 * PhysX Snippet: Immediate Mode
 *
 * 本示例演示如何使用即时模式(Immediate Mode)进行低层次物理计算。
 *
 * 理论基础：
 *
 * 1. 即时模式 vs 场景模式
 *    场景模式(Scene Mode)：
 *    - 创建PxScene，添加actors
 *    - 调用simulate()和fetchResults()
 *    - PhysX管理所有状态
 *    - 高层API，易于使用
 *
 *    即时模式(Immediate Mode)：
 *    - 不创建Scene
 *    - 直接调用碰撞检测和约束求解
 *    - 用户管理所有状态
 *    - 低层API，更灵活
 *
 * 2. 即时模式的优势
 *    - 更小的内存占用
 *    - 更精确的控制
 *    - 无Scene开销
 *    - 适合自定义物理管线
 *    - 可以与其他物理引擎混用
 *
 * 3. 碰撞检测流程
 *    即时模式的碰撞检测：
 *    ```
 *    PxGeometry& geom1, geom2;
 *    PxTransform& pose1, pose2;
 *    PxContactBuffer contactBuffer;
 *
 *    PxGenerateContacts(&geom1, &geom2,
 *                       &pose1, &pose2,
 *                       contactBuffer, ...);
 *    ```
 *
 * 4. 约束求解
 *    Position-Based Dynamics (PBD)：
 *    for each constraint:
 *        Δx = solve_constraint()
 *        apply Δx to positions
 *
 *    Impulse-Based Dynamics：
 *    for each contact:
 *        J = compute_jacobian()
 *        λ = solve(J, v)
 *        apply_impulse(λ)
 *
 * 5. 时间积分
 *    显式欧拉积分：
 *    v' = v + a × dt
 *    x' = x + v' × dt
 *
 *    半隐式欧拉（Symplectic Euler）：
 *    v' = v + a × dt
 *    x' = x + v × dt
 *
 * 6. 适用场景
 *    即时模式适合：
 *    - 简单物理模拟
 *    - 自定义物理系统
 *    - 工具和编辑器
 *    - 性能关键代码
 *    - 教学和研究
 *
 * 7. 局限性
 *    - 需要手动管理状态
 *    - 没有宽相剔除
 *    - 没有场景查询
 *    - 缺少高级功能（关节、软体等）
 *    - 更复杂的代码
 */

#include "PxPhysicsAPI.h"
#include "../common/PxPhysXCommon.h"
#include <vector>
#include <cmath>
#include <chrono>

using namespace physx;

// 全局变量
static PxDefaultAllocator gAllocator;
static PxDefaultErrorCallback gErrorCallback;
static PxFoundation* gFoundation = nullptr;
static PxPhysics* gPhysics = nullptr;

/**
 * 简单的刚体表示
 */
struct RigidBody {
    PxTransform pose;
    PxVec3 linearVelocity;
    PxVec3 angularVelocity;
    PxReal mass;
    PxReal invMass;
    PxMat33 inertia;
    PxMat33 invInertia;
    PxGeometryHolder geometry;

    RigidBody() : mass(1.0f), invMass(1.0f),
                  linearVelocity(0), angularVelocity(0) {
        pose = PxTransform(PxIdentity);
        inertia = PxMat33(PxIdentity);
        invInertia = PxMat33(PxIdentity);
    }

    void setMass(PxReal m) {
        mass = m;
        invMass = (m > 0.0f) ? (1.0f / m) : 0.0f;
    }

    void setInertia(const PxVec3& diag) {
        inertia = PxMat33::createDiagonal(diag);
        invInertia = PxMat33::createDiagonal(
            PxVec3(1.0f / diag.x, 1.0f / diag.y, 1.0f / diag.z));
    }
};

/**
 * 应用重力
 */
void applyGravity(RigidBody& body, const PxVec3& gravity, PxReal dt) {
    if (body.invMass > 0.0f) {
        body.linearVelocity += gravity * dt;
    }
}

/**
 * 时间积分（半隐式欧拉）
 */
void integrate(RigidBody& body, PxReal dt) {
    if (body.invMass == 0.0f) return;  // 静态物体

    // 更新位置
    body.pose.p += body.linearVelocity * dt;

    // 更新旋转
    if (body.angularVelocity.magnitudeSquared() > 1e-6f) {
        PxReal angle = body.angularVelocity.magnitude() * dt;
        PxVec3 axis = body.angularVelocity.getNormalized();
        PxQuat dq(angle, axis);
        body.pose.q = (body.pose.q * dq).getNormalized();
    }
}

/**
 * 碰撞检测
 */
bool detectCollision(const RigidBody& body1, const RigidBody& body2,
                     PxContactBuffer& contactBuffer) {
    const PxGeometry* geom1 = &body1.geometry.any();
    const PxGeometry* geom2 = &body2.geometry.any();

    return PxGenerateContacts(
        geom1, geom2,
        &body1.pose, &body2.pose,
        nullptr, 0,
        contactBuffer,
        0.0f, 0.0f,
        PxTolerancesScale()
    );
}

/**
 * 简单的冲量求解
 */
void resolveCollision(RigidBody& body1, RigidBody& body2,
                      const PxContactPoint& contact,
                      PxReal restitution = 0.5f) {
    PxVec3 normal = contact.normal;
    PxVec3 point = contact.point;

    // 相对速度
    PxVec3 r1 = point - body1.pose.p;
    PxVec3 r2 = point - body2.pose.p;

    PxVec3 v1 = body1.linearVelocity + body1.angularVelocity.cross(r1);
    PxVec3 v2 = body2.linearVelocity + body2.angularVelocity.cross(r2);
    PxVec3 relVel = v1 - v2;

    // 法向分量
    PxReal vn = relVel.dot(normal);
    if (vn >= 0.0f) return;  // 已经分离

    // 计算冲量
    PxReal num = -(1.0f + restitution) * vn;

    PxVec3 t1 = r1.cross(normal);
    PxVec3 t2 = r2.cross(normal);
    PxReal denom = body1.invMass + body2.invMass +
                   (body1.invInertia * t1).dot(t1) +
                   (body2.invInertia * t2).dot(t2);

    PxReal j = num / denom;
    PxVec3 impulse = normal * j;

    // 应用冲量
    body1.linearVelocity += impulse * body1.invMass;
    body1.angularVelocity += body1.invInertia * r1.cross(impulse);

    body2.linearVelocity -= impulse * body2.invMass;
    body2.angularVelocity -= body2.invInertia * r2.cross(impulse);
}

/**
 * 场景1：基本的即时模式仿真
 */
void testScene1_BasicImmediateMode() {
    printf("=== Scene 1: Basic Immediate Mode Simulation ===\n");

    // 创建两个球体
    RigidBody body1, body2;

    body1.geometry = PxSphereGeometry(0.5f);
    body1.pose.p = PxVec3(0, 5, 0);
    body1.setMass(1.0f);
    body1.setInertia(PxVec3(0.1f, 0.1f, 0.1f));

    body2.geometry = PxSphereGeometry(0.5f);
    body2.pose.p = PxVec3(0, 0, 0);
    body2.setMass(0.0f);  // 静态
    body2.setInertia(PxVec3(1.0f, 1.0f, 1.0f));

    printf("Created 2 spheres (1 dynamic, 1 static)\n");

    // 仿真循环
    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 120;
    const PxVec3 gravity(0, -9.81f, 0);

    int collisionCount = 0;

    for (int step = 0; step < numSteps; ++step) {
        // 应用重力
        applyGravity(body1, gravity, dt);

        // 时间积分
        integrate(body1, dt);

        // 碰撞检测
        PxContactBuffer contactBuffer;
        if (detectCollision(body1, body2, contactBuffer)) {
            collisionCount++;

            // 解决碰撞
            for (PxU32 i = 0; i < contactBuffer.count; ++i) {
                resolveCollision(body1, body2, contactBuffer.contacts[i], 0.5f);
            }

            // 位置修正（简单的推出）
            if (contactBuffer.count > 0) {
                PxVec3 normal = contactBuffer.contacts[0].normal;
                PxReal depth = contactBuffer.contacts[0].separation;
                if (depth < 0.0f) {
                    body1.pose.p -= normal * depth * 0.5f;
                }
            }
        }

        // 每秒输出一次
        if (step % 60 == 0) {
            printf("  t=%.1fs: pos=(%.2f, %.2f, %.2f), vel=(%.2f, %.2f, %.2f)\n",
                   step * dt,
                   body1.pose.p.x, body1.pose.p.y, body1.pose.p.z,
                   body1.linearVelocity.x, body1.linearVelocity.y, body1.linearVelocity.z);
        }
    }

    printf("Total collisions detected: %d\n", collisionCount);
    printf("Final position: (%.2f, %.2f, %.2f)\n",
           body1.pose.p.x, body1.pose.p.y, body1.pose.p.z);
}

/**
 * 场景2：多物体仿真
 */
void testScene2_MultipleObjects() {
    printf("\n=== Scene 2: Multiple Objects ===\n");

    std::vector<RigidBody> bodies;

    // 创建地面
    RigidBody ground;
    ground.geometry = PxBoxGeometry(10, 0.5f, 10);
    ground.pose.p = PxVec3(0, -0.5f, 0);
    ground.setMass(0.0f);
    bodies.push_back(ground);

    // 创建多个球体
    const int numBalls = 5;
    for (int i = 0; i < numBalls; ++i) {
        RigidBody ball;
        ball.geometry = PxSphereGeometry(0.3f);
        ball.pose.p = PxVec3(i * 0.5f - 1.0f, 3 + i * 0.5f, 0);
        ball.setMass(1.0f);
        ball.setInertia(PxVec3(0.036f, 0.036f, 0.036f));
        bodies.push_back(ball);
    }

    printf("Created %zu objects (%d dynamic)\n", bodies.size(), numBalls);

    // 仿真
    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 180;
    const PxVec3 gravity(0, -9.81f, 0);

    for (int step = 0; step < numSteps; ++step) {
        // 应用力和积分
        for (size_t i = 1; i < bodies.size(); ++i) {
            applyGravity(bodies[i], gravity, dt);
            integrate(bodies[i], dt);
        }

        // 碰撞检测和响应
        for (size_t i = 0; i < bodies.size(); ++i) {
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                PxContactBuffer contactBuffer;
                if (detectCollision(bodies[i], bodies[j], contactBuffer)) {
                    for (PxU32 k = 0; k < contactBuffer.count; ++k) {
                        resolveCollision(bodies[i], bodies[j],
                                       contactBuffer.contacts[k], 0.3f);
                    }
                }
            }
        }
    }

    printf("Simulation completed\n");
    printf("Final positions:\n");
    for (size_t i = 1; i < bodies.size(); ++i) {
        printf("  Ball %zu: (%.2f, %.2f, %.2f)\n",
               i, bodies[i].pose.p.x, bodies[i].pose.p.y, bodies[i].pose.p.z);
    }
}

/**
 * 场景3：性能对比
 */
void testScene3_PerformanceComparison() {
    printf("\n=== Scene 3: Performance Comparison ===\n");

    const int numObjects = 20;
    const int numSteps = 100;
    const PxReal dt = 1.0f / 60.0f;
    const PxVec3 gravity(0, -9.81f, 0);

    // 即时模式
    {
        std::vector<RigidBody> bodies;

        // 地面
        RigidBody ground;
        ground.geometry = PxBoxGeometry(10, 0.5f, 10);
        ground.pose.p = PxVec3(0, -0.5f, 0);
        ground.setMass(0.0f);
        bodies.push_back(ground);

        // 动态物体
        for (int i = 0; i < numObjects; ++i) {
            RigidBody ball;
            ball.geometry = PxSphereGeometry(0.3f);
            ball.pose.p = PxVec3((i % 5) * 0.7f - 1.4f, 2 + (i / 5) * 0.7f, 0);
            ball.setMass(1.0f);
            ball.setInertia(PxVec3(0.036f, 0.036f, 0.036f));
            bodies.push_back(ball);
        }

        auto start = std::chrono::high_resolution_clock::now();

        for (int step = 0; step < numSteps; ++step) {
            for (size_t i = 1; i < bodies.size(); ++i) {
                applyGravity(bodies[i], gravity, dt);
                integrate(bodies[i], dt);
            }

            for (size_t i = 0; i < bodies.size(); ++i) {
                for (size_t j = i + 1; j < bodies.size(); ++j) {
                    PxContactBuffer contactBuffer;
                    if (detectCollision(bodies[i], bodies[j], contactBuffer)) {
                        for (PxU32 k = 0; k < contactBuffer.count; ++k) {
                            resolveCollision(bodies[i], bodies[j],
                                           contactBuffer.contacts[k], 0.3f);
                        }
                    }
                }
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        long immTime = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start).count();

        printf("Immediate Mode:\n");
        printf("  Objects: %d\n", numObjects);
        printf("  Steps: %d\n", numSteps);
        printf("  Time: %ld μs\n", immTime);
        printf("  Avg per step: %.2f μs\n",
               static_cast<float>(immTime) / numSteps);
    }

    printf("\nNote: Immediate mode is simpler but lacks optimizations\n");
    printf("Scene mode would be faster for complex scenarios\n");
}

/**
 * 场景4：自定义约束
 */
void testScene4_CustomConstraints() {
    printf("\n=== Scene 4: Custom Constraints ===\n");

    // 创建两个物体，用距离约束连接
    RigidBody body1, body2;

    body1.geometry = PxSphereGeometry(0.3f);
    body1.pose.p = PxVec3(-1, 3, 0);
    body1.setMass(1.0f);
    body1.setInertia(PxVec3(0.036f, 0.036f, 0.036f));

    body2.geometry = PxSphereGeometry(0.3f);
    body2.pose.p = PxVec3(1, 3, 0);
    body2.setMass(1.0f);
    body2.setInertia(PxVec3(0.036f, 0.036f, 0.036f));

    const PxReal constraintLength = 2.0f;

    printf("Created 2 bodies with distance constraint (%.2f m)\n", constraintLength);

    // 仿真
    const PxReal dt = 1.0f / 60.0f;
    const int numSteps = 300;
    const PxVec3 gravity(0, -9.81f, 0);

    for (int step = 0; step < numSteps; ++step) {
        // 应用重力
        applyGravity(body1, gravity, dt);
        applyGravity(body2, gravity, dt);

        // 积分
        integrate(body1, dt);
        integrate(body2, dt);

        // 距离约束
        PxVec3 diff = body2.pose.p - body1.pose.p;
        PxReal dist = diff.magnitude();
        if (dist > 1e-6f) {
            PxReal error = dist - constraintLength;
            if (PxAbs(error) > 0.01f) {
                PxVec3 correction = diff * (error / dist) * 0.5f;
                body1.pose.p += correction;
                body2.pose.p -= correction;
            }
        }

        // 每秒输出一次
        if (step % 60 == 0) {
            PxReal currentDist = (body2.pose.p - body1.pose.p).magnitude();
            printf("  t=%.1fs: distance=%.3f m (target=%.2f m)\n",
                   step * dt, currentDist, constraintLength);
        }
    }
}

/**
 * 场景5：即时模式的优势展示
 */
void testScene5_ImmediateModeAdvantages() {
    printf("\n=== Scene 5: Immediate Mode Advantages ===\n");

    printf("\nDemonstrating precise control:\n");

    RigidBody body;
    body.geometry = PxSphereGeometry(0.5f);
    body.pose.p = PxVec3(0, 5, 0);
    body.setMass(1.0f);
    body.setInertia(PxVec3(0.1f, 0.1f, 0.1f));

    const PxReal dt = 1.0f / 60.0f;

    // 精确控制每一帧
    for (int step = 0; step < 60; ++step) {
        // 自定义力模型（非标准重力）
        PxReal customGravity = -9.81f * (1.0f + 0.1f * PxSin(step * 0.1f));
        body.linearVelocity.y += customGravity * dt;

        integrate(body, dt);

        if (step % 10 == 0) {
            printf("  Step %d: y=%.3f, vy=%.3f, g=%.3f\n",
                   step, body.pose.p.y, body.linearVelocity.y, customGravity);
        }
    }

    printf("\nAdvantages demonstrated:\n");
    printf("  - Full control over every timestep\n");
    printf("  - Custom force models\n");
    printf("  - No Scene overhead\n");
    printf("  - Direct access to all state\n");
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

    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation,
                               PxTolerancesScale(), true, nullptr);
    if (!gPhysics) {
        printf("PxCreatePhysics failed!\n");
        return false;
    }

    return true;
}

/**
 * 清理
 */
void cleanupPhysX() {
    PX_RELEASE(gPhysics);
    PX_RELEASE(gFoundation);
}

/**
 * 主函数
 */
int main() {
    printf("PhysX Immediate Mode Example\n");
    printf("============================\n\n");

    if (!initPhysX()) {
        return 1;
    }

    // 测试所有场景
    testScene1_BasicImmediateMode();
    testScene2_MultipleObjects();
    testScene3_PerformanceComparison();
    testScene4_CustomConstraints();
    testScene5_ImmediateModeAdvantages();

    printf("\n=== Summary ===\n");
    printf("Demonstrated immediate mode features:\n");
    printf("- Direct collision detection (PxGenerateContacts)\n");
    printf("- Manual integration and constraint solving\n");
    printf("- Multiple object simulation\n");
    printf("- Custom constraints (distance constraint)\n");
    printf("- Full control over physics pipeline\n");
    printf("\nKey insights:\n");
    printf("- No Scene required\n");
    printf("- Lower memory footprint\n");
    printf("- Full control over timestep\n");
    printf("- Custom force models possible\n");
    printf("- More code complexity\n");
    printf("\nUse cases:\n");
    printf("- Simple physics simulations\n");
    printf("- Custom physics pipelines\n");
    printf("- Tools and editors\n");
    printf("- Educational purposes\n");
    printf("- Performance-critical code\n");

    cleanupPhysX();
    printf("\nExample completed successfully!\n");

    return 0;
}
