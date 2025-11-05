/**
 * @file example_collection.cpp
 * @brief Example demonstrating CollectionLoader for batch loading
 *
 * This example shows how to use CollectionLoader to load serialized
 * collections and instantiate objects in scenes.
 */

#include "PhysXManager.h"
#include "RigidBody/RigidBodyManager.h"
#include "Utility/SerializationManager.h"
#include "Utility/CollectionLoader.h"
#include <iostream>
#include <filesystem>

using namespace PhysXWrapper;
namespace fs = std::filesystem;

// ============================================================================
// Test 1: Create and Save Collection
// ============================================================================

void testCreateAndSaveCollection()
{
    std::cout << "\n=== Test 1: Create and Save Collection ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Create some objects
    RigidBodyManager bodyManager;
    bodyManager.initialize(physics, scene);

    std::cout << "\nCreating physics objects..." << std::endl;

    // Ground plane
    bodyManager.createGroundPlane();

    // Create some boxes
    for (int i = 0; i < 5; i++) {
        RigidBodyConfig config;
        config.type = RigidBodyType::DYNAMIC;
        config.position = PxVec3(i * 2.0f - 4.0f, 5, 0);
        bodyManager.createBox(config, PxVec3(0.5f, 0.5f, 0.5f));
    }

    // Create some spheres
    for (int i = 0; i < 3; i++) {
        RigidBodyConfig config;
        config.type = RigidBodyType::DYNAMIC;
        config.position = PxVec3(i * 2.0f - 2.0f, 8, 0);
        bodyManager.createSphere(config, 0.5f);
    }

    std::cout << "✓ Created " << scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC)
              << " actors" << std::endl;

    // Initialize serialization manager
    SerializationManager serializer;
    serializer.initialize(physics);

    // Create collection from scene
    std::cout << "\nCreating collection from scene..." << std::endl;
    PxCollection* collection = serializer.createCollectionFromScene(scene);

    if (collection) {
        std::cout << "✓ Created collection with " << collection->getNbObjects() << " objects" << std::endl;

        // Save to file
        std::string filename = "test_collection.bin";
        std::cout << "\nSaving collection to file: " << filename << std::endl;

        SerializationManager::SerializationConfig config;
        config.format = SerializationManager::SerializationFormat::BINARY;

        if (serializer.serializeCollectionToFile(collection, filename, config)) {
            std::cout << "✓ Successfully saved collection" << std::endl;

            // Check file size
            if (fs::exists(filename)) {
                auto fileSize = fs::file_size(filename);
                std::cout << "  File size: " << fileSize << " bytes" << std::endl;
            }
        }

        collection->release();
    }

    serializer.cleanup();
    bodyManager.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Create and save collection test passed" << std::endl;
}

// ============================================================================
// Test 2: Load Collection from File
// ============================================================================

void testLoadCollectionFromFile()
{
    std::cout << "\n=== Test 2: Load Collection from File ===" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Initialize collection loader
    CollectionLoader loader;
    if (!loader.initialize(physics, scene)) {
        std::cerr << "Failed to initialize CollectionLoader" << std::endl;
        return;
    }

    // Load collection from file
    std::string filename = "test_collection.bin";

    if (!fs::exists(filename)) {
        std::cout << "File does not exist: " << filename << std::endl;
        std::cout << "Skipping test (run Test 1 first)" << std::endl;
        loader.cleanup();
        physxManager.cleanup();
        return;
    }

    std::cout << "\nLoading collection from file: " << filename << std::endl;

    CollectionLoader::LoadConfig config;
    config.autoAddToScene = true;
    config.printStats = true;

    auto result = loader.loadFromFile(filename, config);

    if (result.success) {
        std::cout << "✓ Successfully loaded collection" << std::endl;
        std::cout << "  Objects: " << result.objectCount << std::endl;
        std::cout << "  Load time: " << result.loadTimeMs << " ms" << std::endl;

        // Get statistics
        auto stats = CollectionLoader::getStats(result.collection);
        std::cout << "\nCollection contents:" << std::endl;
        stats.print();

        // Simulate
        std::cout << "\nSimulating loaded scene..." << std::endl;
        for (int i = 0; i < 100; i++) {
            scene->simulate(1.0f / 60.0f);
            scene->fetchResults(true);
        }
        std::cout << "✓ Simulation completed" << std::endl;

    } else {
        std::cerr << "Failed to load collection: " << result.errorMessage << std::endl;
    }

    loader.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Load collection from file test passed" << std::endl;
}

