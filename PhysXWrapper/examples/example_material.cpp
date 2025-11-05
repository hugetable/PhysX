/**
 * @file example_material.cpp
 * @brief Example demonstrating MaterialLibrary usage
 *
 * This example shows how to use the MaterialLibrary class for
 * accessing predefined materials and creating custom materials
 * with realistic physical properties.
 */

#include "PhysXCore.h"
#include "Utility/MaterialLibrary.h"
#include <iostream>

using namespace PhysXWrapper;

// ============================================================================
// Helper Functions
// ============================================================================

void printSeparator(const std::string& title)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================\n" << std::endl;
}

void createGround(PxPhysics* physics, PxScene* scene, PxMaterial* material)
{
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);
}

void createBox(PxPhysics* physics, PxScene* scene, const PxVec3& position,
               PxMaterial* material, const std::string& name)
{
    PxRigidDynamic* box = PxCreateDynamic(*physics,
                                           PxTransform(position),
                                           PxBoxGeometry(0.5f, 0.5f, 0.5f),
                                           *material,
                                           10.0f);
    scene->addActor(*box);
    std::cout << "Created " << name << " box at height " << position.y << std::endl;
}

void createSphere(PxPhysics* physics, PxScene* scene, const PxVec3& position,
                  PxMaterial* material, const std::string& name)
{
    PxRigidDynamic* sphere = PxCreateDynamic(*physics,
                                              PxTransform(position),
                                              PxSphereGeometry(0.5f),
                                              *material,
                                              10.0f);
    scene->addActor(*sphere);
    std::cout << "Created " << name << " sphere at height " << position.y << std::endl;
}

// ============================================================================
// Test 1: Predefined Materials
// ============================================================================

void test_PredefinedMaterials()
{
    printSeparator("Test 1: Predefined Materials");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    MaterialLibrary library;
    library.initialize(physics);

    std::cout << "Material library initialized with " << library.getMaterialCount() << " materials" << std::endl;

    // Print some materials
    std::cout << "\nSample materials:\n" << std::endl;
    library.printMaterial("wood");
    std::cout << std::endl;
    library.printMaterial("metal");
    std::cout << std::endl;
    library.printMaterial("rubber");
    std::cout << std::endl;
    library.printMaterial("ice");

    core.cleanup();
}

// ============================================================================
// Test 2: Material Categories
// ============================================================================

