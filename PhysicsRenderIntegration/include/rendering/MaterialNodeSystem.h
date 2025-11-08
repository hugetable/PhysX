// MaterialNodeSystem.h - 材质节点系统
// 节点式材质图，类似于 Blender/Substance

#ifndef MATERIAL_NODE_SYSTEM_H
#define MATERIAL_NODE_SYSTEM_H

#include <cuda.h>
#include <vector>
#include <string>
#include <map>
#include <memory>

namespace PhysicsRender {

/**
 * @brief 节点类型
 */
enum class MaterialNodeType {
    // 输入节点
    TEXTURE_IMAGE,          // 图像纹理
    TEXTURE_COORDINATE,     // 纹理坐标
    GEOMETRY_INFO,          // 几何信息（法线、位置等）
    OBJECT_INFO,            // 对象信息

    // 着色器节点
    BSDF_PRINCIPLED,        // Disney Principled BSDF
    BSDF_DIFFUSE,           // 漫反射 BSDF
    BSDF_GLOSSY,            // 光泽 BSDF
    BSDF_GLASS,             // 玻璃 BSDF
    BSDF_TRANSPARENT,       // 透明 BSDF
    EMISSION,               // 发光

    // 混合节点
    MIX_SHADER,             // 混合着色器
    ADD_SHADER,             // 相加着色器

    // 颜色节点
    MIX_RGB,                // 混合 RGB
    RGB_CURVES,             // RGB 曲线
    HUE_SATURATION,         // 色相饱和度
    BRIGHTNESS_CONTRAST,    // 亮度对比度
    INVERT,                 // 反转

    // 矢量节点
    NORMAL_MAP,             // 法线贴图
    BUMP,                   // 凹凸贴图
    MAPPING,                // 映射
    VECTOR_MATH,            // 矢量数学

    // 转换节点
    MATH,                   // 数学运算
    SEPARATE_RGB,           // 分离 RGB
    COMBINE_RGB,            // 组合 RGB
    SEPARATE_XYZ,           // 分离 XYZ
    COMBINE_XYZ,            // 组合 XYZ

    // 程序化纹理
    NOISE_TEXTURE,          // 噪声纹理
    VORONOI_TEXTURE,        // Voronoi 纹理
    GRADIENT_TEXTURE,       // 渐变纹理
    CHECKER_TEXTURE,        // 棋盘纹理

    // 输出节点
    MATERIAL_OUTPUT         // 材质输出
};

/**
 * @brief 节点输入/输出槽类型
 */
enum class SocketType {
    FLOAT,
    FLOAT3,  // RGB 或 Vector
    BSDF,    // 着色器
    FLOAT4   // RGBA
};

/**
 * @brief 节点槽（输入/输出）
 */
struct MaterialNodeSocket {
    std::string name;
    SocketType type;

    // 默认值
    union {
        float floatValue;
        float3 float3Value;
        float4 float4Value;
    } defaultValue;

    // 连接信息
    int connectedNodeID = -1;       // 连接的节点 ID
    int connectedSocketIndex = -1;  // 连接的槽索引
    bool isConnected = false;
};

/**
 * @brief 材质节点基类
 */
struct MaterialNode {
    int nodeID = -1;
    MaterialNodeType type;
    std::string name;

    std::vector<MaterialNodeSocket> inputs;
    std::vector<MaterialNodeSocket> outputs;

    // 节点特定参数（使用 union 节省内存）
    union NodeParams {
        // Principled BSDF
        struct {
            float metallic;
            float roughness;
            float ior;
            float transmission;
            float specular;
        } principled;

        // 纹理
        struct {
            cudaTextureObject_t texture;
            int colorSpace;  // 0=sRGB, 1=Linear, 2=Non-Color
        } texture;

        // 噪声
        struct {
            int octaves;
            float scale;
            float detail;
            float distortion;
        } noise;

        // 数学运算
        struct {
            int operation;  // 0=Add, 1=Subtract, 2=Multiply, etc.
            bool clamp;
        } math;

        // Mix
        struct {
            int blendMode;  // 0=Mix, 1=Add, 2=Multiply, etc.
            bool clamp;
        } mix;

    } params;

    // GPU 数据索引
    int gpuDataIndex = -1;
};

/**
 * @brief 材质图
 */
class MaterialGraph {
public:
    /**
     * @brief 构造函数
     */
    MaterialGraph();

    /**
     * @brief 析构函数
     */
    ~MaterialGraph();

    /**
     * @brief 添加节点
     * @param type 节点类型
     * @param name 节点名称
     * @return 节点 ID
     */
    int addNode(MaterialNodeType type, const std::string& name = "");

    /**
     * @brief 移除节点
     * @param nodeID 节点 ID
     */
    void removeNode(int nodeID);

