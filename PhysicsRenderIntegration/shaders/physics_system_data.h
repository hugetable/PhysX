// physics_system_data.h - 系统数据（设备端）

#ifndef PHYSICS_SYSTEM_DATA_H
#define PHYSICS_SYSTEM_DATA_H

#include <optix.h>
#include "physics_material_definition.h"
#include "camera_definition.h"
#include "light_definition.h"

/**
 * @brief 物理渲染系统数据
 *
 * 传递给 OptiX 着色器的全局数据
 */
struct PhysicsSystemData {
    // OptiX 场景
    OptixTraversableHandle topObject;

    // 输出缓冲区
    CUdeviceptr outputBuffer;    // 累积输出
    CUdeviceptr accumBuffer;     // 累积缓冲区（可选）

    // 相机、光源、材质
    CameraDefinition* cameraDefinitions;
    LightDefinition* lightDefinitions;
    PhysicsMaterialDefinition* materialDefinitions;

    // 环境贴图
    cudaTextureObject_t envTexture;
    float* envCDF_U;
    float* envCDF_V;
    unsigned int envWidth;
    unsigned int envHeight;
    float envIntegral;
    float envRotation;

    // 渲染参数
    int2 resolution;
    int2 pathLengths;  // min, max
    int iterationIndex;
    int samplesSqrt;

    // 场景参数
    float sceneEpsilon;
    float clockScale;

    // 数量
    int numCameras;
    int numLights;
    int numMaterials;

    // 物理相关
    float simulationTime;        // 模拟时间
    int frameIndex;              // 帧索引

    // 设备相关
    int deviceCount;
    int deviceIndex;
};

/**
 * @brief 几何实例数据（SBT Record）
 */
struct PhysicsGeometryInstanceData {
    // 几何数据
    CUdeviceptr attributes;      // 顶点属性
    CUdeviceptr indices;         // 索引数据

    // IDs
    int materialID;
    int lightID;                 // -1 表示非发光体

    // 物理对象 ID（用于调试）
    int physicsObjectID;
};

#endif // PHYSICS_SYSTEM_DATA_H
