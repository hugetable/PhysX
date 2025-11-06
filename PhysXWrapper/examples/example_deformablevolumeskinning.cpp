/**
 * PhysX Snippet: DeformableVolumeSkinning
 *
 * 演示可变形体积的骨骼蒙皮（Soft Body Skinning）
 *
 * 核心功能:
 * 1. 软体体积绑定到骨骼系统
 * 2. 线性混合蒙皮（Linear Blend Skinning, LBS）
 * 3. 双四元数蒙皮（Dual Quaternion Skinning, DQS）
 * 4. 体积蒙皮（Volume Skinning）
 *
 * 物理背景:
 *
 * 蒙皮（Skinning）:
 * 将可变形网格绑定到骨骼层次结构的技术，使网格跟随骨骼运动。
 * 广泛应用于角色动画、软体机器人、生物力学仿真。
 *
 * 线性混合蒙皮（LBS）:
 * 顶点位置: v' = Σ w_i · T_i · v
 * 其中:
 * - w_i: 骨骼i的权重（Σw_i = 1）
 * - T_i: 骨骼i的蒙皮变换矩阵
 * - T_i = T_current · T_bind^(-1)
 *
 * 优点: 计算快速、实现简单
 * 缺点: Candy-wrapper伪影、体积损失
 *
 * 双四元数蒙皮（DQS）:
 * 使用双四元数表示刚体变换，避免线性插值的体积损失
 * q' = normalize(Σ w_i · q_i)
 * v' = q' * v * conj(q') + t'
 *
 * 优点: 保持体积、无Candy-wrapper伪影
 * 缺点: 计算稍复杂
 *
 * 体积蒙皮（Volume Skinning）:
 * 不仅蒙皮表面，还蒙皮内部体积顶点
 * 用于软体变形体的骨骼驱动动画
 *
 * 应用场景:
 * 1. 角色动画中的软体部分（脂肪、肌肉）
 * 2. 软体机器人驱动
 * 3. 医学仿真（器官、组织）
 * 4. 游戏中的角色变形
 *
 * 实现要点:
 * - 绑定姿势（Bind Pose）存储初始骨骼配置
 * - 蒙皮权重通常手动绘制或自动计算（热扩散）
 * - 每帧计算蒙皮变换并更新顶点位置
 * - 权重归一化确保 Σw_i = 1
 *
 * 注意:
 * ⚠️ 本示例演示的是骨骼驱动的软体动画，不涉及真实软体物理模拟
 * ⚠️ 对于大量顶点，建议使用GPU加速蒙皮计算
 * ⚠️ 权重平滑对避免不自然的折痕至关重要
 */

#include <PxPhysicsAPI.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <map>

using namespace physx;

// ============================================================================
// 数据结构
// ============================================================================

/**
 * 骨骼结构
 */
struct Bone {
    std::string name;
    PxTransform bindPose;       // 绑定姿势（初始姿势）
    PxTransform currentPose;    // 当前姿势
    int parentIndex;            // 父骨骼索引（-1表示根骨骼）

    Bone() : parentIndex(-1) {}
};

/**
 * 骨架（Skeleton）
 */
struct Skeleton {
    std::vector<Bone> bones;

    int addBone(const std::string& name, const PxTransform& bindPose, int parent = -1) {
        Bone bone;
        bone.name = name;
        bone.bindPose = bindPose;
        bone.currentPose = bindPose;
        bone.parentIndex = parent;
        bones.push_back(bone);
        return static_cast<int>(bones.size() - 1);
    }
};

/**
 * 顶点权重
 */
struct VertexWeight {
    int boneIndex;
    PxReal weight;

    VertexWeight(int idx, PxReal w) : boneIndex(idx), weight(w) {}
};

/**
 * 蒙皮类型
 */
enum class SkinningMethod {
    LinearBlendSkinning,        // 线性混合蒙皮
    DualQuaternionSkinning      // 双四元数蒙皮
};

/**
 * 蒙皮软体体积
 */
