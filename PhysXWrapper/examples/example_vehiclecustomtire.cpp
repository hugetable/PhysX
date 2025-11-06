/**
 * PhysX Snippet: VehicleCustomTire
 *
 * 演示自定义轮胎模型（Custom Tire Models）
 *
 * 核心功能:
 * 1. 多种轮胎力模型
 * 2. Pacejka魔术公式
 * 3. 轮胎滑移特性
 * 4. 温度和磨损模型
 *
 * 物理背景:
 *
 * 轮胎力学（Tire Mechanics）:
 * 轮胎是车辆与地面唯一的接触点，产生驱动、制动和转向力。
 * 轮胎力学行为非常复杂，受多种因素影响。
 *
 * 滑移率（Slip Ratio）:
 *
 * 纵向滑移率:
 * κ = (ω × r - v_x) / max(|v_x|, v_min)
 * 其中:
 * - ω: 车轮角速度
 * - r: 车轮半径
 * - v_x: 车轮中心纵向速度
 *
 * 侧偏角（Slip Angle）:
 * α = arctan(v_y / |v_x|)
 * 其中:
 * - v_y: 车轮横向速度
 * - v_x: 车轮纵向速度
 *
 * Pacejka魔术公式（Magic Formula）:
 *
 * 完整形式:
 * F(x) = D × sin(C × arctan(B × x - E × (B × x - arctan(B × x))))
 *
 * 简化形式:
 * F(x) = D × sin(C × arctan(B × x))
 *
 * 参数含义:
 * - B: 刚度因子（Stiffness Factor）
 * - C: 形状因子（Shape Factor）
 * - D: 峰值因子（Peak Factor）
 * - E: 曲率因子（Curvature Factor）
 *
 * 典型值（纵向）:
 * - B_x: 10-15
 * - C_x: 1.3-1.5
 * - D_x: μ × F_z（峰值摩擦力）
 *
 * 典型值（横向）:
 * - B_y: 8-12
 * - C_y: 1.0-1.3
 * - D_y: μ × F_z
 *
 * 简化线性模型:
 *
 * 纵向力:
 * F_x = C_x × κ  (小滑移率)
 * F_x = μ × F_z × sign(κ)  (大滑移率，饱和)
 *
 * 横向力:
 * F_y = C_y × α  (小侧偏角)
 * F_y = μ × F_z × sign(α)  (大侧偏角，饱和)
 *
 * 其中 C_x, C_y 是侧偏刚度
 *
 * 摩擦圆模型（Friction Circle）:
 *
 * 轮胎总抓地力受限:
 * √(F_x² + F_y²) ≤ μ × F_z
 *
 * 纵向和横向力相互竞争:
 * F_x_max = √((μ × F_z)² - F_y²)
 * F_y_max = √((μ × F_z)² - F_x²)
 *
 * 载荷敏感性（Load Sensitivity）:
 *
 * 实际摩擦系数随载荷降低:
 * μ_actual = μ_0 × (F_z / F_z_nominal)^(-0.1 to -0.3)
 *
 * 轮胎温度模型:
 *
 * 热生成:
 * Q = |F_x × v_slip_x| + |F_y × v_slip_y|
 *
 * 温度演化:
 * dT/dt = (Q - h × (T - T_ambient)) / (m × c_p)
 *
 * 温度对摩擦的影响:
 * μ(T) = μ_0 × (1 + k_T × (T - T_optimal))
 *
 * 轮胎磨损模型:
 *
 * 磨损率:
 * dW/dt = k_wear × |F_friction| × |v_slip|
 *
 * 磨损对性能的影响:
 * μ(W) = μ_0 × (1 - k_W × W)
 *
 * 应用场景:
 * 1. 赛车仿真（精确操控）
 * 2. 驾驶培训（滑移感知）
 * 3. 车辆设计（轮胎选型）
 * 4. 安全测试（极限工况）
 *
 * 注意:
 * ⚠️ Pacejka公式参数需要实验标定
 * ⚠️ 温度和磨损模型需要大量测试数据
 * ⚠️ 不同路面摩擦系数差异大
 */

#include <PxPhysicsAPI.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace physx;

// ============================================================================
// 数据结构
// ============================================================================

/**
 * 轮胎模型类型
 */
