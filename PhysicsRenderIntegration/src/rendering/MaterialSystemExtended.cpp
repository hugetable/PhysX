// MaterialSystemExtended.cpp - 扩展材质系统实现

#include "rendering/MaterialSystemExtended.h"
#include "config.h"
#include <iostream>
#include <cstring>
#include <fstream>

namespace PhysicsRender {

// ============================================================================
// 材质预设实现
// ============================================================================

namespace MaterialPresets {

ExtendedMaterialParams Chrome() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.55f, 0.55f, 0.55f);
    mat.metallic = 1.0f;
    mat.roughness = 0.05f;
    mat.ior = 2.5f;
    mat.brdfType = BRDFType::GGX;
    return mat;
}

ExtendedMaterialParams Gold() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(1.0f, 0.766f, 0.336f);
    mat.metallic = 1.0f;
    mat.roughness = 0.15f;
    mat.ior = 0.47f;
    mat.brdfType = BRDFType::GGX;
    return mat;
}

ExtendedMaterialParams Copper() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.955f, 0.637f, 0.538f);
    mat.metallic = 1.0f;
    mat.roughness = 0.2f;
    mat.ior = 1.0f;
    mat.brdfType = BRDFType::GGX;
    return mat;
}

ExtendedMaterialParams Aluminum() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.913f, 0.921f, 0.925f);
    mat.metallic = 1.0f;
    mat.roughness = 0.1f;
    mat.ior = 1.44f;
    mat.brdfType = BRDFType::GGX;
    return mat;
}

ExtendedMaterialParams Iron() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.560f, 0.570f, 0.580f);
    mat.metallic = 1.0f;
    mat.roughness = 0.35f;
    mat.ior = 2.95f;
    mat.brdfType = BRDFType::GGX;
    return mat;
}

ExtendedMaterialParams Plastic() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.8f, 0.1f, 0.1f);
    mat.metallic = 0.0f;
    mat.roughness = 0.4f;
    mat.ior = 1.5f;
    mat.brdfType = BRDFType::GGX;
    return mat;
}

ExtendedMaterialParams Rubber() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.05f, 0.05f, 0.05f);
    mat.metallic = 0.0f;
    mat.roughness = 0.9f;
    mat.ior = 1.52f;
    mat.brdfType = BRDFType::OREN_NAYAR;
    return mat;
}

ExtendedMaterialParams Wood() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.55f, 0.35f, 0.20f);
    mat.metallic = 0.0f;
    mat.roughness = 0.7f;
    mat.ior = 1.5f;
    mat.brdfType = BRDFType::OREN_NAYAR;
    return mat;
}

ExtendedMaterialParams Concrete() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.6f, 0.6f, 0.6f);
    mat.metallic = 0.0f;
    mat.roughness = 0.85f;
    mat.ior = 1.5f;
    mat.brdfType = BRDFType::OREN_NAYAR;
    return mat;
}

ExtendedMaterialParams Ceramic() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.9f, 0.9f, 0.95f);
    mat.metallic = 0.0f;
    mat.roughness = 0.15f;
    mat.ior = 1.5f;
    mat.brdfType = BRDFType::GGX;
    return mat;
}

ExtendedMaterialParams Glass() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(1.0f, 1.0f, 1.0f);
    mat.metallic = 0.0f;
    mat.roughness = 0.0f;
    mat.ior = 1.5f;
    mat.opacity = 0.0f;
    mat.transmittance = make_float3(0.95f, 0.95f, 0.95f);
    mat.brdfType = BRDFType::GGX;
    mat.flags = MAT_FLAG_TRANSPARENT | MAT_FLAG_THIN_WALLED;
    return mat;
}

ExtendedMaterialParams Water() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.5f, 0.7f, 0.8f);
    mat.metallic = 0.0f;
    mat.roughness = 0.0f;
    mat.ior = 1.33f;
    mat.opacity = 0.1f;
    mat.transmittance = make_float3(0.9f, 0.95f, 0.98f);
    mat.brdfType = BRDFType::GGX;
    mat.flags = MAT_FLAG_TRANSPARENT;
    return mat;
}

ExtendedMaterialParams ClearPlastic() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(1.0f, 1.0f, 1.0f);
    mat.metallic = 0.0f;
    mat.roughness = 0.1f;
    mat.ior = 1.5f;
    mat.opacity = 0.2f;
    mat.transmittance = make_float3(0.98f, 0.98f, 0.98f);
    mat.brdfType = BRDFType::GGX;
    mat.flags = MAT_FLAG_TRANSPARENT;
    return mat;
}