struct SkinnedDeformableVolume {
    // 骨架
    Skeleton skeleton;

    // 几何数据
    std::vector<PxVec3> bindPoseVertices;       // 绑定姿势的顶点
    std::vector<PxVec3> skinnedVertices;        // 蒙皮后的顶点
    std::vector<PxU32> tetrahedra;              // 四面体索引

    // 蒙皮数据
    std::vector<std::vector<VertexWeight>> vertexWeights;  // 每个顶点的骨骼权重

    // 方法
    SkinningMethod method;

    SkinnedDeformableVolume() : method(SkinningMethod::LinearBlendSkinning) {}
};

// ============================================================================
// 双四元数工具
// ============================================================================

/**
 * 双四元数（Dual Quaternion）
 * dq = q_r + ε * q_d
 * 其中 q_r 是旋转部分，q_d 是平移部分
 */
struct DualQuaternion {
    PxQuat real;       // 旋转四元数
    PxQuat dual;       // 平移四元数

    DualQuaternion() : real(PxIdentity), dual(0, 0, 0, 0) {}

    DualQuaternion(const PxTransform& transform) {
        real = transform.q;

        // dual = 0.5 * translation_quaternion * real
        PxQuat translationQuat(transform.p.x, transform.p.y, transform.p.z, 0.0f);
        dual = translationQuat * real * 0.5f;
    }

    // 双四元数加法
    DualQuaternion operator+(const DualQuaternion& other) const {
        DualQuaternion result;
        result.real = PxQuat(real.x + other.real.x, real.y + other.real.y,
                             real.z + other.real.z, real.w + other.real.w);
        result.dual = PxQuat(dual.x + other.dual.x, dual.y + other.dual.y,
                             dual.z + other.dual.z, dual.w + other.dual.w);
        return result;
    }

    // 标量乘法
    DualQuaternion operator*(PxReal scalar) const {
        DualQuaternion result;
        result.real = PxQuat(real.x * scalar, real.y * scalar, real.z * scalar, real.w * scalar);
        result.dual = PxQuat(dual.x * scalar, dual.y * scalar, dual.z * scalar, dual.w * scalar);
        return result;
    }

    // 归一化
    void normalize() {
        PxReal magnitude = PxSqrt(real.x*real.x + real.y*real.y + real.z*real.z + real.w*real.w);
        if (magnitude > 1e-6f) {
            real = PxQuat(real.x/magnitude, real.y/magnitude, real.z/magnitude, real.w/magnitude);
            dual = PxQuat(dual.x/magnitude, dual.y/magnitude, dual.z/magnitude, dual.w/magnitude);
        }
    }

    // 转换为Transform
    PxTransform toTransform() const {
        PxTransform result;
        result.q = real;

        // translation = 2 * dual * conj(real)
        PxQuat conjReal = real.getConjugate();
        PxQuat temp = dual * conjReal * 2.0f;
        result.p = PxVec3(temp.x, temp.y, temp.z);

        return result;
    }

    // 变换点
    PxVec3 transformPoint(const PxVec3& point) const {
        PxTransform t = toTransform();
        return t.transform(point);
    }
};

// ============================================================================
// 蒙皮算法
// ============================================================================

/**
 * 计算全局骨骼变换（考虑骨骼层次）
 */
PxTransform computeGlobalTransform(const Skeleton& skeleton, int boneIndex) {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(skeleton.bones.size())) {
        return PxTransform(PxIdentity);
    }

    const Bone& bone = skeleton.bones[boneIndex];

    if (bone.parentIndex < 0) {
        return bone.currentPose;
    } else {
        PxTransform parentTransform = computeGlobalTransform(skeleton, bone.parentIndex);
        return parentTransform * bone.currentPose;
    }
}

/**
 * 线性混合蒙皮
 */