    /**
     * @brief 连接节点
     * @param fromNodeID 源节点 ID
     * @param fromSocketIndex 源槽索引
     * @param toNodeID 目标节点 ID
     * @param toSocketIndex 目标槽索引
     * @return 是否成功
     */
    bool connectNodes(
        int fromNodeID,
        int fromSocketIndex,
        int toNodeID,
        int toSocketIndex
    );

    /**
     * @brief 断开连接
     * @param nodeID 节点 ID
     * @param socketIndex 槽索引
     */
    void disconnectSocket(int nodeID, int socketIndex);

    /**
     * @brief 设置节点参数
     * @param nodeID 节点 ID
     * @param paramName 参数名
     * @param value 值
     */
    void setNodeParameter(int nodeID, const std::string& paramName, float value);
    void setNodeParameter(int nodeID, const std::string& paramName, const float3& value);

    /**
     * @brief 设置输入默认值
     * @param nodeID 节点 ID
     * @param socketIndex 槽索引
     * @param value 值
     */
    void setInputDefault(int nodeID, int socketIndex, float value);
    void setInputDefault(int nodeID, int socketIndex, const float3& value);

    /**
     * @brief 编译材质图
     * @return 是否成功
     */
    bool compile();

    /**
     * @brief 上传到 GPU
     * @return GPU 数据指针
     */
    CUdeviceptr uploadToGPU();

    /**
     * @brief 从文件加载材质图
     * @param filepath 文件路径（JSON 格式）
     * @return 是否成功
     */
    bool loadFromFile(const char* filepath);

    /**
     * @brief 保存材质图到文件
     * @param filepath 文件路径（JSON 格式）
     * @return 是否成功
     */
    bool saveToFile(const char* filepath) const;

    /**
     * @brief 获取节点
     */
    MaterialNode* getNode(int nodeID);
    const MaterialNode* getNode(int nodeID) const;

    /**
     * @brief 获取所有节点
     */
    const std::vector<MaterialNode>& getNodes() const { return nodes_; }

    /**
     * @brief 验证材质图（检查循环依赖等）
     */
    bool validate() const;

private:
    std::vector<MaterialNode> nodes_;
    int nextNodeID_;

    // 编译后的数据
    CUdeviceptr d_graphData_;
    bool compiled_;

    /**
     * @brief 拓扑排序（用于评估顺序）
     */
    std::vector<int> topologicalSort() const;

    /**
     * @brief 检测循环依赖
     */
    bool hasCycle() const;

    /**
     * @brief 创建节点的默认槽
     */
    void createDefaultSockets(MaterialNode& node);
};

/**
 * @brief 材质图管理器
 */
class MaterialGraphManager {
public:
    /**
     * @brief 构造函数
     */
    MaterialGraphManager();

    /**
     * @brief 析构函数
     */
    ~MaterialGraphManager();

    /**
     * @brief 创建新材质图
     * @param name 名称
     * @return 材质图 ID
     */
    int createGraph(const std::string& name);

    /**
     * @brief 移除材质图
     * @param graphID 材质图 ID
     */
    void removeGraph(int graphID);

    /**
     * @brief 获取材质图
     */
    MaterialGraph* getGraph(int graphID);
    const MaterialGraph* getGraph(int graphID) const;

    /**
     * @brief 编译所有材质图
     */
    bool compileAll();

    /**
     * @brief 上传所有材质图到 GPU
     */
    void uploadAllToGPU();

    /**
     * @brief 加载材质库
     * @param directory 目录路径
     * @return 加载的材质数量
     */
    int loadMaterialLibrary(const char* directory);

private:
    std::map<int, std::unique_ptr<MaterialGraph>> graphs_;
    int nextGraphID_;
};

/**
 * @brief 设备端材质图评估
 */
namespace MaterialGraphEvaluation {

/**
 * @brief 着色上下文（评估时传递的数据）
 */
struct ShadingContext {
    float3 position;        // 世界空间位置
    float3 normal;          // 几何法线
    float3 shadingNormal;   // 着色法线
    float3 tangent;         // 切线
    float3 bitangent;       // 副切线
    float2 uv;              // 纹理坐标
    float3 viewDirection;   // 视角方向

    // 光线追踪信息
    float3 rayOrigin;
    float3 rayDirection;
    float rayDistance;

    // 对象信息
    int objectID;
    int materialID;
};

/**
 * @brief 评估材质图
 * @param graphData 材质图数据（GPU）
 * @param context 着色上下文
 * @param outBSDF 输出 BSDF
 * @param outEmission 输出发光
 */
__device__ void evaluateMaterialGraph(
    const void* graphData,
    const ShadingContext& context,
    float3& outAlbedo,
    float& outMetallic,
    float& outRoughness,
    float3& outEmission
);

/**
 * @brief 评估单个节点
 */
__device__ void evaluateNode(
    const MaterialNode& node,
    const ShadingContext& context,
    float* outputs
);

} // namespace MaterialGraphEvaluation

} // namespace PhysicsRender

#endif // MATERIAL_NODE_SYSTEM_H
