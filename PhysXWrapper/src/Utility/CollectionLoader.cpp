/**
 * @file CollectionLoader.cpp
 * @brief Implementation of CollectionLoader class
 */

#include "Utility/CollectionLoader.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>

namespace PhysXWrapper {

// ============================================================================
// CollectionStats Implementation
// ============================================================================

void CollectionLoader::CollectionStats::print() const
{
    std::cout << "Collection Statistics:" << std::endl;
    std::cout << "  Actors: " << numActors << " (Static: " << numStaticActors
              << ", Dynamic: " << numDynamicActors << ")" << std::endl;
    std::cout << "  Shapes: " << numShapes << std::endl;
    std::cout << "  Materials: " << numMaterials << std::endl;
    std::cout << "  Joints: " << numJoints << std::endl;
    std::cout << "  Articulations: " << numArticulations << std::endl;
    std::cout << "  Aggregates: " << numAggregates << std::endl;
}

// ============================================================================
// CollectionLoader::Impl
// ============================================================================

class CollectionLoader::Impl {
public:
    PxPhysics* m_physics = nullptr;
    PxScene* m_scene = nullptr;
    PxSerializationRegistry* m_serializationRegistry = nullptr;
    LoadConfig m_defaultConfig;

    std::vector<PxCollection*> m_collections;
    bool m_initialized = false;
};

// ============================================================================
// Construction/Destruction
// ============================================================================

CollectionLoader::CollectionLoader()
    : m_impl(std::make_unique<Impl>())
{
}

CollectionLoader::~CollectionLoader()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool CollectionLoader::initialize(PxPhysics* physics, PxScene* scene)
{
    if (!physics) {
        std::cerr << "CollectionLoader::initialize: physics is null" << std::endl;
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_scene = scene;

    // Create serialization registry
    m_impl->m_serializationRegistry = PxSerialization::createSerializationRegistry(*physics);
    if (!m_impl->m_serializationRegistry) {
        std::cerr << "CollectionLoader::initialize: Failed to create serialization registry" << std::endl;
        return false;
    }

    m_impl->m_initialized = true;
    return true;
}

void CollectionLoader::cleanup()
{
    releaseAllCollections();

    if (m_impl->m_serializationRegistry) {
        m_impl->m_serializationRegistry->release();
        m_impl->m_serializationRegistry = nullptr;
    }

    m_impl->m_initialized = false;
}

bool CollectionLoader::isInitialized() const
{
    return m_impl->m_initialized;
}

void CollectionLoader::setScene(PxScene* scene)
{
    m_impl->m_scene = scene;
}

// ============================================================================
// File Loading
// ============================================================================

CollectionLoader::LoadResult CollectionLoader::loadFromFile(const std::string& filename,
                                                             const LoadConfig& config)
{
    if (!m_impl->m_initialized) {
        LoadResult result;
        result.errorMessage = "CollectionLoader not initialized";
        return result;
    }

    // Determine file type
    if (isXMLFile(filename)) {
        return loadFromXMLFile(filename, config);
    } else if (isBinaryFile(filename)) {
        return loadFromBinaryFile(filename, config);
    } else {
        LoadResult result;
        result.errorMessage = "Unknown file format";
        return result;
    }
}

CollectionLoader::LoadResult CollectionLoader::loadFromBinaryFile(const std::string& filename,
                                                                   const LoadConfig& config)
{
    LoadResult result;

    if (!m_impl->m_initialized) {
        result.errorMessage = "CollectionLoader not initialized";
        return result;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Open file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        result.errorMessage = "Failed to open file: " + filename;
        return result;
    }

    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read file into buffer with alignment
    const size_t alignment = PX_SERIAL_FILE_ALIGN;
    size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);

    PxU8* buffer = static_cast<PxU8*>(PxGetFoundation().getAllocatorCallback().allocate(
        alignedSize, "SerializedCollection", __FILE__, __LINE__));

    if (!buffer) {
        result.errorMessage = "Failed to allocate memory";
        return result;
    }

    // Align buffer
    void* alignedBuffer = reinterpret_cast<void*>(
        (reinterpret_cast<size_t>(buffer) + alignment - 1) & ~(alignment - 1));

    file.read(reinterpret_cast<char*>(alignedBuffer), size);
    file.close();

    // Deserialize
    PxCollection* collection = PxSerialization::createCollectionFromBinary(
        alignedBuffer,
        *m_impl->m_serializationRegistry
    );

    PxGetFoundation().getAllocatorCallback().deallocate(buffer);

    if (!collection) {
        result.errorMessage = "Failed to deserialize collection";
        return result;
    }

    // Complete collection
    completeCollection(collection);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.loadTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    result.collection = collection;
    result.objectCount = collection->getNbObjects();
    result.success = true;

    m_impl->m_collections.push_back(collection);

    if (config.printStats) {
        std::cout << "Loaded binary collection: " << filename << std::endl;
        std::cout << "  Objects: " << result.objectCount << std::endl;
        std::cout << "  Load time: " << result.loadTimeMs << " ms" << std::endl;
    }

    if (config.autoAddToScene && m_impl->m_scene) {
        addCollectionToScene(collection);
    }

    return result;
}

CollectionLoader::LoadResult CollectionLoader::loadFromXMLFile(const std::string& filename,
                                                                const LoadConfig& config)
{
    LoadResult result;
    result.errorMessage = "XML loading not implemented (requires repx extension)";
    return result;
}

std::vector<CollectionLoader::LoadResult> CollectionLoader::loadMultipleFiles(
    const std::vector<std::string>& filenames,
    const LoadConfig& config)
{
    std::vector<LoadResult> results;

    if (filenames.empty()) {
        return results;
    }

    // Load first file (should be complete)
    LoadResult firstResult = loadFromFile(filenames[0], config);
    results.push_back(firstResult);

    if (!firstResult.success) {
        std::cerr << "Failed to load first collection: " << firstResult.errorMessage << std::endl;
        return results;
    }

    // Load remaining files (may have dependencies on first)
    for (size_t i = 1; i < filenames.size(); i++) {
        LoadResult loadResult = loadFromFile(filenames[i], config);
        results.push_back(loadResult);

        if (!loadResult.success) {
            std::cerr << "Failed to load collection " << i << ": " << loadResult.errorMessage << std::endl;
        }
    }

    return results;
}

// ============================================================================
// Memory Loading
// ============================================================================

CollectionLoader::LoadResult CollectionLoader::loadFromMemory(const void* data, PxU32 size,
                                                               const LoadConfig& config)
{
    LoadResult result;

    if (!m_impl->m_initialized) {
        result.errorMessage = "CollectionLoader not initialized";
        return result;
    }

    if (!data || size == 0) {
        result.errorMessage = "Invalid data or size";
        return result;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Deserialize from memory
    PxCollection* collection = PxSerialization::createCollectionFromBinary(
        data,
        *m_impl->m_serializationRegistry
    );

    if (!collection) {
        result.errorMessage = "Failed to deserialize collection from memory";
        return result;
    }

    // Complete collection
    completeCollection(collection);

    auto endTime = std::chrono::high_resolution_clock::now();
    result.loadTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    result.collection = collection;
    result.objectCount = collection->getNbObjects();
    result.success = true;

    m_impl->m_collections.push_back(collection);

    if (config.printStats) {
        std::cout << "Loaded collection from memory" << std::endl;
        std::cout << "  Objects: " << result.objectCount << std::endl;
        std::cout << "  Load time: " << result.loadTimeMs << " ms" << std::endl;
    }

    if (config.autoAddToScene && m_impl->m_scene) {
        addCollectionToScene(collection);
    }

    return result;
}

CollectionLoader::LoadResult CollectionLoader::loadFromBuffer(const std::vector<PxU8>& buffer,
                                                               const LoadConfig& config)
{
    return loadFromMemory(buffer.data(), static_cast<PxU32>(buffer.size()), config);
}

// ============================================================================
// Scene Integration
// ============================================================================

bool CollectionLoader::addCollectionToScene(PxCollection* collection)
{
    if (!collection) {
        std::cerr << "CollectionLoader::addCollectionToScene: collection is null" << std::endl;
        return false;
    }

    if (!m_impl->m_scene) {
        std::cerr << "CollectionLoader::addCollectionToScene: scene is null" << std::endl;
        return false;
    }

    // Add all actors from collection to scene
    PxU32 numObjects = collection->getNbObjects();
    for (PxU32 i = 0; i < numObjects; i++) {
        PxBase* obj = &collection->getObject(i);

        // Try to cast to actor
        PxActor* actor = obj->is<PxActor>();
        if (actor) {
            m_impl->m_scene->addActor(*actor);
        }

        // Try to cast to aggregate
        PxAggregate* aggregate = obj->is<PxAggregate>();
        if (aggregate) {
            m_impl->m_scene->addAggregate(*aggregate);
        }
    }

    return true;
}

bool CollectionLoader::removeCollectionFromScene(PxCollection* collection)
{
    if (!collection) {
        return false;
    }

    if (!m_impl->m_scene) {
        return false;
    }

    // Remove all actors from scene
    PxU32 numObjects = collection->getNbObjects();
    for (PxU32 i = 0; i < numObjects; i++) {
        PxBase* obj = &collection->getObject(i);

        PxActor* actor = obj->is<PxActor>();
        if (actor) {
            m_impl->m_scene->removeActor(*actor);
        }

        PxAggregate* aggregate = obj->is<PxAggregate>();
        if (aggregate) {
            m_impl->m_scene->removeAggregate(*aggregate);
        }
    }

    return true;
}

PxU32 CollectionLoader::addAllCollectionsToScene()
{
    if (!m_impl->m_scene) {
        return 0;
    }

    PxU32 count = 0;
    for (PxCollection* collection : m_impl->m_collections) {
        if (addCollectionToScene(collection)) {
            count++;
        }
    }

    return count;
}

// ============================================================================
// Collection Management
// ============================================================================

PxU32 CollectionLoader::getCollectionCount() const
{
    return static_cast<PxU32>(m_impl->m_collections.size());
}

PxCollection* CollectionLoader::getCollection(PxU32 index) const
{
    if (index >= m_impl->m_collections.size()) {
        return nullptr;
    }
    return m_impl->m_collections[index];
}

void CollectionLoader::releaseCollection(PxCollection* collection)
{
    if (!collection) return;

    // Remove from list
    auto it = std::find(m_impl->m_collections.begin(), m_impl->m_collections.end(), collection);
    if (it != m_impl->m_collections.end()) {
        m_impl->m_collections.erase(it);
    }

    // Release collection
    collection->release();
}

void CollectionLoader::releaseAllCollections()
{
    for (PxCollection* collection : m_impl->m_collections) {
        if (collection) {
            collection->release();
        }
    }
    m_impl->m_collections.clear();
}

// ============================================================================
// Analysis and Statistics
// ============================================================================

CollectionLoader::CollectionStats CollectionLoader::getStats(PxCollection* collection)
{
    CollectionStats stats;

    if (!collection) {
        return stats;
    }

    PxU32 numObjects = collection->getNbObjects();
    for (PxU32 i = 0; i < numObjects; i++) {
        PxBase* obj = &collection->getObject(i);

        if (obj->is<PxRigidStatic>()) {
            stats.numStaticActors++;
            stats.numActors++;
        } else if (obj->is<PxRigidDynamic>()) {
            stats.numDynamicActors++;
            stats.numActors++;
        } else if (obj->is<PxShape>()) {
            stats.numShapes++;
        } else if (obj->is<PxMaterial>()) {
            stats.numMaterials++;
        } else if (obj->is<PxJoint>()) {
            stats.numJoints++;
        } else if (obj->is<PxArticulationReducedCoordinate>()) {
            stats.numArticulations++;
        } else if (obj->is<PxAggregate>()) {
            stats.numAggregates++;
        }
    }

    return stats;
}

void CollectionLoader::printCollection(PxCollection* collection, bool detailed)
{
    if (!collection) {
        std::cout << "Collection is null" << std::endl;
        return;
    }

    CollectionStats stats = getStats(collection);
    std::cout << "Collection contains " << collection->getNbObjects() << " objects:" << std::endl;
    stats.print();

    if (detailed) {
        std::cout << "\nDetailed contents:" << std::endl;
        PxU32 numObjects = collection->getNbObjects();
        for (PxU32 i = 0; i < numObjects; i++) {
            PxBase* obj = &collection->getObject(i);
            std::cout << "  [" << i << "] " << obj->getConcreteTypeName() << std::endl;
        }
    }
}

bool CollectionLoader::validateCollection(PxCollection* collection,
                                          std::vector<std::string>* errorMessages)
{
    if (!collection) {
        if (errorMessages) {
            errorMessages->push_back("Collection is null");
        }
        return false;
    }

    bool valid = true;
    PxU32 numObjects = collection->getNbObjects();

    if (numObjects == 0) {
        if (errorMessages) {
            errorMessages->push_back("Collection is empty");
        }
        valid = false;
    }

    // Check for null objects
    for (PxU32 i = 0; i < numObjects; i++) {
        PxBase* obj = &collection->getObject(i);
        if (!obj) {
            if (errorMessages) {
                errorMessages->push_back("Object " + std::to_string(i) + " is null");
            }
            valid = false;
        }
    }

    return valid;
}

bool CollectionLoader::isXMLFile(const std::string& filename)
{
    if (filename.length() < 4) {
        return false;
    }

    std::string ext = filename.substr(filename.length() - 4);
    return (ext == ".xml" || ext == ".XML");
}

bool CollectionLoader::isBinaryFile(const std::string& filename)
{
    if (filename.length() < 4) {
        return false;
    }

    std::string ext = filename.substr(filename.length() - 4);
    return (ext == ".bin" || ext == ".BIN");
}

// ============================================================================
// Configuration
// ============================================================================

void CollectionLoader::setDefaultConfig(const LoadConfig& config)
{
    m_impl->m_defaultConfig = config;
}

const CollectionLoader::LoadConfig& CollectionLoader::getDefaultConfig() const
{
    return m_impl->m_defaultConfig;
}

PxPhysics* CollectionLoader::getPhysics() const
{
    return m_impl->m_physics;
}

PxScene* CollectionLoader::getScene() const
{
    return m_impl->m_scene;
}

// ============================================================================
// Helper Methods
// ============================================================================

void CollectionLoader::completeCollection(PxCollection* collection)
{
    if (!collection) return;

    // Complete the collection by resolving external references
    PxSerialization::complete(*collection, *m_impl->m_serializationRegistry);
}

} // namespace PhysXWrapper