// ============================================================================
// Test 3: Load Collection from Memory
// ============================================================================

void testLoadCollectionFromMemory()
{
    std::cout << "\n=== Test 3: Load Collection from Memory ===" << std::endl;

    // First, read file into memory
    std::string filename = "test_collection.bin";

    if (!fs::exists(filename)) {
        std::cout << "File does not exist: " << filename << std::endl;
        std::cout << "Skipping test (run Test 1 first)" << std::endl;
        return;
    }

    std::cout << "\nReading file into memory: " << filename << std::endl;

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<PxU8> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    file.close();

    std::cout << "✓ Read " << size << " bytes into memory" << std::endl;

    // Initialize PhysX
    PhysXManager physxManager;
    if (!physxManager.initialize()) {
        std::cerr << "Failed to initialize PhysX" << std::endl;
        return;
    }

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Initialize collection loader
    CollectionLoader loader;
    loader.initialize(physics, scene);

    // Load from memory
    std::cout << "\nLoading collection from memory buffer..." << std::endl;

    CollectionLoader::LoadConfig config;
    config.autoAddToScene = true;
    config.printStats = true;

    auto result = loader.loadFromBuffer(buffer, config);

    if (result.success) {
        std::cout << "✓ Successfully loaded collection from memory" << std::endl;
        std::cout << "  Load time: " << result.loadTimeMs << " ms" << std::endl;
    } else {
        std::cerr << "Failed to load collection: " << result.errorMessage << std::endl;
    }

    loader.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Load collection from memory test passed" << std::endl;
}

// ============================================================================
// Test 4: Multiple Collections
// ============================================================================

void testMultipleCollections()
{
    std::cout << "\n=== Test 4: Multiple Collections ===" << std::endl;

    // Create multiple test files first
    std::cout << "\nCreating test collections..." << std::endl;

    for (int collectionId = 0; collectionId < 3; collectionId++) {
        PhysXManager physxManager;
        physxManager.initialize();

        PxScene* scene = physxManager.getScene();
        PxPhysics* physics = physxManager.getPhysics();

        RigidBodyManager bodyManager;
        bodyManager.initialize(physics, scene);

        // Create different objects for each collection
        for (int i = 0; i < 3; i++) {
            RigidBodyConfig config;
            config.type = RigidBodyType::DYNAMIC;
            config.position = PxVec3(i * 2.0f, 5 + collectionId * 3.0f, collectionId * 2.0f);
            bodyManager.createBox(config, PxVec3(0.5f, 0.5f, 0.5f));
        }

        SerializationManager serializer;
        serializer.initialize(physics);

        PxCollection* collection = serializer.createCollectionFromScene(scene);
        std::string filename = "test_collection_" + std::to_string(collectionId) + ".bin";

        SerializationManager::SerializationConfig config;
        serializer.serializeCollectionToFile(collection, filename, config);

        std::cout << "  Created: " << filename << std::endl;

        collection->release();
        serializer.cleanup();
        bodyManager.cleanup();
        physxManager.cleanup();
    }

    // Now load all collections
    std::cout << "\nLoading multiple collections..." << std::endl;

    PhysXManager physxManager;
    physxManager.initialize();

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    CollectionLoader loader;
    loader.initialize(physics, scene);

    std::vector<std::string> filenames = {
        "test_collection_0.bin",
        "test_collection_1.bin",
        "test_collection_2.bin"
    };

    auto results = loader.loadMultipleFiles(filenames);

    std::cout << "\nLoad results:" << std::endl;
    for (size_t i = 0; i < results.size(); i++) {
        if (results[i].success) {
            std::cout << "  Collection " << i << ": ✓ Loaded " << results[i].objectCount
                      << " objects in " << results[i].loadTimeMs << " ms" << std::endl;
        } else {
            std::cout << "  Collection " << i << ": ✗ Failed - " << results[i].errorMessage << std::endl;
        }
    }

    // Add all to scene
    std::cout << "\nAdding all collections to scene..." << std::endl;
    PxU32 addedCount = loader.addAllCollectionsToScene();
    std::cout << "✓ Added " << addedCount << " collections to scene" << std::endl;

    // Print scene statistics
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    std::cout << "  Total actors in scene: " << numActors << std::endl;

    loader.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Multiple collections test passed" << std::endl;
}

