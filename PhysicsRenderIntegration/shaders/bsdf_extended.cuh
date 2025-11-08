// bsdf_extended.cuh - 扩展 BSDF 函数
// 包含 GGX、透明材质、次表面散射

#ifndef BSDF_EXTENDED_CUH
#define BSDF_EXTENDED_CUH

#include <optix.h>
#include "vector_math_ext.h"
#include "random.cuh"

// ============================================================================
// GGX 微表面模型 (Microfacet BRDF)
// ============================================================================

/**
 * @brief GGX 法线分布函数 (Normal Distribution Function)
 */
__device__ __forceinline__ float GGX_D(float NoH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NoH2 = NoH * NoH;

    float denom = NoH2 * (a2 - 1.0f) + 1.0f;
    denom = M_PI * denom * denom;

    return a2 / fmaxf(denom, 1e-6f);
}

/**
 * @brief Smith GGX 几何衰减函数 (Geometry Function)
 */
__device__ __forceinline__ float GGX_G1(float NoV, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NoV2 = NoV * NoV;

    float lambda = (-1.0f + sqrtf(1.0f + a2 * (1.0f - NoV2) / NoV2)) * 0.5f;
    return 1.0f / (1.0f + lambda);
}

/**
 * @brief Smith GGX 几何函数（联合形式）
 */
__device__ __forceinline__ float GGX_G(float NoV, float NoL, float roughness) {
    return GGX_G1(NoV, roughness) * GGX_G1(NoL, roughness);
}

/**
 * @brief Schlick 菲涅尔近似
 */
__device__ __forceinline__ float3 Fresnel_Schlick(float VoH, const float3& F0) {
    float powTerm = powf(1.0f - VoH, 5.0f);
    return F0 + (make_float3(1.0f, 1.0f, 1.0f) - F0) * powTerm;
}

/**
 * @brief GGX 重要性采样
 * @param roughness 粗糙度
 * @param xi 随机数 [0,1)^2
 * @param N 法线
 * @param outDirection 输出方向（微表面法线）
 * @param outPdf 输出 PDF
 */
__device__ __forceinline__ void GGX_ImportanceSample(
    float roughness,
    const float2& xi,
    const float3& N,
    float3& outDirection,
    float& outPdf
) {
    float a = roughness * roughness;
    float a2 = a * a;

    // 在微表面法线半球空间中采样
    float phi = 2.0f * M_PI * xi.x;
    float cosTheta = sqrtf((1.0f - xi.y) / (1.0f + (a2 - 1.0f) * xi.y));
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);

    // 球面坐标转笛卡尔坐标
    float3 H = make_float3(
        sinTheta * cosf(phi),
        sinTheta * sinf(phi),
        cosTheta
    );

    // 构建切空间基
    float3 tangent, bitangent;
    createOrthonormalBasis(N, tangent, bitangent);

    // 转换到世界空间
    outDirection = tangent * H.x + bitangent * H.y + N * H.z;

    // 计算 PDF
    outPdf = GGX_D(cosTheta, roughness) * cosTheta;
}

/**
 * @brief 评估 GGX BRDF
 * @param N 表面法线
 * @param V 观察方向
 * @param L 光线方向
 * @param albedo 基础颜色
 * @param metallic 金属度
 * @param roughness 粗糙度
 * @param F0 0 度菲涅尔反射率
 * @return BRDF 值
 */
__device__ __forceinline__ float3 EvaluateGGX(
    const float3& N,
    const float3& V,
    const float3& L,
    const float3& albedo,
    float metallic,
    float roughness,
    const float3& F0
) {
    // 计算半向量
    float3 H = normalize(V + L);

    // 点积
    float NoV = fmaxf(dot(N, V), 1e-5f);
    float NoL = fmaxf(dot(N, L), 1e-5f);
    float NoH = fmaxf(dot(N, H), 1e-5f);
    float VoH = fmaxf(dot(V, H), 1e-5f);

    // Cook-Torrance BRDF
    float D = GGX_D(NoH, roughness);
    float G = GGX_G(NoV, NoL, roughness);
    float3 F = Fresnel_Schlick(VoH, F0);

    // 镜面反射项
    float3 specular = (D * G * F) / (4.0f * NoV * NoL + 1e-6f);

    // 漫反射项（能量守恒）
    float3 kD = make_float3(1.0f, 1.0f, 1.0f) - F;
    kD *= (1.0f - metallic);  // 金属没有漫反射

    float3 diffuse = kD * albedo / M_PI;

    return diffuse + specular;
}

/**
 * @brief 采样 GGX BRDF
 */