void test_MaterialCategories()
{
    printSeparator("Test 2: Material Categories");

    PhysXCore core;
    core.initialize();

    MaterialLibrary library;
    library.initialize(core.getPhysics());

    // Print materials by category
    std::vector<MaterialLibrary::MaterialCategory> categories = {
        MaterialLibrary::MaterialCategory::COMMON,
        MaterialLibrary::MaterialCategory::METAL,
        MaterialLibrary::MaterialCategory::TERRAIN,
        MaterialLibrary::MaterialCategory::SPECIAL
    };

    for (auto category : categories) {
        std::cout << "\nCategory: " << MaterialLibrary::getCategoryName(category) << std::endl;
        std::vector<std::string> materials = library.getMaterialsByCategory(category);
        std::cout << "Materials (" << materials.size() << "): ";
        for (size_t i = 0; i < materials.size(); i++) {
            std::cout << materials[i];
            if (i < materials.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }

    // Print all materials
    std::cout << "\n\nAll materials:" << std::endl;
    std::vector<std::string> allMaterials = library.getAllMaterialNames();
    for (const auto& name : allMaterials) {
        std::cout << "  - " << name << std::endl;
    }

    core.cleanup();
}

// ============================================================================
// Test 3: Friction Comparison
// ============================================================================

void test_FrictionComparison()
{
    printSeparator("Test 3: Friction Comparison");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    MaterialLibrary library;
    library.initialize(physics);

    // Create sloped ground with concrete
    PxMaterial* groundMat = library.getConcrete();
    PxQuat rotation(PxPi / 6.0f, PxVec3(0, 0, 1)); // 30 degree slope
    PxRigidStatic* slope = PxCreateStatic(*physics,
                                           PxTransform(PxVec3(0, 0, 0), rotation),
                                           PxBoxGeometry(10.0f, 0.2f, 10.0f),
                                           *groundMat);
    scene->addActor(*slope);

    std::cout << "Created sloped surface (30 degrees)" << std::endl;

    // Create boxes with different materials
    PxReal startHeight = 5.0f;
    PxReal spacing = 3.0f;

    std::cout << "\nDropping boxes with different friction:" << std::endl;

    createBox(physics, scene, PxVec3(-6, startHeight, 0), library.getIce(), "ice");
    createBox(physics, scene, PxVec3(-3, startHeight, 0), library.getMetal(), "metal");
    createBox(physics, scene, PxVec3(0, startHeight, 0), library.getWood(), "wood");
    createBox(physics, scene, PxVec3(3, startHeight, 0), library.getRubber(), "rubber");
    createBox(physics, scene, PxVec3(6, startHeight, 0), library.getMud(), "mud");

    // Simulate
    std::cout << "\nSimulating friction on slope..." << std::endl;
    for (int i = 0; i < 300; i++) {
        scene->simulate(0.016f);
        scene->fetchResults(true);
    }

    std::cout << "Simulation complete - ice slides fastest, mud slowest" << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 4: Restitution (Bounce) Comparison
// ============================================================================

void test_RestitutionComparison()
{
    printSeparator("Test 4: Restitution (Bounce) Comparison");

    PhysXCore core;
    core.initialize();

    PxPhysics* physics = core.getPhysics();
    PxScene* scene = core.getScene();

    MaterialLibrary library;
    library.initialize(physics);

    // Create ground with concrete
    createGround(physics, scene, library.getConcrete());

    // Drop spheres with different restitution
    PxReal dropHeight = 10.0f;

    std::cout << "Dropping spheres from height " << dropHeight << " meters:\n" << std::endl;

    createSphere(physics, scene, PxVec3(-6, dropHeight, 0), library.getSand(), "sand");
    createSphere(physics, scene, PxVec3(-3, dropHeight, 0), library.getWood(), "wood");
    createSphere(physics, scene, PxVec3(0, dropHeight, 0), library.getMetal(), "metal");
    createSphere(physics, scene, PxVec3(3, dropHeight, 0), library.getGlass(), "glass");
    createSphere(physics, scene, PxVec3(6, dropHeight, 0), library.getRubber(), "rubber");

    // Simulate and track bounces
    std::cout << "\nSimulating bounces..." << std::endl;
    for (int i = 0; i < 400; i++) {
        scene->simulate(0.016f);
        scene->fetchResults(true);
    }

    std::cout << "Simulation complete - rubber bounces highest, sand doesn't bounce" << std::endl;

    core.cleanup();
}

// ============================================================================
// Test 5: Custom Materials
// ============================================================================

void test_CustomMaterials()
{
    printSeparator("Test 5: Custom Materials");

    PhysXCore core;
    core.initialize();

    MaterialLibrary library;
    library.initialize(core.getPhysics());

    std::cout << "Creating custom materials...\n" << std::endl;

    // Create custom material
    PxMaterial* custom1 = library.createMaterial("my_custom", 0.7f, 0.6f, 0.8f);
    if (custom1) {
        std::cout << "Created custom material:" << std::endl;
        library.printMaterial("my_custom");
    }

    // Create from properties
    MaterialLibrary::MaterialProperties props;
    props.name = "super_sticky";
    props.staticFriction = 2.0f;
    props.dynamicFriction = 1.5f;
    props.restitution = 0.1f;
    props.description = "Super sticky material for climbing";

    PxMaterial* custom2 = library.createMaterial(props);
    if (custom2) {
        std::cout << "\nCreated custom material from properties:" << std::endl;
        library.printMaterial("super_sticky");
    }

    std::cout << "\nTotal materials: " << library.getMaterialCount() << std::endl;

    // Test custom materials in category
    std::cout << "\nCustom materials:" << std::endl;
    std::vector<std::string> customMats = library.getMaterialsByCategory(MaterialLibrary::MaterialCategory::CUSTOM);
    for (const auto& name : customMats) {
        std::cout << "  - " << name << std::endl;
    }

    core.cleanup();
}

// ============================================================================
// Test 6: Material Combination
// ============================================================================

void test_MaterialCombination()
{
    printSeparator("Test 6: Material Combination");

    PhysXCore core;
    core.initialize();

    MaterialLibrary library;
    library.initialize(core.getPhysics());

    std::cout << "Original materials:\n" << std::endl;
    library.printMaterial("wood");
    std::cout << std::endl;
    library.printMaterial("metal");
    std::cout << std::endl;

    // Combine materials
    std::cout << "\nCombining wood and metal (average):" << std::endl;
    PxMaterial* combined = library.combineMaterials("wood", "metal", "wood_metal_mix");
    if (combined) {
        library.printMaterial("wood_metal_mix");
    }

    // Interpolate materials
    std::cout << "\nInterpolating between ice and rubber (t=0.3):" << std::endl;
    PxMaterial* interpolated = library.interpolateMaterials("ice", "rubber", 0.3f, "ice_rubber_blend");
    if (interpolated) {
        library.printMaterial("ice_rubber_blend");
    }

    std::cout << "\nFor comparison:" << std::endl;
    library.printMaterial("ice");
    std::cout << std::endl;
    library.printMaterial("rubber");

    core.cleanup();
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "PhysXWrapper - MaterialLibrary Example" << std::endl;
    std::cout << "======================================\n" << std::endl;

    try {
        test_PredefinedMaterials();
        test_MaterialCategories();
        test_FrictionComparison();
        test_RestitutionComparison();
        test_CustomMaterials();
        test_MaterialCombination();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