// ============================================================================
// Test 5: Collection Statistics and Validation
// ============================================================================

void testCollectionStatisticsAndValidation()
{
    std::cout << "\n=== Test 5: Collection Statistics and Validation ===" << std::endl;

    std::string filename = "test_collection.bin";

    if (!fs::exists(filename)) {
        std::cout << "File does not exist: " << filename << std::endl;
        std::cout << "Skipping test (run Test 1 first)" << std::endl;
        return;
    }

    // Initialize PhysX
    PhysXManager physxManager;
    physxManager.initialize();

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Initialize loader
    CollectionLoader loader;
    loader.initialize(physics, scene);

    // Load collection
    std::cout << "\nLoading collection..." << std::endl;
    auto result = loader.loadFromFile(filename);

    if (!result.success) {
        std::cerr << "Failed to load collection" << std::endl;
        loader.cleanup();
        physxManager.cleanup();
        return;
    }

    // Get statistics
    std::cout << "\nCollection statistics:" << std::endl;
    auto stats = CollectionLoader::getStats(result.collection);
    stats.print();

    // Print collection contents
    std::cout << "\nCollection contents:" << std::endl;
    CollectionLoader::printCollection(result.collection, false);

    // Validate collection
    std::cout << "\nValidating collection..." << std::endl;
    std::vector<std::string> errors;
    bool valid = CollectionLoader::validateCollection(result.collection, &errors);

    if (valid) {
        std::cout << "✓ Collection is valid" << std::endl;
    } else {
        std::cout << "✗ Collection is invalid:" << std::endl;
        for (const auto& error : errors) {
            std::cout << "  - " << error << std::endl;
        }
    }

    // Test file type detection
    std::cout << "\nFile type detection:" << std::endl;
    std::cout << "  Is XML: " << (CollectionLoader::isXMLFile(filename) ? "YES" : "NO") << std::endl;
    std::cout << "  Is Binary: " << (CollectionLoader::isBinaryFile(filename) ? "YES" : "NO") << std::endl;

    loader.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Collection statistics and validation test passed" << std::endl;
}

// ============================================================================
// Test 6: Collection Management
// ============================================================================

void testCollectionManagement()
{
    std::cout << "\n=== Test 6: Collection Management ===" << std::endl;

    std::string filename = "test_collection.bin";

    if (!fs::exists(filename)) {
        std::cout << "File does not exist: " << filename << std::endl;
        std::cout << "Skipping test (run Test 1 first)" << std::endl;
        return;
    }

    // Initialize PhysX
    PhysXManager physxManager;
    physxManager.initialize();

    PxScene* scene = physxManager.getScene();
    PxPhysics* physics = physxManager.getPhysics();

    // Initialize loader
    CollectionLoader loader;
    loader.initialize(physics, scene);

    // Load multiple instances of the same collection
    std::cout << "\nLoading collection multiple times..." << std::endl;
    for (int i = 0; i < 3; i++) {
        auto result = loader.loadFromFile(filename);
        if (result.success) {
            std::cout << "  Loaded instance " << (i + 1) << std::endl;
        }
    }

    // Check collection count
    PxU32 count = loader.getCollectionCount();
    std::cout << "\nManaged collections: " << count << std::endl;

    // Access individual collections
    std::cout << "\nAccessing individual collections:" << std::endl;
    for (PxU32 i = 0; i < count; i++) {
        PxCollection* collection = loader.getCollection(i);
        if (collection) {
            std::cout << "  Collection " << i << ": " << collection->getNbObjects() << " objects" << std::endl;
        }
    }

    // Release one collection
    if (count > 0) {
        std::cout << "\nReleasing first collection..." << std::endl;
        PxCollection* firstCollection = loader.getCollection(0);
        loader.releaseCollection(firstCollection);
        std::cout << "  Remaining collections: " << loader.getCollectionCount() << std::endl;
    }

    // Release all
    std::cout << "\nReleasing all collections..." << std::endl;
    loader.releaseAllCollections();
    std::cout << "  Collections after cleanup: " << loader.getCollectionCount() << std::endl;

    loader.cleanup();
    physxManager.cleanup();

    std::cout << "✓ Collection management test passed" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "PhysXWrapper - CollectionLoader Example" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        testCreateAndSaveCollection();
        testLoadCollectionFromFile();
        testLoadCollectionFromMemory();
        testMultipleCollections();
        testCollectionStatisticsAndValidation();
        testCollectionManagement();

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests passed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