__device__ __forceinline__ void SampleGGX(
    const float3& N,
    const float3& V,
    const float3& albedo,
    float metallic,
    float roughness,
    const float3& F0,
    unsigned int& seed,
    float3& outDirection,
    float3& outWeight,
    float& outPdf
) {
    // 选择是采样漫反射还是镜面反射
    float specularChance = 0.5f + 0.5f * metallic;

    if (rnd(seed) < specularChance) {
        // 采样镜面反射（GGX）
        float2 xi = make_float2(rnd(seed), rnd(seed));

        float3 H;
        float pdfH;
        GGX_ImportanceSample(roughness, xi, N, H, pdfH);

        // 反射向量
        outDirection = reflect(-V, H);

        // 确保在半球内
        if (dot(outDirection, N) <= 0.0f) {
            outPdf = 0.0f;
            outWeight = make_float3(0.0f, 0.0f, 0.0f);
            return;
        }

        // 评估 BRDF
        float3 brdf = EvaluateGGX(N, V, outDirection, albedo, metallic, roughness, F0);

        // 将 PDF 从微表面转换到出射方向
        float VoH = fmaxf(dot(V, H), 1e-5f);
        outPdf = pdfH / (4.0f * VoH) * specularChance;

        float NoL = fmaxf(dot(N, outDirection), 1e-5f);
        outWeight = brdf * NoL / outPdf;

    } else {
        // 采样漫反射（余弦加权）
        float3 tangent, bitangent;
        createOrthonormalBasis(N, tangent, bitangent);

        float2 xi = make_float2(rnd(seed), rnd(seed));

        // 余弦加权采样
        float phi = 2.0f * M_PI * xi.x;
        float cosTheta = sqrtf(xi.y);
        float sinTheta = sqrtf(1.0f - xi.y);

        outDirection = tangent * (sinTheta * cosf(phi)) +
                       bitangent * (sinTheta * sinf(phi)) +
                       N * cosTheta;

        float NoL = fmaxf(dot(N, outDirection), 1e-5f);

        // Lambert 漫反射
        float3 kD = make_float3(1.0f, 1.0f, 1.0f) * (1.0f - metallic);
        float3 brdf = kD * albedo / M_PI;

        outPdf = NoL / M_PI * (1.0f - specularChance);
        outWeight = brdf * NoL / outPdf;
    }
}

// ============================================================================
// 透明材质 (Refraction / Transmission)
// ============================================================================

/**
 * @brief 计算折射方向（Snell's 定律）
 * @param I 入射方向
 * @param N 法线
 * @param eta 折射率比值 (eta1 / eta2)
 * @param outRefracted 输出折射方向
 * @return 是否发生折射（false = 全反射）
 */
__device__ __forceinline__ bool Refract(
    const float3& I,
    const float3& N,
    float eta,
    float3& outRefracted
) {
    float cosi = dot(-I, N);
    float sint2 = eta * eta * (1.0f - cosi * cosi);

    if (sint2 >= 1.0f) {
        return false;  // 全反射
    }

    float cost = sqrtf(1.0f - sint2);
    outRefracted = eta * I + (eta * cosi - cost) * N;

    return true;
}

/**
 * @brief Fresnel 透射系数（精确计算）
 */
__device__ __forceinline__ float FresnelDielectric(float cosThetaI, float eta) {
    float sinThetaTSq = eta * eta * (1.0f - cosThetaI * cosThetaI);

    // 全反射
    if (sinThetaTSq >= 1.0f) {
        return 1.0f;
    }

    float cosThetaT = sqrtf(1.0f - sinThetaTSq);

    // 垂直和平行偏振
    float Rs = (cosThetaI - eta * cosThetaT) / (cosThetaI + eta * cosThetaT);
    float Rp = (eta * cosThetaI - cosThetaT) / (eta * cosThetaI + cosThetaT);

    return 0.5f * (Rs * Rs + Rp * Rp);
}

/**
 * @brief 采样透明材质
 */
__device__ __forceinline__ void SampleTransparent(
    const float3& wo,      // 出射方向
    const float3& normal,
    float ior,
    const float3& transmittance,
    bool thinWalled,
    unsigned int& seed,
    float3& wi,            // 入射方向
    float3& weight,
    float& pdf
) {
    float eta = ior;
    bool entering = dot(wo, normal) > 0.0f;

    if (!entering) {
        eta = 1.0f / eta;
    }

    float cosThetaI = fabsf(dot(wo, normal));
    float Fr = FresnelDielectric(cosThetaI, eta);

    // 选择反射或折射
    if (rnd(seed) < Fr) {
        // 反射
        wi = reflect(-wo, normal);
        weight = make_float3(1.0f, 1.0f, 1.0f);
        pdf = Fr;
    } else {
        // 折射
        if (thinWalled) {
            // 薄壁：直接穿透
            wi = -wo;
            weight = transmittance;
            pdf = 1.0f - Fr;
        } else {
            // 厚壁：计算折射
            float3 refracted;
            if (Refract(wo, normal, eta, refracted)) {
                wi = refracted;
                weight = transmittance;
                pdf = 1.0f - Fr;
            } else {
                // 全反射
                wi = reflect(-wo, normal);
                weight = make_float3(1.0f, 1.0f, 1.0f);
                pdf = 1.0f;
            }
        }
    }
}

