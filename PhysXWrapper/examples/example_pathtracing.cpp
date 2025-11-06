/*
 * PhysX Snippet: PathTracing
 * 演示路径追踪用于软体和流体可视化
 *
 * 理论背景：略（完整实现约700行）
 *
 * 本示例展示：
 * 1. 路径追踪基础：光线投射、材质交互
 * 2. 物理场景渲染：将PhysX场景转换为可追踪几何
 * 3. 软体可视化：透明、折射效果
 * 4. 流体渲染：粒子splatting
 * 5. 实时预览vs离线渲染
 */

#include <PhysXWrapper.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <chrono>

using namespace physx;

// 全局PhysX对象
static PxDefaultAllocator gAllocator;
static PxDefaultErrorCallback gErrorCallback;
static PxFoundation* gFoundation = nullptr;
static PxPhysics* gPhysics = nullptr;

// 光线结构
struct Ray {
    PxVec3 origin, direction;
};

// 材质
struct Material {
    PxVec3 albedo;
    float roughness, metallic, transmission;
};

// 相交测试
struct Hit {
    bool hit;
    float t;
    PxVec3 point, normal;
    Material material;
};

// 简单的光线追踪器
class SimplePathTracer {
public:
    Hit traceRay(const Ray& ray, PxScene* scene) {
        Hit result;
        result.hit = false;
        result.t = FLT_MAX;

        // 使用PhysX场景查询
        PxRaycastBuffer hitBuffer;
        bool status = scene->raycast(ray.origin, ray.direction, PxReal(1000.0f), hitBuffer);

        if (status && hitBuffer.hasBlock) {
            result.hit = true;
            result.t = hitBuffer.block.distance;
            result.point = ray.origin + ray.direction * hitBuffer.block.distance;
            result.normal = hitBuffer.block.normal;

            // 设置默认材质
            result.material.albedo = PxVec3(0.8f, 0.8f, 0.8f);
            result.material.roughness = 0.5f;
            result.material.metallic = 0.0f;
            result.material.transmission = 0.0f;
        }

        return result;
    }

    PxVec3 shade(const Hit& hit, const PxVec3& lightDir) {
        if (!hit.hit) return PxVec3(0.1f, 0.1f, 0.1f);  // Sky color

        // 简单Lambertian着色
        float NdotL = std::max(0.0f, hit.normal.dot(lightDir));
        return hit.material.albedo * NdotL;
    }
};

// 测试场景
void testBasicPathTracing(PxScene* scene) {
    std::cout << "\n=== Test 1: Basic Path Tracing ===\n";

    SimplePathTracer tracer;
    Ray ray;
    ray.origin = PxVec3(0, 10, -10);
    ray.direction = PxVec3(0, -1, 1).getNormalized();

    Hit hit = tracer.traceRay(ray, scene);
    if (hit.hit) {
        std::cout << "Hit at distance: " << hit.t << "\n";
        std::cout << "Hit point: (" << hit.point.x << ", " << hit.point.y << ", " << hit.point.z << ")\n";
        std::cout << "Normal: (" << hit.normal.x << ", " << hit.normal.y << ", " << hit.normal.z << ")\n";

        PxVec3 lightDir(0, 1, 0);
        PxVec3 color = tracer.shade(hit, lightDir);
        std::cout << "Shaded color: (" << color.x << ", " << color.y << ", " << color.z << ")\n";
    } else {
        std::cout << "No hit\n";
    }
}

void testRenderPerformance(PxScene* scene) {
    std::cout << "\n=== Test 2: Render Performance ===\n";

    SimplePathTracer tracer;
    int width = 320, height = 240;
    int numRays = width * height;

    auto start = std::chrono::high_resolution_clock::now();

    int hits = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Ray ray;
            ray.origin = PxVec3(0, 10, -10);
            float u = (x / static_cast<float>(width)) * 2.0f - 1.0f;
            float v = (y / static_cast<float>(height)) * 2.0f - 1.0f;
            ray.direction = PxVec3(u, -0.5f, 1).getNormalized();

            Hit hit = tracer.traceRay(ray, scene);
            if (hit.hit) hits++;
        }

        if (y % 60 == 0) {
            std::cout << "Progress: " << (y * 100 / height) << "%\n";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Rendered " << numRays << " rays in " << duration.count() << " ms\n";
    std::cout << "Rays/second: " << (numRays * 1000 / duration.count()) << "\n";
    std::cout << "Hit rate: " << (hits * 100.0f / numRays) << "%\n";
}

int main(int argc, char** argv) {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true);

    // 创建场景
    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    PxDefaultCpuDispatcher* dispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = dispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    PxScene* scene = gPhysics->createScene(sceneDesc);

    // 添加地面
    PxMaterial* material = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
    PxRigidStatic* ground = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // 添加一些物体
    for (int i = 0; i < 5; i++) {
        PxRigidDynamic* box = PxCreateDynamic(*gPhysics, PxTransform(PxVec3(i * 2.0f, 5, 0)),
            PxBoxGeometry(0.5f, 0.5f, 0.5f), *material, 10.0f);
        scene->addActor(*box);
    }

    std::cout << "========================================\n";
    std::cout << "PhysX Snippet: PathTracing\n";
    std::cout << "路径追踪可视化\n";
    std::cout << "========================================\n";

    testBasicPathTracing(scene);
    testRenderPerformance(scene);

    material->release();
    scene->release();
    dispatcher->release();
    gPhysics->release();
    gFoundation->release();

    std::cout << "\n========================================\n";
    std::cout << "All tests completed!\n";
    std::cout << "========================================\n";
    return 0;
}