void performLinearBlendSkinning(SkinnedDeformableVolume& volume) {
    for (size_t vertIdx = 0; vertIdx < volume.bindPoseVertices.size(); ++vertIdx) {
        const PxVec3& bindPos = volume.bindPoseVertices[vertIdx];
        PxVec3 skinnedPos(0, 0, 0);

        for (const auto& weight : volume.vertexWeights[vertIdx]) {
            const Bone& bone = volume.skeleton.bones[weight.boneIndex];

            // 计算蒙皮变换: T_skin = T_current * T_bind^(-1)
            PxTransform bindTransform = computeGlobalTransform(volume.skeleton, weight.boneIndex);
            PxTransform invBindPose = bone.bindPose.getInverse();
            PxTransform skinningTransform = bindTransform * invBindPose;

            // 变换顶点并加权
            PxVec3 transformedPos = skinningTransform.transform(bindPos);
            skinnedPos += transformedPos * weight.weight;
        }

        volume.skinnedVertices[vertIdx] = skinnedPos;
    }
}

/**
 * 双四元数蒙皮
 */
void performDualQuaternionSkinning(SkinnedDeformableVolume& volume) {
    for (size_t vertIdx = 0; vertIdx < volume.bindPoseVertices.size(); ++vertIdx) {
        const PxVec3& bindPos = volume.bindPoseVertices[vertIdx];

        // 混合双四元数
        DualQuaternion blendedDQ;
        bool firstWeight = true;

        for (const auto& weight : volume.vertexWeights[vertIdx]) {
            const Bone& bone = volume.skeleton.bones[weight.boneIndex];

            // 计算蒙皮变换
            PxTransform bindTransform = computeGlobalTransform(volume.skeleton, weight.boneIndex);
            PxTransform invBindPose = bone.bindPose.getInverse();
            PxTransform skinningTransform = bindTransform * invBindPose;

            DualQuaternion dq(skinningTransform);

            // 确保四元数在同一半球（避免插值走捷径）
            if (!firstWeight) {
                PxReal dot = blendedDQ.real.x * dq.real.x + blendedDQ.real.y * dq.real.y +
                             blendedDQ.real.z * dq.real.z + blendedDQ.real.w * dq.real.w;
                if (dot < 0.0f) {
                    dq = dq * (-1.0f);
                }
            }

            blendedDQ = blendedDQ + (dq * weight.weight);
            firstWeight = false;
        }

        // 归一化并变换顶点
        blendedDQ.normalize();
        volume.skinnedVertices[vertIdx] = blendedDQ.transformPoint(bindPos);
    }
}

/**
 * 执行蒙皮
 */
void performSkinning(SkinnedDeformableVolume& volume) {
    if (volume.method == SkinningMethod::LinearBlendSkinning) {
        performLinearBlendSkinning(volume);
    } else {
        performDualQuaternionSkinning(volume);
    }
}

// ============================================================================
// 几何生成
// ============================================================================

/**
 * 生成圆柱体软体体积（用于手臂等）
 */
SkinnedDeformableVolume createCylinderVolume(const PxVec3& baseCenter, PxReal radius,
                                              PxReal height, PxU32 radialSegments, PxU32 heightSegments) {
    SkinnedDeformableVolume volume;

    // 生成圆柱体顶点
    for (PxU32 h = 0; h <= heightSegments; ++h) {
        PxReal y = baseCenter.y + (height / heightSegments) * h;

        for (PxU32 r = 0; r < radialSegments; ++r) {
            PxReal angle = (2.0f * PxPi * r) / radialSegments;
            PxReal x = baseCenter.x + radius * PxCos(angle);
            PxReal z = baseCenter.z + radius * PxSin(angle);

            volume.bindPoseVertices.push_back(PxVec3(x, y, z));
            volume.skinnedVertices.push_back(PxVec3(x, y, z));
        }
    }

    // 简化：不生成完整的四面体网格（仅用于蒙皮演示）
    // 实际应用中需要完整的体积四面体化

    return volume;
}

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 简单手臂蒙皮
 */