enum class TireModelType {
    Linear,         // 线性模型（简单）
    SimplePacejka,  // 简化Pacejka公式
    FullPacejka,    // 完整Pacejka公式
    FrictionCircle  // 摩擦圆模型
};

/**
 * Pacejka魔术公式参数
 */
struct PacejkaParams {
    // 纵向参数
    PxReal B_x;  // 刚度因子
    PxReal C_x;  // 形状因子
    PxReal D_x;  // 峰值因子
    PxReal E_x;  // 曲率因子

    // 横向参数
    PxReal B_y;
    PxReal C_y;
    PxReal D_y;
    PxReal E_y;

    PacejkaParams() {
        // 默认值（公路轮胎）
        B_x = 12.0f;
        C_x = 1.4f;
        D_x = 1.0f;  // 乘以 μ × F_z
        E_x = 0.5f;

        B_y = 10.0f;
        C_y = 1.2f;
        D_y = 1.0f;
        E_y = -0.5f;
    }
};

/**
 * 轮胎状态
 */
struct TireState {
    PxReal slipRatio;        // 滑移率 κ
    PxReal slipAngle;        // 侧偏角 α（弧度）
    PxReal normalForce;      // 法向力 F_z
    PxReal longitudinalForce; // 纵向力 F_x
    PxReal lateralForce;     // 横向力 F_y
    PxReal temperature;      // 温度（℃）
    PxReal wear;             // 磨损（0-1）

    TireState() : slipRatio(0), slipAngle(0), normalForce(0),
                  longitudinalForce(0), lateralForce(0),
                  temperature(20.0f), wear(0.0f) {}
};

/**
 * 轮胎配置
 */
struct TireConfig {
    TireModelType modelType;
    PacejkaParams pacejka;

    // 物理参数
    PxReal radius;           // 半径
    PxReal width;            // 宽度
    PxReal mass;             // 质量
    PxReal inertia;          // 转动惯量

    // 摩擦参数
    PxReal peakFriction;     // 峰值摩擦系数
    PxReal slideFriction;    // 滑动摩擦系数
    PxReal cornerStiffness;  // 侧偏刚度（N/rad）
    PxReal longStiffness;    // 纵向刚度（N）

    // 载荷敏感性
    PxReal loadSensitivity;  // -0.1 to -0.3

    // 温度模型
    PxReal thermalCapacity;  // 热容（J/K）
    PxReal heatTransferCoeff; // 传热系数（W/K）
    PxReal optimalTemp;      // 最优温度（℃）
    PxReal tempFrictionCoeff; // 温度影响系数

    // 磨损模型
    PxReal wearCoefficient;  // 磨损系数
    PxReal wearFrictionCoeff; // 磨损影响系数

    TireConfig() {
        modelType = TireModelType::SimplePacejka;
        radius = 0.35f;
        width = 0.225f;
        mass = 10.0f;
        inertia = mass * radius * radius * 0.5f;

        peakFriction = 1.0f;
        slideFriction = 0.8f;
        cornerStiffness = 50000.0f;
        longStiffness = 100000.0f;
        loadSensitivity = -0.15f;

        thermalCapacity = 1000.0f;
        heatTransferCoeff = 50.0f;
        optimalTemp = 80.0f;
        tempFrictionCoeff = 0.002f;

        wearCoefficient = 0.00001f;
        wearFrictionCoeff = 0.3f;
    }
};

// ============================================================================
// 轮胎力计算
// ============================================================================

/**
 * 计算载荷敏感的摩擦系数
 */
PxReal calculateLoadSensitiveFriction(PxReal baseFriction, PxReal normalForce,
                                      PxReal nominalLoad, PxReal sensitivity) {
    PxReal loadRatio = normalForce / nominalLoad;
    return baseFriction * PxPow(loadRatio, sensitivity);
}

/**
 * Pacejka魔术公式（简化版）
 */
PxReal pacejkaSimple(PxReal input, PxReal B, PxReal C, PxReal D) {
    return D * PxSin(C * PxAtan(B * input));
}

/**
 * Pacejka魔术公式（完整版）
 */
PxReal pacejkaFull(PxReal input, PxReal B, PxReal C, PxReal D, PxReal E) {
    PxReal x = B * input;
    PxReal arctan_x = PxAtan(x);
    return D * PxSin(C * PxAtan(x - E * (x - arctan_x)));
}

