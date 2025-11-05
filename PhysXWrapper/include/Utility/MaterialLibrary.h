/**
 * @file MaterialLibrary.h
 * @brief Material library with predefined physical materials
 *
 * This class provides a library of common physical materials with
 * realistic friction and restitution values, making it easy to
 * apply realistic material properties to physics objects.
 *
 * Based on real-world material properties
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <string>
#include <map>
#include <vector>

using namespace physx;

namespace PhysXWrapper {

/**
 * @class MaterialLibrary
 * @brief Library of predefined physical materials
 *
 * MaterialLibrary provides a collection of predefined materials with
 * realistic physical properties. This makes it easy to create objects
 * with accurate material behavior without manually tuning parameters.
 *
 * Materials included:
 * - Common materials (wood, metal, plastic, rubber, stone, etc.)
 * - Specialized materials (ice, glass, concrete, sand, etc.)
 * - Custom materials (user-defined)
 *
 * Each material has:
 * - Static friction (resistance to start moving)
 * - Dynamic friction (resistance while moving)
 * - Restitution (bounciness, 0 = no bounce, 1 = perfect bounce)
 *
 * Usage:
 * @code
 * MaterialLibrary library;
 * library.initialize(physics);
 *
 * // Get predefined material
 * PxMaterial* wood = library.getMaterial("wood");
 * PxMaterial* metal = library.getMaterial("metal");
 *
 * // Create custom material
 * library.createMaterial("custom", 0.6f, 0.5f, 0.3f);
 * PxMaterial* custom = library.getMaterial("custom");
 * @endcode
 */
class MaterialLibrary {
public:
    /**
     * @brief Material properties
     */
    struct MaterialProperties {
        std::string name;           ///< Material name
        PxReal staticFriction;      ///< Static friction coefficient [0, inf)
        PxReal dynamicFriction;     ///< Dynamic friction coefficient [0, inf)
        PxReal restitution;         ///< Restitution (bounciness) [0, 1]
        std::string description;    ///< Material description

        MaterialProperties()
            : staticFriction(0.5f)
            , dynamicFriction(0.5f)
            , restitution(0.5f)
        {}

        MaterialProperties(const std::string& n, PxReal sf, PxReal df, PxReal r, const std::string& desc = "")
            : name(n)
            , staticFriction(sf)
            , dynamicFriction(df)
            , restitution(r)
            , description(desc)
        {}
    };

    /**
     * @brief Material category
     */
    enum class MaterialCategory {
        COMMON,         ///< Common materials
        METAL,          ///< Metal materials
        ORGANIC,        ///< Organic materials
        SYNTHETIC,      ///< Synthetic materials
        TERRAIN,        ///< Terrain materials
        SPECIAL,        ///< Special materials
        CUSTOM          ///< Custom user materials
    };

public:
    /**
     * @brief Constructor
     */
    MaterialLibrary();

    /**
     * @brief Destructor
     */
    ~MaterialLibrary();

    // Disable copy
    MaterialLibrary(const MaterialLibrary&) = delete;
    MaterialLibrary& operator=(const MaterialLibrary&) = delete;

    /**
     * @brief Initialize material library
     * @param physics PhysX physics instance
     * @return true if successful
     */
    bool initialize(PxPhysics* physics);

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     * @return true if initialized
     */
    bool isInitialized() const;

    // ========================================================================
    // Material Access
    // ========================================================================

    /**
     * @brief Get material by name
     * @param name Material name
     * @return Material pointer (or nullptr if not found)
     */
    PxMaterial* getMaterial(const std::string& name) const;

    /**
     * @brief Get material properties
     * @param name Material name
     * @return Material properties
     */
    MaterialProperties getProperties(const std::string& name) const;

    /**
     * @brief Check if material exists
     * @param name Material name
     * @return true if exists
     */
    bool hasMaterial(const std::string& name) const;

    // ========================================================================
    // Material Creation
    // ========================================================================