SkinnedDeformableVolume createArmSkinningScene() {
    std::cout << "\n=== 场景1: 简单手臂蒙皮 ===" << std::endl;
    std::cout << "一个圆柱体软体绑定到2骨骼手臂（上臂+前臂）" << std::endl;

    SkinnedDeformableVolume volume = createCylinderVolume(PxVec3(0, 0, 0), 0.3f, 2.0f, 8, 10);

    // 创建骨架：上臂 + 前臂
    int upperArmBone = volume.skeleton.addBone("UpperArm", PxTransform(PxVec3(0, 1, 0)), -1);
    int foreArmBone = volume.skeleton.addBone("ForeArm", PxTransform(PxVec3(0, 2, 0)), upperArmBone);

    // 设置顶点权重（基于高度自动分配）
    volume.vertexWeights.resize(volume.bindPoseVertices.size());

    for (size_t i = 0; i < volume.bindPoseVertices.size(); ++i) {
        PxReal y = volume.bindPoseVertices[i].y;

        // 基于高度线性插值权重
        PxReal t = PxClamp(y / 2.0f, 0.0f, 1.0f);
        PxReal upperWeight = 1.0f - t;
        PxReal foreWeight = t;

        if (upperWeight > 0.01f) {
            volume.vertexWeights[i].push_back(VertexWeight(upperArmBone, upperWeight));
        }
        if (foreWeight > 0.01f) {
            volume.vertexWeights[i].push_back(VertexWeight(foreArmBone, foreWeight));
        }
    }

    volume.method = SkinningMethod::LinearBlendSkinning;

    std::cout << "骨架: 2个骨骼（上臂、前臂）" << std::endl;
    std::cout << "顶点: " << volume.bindPoseVertices.size() << "个" << std::endl;
    std::cout << "蒙皮方法: 线性混合蒙皮（LBS）" << std::endl;

    return volume;
}

/**
 * 场景2: 旋转动画对比LBS vs DQS
 */
void demonstrateSkinningComparison() {
    std::cout << "\n=== 场景2: LBS vs DQS 对比 ===" << std::endl;
    std::cout << "演示两种蒙皮方法在180度旋转时的差异" << std::endl;

    // 创建LBS版本
    SkinnedDeformableVolume volumeLBS = createCylinderVolume(PxVec3(-2, 0, 0), 0.25f, 1.5f, 8, 8);
    volumeLBS.skeleton.addBone("Bone0", PxTransform(PxVec3(-2, 0.5f, 0)), -1);
    volumeLBS.skeleton.addBone("Bone1", PxTransform(PxVec3(-2, 1.0f, 0)), 0);

    // 设置权重
    volumeLBS.vertexWeights.resize(volumeLBS.bindPoseVertices.size());
    for (size_t i = 0; i < volumeLBS.bindPoseVertices.size(); ++i) {
        PxReal y = volumeLBS.bindPoseVertices[i].y;
        PxReal t = PxClamp((y + 2.0f) / 1.5f, 0.0f, 1.0f);
        volumeLBS.vertexWeights[i].push_back(VertexWeight(0, 1.0f - t));
        volumeLBS.vertexWeights[i].push_back(VertexWeight(1, t));
    }
    volumeLBS.method = SkinningMethod::LinearBlendSkinning;

    // 创建DQS版本（完全相同的几何和权重）
    SkinnedDeformableVolume volumeDQS = volumeLBS;
    volumeDQS.method = SkinningMethod::DualQuaternionSkinning;

    // 应用180度旋转到Bone1
    PxQuat rotation180(PxPi, PxVec3(0, 0, 1));  // 绕Z轴旋转180度
    volumeLBS.skeleton.bones[1].currentPose.q = rotation180;
    volumeDQS.skeleton.bones[1].currentPose.q = rotation180;

    // 执行蒙皮
    performSkinning(volumeLBS);
    performSkinning(volumeDQS);

    // 比较中间顶点（最容易出现Candy-wrapper伪影的位置）
    size_t midVertexIdx = volumeLBS.bindPoseVertices.size() / 2;

    std::cout << "\n中间顶点位置比较（180度旋转）:" << std::endl;
    std::cout << "原始位置: (" << volumeLBS.bindPoseVertices[midVertexIdx].x << ", "
              << volumeLBS.bindPoseVertices[midVertexIdx].y << ", "
              << volumeLBS.bindPoseVertices[midVertexIdx].z << ")" << std::endl;

    std::cout << "LBS蒙皮后: (" << volumeLBS.skinnedVertices[midVertexIdx].x << ", "
              << volumeLBS.skinnedVertices[midVertexIdx].y << ", "
              << volumeLBS.skinnedVertices[midVertexIdx].z << ")" << std::endl;

    std::cout << "DQS蒙皮后: (" << volumeDQS.skinnedVertices[midVertexIdx].x << ", "
              << volumeDQS.skinnedVertices[midVertexIdx].y << ", "
              << volumeDQS.skinnedVertices[midVertexIdx].z << ")" << std::endl;

    // 计算体积（粗略估计）
    PxReal volumeLBS_estimate = 0.0f;
    PxReal volumeDQS_estimate = 0.0f;
    for (const auto& v : volumeLBS.skinnedVertices) {
        volumeLBS_estimate += v.magnitudeSquared();
    }
    for (const auto& v : volumeDQS.skinnedVertices) {
        volumeDQS_estimate += v.magnitudeSquared();
    }

    std::cout << "\n体积估计（越接近原始越好）:" << std::endl;
    std::cout << "LBS: " << volumeLBS_estimate << std::endl;
    std::cout << "DQS: " << volumeDQS_estimate << std::endl;
    std::cout << "\n结论: DQS在大旋转时保持体积更好，避免了Candy-wrapper伪影" << std::endl;
}