/**
 * 线性轮胎模型
 */
void calculateLinearTireForces(TireState& state, const TireConfig& config) {
    // 纵向力
    if (PxAbs(state.slipRatio) < 0.1f) {
        // 线性区域
        state.longitudinalForce = config.longStiffness * state.slipRatio;
    } else {
        // 饱和区域
        state.longitudinalForce = config.peakFriction * state.normalForce * PxSign(state.slipRatio);
    }

    // 横向力
    if (PxAbs(state.slipAngle) < 0.1f) {
        // 线性区域
        state.lateralForce = -config.cornerStiffness * state.slipAngle;
    } else {
        // 饱和区域
        state.lateralForce = -config.peakFriction * state.normalForce * PxSign(state.slipAngle);
    }
}

/**
 * Pacejka轮胎模型（简化）
 */
void calculatePacejkaTireForces(TireState& state, const TireConfig& config) {
    PxReal effectiveFriction = config.peakFriction;

    // 考虑温度影响
    effectiveFriction *= (1.0f + config.tempFrictionCoeff * (state.temperature - config.optimalTemp));

    // 考虑磨损影响
    effectiveFriction *= (1.0f - config.wearFrictionCoeff * state.wear);

    // 载荷敏感性
    PxReal nominalLoad = 4000.0f;  // 假设标称载荷4000N
    effectiveFriction = calculateLoadSensitiveFriction(effectiveFriction, state.normalForce,
                                                        nominalLoad, config.loadSensitivity);

    // 纵向力（Pacejka）
    PxReal D_x = config.pacejka.D_x * effectiveFriction * state.normalForce;
    state.longitudinalForce = pacejkaSimple(state.slipRatio,
                                             config.pacejka.B_x,
                                             config.pacejka.C_x,
                                             D_x);

    // 横向力（Pacejka）
    PxReal D_y = config.pacejka.D_y * effectiveFriction * state.normalForce;
    state.lateralForce = -pacejkaSimple(state.slipAngle,
                                         config.pacejka.B_y,
                                         config.pacejka.C_y,
                                         D_y);
}

/**
 * 摩擦圆模型
 */
void applyFrictionCircle(TireState& state, const TireConfig& config) {
    PxReal maxFriction = config.peakFriction * state.normalForce;
    PxReal totalForce = PxSqrt(state.longitudinalForce * state.longitudinalForce +
                                state.lateralForce * state.lateralForce);

    if (totalForce > maxFriction) {
        PxReal scale = maxFriction / totalForce;
        state.longitudinalForce *= scale;
        state.lateralForce *= scale;
    }
}

/**
 * 更新轮胎温度
 */
void updateTireTemperature(TireState& state, const TireConfig& config, PxReal dt,
                           PxReal longitudinalSlipVel, PxReal lateralSlipVel) {
    // 热生成（摩擦功）
    PxReal heatGeneration = PxAbs(state.longitudinalForce * longitudinalSlipVel) +
                            PxAbs(state.lateralForce * lateralSlipVel);

    // 热传递
    PxReal ambientTemp = 20.0f;
    PxReal heatLoss = config.heatTransferCoeff * (state.temperature - ambientTemp);

    // 温度变化
    PxReal dT = (heatGeneration - heatLoss) / config.thermalCapacity;
    state.temperature += dT * dt;

    // 限制温度范围
    state.temperature = PxClamp(state.temperature, 0.0f, 150.0f);
}

/**
 * 更新轮胎磨损
 */
void updateTireWear(TireState& state, const TireConfig& config, PxReal dt,
                    PxReal slipVelocityMag) {
    PxReal frictionForce = PxSqrt(state.longitudinalForce * state.longitudinalForce +
                                   state.lateralForce * state.lateralForce);

    PxReal wearRate = config.wearCoefficient * frictionForce * slipVelocityMag;
    state.wear += wearRate * dt;

    // 限制磨损范围
    state.wear = PxClamp(state.wear, 0.0f, 1.0f);
}

// ============================================================================
// 场景示例
// ============================================================================

/**
 * 场景1: 轮胎模型对比
 */