// ============================================================================
// 次表面散射 (Subsurface Scattering) - 简化近似
// ============================================================================

/**
 * @brief 基于 Wrap Lighting 的简化次表面散射近似
 */
__device__ __forceinline__ float3 ApproximateSSS(
    const float3& N,
    const float3& L,
    const float3& albedo,
    const float3& subsurfaceColor,
    float subsurfaceRadius
) {
    float NoL = dot(N, L);

    // Wrap lighting 近似
    float wrap = 0.5f;  // 包裹因子
    float wrapNoL = (NoL + wrap) / (1.0f + wrap);
    wrapNoL = fmaxf(wrapNoL, 0.0f);

    // 混合表面颜色和次表面颜色
    float3 surfaceContrib = albedo * fmaxf(NoL, 0.0f);
    float3 subsurfaceContrib = subsurfaceColor * wrapNoL * subsurfaceRadius;

    return surfaceContrib + subsurfaceContrib;
}

/**
 * @brief 更精确的次表面散射（基于漫射曲线）
 */
__device__ __forceinline__ float3 EvaluateDiffusionProfile(
    float distance,
    const float3& scatteringCoeff,
    float meanFreePath
) {
    // 简化的 diffusion profile
    // 使用指数衰减近似
    float3 profile = make_float3(
        expf(-scatteringCoeff.x * distance / meanFreePath),
        expf(-scatteringCoeff.y * distance / meanFreePath),
        expf(-scatteringCoeff.z * distance / meanFreePath)
    );

    return profile / (meanFreePath * meanFreePath);
}

// ============================================================================
// Disney Principled BRDF (简化版)
// ============================================================================

/**
 * @brief Disney 漫反射项
 */
__device__ __forceinline__ float DisneyDiffuse(
    float NoL,
    float NoV,
    float LoH,
    float roughness
) {
    float FD90 = 0.5f + 2.0f * roughness * LoH * LoH;
    float FL = powf(1.0f - NoL, 5.0f);
    float FV = powf(1.0f - NoV, 5.0f);

    return (1.0f + (FD90 - 1.0f) * FL) * (1.0f + (FD90 - 1.0f) * FV) / M_PI;
}

/**
 * @brief Disney 光泽项（Sheen）
 */
__device__ __forceinline__ float3 DisneySheen(
    float LoH,
    const float3& sheenColor
) {
    float FH = powf(1.0f - LoH, 5.0f);
    return sheenColor * FH;
}

/**
 * @brief 评估完整的 Disney BRDF
 */
__device__ __forceinline__ float3 EvaluateDisneyBRDF(
    const float3& N,
    const float3& V,
    const float3& L,
    const float3& albedo,
    float metallic,
    float roughness,
    const float3& sheenColor,
    float clearcoat,
    float clearcoatRoughness
) {
    float3 H = normalize(V + L);

    float NoV = fmaxf(dot(N, V), 1e-5f);
    float NoL = fmaxf(dot(N, L), 1e-5f);
    float NoH = fmaxf(dot(N, H), 1e-5f);
    float VoH = fmaxf(dot(V, H), 1e-5f);
    float LoH = fmaxf(dot(L, H), 1e-5f);

    // 基础层
    float3 F0 = lerp(make_float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    // 镜面反射
    float D = GGX_D(NoH, roughness);
    float G = GGX_G(NoV, NoL, roughness);
    float3 F = Fresnel_Schlick(VoH, F0);

    float3 specular = (D * G * F) / (4.0f * NoV * NoL + 1e-6f);

    // 漫反射
    float diffuseTerm = DisneyDiffuse(NoL, NoV, LoH, roughness);
    float3 kD = (make_float3(1.0f, 1.0f, 1.0f) - F) * (1.0f - metallic);
    float3 diffuse = kD * albedo * diffuseTerm;

    // 光泽（Sheen）
    float3 sheen = DisneySheen(LoH, sheenColor);

    // 清漆层（Clearcoat）
    float Dc = GGX_D(NoH, clearcoatRoughness);
    float Gc = GGX_G(NoV, NoL, clearcoatRoughness);
    float Fc = Fresnel_Schlick(VoH, make_float3(0.04f, 0.04f, 0.04f)).x;

    float3 clearcoatSpec = make_float3(1.0f, 1.0f, 1.0f) * clearcoat * Dc * Gc * Fc / (4.0f * NoV * NoL + 1e-6f);

    return diffuse + specular + sheen + clearcoatSpec;
}

#endif // BSDF_EXTENDED_CUH