ExtendedMaterialParams FrostedGlass() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(1.0f, 1.0f, 1.0f);
    mat.metallic = 0.0f;
    mat.roughness = 0.3f;
    mat.ior = 1.5f;
    mat.opacity = 0.1f;
    mat.transmittance = make_float3(0.95f, 0.95f, 0.95f);
    mat.brdfType = BRDFType::GGX;
    mat.flags = MAT_FLAG_TRANSPARENT;
    return mat;
}

ExtendedMaterialParams Skin() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.95f, 0.75f, 0.65f);
    mat.metallic = 0.0f;
    mat.roughness = 0.4f;
    mat.ior = 1.4f;
    mat.subsurfaceColor = make_float3(0.9f, 0.3f, 0.2f);
    mat.subsurfaceRadius = 0.01f;
    mat.subsurfaceScale = 1.0f;
    mat.brdfType = BRDFType::GGX;
    mat.flags = MAT_FLAG_SUBSURFACE;
    return mat;
}

ExtendedMaterialParams Marble() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.95f, 0.95f, 0.95f);
    mat.metallic = 0.0f;
    mat.roughness = 0.2f;
    mat.ior = 1.5f;
    mat.subsurfaceColor = make_float3(0.9f, 0.9f, 0.85f);
    mat.subsurfaceRadius = 0.005f;
    mat.subsurfaceScale = 0.5f;
    mat.brdfType = BRDFType::GGX;
    mat.flags = MAT_FLAG_SUBSURFACE;
    return mat;
}

ExtendedMaterialParams Wax() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.95f, 0.90f, 0.80f);
    mat.metallic = 0.0f;
    mat.roughness = 0.3f;
    mat.ior = 1.45f;
    mat.subsurfaceColor = make_float3(0.95f, 0.85f, 0.70f);
    mat.subsurfaceRadius = 0.02f;
    mat.subsurfaceScale = 1.5f;
    mat.brdfType = BRDFType::GGX;
    mat.flags = MAT_FLAG_SUBSURFACE;
    return mat;
}

ExtendedMaterialParams Milk() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.98f, 0.98f, 0.96f);
    mat.metallic = 0.0f;
    mat.roughness = 0.1f;
    mat.ior = 1.35f;
    mat.subsurfaceColor = make_float3(0.98f, 0.96f, 0.94f);
    mat.subsurfaceRadius = 0.1f;
    mat.subsurfaceScale = 2.0f;
    mat.brdfType = BRDFType::GGX;
    mat.flags = MAT_FLAG_SUBSURFACE | MAT_FLAG_TRANSPARENT;
    mat.opacity = 0.5f;
    return mat;
}

ExtendedMaterialParams Emissive(const float3& color, float intensity) {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.1f, 0.1f, 0.1f);
    mat.emission = make_float3(
        color.x * intensity,
        color.y * intensity,
        color.z * intensity
    );
    mat.metallic = 0.0f;
    mat.roughness = 1.0f;
    mat.ior = 1.5f;
    mat.brdfType = BRDFType::LAMBERT;
    mat.flags = MAT_FLAG_EMISSIVE;
    return mat;
}

ExtendedMaterialParams Velvet() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.4f, 0.1f, 0.2f);
    mat.metallic = 0.0f;
    mat.roughness = 1.0f;
    mat.ior = 1.5f;
    mat.sheenColor = make_float3(0.8f, 0.3f, 0.4f);
    mat.sheenRoughness = 0.7f;
    mat.brdfType = BRDFType::DISNEY;
    return mat;
}

ExtendedMaterialParams Silk() {
    ExtendedMaterialParams mat;
    mat.albedo = make_float3(0.9f, 0.9f, 0.95f);
    mat.metallic = 0.0f;
    mat.roughness = 0.3f;
    mat.ior = 1.5f;
    mat.sheenColor = make_float3(1.0f, 1.0f, 1.0f);
    mat.sheenRoughness = 0.3f;
    mat.anisotropy = 0.5f;
    mat.brdfType = BRDFType::DISNEY;
    return mat;
}

} // namespace MaterialPresets

// ============================================================================
// MaterialSystemManager 实现
// ============================================================================

MaterialSystemManager::MaterialSystemManager()
    : d_materialBuffer_(0)
    , bufferSize_(0)
{
}

MaterialSystemManager::~MaterialSystemManager() {
    if (d_materialBuffer_) {
        cudaFree(reinterpret_cast<void*>(d_materialBuffer_));
        d_materialBuffer_ = 0;
    }
}