void demonstrateTireModelComparison() {
    std::cout << "\n=== 场景1: 轮胎模型对比 ===" << std::endl;
    std::cout << "对比不同滑移率下的纵向力" << std::endl;

    TireConfig configLinear, configPacejka;
    configLinear.modelType = TireModelType::Linear;
    configPacejka.modelType = TireModelType::SimplePacejka;

    std::cout << "\n滑移率\t线性模型(N)\tPacejka模型(N)" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;

    for (int i = 0; i <= 20; ++i) {
        PxReal slipRatio = i * 0.05f;  // 0到1.0

        TireState stateLinear, statePacejka;
        stateLinear.slipRatio = slipRatio;
        statePacejka.slipRatio = slipRatio;
        stateLinear.normalForce = 4000.0f;
        statePacejka.normalForce = 4000.0f;

        calculateLinearTireForces(stateLinear, configLinear);
        calculatePacejkaTireForces(statePacejka, configPacejka);

        std::cout << slipRatio << "\t"
                  << stateLinear.longitudinalForce << "\t\t"
                  << statePacejka.longitudinalForce << std::endl;
    }

    std::cout << "\n特性对比:" << std::endl;
    std::cout << "- 线性模型：小滑移率准确，大滑移率直接饱和" << std::endl;
    std::cout << "- Pacejka模型：平滑过渡，峰值后逐渐下降（真实）" << std::endl;
}

/**
 * 场景2: 摩擦圆效应
 */
void demonstrateFrictionCircle() {
    std::cout << "\n=== 场景2: 摩擦圆效应 ===" << std::endl;
    std::cout << "演示纵向力和横向力的相互影响" << std::endl;

    TireConfig config;
    config.peakFriction = 1.0f;

    std::cout << "\n横向力(N)\t纵向力最大值(N)" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    for (int i = 0; i <= 10; ++i) {
        PxReal lateralForce = i * 400.0f;  // 0到4000N

        TireState state;
        state.normalForce = 4000.0f;
        state.lateralForce = lateralForce;
        state.longitudinalForce = 4000.0f;  // 尝试最大值

        applyFrictionCircle(state, config);

        std::cout << lateralForce << "\t\t"
                  << state.longitudinalForce << std::endl;
    }

    std::cout << "\n摩擦圆特性:" << std::endl;
    std::cout << "- 转向时（横向力增加），可用驱动力减少" << std::endl;
    std::cout << "- 总摩擦力受限: √(F_x² + F_y²) ≤ μ × F_z" << std::endl;
    std::cout << "- 赛车手必须平衡刹车和转向" << std::endl;
}

/**
 * 场景3: 轮胎温度演化
 */
void demonstrateTireTemperature() {
    std::cout << "\n=== 场景3: 轮胎温度演化 ===" << std::endl;
    std::cout << "模拟赛车轮胎从冷胎到工作温度" << std::endl;

    TireConfig config;
    TireState state;
    state.normalForce = 5000.0f;  // 赛车载荷
    state.temperature = 20.0f;    // 环境温度

    const PxReal dt = 0.1f;  // 100ms
    const int numSteps = 300;  // 30秒

    std::cout << "\n时间(s)\t温度(℃)\t摩擦系数" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    for (int step = 0; step < numSteps; ++step) {
        // 模拟激烈驾驶：高滑移速度
        PxReal slipVelLong = 5.0f;  // 5 m/s滑移
        PxReal slipVelLat = 3.0f;   // 3 m/s横向滑移

        state.slipRatio = 0.2f;
        state.slipAngle = 0.1f;

        calculatePacejkaTireForces(state, config);
        updateTireTemperature(state, config, dt, slipVelLong, slipVelLat);

        PxReal effectiveFriction = config.peakFriction *
                                   (1.0f + config.tempFrictionCoeff * (state.temperature - config.optimalTemp));

        if (step % 30 == 0) {
            std::cout << step * dt << "\t"
                      << state.temperature << "\t\t"
                      << effectiveFriction << std::endl;
        }
    }

    std::cout << "\n温度影响:" << std::endl;
    std::cout << "- 冷胎：摩擦低，需要预热（Warm-up Lap）" << std::endl;
    std::cout << "- 最优温度（~80℃）：摩擦峰值" << std::endl;
    std::cout << "- 过热（>100℃）：摩擦下降，轮胎退化" << std::endl;
}

