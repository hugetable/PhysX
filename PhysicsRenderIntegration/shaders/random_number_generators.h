// random_number_generators.h - 随机数生成器

#ifndef RANDOM_NUMBER_GENERATORS_H
#define RANDOM_NUMBER_GENERATORS_H

/**
 * @brief TEA (Tiny Encryption Algorithm) - 用于随机数生成
 *
 * @tparam N 迭代次数（越多越随机，但越慢）
 * @param val0 输入值 0
 * @param val1 输入值 1
 * @return 伪随机 uint
 */
template<unsigned int N>
__device__ __forceinline__ unsigned int tea(unsigned int val0, unsigned int val1) {
    unsigned int v0 = val0;
    unsigned int v1 = val1;
    unsigned int s0 = 0;

    for (unsigned int n = 0; n < N; n++) {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }

    return v1;
}

/**
 * @brief LCG (Linear Congruential Generator)
 *
 * @param prev 前一个随机数
 * @return 下一个随机数
 */
__device__ __forceinline__ unsigned int lcg(unsigned int prev) {
    const unsigned int LCG_A = 1664525u;
    const unsigned int LCG_C = 1013904223u;
    prev = (LCG_A * prev + LCG_C);
    return prev & 0x00FFFFFF;
}

/**
 * @brief 生成 [0, 1) 的随机浮点数
 *
 * @param seed 随机数种子（会被修改）
 * @return 随机浮点数
 */
__device__ __forceinline__ float rng(unsigned int& seed) {
    seed = lcg(seed);
    return static_cast<float>(seed) / static_cast<float>(0x01000000);
}

/**
 * @brief 生成 [0, 1) 的随机浮点数（2D）
 *
 * @param seed 随机数种子（会被修改）
 * @return 随机 float2
 */
__device__ __forceinline__ float2 rng2(unsigned int& seed) {
    return make_float2(rng(seed), rng(seed));
}

/**
 * @brief 生成 [0, 1) 的随机浮点数（3D）
 *
 * @param seed 随机数种子（会被修改）
 * @return 随机 float3
 */
__device__ __forceinline__ float3 rng3(unsigned int& seed) {
    return make_float3(rng(seed), rng(seed), rng(seed));
}

#endif // RANDOM_NUMBER_GENERATORS_H