    /**
     * @brief Create custom material
     * @param name Material name
     * @param staticFriction Static friction
     * @param dynamicFriction Dynamic friction
     * @param restitution Restitution
     * @return Material pointer (or nullptr on failure)
     */
    PxMaterial* createMaterial(const std::string& name,
                                PxReal staticFriction,
                                PxReal dynamicFriction,
                                PxReal restitution);

    /**
     * @brief Create material from properties
     * @param props Material properties
     * @return Material pointer (or nullptr on failure)
     */
    PxMaterial* createMaterial(const MaterialProperties& props);

    /**
     * @brief Remove material
     * @param name Material name
     * @return true if successful
     */
    bool removeMaterial(const std::string& name);

    // ========================================================================
    // Predefined Materials - Common
    // ========================================================================

    /**
     * @brief Get default material (medium friction, medium bounce)
     * @return Material pointer
     */
    PxMaterial* getDefault() const;

    /**
     * @brief Get wood material
     * @return Material pointer
     */
    PxMaterial* getWood() const;

    /**
     * @brief Get metal material
     * @return Material pointer
     */
    PxMaterial* getMetal() const;

    /**
     * @brief Get plastic material
     * @return Material pointer
     */
    PxMaterial* getPlastic() const;

    /**
     * @brief Get rubber material
     * @return Material pointer
     */
    PxMaterial* getRubber() const;

    /**
     * @brief Get stone material
     * @return Material pointer
     */
    PxMaterial* getStone() const;

    /**
     * @brief Get glass material
     * @return Material pointer
     */
    PxMaterial* getGlass() const;

    /**
     * @brief Get ice material
     * @return Material pointer
     */
    PxMaterial* getIce() const;

    // ========================================================================
    // Predefined Materials - Specialized
    // ========================================================================

    /**
     * @brief Get concrete material
     * @return Material pointer
     */
    PxMaterial* getConcrete() const;

    /**
     * @brief Get sand material
     * @return Material pointer
     */
    PxMaterial* getSand() const;

    /**
     * @brief Get mud material
     * @return Material pointer
     */
    PxMaterial* getMud() const;

    /**
     * @brief Get leather material
     * @return Material pointer
     */
    PxMaterial* getLeather() const;

    /**
     * @brief Get fabric material
     * @return Material pointer
     */
    PxMaterial* getFabric() const;

    // ========================================================================
    // Material Categories
    // ========================================================================

    /**
     * @brief Get materials by category
     * @param category Material category
     * @return Vector of material names
     */
    std::vector<std::string> getMaterialsByCategory(MaterialCategory category) const;

    /**
     * @brief Get all material names
     * @return Vector of all material names
     */
    std::vector<std::string> getAllMaterialNames() const;

    /**
     * @brief Get material count
     * @return Number of materials
     */
    PxU32 getMaterialCount() const;

    // ========================================================================
    // Material Combination
    // ========================================================================

    /**
     * @brief Combine two materials (averaging properties)
     * @param name1 First material name
     * @param name2 Second material name
     * @param resultName Result material name
     * @return Combined material pointer (or nullptr on failure)
     */
    PxMaterial* combineMaterials(const std::string& name1,
                                  const std::string& name2,
                                  const std::string& resultName);

    /**
     * @brief Interpolate between two materials
     * @param name1 First material name
     * @param name2 Second material name
     * @param t Interpolation factor [0, 1]
     * @param resultName Result material name
     * @return Interpolated material pointer (or nullptr on failure)
     */
    PxMaterial* interpolateMaterials(const std::string& name1,
                                      const std::string& name2,
                                      PxReal t,
                                      const std::string& resultName);

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Print material info
     * @param name Material name
     */
    void printMaterial(const std::string& name) const;

    /**
     * @brief Print all materials
     */
    void printAllMaterials() const;

    /**
     * @brief Print materials by category
     * @param category Material category
     */
    void printCategory(MaterialCategory category) const;

    /**
     * @brief Get category name
     * @param category Material category
     * @return Category name string
     */
    static std::string getCategoryName(MaterialCategory category);

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Get PhysX physics instance
     * @return Physics instance
     */
    PxPhysics* getPhysics() const;

private:
    /**
     * @brief Initialize predefined materials
     */
    void initializePredefinedMaterials();

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace PhysXWrapper