/**
 * 场景4: 轮胎磨损影响
 */
void demonstrateTireWear() {
    std::cout << "\n=== 场景4: 轮胎磨损影响 ===" << std::endl;
    std::cout << "模拟长时间使用后的轮胎性能衰减" << std::endl;

    TireConfig config;
    TireState state;
    state.normalForce = 4000.0f;

    const PxReal dt = 0.1f;
    const int numSteps = 5000;  // 模拟500秒（约8分钟）

    std::cout << "\n磨损度\t摩擦系数\t纵向力(N)" << std::endl;
    std::cout << "---------------------------------------" << std::endl;

    for (int step = 0; step < numSteps; ++step) {
        // 模拟正常驾驶
        state.slipRatio = 0.05f;
        state.slipAngle = 0.02f;

        calculatePacejkaTireForces(state, config);

        PxReal slipVel = 2.0f;  // 平均滑移速度
        updateTireWear(state, config, dt, slipVel);

        if (step % 500 == 0) {
            PxReal effectiveFriction = config.peakFriction *
                                       (1.0f - config.wearFrictionCoeff * state.wear);

            std::cout << state.wear << "\t"
                      << effectiveFriction << "\t\t"
                      << state.longitudinalForce << std::endl;
        }
    }

    std::cout << "\n磨损影响:" << std::endl;
    std::cout << "- 新轮胎：最大抓地力" << std::endl;
    std::cout << "- 磨损增加：摩擦系数下降，制动距离增加" << std::endl;
    std::cout << "- 严重磨损（>0.7）：危险，建议更换" << std::endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "PhysX Snippet: VehicleCustomTire" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "\n演示自定义轮胎模型" << std::endl;

    // 运行4个场景
    demonstrateTireModelComparison();
    demonstrateFrictionCircle();
    demonstrateTireTemperature();
    demonstrateTireWear();

    std::cout << "\n=== 总结 ===" << std::endl;
    std::cout << "\n轮胎模型对比:" << std::endl;
    std::cout << "┌────────────────┬──────────┬──────────┬──────────┐" << std::endl;
    std::cout << "│ 模型           │ 精度     │ 性能     │ 应用     │" << std::endl;
    std::cout << "├────────────────┼──────────┼──────────┼──────────┤" << std::endl;
    std::cout << "│ 线性模型       │ 低       │ 很高     │ 游戏     │" << std::endl;
    std::cout << "│ 简化Pacejka    │ 中       │ 高       │ 仿真     │" << std::endl;
    std::cout << "│ 完整Pacejka    │ 高       │ 中       │ 赛车     │" << std::endl;
    std::cout << "│ 摩擦圆         │ 中       │ 高       │ 街机     │" << std::endl;
    std::cout << "└────────────────┴──────────┴──────────┴──────────┘" << std::endl;

    std::cout << "\n关键公式:" << std::endl;
    std::cout << "滑移率: κ = (ω × r - v_x) / |v_x|" << std::endl;
    std::cout << "侧偏角: α = arctan(v_y / v_x)" << std::endl;
    std::cout << "Pacejka: F = D × sin(C × arctan(B × x - E × (B × x - arctan(B × x))))" << std::endl;
    std::cout << "摩擦圆: √(F_x² + F_y²) ≤ μ × F_z" << std::endl;

    std::cout << "\n工程应用:" << std::endl;
    std::cout << "1. 赛车仿真 - 完整Pacejka，实时温度/磨损" << std::endl;
    std::cout << "2. 驾驶培训 - 简化模型，强调滑移感知" << std::endl;
    std::cout << "3. 车辆调校 - 轮胎参数标定，性能优化" << std::endl;
    std::cout << "4. 安全测试 - 极限工况，ABS/ESP开发" << std::endl;

    std::cout << "\n⚠️ 注意事项:" << std::endl;
    std::cout << "- Pacejka参数需要实验室标定" << std::endl;
    std::cout << "- 温度模型需考虑路面和气候" << std::endl;
    std::cout << "- 磨损模型依赖驾驶风格和路况" << std::endl;
    std::cout << "- 不同品牌轮胎特性差异很大" << std::endl;

    return 0;
}