int MaterialSystemManager::addMaterial(const ExtendedMaterialParams& params) {
    int index = static_cast<int>(materials_.size());
    ExtendedMaterialParams mat = params;
    mat.materialID = index;
    materials_.push_back(mat);

    std::cout << "Material " << index << " added (BRDF: "
              << static_cast<int>(mat.brdfType) << ")" << std::endl;

    return index;
}

void MaterialSystemManager::updateMaterial(int index, const ExtendedMaterialParams& params) {
    if (index >= 0 && index < static_cast<int>(materials_.size())) {
        materials_[index] = params;
        materials_[index].materialID = index;
    }
}

void MaterialSystemManager::removeMaterial(int index) {
    if (index >= 0 && index < static_cast<int>(materials_.size())) {
        materials_.erase(materials_.begin() + index);

        // 重新分配材质 IDs
        for (int i = 0; i < static_cast<int>(materials_.size()); ++i) {
            materials_[i].materialID = i;
        }
    }
}

void MaterialSystemManager::updatePhysicsProperties(
    int index,
    float temperature,
    float damageLevel,
    float wetness
) {
    if (index >= 0 && index < static_cast<int>(materials_.size())) {
        materials_[index].temperature = temperature;
        materials_[index].damageLevel = damageLevel;
        materials_[index].wetness = wetness;
    }
}

PhysicsMaterialDefinition MaterialSystemManager::convertToDeviceMaterial(
    const ExtendedMaterialParams& params
) {
    PhysicsMaterialDefinition devMat;
    std::memset(&devMat, 0, sizeof(PhysicsMaterialDefinition));

    // 基础 PBR 参数
    devMat.albedo = params.albedo;
    devMat.emission = params.emission;
    devMat.metallic = params.metallic;
    devMat.roughness = params.roughness;
    devMat.ior = params.ior;

    // 纹理
    devMat.textureAlbedo = params.textureAlbedo;
    devMat.textureNormal = params.textureNormal;
    devMat.textureMetallic = params.textureMetallic;
    devMat.textureRoughness = params.textureRoughness;

    // 物理属性
    devMat.density = params.density;
    devMat.temperature = params.temperature;
    devMat.damageLevel = params.damageLevel;
    devMat.wetness = params.wetness;

    // 标志位
    devMat.flags = params.flags;

    return devMat;
}

CUdeviceptr MaterialSystemManager::uploadToGPU() {
    if (materials_.empty()) {
        std::cerr << "Warning: No materials to upload" << std::endl;
        return 0;
    }

    // 转换所有材质到设备格式
    deviceMaterials_.clear();
    deviceMaterials_.reserve(materials_.size());

    for (const auto& mat : materials_) {
        deviceMaterials_.push_back(convertToDeviceMaterial(mat));
    }

    // 计算所需缓冲区大小
    size_t requiredSize = deviceMaterials_.size() * sizeof(PhysicsMaterialDefinition);

    // 如果缓冲区不够大，重新分配
    if (bufferSize_ < requiredSize) {
        if (d_materialBuffer_) {
            cudaFree(reinterpret_cast<void*>(d_materialBuffer_));
        }

        cudaError_t err = cudaMalloc(
            reinterpret_cast<void**>(&d_materialBuffer_),
            requiredSize
        );

        if (err != cudaSuccess) {
            std::cerr << "Failed to allocate material buffer: "
                      << cudaGetErrorString(err) << std::endl;
            d_materialBuffer_ = 0;
            bufferSize_ = 0;
            return 0;
        }

        bufferSize_ = requiredSize;
    }

    // 上传数据
    cudaError_t err = cudaMemcpy(
        reinterpret_cast<void*>(d_materialBuffer_),
        deviceMaterials_.data(),
        requiredSize,
        cudaMemcpyHostToDevice
    );

    if (err != cudaSuccess) {
        std::cerr << "Failed to upload material data: "
                  << cudaGetErrorString(err) << std::endl;
        return 0;
    }

    std::cout << "Uploaded " << deviceMaterials_.size()
              << " materials to GPU (" << requiredSize << " bytes)" << std::endl;

    return d_materialBuffer_;
}

int MaterialSystemManager::loadMaterialLibrary(const char* filename) {
    // TODO: 实现材质库加载（JSON 或二进制格式）
    std::cout << "Material library loading not yet implemented" << std::endl;
    return 0;
}

bool MaterialSystemManager::saveMaterialLibrary(const char* filename) const {
    // TODO: 实现材质库保存
    std::cout << "Material library saving not yet implemented" << std::endl;
    return false;
}

} // namespace PhysicsRender