/**
 * 场景3: 多骨骼手臂动画
 */
void demonstrateMultiBoneAnimation() {
    std::cout << "\n=== 场景3: 多骨骼手臂动画 ===" << std::endl;
    std::cout << "3骨骼手臂（肩、肘、腕）执行弯曲动画" << std::endl;

    SkinnedDeformableVolume volume = createCylinderVolume(PxVec3(0, 0, 0), 0.25f, 3.0f, 8, 15);

    // 创建3骨骼骨架
    int shoulderBone = volume.skeleton.addBone("Shoulder", PxTransform(PxVec3(0, 0.5f, 0)), -1);
    int elbowBone = volume.skeleton.addBone("Elbow", PxTransform(PxVec3(0, 1.5f, 0)), shoulderBone);
    int wristBone = volume.skeleton.addBone("Wrist", PxTransform(PxVec3(0, 2.5f, 0)), elbowBone);

    // 设置权重（3段，每段主要受一个骨骼影响）
    volume.vertexWeights.resize(volume.bindPoseVertices.size());
    for (size_t i = 0; i < volume.bindPoseVertices.size(); ++i) {
        PxReal y = volume.bindPoseVertices[i].y;

        if (y < 1.0f) {
            // 肩部区域
            volume.vertexWeights[i].push_back(VertexWeight(shoulderBone, 1.0f - y));
            volume.vertexWeights[i].push_back(VertexWeight(elbowBone, y));
        } else if (y < 2.0f) {
            // 肘部区域
            PxReal t = y - 1.0f;
            volume.vertexWeights[i].push_back(VertexWeight(elbowBone, 1.0f - t));
            volume.vertexWeights[i].push_back(VertexWeight(wristBone, t));
        } else {
            // 腕部区域
            volume.vertexWeights[i].push_back(VertexWeight(wristBone, 1.0f));
        }
    }

    volume.method = SkinningMethod::DualQuaternionSkinning;

    // 执行弯曲动画（5个关键帧）
    std::cout << "\n执行弯曲动画（5帧）:" << std::endl;

    for (int frame = 0; frame <= 4; ++frame) {
        PxReal t = frame / 4.0f;  // 0到1

        // 肘关节弯曲90度
        PxReal elbowAngle = t * PxPiDivTwo;
        volume.skeleton.bones[elbowBone].currentPose.q = PxQuat(elbowAngle, PxVec3(1, 0, 0));

        // 腕关节弯曲45度
        PxReal wristAngle = t * PxPiDivFour;
        volume.skeleton.bones[wristBone].currentPose.q = PxQuat(wristAngle, PxVec3(0, 0, 1));

        // 执行蒙皮
        performSkinning(volume);

        // 输出末端位置
        size_t tipVertexIdx = volume.bindPoseVertices.size() - 1;
        std::cout << "帧 " << frame << ": 手腕末端位置 ("
                  << volume.skinnedVertices[tipVertexIdx].x << ", "
                  << volume.skinnedVertices[tipVertexIdx].y << ", "
                  << volume.skinnedVertices[tipVertexIdx].z << ")" << std::endl;
    }

    std::cout << "\n动画完成：手臂从伸直状态弯曲到肘90度+腕45度" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: DeformableVolumeSkinning" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "\n演示可变形体积的骨骼蒙皮技术" << std::endl;

    // 初始化PhysX（虽然这个示例主要是数学计算，不需要完整的PhysX场景）
    PxDefaultAllocator allocator;
    PxDefaultErrorCallback errorCallback;

    PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
    if (!foundation) {
        std::cerr << "PxCreateFoundation failed!" << std::endl;
        return 1;
    }

    // 运行3个场景
    SkinnedDeformableVolume armScene = createArmSkinningScene();

    // 测试手臂弯曲
    std::cout << "\n测试手臂弯曲动画（前臂旋转45度）:" << std::endl;
    armScene.skeleton.bones[1].currentPose.q = PxQuat(PxPiDivFour, PxVec3(1, 0, 0));
    performSkinning(armScene);

    size_t midVertex = armScene.bindPoseVertices.size() / 2;
    std::cout << "中间顶点移动: ("
              << armScene.bindPoseVertices[midVertex].x << ", "
              << armScene.bindPoseVertices[midVertex].y << ", "
              << armScene.bindPoseVertices[midVertex].z << ") -> ("
              << armScene.skinnedVertices[midVertex].x << ", "
              << armScene.skinnedVertices[midVertex].y << ", "
              << armScene.skinnedVertices[midVertex].z << ")" << std::endl;

    // 运行场景2和3
    demonstrateSkinningComparison();
    demonstrateMultiBoneAnimation();

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\n蒙皮技术对比:" << std::endl;
    std::cout << "┌─────────────────┬──────────────┬────────────────────┐" << std::endl;
    std::cout << "│ 蒙皮方法        │ 优点         │ 缺点               │" << std::endl;
    std::cout << "├─────────────────┼──────────────┼────────────────────┤" << std::endl;
    std::cout << "│ 线性混合(LBS)   │ 快速、简单   │ 体积损失、伪影     │" << std::endl;
    std::cout << "│ 双四元数(DQS)   │ 保持体积好   │ 稍复杂             │" << std::endl;
    std::cout << "└─────────────────┴──────────────┴────────────────────┘" << std::endl;

    std::cout << "\n应用建议:" << std::endl;
    std::cout << "- 小旋转（<30度）：LBS足够，性能更好" << std::endl;
    std::cout << "- 大旋转（>60度）：推荐DQS，避免Candy-wrapper伪影" << std::endl;
    std::cout << "- 实时角色动画：现代游戏引擎多使用DQS" << std::endl;
    std::cout << "- GPU实现：两种方法都可高效并行化" << std::endl;

    std::cout << "\n关键公式:" << std::endl;
    std::cout << "LBS: v' = Σ w_i · (T_current · T_bind^(-1)) · v" << std::endl;
    std::cout << "DQS: v' = normalize(Σ w_i · dq_i).transform(v)" << std::endl;

    // 清理
    foundation->release();

    return 0;
}
