/**
 * @file MaterialLibrary.cpp
 * @brief Implementation of MaterialLibrary class
 */

#include "Utility/MaterialLibrary.h"
#include <iostream>
#include <algorithm>

namespace PhysXWrapper {

// ============================================================================
// MaterialLibrary::Impl
// ============================================================================

class MaterialLibrary::Impl {
public:
    PxPhysics* m_physics = nullptr;

    std::map<std::string, PxMaterial*> m_materials;
    std::map<std::string, MaterialProperties> m_properties;
    std::map<std::string, MaterialCategory> m_categories;

    bool m_initialized = false;
};

// ============================================================================
// Construction/Destruction
// ============================================================================

MaterialLibrary::MaterialLibrary()
    : m_impl(std::make_unique<Impl>())
{
}

MaterialLibrary::~MaterialLibrary()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool MaterialLibrary::initialize(PxPhysics* physics)
{
    if (!physics) {
        std::cerr << "MaterialLibrary::initialize: physics is null" << std::endl;
        return false;
    }

    m_impl->m_physics = physics;
    initializePredefinedMaterials();

    m_impl->m_initialized = true;
    return true;
}

void MaterialLibrary::cleanup()
{
    // Release all materials
    for (auto& pair : m_impl->m_materials) {
        if (pair.second) {
            pair.second->release();
        }
    }

    m_impl->m_materials.clear();
    m_impl->m_properties.clear();
    m_impl->m_categories.clear();

    m_impl->m_initialized = false;
}

bool MaterialLibrary::isInitialized() const
{
    return m_impl->m_initialized;
}

// ============================================================================
// Material Access
// ============================================================================

PxMaterial* MaterialLibrary::getMaterial(const std::string& name) const
{
    auto it = m_impl->m_materials.find(name);
    if (it != m_impl->m_materials.end()) {
        return it->second;
    }
    return nullptr;
}

MaterialLibrary::MaterialProperties MaterialLibrary::getProperties(const std::string& name) const
{
    auto it = m_impl->m_properties.find(name);
    if (it != m_impl->m_properties.end()) {
        return it->second;
    }
    return MaterialProperties();
}

bool MaterialLibrary::hasMaterial(const std::string& name) const
{
    return m_impl->m_materials.find(name) != m_impl->m_materials.end();
}

// ============================================================================
// Material Creation
// ============================================================================

PxMaterial* MaterialLibrary::createMaterial(const std::string& name,
                                             PxReal staticFriction,
                                             PxReal dynamicFriction,
                                             PxReal restitution)
{
    if (!m_impl->m_initialized) {
        std::cerr << "MaterialLibrary::createMaterial: Not initialized" << std::endl;
        return nullptr;
    }

    // Check if material already exists
    if (hasMaterial(name)) {
        std::cerr << "MaterialLibrary::createMaterial: Material '" << name << "' already exists" << std::endl;
        return getMaterial(name);
    }

    // Create material
    PxMaterial* material = m_impl->m_physics->createMaterial(staticFriction, dynamicFriction, restitution);
    if (!material) {
        std::cerr << "MaterialLibrary::createMaterial: Failed to create material" << std::endl;
        return nullptr;
    }

    // Store material and properties
    m_impl->m_materials[name] = material;
    m_impl->m_properties[name] = MaterialProperties(name, staticFriction, dynamicFriction, restitution);
    m_impl->m_categories[name] = MaterialCategory::CUSTOM;

    return material;
}

PxMaterial* MaterialLibrary::createMaterial(const MaterialProperties& props)
{
    PxMaterial* mat = createMaterial(props.name, props.staticFriction, props.dynamicFriction, props.restitution);
    if (mat) {
        m_impl->m_properties[props.name] = props;
    }
    return mat;
}

bool MaterialLibrary::removeMaterial(const std::string& name)
{
    auto it = m_impl->m_materials.find(name);
    if (it == m_impl->m_materials.end()) {
        return false;
    }

    // Release material
    if (it->second) {
        it->second->release();
    }

    m_impl->m_materials.erase(it);
    m_impl->m_properties.erase(name);
    m_impl->m_categories.erase(name);

    return true;
}

// ============================================================================
// Predefined Materials
// ============================================================================

void MaterialLibrary::initializePredefinedMaterials()
{
    // Common materials
    MaterialProperties materials[] = {
        // name, static, dynamic, restitution, description
        {"default", 0.5f, 0.5f, 0.5f, "Default medium friction material"},

        // Common
        {"wood", 0.6f, 0.45f, 0.4f, "Wood (oak, pine)"},
        {"metal", 0.4f, 0.3f, 0.3f, "Metal (steel, iron)"},
        {"plastic", 0.4f, 0.3f, 0.5f, "Plastic (ABS, PVC)"},
        {"rubber", 0.9f, 0.8f, 0.7f, "Rubber (high friction, bouncy)"},
        {"stone", 0.7f, 0.5f, 0.2f, "Stone (granite, marble)"},
        {"glass", 0.4f, 0.3f, 0.6f, "Glass (smooth, slippery when wet)"},
        {"ice", 0.05f, 0.03f, 0.1f, "Ice (very slippery)"},

        // Specialized
        {"concrete", 0.6f, 0.5f, 0.3f, "Concrete (rough surface)"},
        {"sand", 0.7f, 0.6f, 0.1f, "Sand (high friction, no bounce)"},
        {"mud", 0.8f, 0.7f, 0.05f, "Mud (very high friction)"},
        {"leather", 0.6f, 0.5f, 0.3f, "Leather (moderate friction)"},
        {"fabric", 0.5f, 0.4f, 0.2f, "Fabric (cloth, textile)"},

        // Metals
        {"steel", 0.4f, 0.3f, 0.3f, "Steel (hard, smooth)"},
        {"aluminum", 0.35f, 0.25f, 0.4f, "Aluminum (lightweight metal)"},
        {"copper", 0.5f, 0.4f, 0.35f, "Copper (soft metal)"},

        // Special
        {"bouncy", 0.5f, 0.4f, 0.95f, "Super bouncy material"},
        {"slippery", 0.05f, 0.03f, 0.1f, "Very slippery material"},
        {"sticky", 1.5f, 1.2f, 0.1f, "High friction sticky material"},
        {"frictionless", 0.0f, 0.0f, 1.0f, "No friction, perfect bounce"}
    };

    MaterialCategory categories[] = {
        MaterialCategory::COMMON,   // default
        MaterialCategory::COMMON,   // wood
        MaterialCategory::METAL,    // metal
        MaterialCategory::SYNTHETIC,// plastic
        MaterialCategory::SYNTHETIC,// rubber
        MaterialCategory::TERRAIN,  // stone
        MaterialCategory::COMMON,   // glass
        MaterialCategory::SPECIAL,  // ice
        MaterialCategory::TERRAIN,  // concrete
        MaterialCategory::TERRAIN,  // sand
        MaterialCategory::TERRAIN,  // mud
        MaterialCategory::ORGANIC,  // leather
        MaterialCategory::ORGANIC,  // fabric
        MaterialCategory::METAL,    // steel
        MaterialCategory::METAL,    // aluminum
        MaterialCategory::METAL,    // copper
        MaterialCategory::SPECIAL,  // bouncy
        MaterialCategory::SPECIAL,  // slippery
        MaterialCategory::SPECIAL,  // sticky
        MaterialCategory::SPECIAL   // frictionless
    };

    int numMaterials = sizeof(materials) / sizeof(MaterialProperties);

    for (int i = 0; i < numMaterials; i++) {
        const MaterialProperties& props = materials[i];
        PxMaterial* mat = m_impl->m_physics->createMaterial(
            props.staticFriction,
            props.dynamicFriction,
            props.restitution
        );

        if (mat) {
            m_impl->m_materials[props.name] = mat;
            m_impl->m_properties[props.name] = props;
            m_impl->m_categories[props.name] = categories[i];
        }
    }
}

PxMaterial* MaterialLibrary::getDefault() const { return getMaterial("default"); }
PxMaterial* MaterialLibrary::getWood() const { return getMaterial("wood"); }
PxMaterial* MaterialLibrary::getMetal() const { return getMaterial("metal"); }
PxMaterial* MaterialLibrary::getPlastic() const { return getMaterial("plastic"); }
PxMaterial* MaterialLibrary::getRubber() const { return getMaterial("rubber"); }
PxMaterial* MaterialLibrary::getStone() const { return getMaterial("stone"); }
PxMaterial* MaterialLibrary::getGlass() const { return getMaterial("glass"); }
PxMaterial* MaterialLibrary::getIce() const { return getMaterial("ice"); }
PxMaterial* MaterialLibrary::getConcrete() const { return getMaterial("concrete"); }
PxMaterial* MaterialLibrary::getSand() const { return getMaterial("sand"); }
PxMaterial* MaterialLibrary::getMud() const { return getMaterial("mud"); }
PxMaterial* MaterialLibrary::getLeather() const { return getMaterial("leather"); }
PxMaterial* MaterialLibrary::getFabric() const { return getMaterial("fabric"); }

// ============================================================================
// Material Categories
// ============================================================================

std::vector<std::string> MaterialLibrary::getMaterialsByCategory(MaterialCategory category) const
{
    std::vector<std::string> result;

    for (const auto& pair : m_impl->m_categories) {
        if (pair.second == category) {
            result.push_back(pair.first);
        }
    }

    return result;
}

std::vector<std::string> MaterialLibrary::getAllMaterialNames() const
{
    std::vector<std::string> result;

    for (const auto& pair : m_impl->m_materials) {
        result.push_back(pair.first);
    }

    std::sort(result.begin(), result.end());
    return result;
}

PxU32 MaterialLibrary::getMaterialCount() const
{
    return static_cast<PxU32>(m_impl->m_materials.size());
}

// ============================================================================
// Material Combination
// ============================================================================

PxMaterial* MaterialLibrary::combineMaterials(const std::string& name1,
                                               const std::string& name2,
                                               const std::string& resultName)
{
    MaterialProperties props1 = getProperties(name1);
    MaterialProperties props2 = getProperties(name2);

    if (props1.name.empty() || props2.name.empty()) {
        std::cerr << "MaterialLibrary::combineMaterials: Invalid material names" << std::endl;
        return nullptr;
    }

    // Average properties
    PxReal staticFriction = (props1.staticFriction + props2.staticFriction) * 0.5f;
    PxReal dynamicFriction = (props1.dynamicFriction + props2.dynamicFriction) * 0.5f;
    PxReal restitution = (props1.restitution + props2.restitution) * 0.5f;

    return createMaterial(resultName, staticFriction, dynamicFriction, restitution);
}

PxMaterial* MaterialLibrary::interpolateMaterials(const std::string& name1,
                                                   const std::string& name2,
                                                   PxReal t,
                                                   const std::string& resultName)
{
    MaterialProperties props1 = getProperties(name1);
    MaterialProperties props2 = getProperties(name2);

    if (props1.name.empty() || props2.name.empty()) {
        std::cerr << "MaterialLibrary::interpolateMaterials: Invalid material names" << std::endl;
        return nullptr;
    }

    t = PxClamp(t, 0.0f, 1.0f);

    // Interpolate properties
    PxReal staticFriction = props1.staticFriction * (1.0f - t) + props2.staticFriction * t;
    PxReal dynamicFriction = props1.dynamicFriction * (1.0f - t) + props2.dynamicFriction * t;
    PxReal restitution = props1.restitution * (1.0f - t) + props2.restitution * t;

    return createMaterial(resultName, staticFriction, dynamicFriction, restitution);
}

// ============================================================================
// Utility
// ============================================================================

void MaterialLibrary::printMaterial(const std::string& name) const
{
    PxMaterial* mat = getMaterial(name);
    if (!mat) {
        std::cout << "Material '" << name << "' not found" << std::endl;
        return;
    }

    MaterialProperties props = getProperties(name);

    std::cout << "Material: " << name << std::endl;
    if (!props.description.empty()) {
        std::cout << "  Description: " << props.description << std::endl;
    }
    std::cout << "  Static Friction: " << props.staticFriction << std::endl;
    std::cout << "  Dynamic Friction: " << props.dynamicFriction << std::endl;
    std::cout << "  Restitution: " << props.restitution << std::endl;

    auto catIt = m_impl->m_categories.find(name);
    if (catIt != m_impl->m_categories.end()) {
        std::cout << "  Category: " << getCategoryName(catIt->second) << std::endl;
    }
}

void MaterialLibrary::printAllMaterials() const
{
    std::cout << "Material Library (" << m_impl->m_materials.size() << " materials):" << std::endl;
    std::cout << "========================================" << std::endl;

    std::vector<std::string> names = getAllMaterialNames();
    for (const auto& name : names) {
        printMaterial(name);
        std::cout << std::endl;
    }
}

void MaterialLibrary::printCategory(MaterialCategory category) const
{
    std::cout << "Category: " << getCategoryName(category) << std::endl;
    std::cout << "========================================" << std::endl;

    std::vector<std::string> materials = getMaterialsByCategory(category);
    for (const auto& name : materials) {
        printMaterial(name);
        std::cout << std::endl;
    }
}

std::string MaterialLibrary::getCategoryName(MaterialCategory category)
{
    switch (category) {
        case MaterialCategory::COMMON: return "Common";
        case MaterialCategory::METAL: return "Metal";
        case MaterialCategory::ORGANIC: return "Organic";
        case MaterialCategory::SYNTHETIC: return "Synthetic";
        case MaterialCategory::TERRAIN: return "Terrain";
        case MaterialCategory::SPECIAL: return "Special";
        case MaterialCategory::CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

// ============================================================================
// Configuration
// ============================================================================

PxPhysics* MaterialLibrary::getPhysics() const
{
    return m_impl->m_physics;
}

} // namespace PhysXWrapper
