/**
 * @file SerializationManager.cpp
 * @brief Implementation of SerializationManager class
 */

#include "Utility/SerializationManager.h"
#include <fstream>
#include <cstdlib>
#include <cstring>

namespace PhysXWrapper {

// ============================================================================
// SerializationMemoryManager Implementation
// ============================================================================

void* SerializationMemoryManager::allocate(size_t size) {
    PxU8* baseAddr = static_cast<PxU8*>(malloc(size + PX_SERIAL_FILE_ALIGN - 1));
    if (!baseAddr) return nullptr;

    void* alignedBlock = reinterpret_cast<void*>(
        (size_t(baseAddr) + PX_SERIAL_FILE_ALIGN - 1) & ~(PX_SERIAL_FILE_ALIGN - 1));

    // Store base address before aligned block for later freeing
    *(reinterpret_cast<PxU8**>(alignedBlock) - 1) = baseAddr;

    return alignedBlock;
}

void SerializationMemoryManager::free(void* ptr) {
    if (!ptr) return;

    // Retrieve base address
    PxU8* baseAddr = *(reinterpret_cast<PxU8**>(ptr) - 1);
    ::free(baseAddr);
}

bool SerializationMemoryManager::isAligned(const void* ptr) {
    return (reinterpret_cast<size_t>(ptr) & (PX_SERIAL_FILE_ALIGN - 1)) == 0;
}

// ============================================================================
// SerializationManager::Impl
// ============================================================================

class SerializationManager::Impl {
public:
    Impl() : m_physics(nullptr), m_initialized(false) {}

    PxPhysics* m_physics;
    bool m_initialized;
    std::string m_lastError;
    std::vector<void*> m_allocatedBlocks;  // Track allocated memory

    void setError(const std::string& error) {
        m_lastError = error;
    }

    void clearError() {
        m_lastError.clear();
    }

    void trackAllocation(void* ptr) {
        m_allocatedBlocks.push_back(ptr);
    }

    void freeAllAllocations() {
        for (void* ptr : m_allocatedBlocks) {
            ::free(ptr);
        }
        m_allocatedBlocks.clear();
    }
};

// ============================================================================
// Construction / Destruction
// ============================================================================

SerializationManager::SerializationManager()
    : m_impl(std::make_unique<Impl>())
{
}

SerializationManager::~SerializationManager() {
    cleanup();
}

SerializationManager::SerializationManager(SerializationManager&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

SerializationManager& SerializationManager::operator=(SerializationManager&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

// ============================================================================
// Initialization
// ============================================================================

bool SerializationManager::initialize(PxPhysics* physics) {
    if (!physics) {
        m_impl->setError("Invalid physics instance");
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_initialized = true;
    m_impl->clearError();

    return true;
}

void SerializationManager::cleanup() {
    if (!m_impl->m_initialized) return;

    m_impl->freeAllAllocations();
    m_impl->m_physics = nullptr;
    m_impl->m_initialized = false;
    m_impl->clearError();
}

bool SerializationManager::isInitialized() const {
    return m_impl->m_initialized;
}

// ========================================================================
// Scene Serialization
// ========================================================================

bool SerializationManager::serializeSceneToFile(
    PxScene* scene,
    const std::string& filename,
    const SerializationConfig& config)
{
    std::vector<PxU8> data = serializeSceneToMemory(scene, config);
    if (data.empty()) {
        return false;
    }

    return writeDataToFile(filename, data.data(), data.size());
}

std::vector<PxU8> SerializationManager::serializeSceneToMemory(
    PxScene* scene,
    const SerializationConfig& config)
{
    std::vector<PxU8> result;

    if (!m_impl->m_initialized || !scene) {
        m_impl->setError("Not initialized or invalid scene");
        return result;
    }

    // Create collection from scene
    PxCollection* collection = createCollectionFromScene(scene, config.includeSharedObjects);
    if (!collection) {
        m_impl->setError("Failed to create collection from scene");
        return result;
    }

    // Serialize collection
    result = serializeCollectionToMemory(collection, config);

    collection->release();
    return result;
}

DeserializationResult SerializationManager::deserializeSceneFromFile(
    PxScene* scene,
    const std::string& filename)
{
    std::vector<PxU8> data = readDataFromFile(filename);
    if (data.empty()) {
        DeserializationResult result;
        result.success = false;
        result.errorMessage = "Failed to read file: " + filename;
        return result;
    }

    return deserializeSceneFromMemory(scene, data);
}

DeserializationResult SerializationManager::deserializeSceneFromMemory(
    PxScene* scene,
    const std::vector<PxU8>& data)
{
    DeserializationResult result;

    if (!m_impl->m_initialized || !scene) {
        result.success = false;
        result.errorMessage = "Not initialized or invalid scene";
        return result;
    }

    // Deserialize collection
    DeserializationResult collectionResult = deserializeCollectionFromMemory(data);
    if (!collectionResult.success) {
        return collectionResult;
    }

    // Add collection to scene
    result.objectCount = addCollectionToScene(scene, collectionResult.collection, nullptr);

    collectionResult.collection->release();

    result.success = true;
    result.collection = nullptr;

    return result;
}

// ========================================================================
// Object Serialization
// ========================================================================

std::vector<PxU8> SerializationManager::serializeActor(
    PxRigidActor* actor,
    const SerializationConfig& config)
{
    std::vector<PxU8> result;

    if (!m_impl->m_initialized || !actor) {
        m_impl->setError("Not initialized or invalid actor");
        return result;
    }

    // Create collection with single actor
    std::vector<PxRigidActor*> actors = {actor};
    PxCollection* collection = createCollectionFromActors(actors, config.includeSharedObjects);

    if (collection) {
        result = serializeCollectionToMemory(collection, config);
        collection->release();
    }

    return result;
}

PxRigidActor* SerializationManager::deserializeActor(
    const std::vector<PxU8>& data,
    PxScene* scene)
{
    DeserializationResult result = deserializeCollectionFromMemory(data);

    if (!result.success || !result.collection) {
        return nullptr;
    }

    // Find first actor in collection
    PxRigidActor* actor = nullptr;
    for (PxU32 i = 0; i < result.collection->getNbObjects(); ++i) {
        actor = result.collection->getObject(i).is<PxRigidActor>();
        if (actor) break;
    }

    if (actor && scene) {
        scene->addActor(*actor);
    }

    result.collection->release();

    return actor;
}

bool SerializationManager::serializeCollectionToFile(
    PxCollection* collection,
    const std::string& filename,
    const SerializationConfig& config)
{
    std::vector<PxU8> data = serializeCollectionToMemory(collection, config);
    if (data.empty()) {
        return false;
    }

    return writeDataToFile(filename, data.data(), data.size());
}

std::vector<PxU8> SerializationManager::serializeCollectionToMemory(
    PxCollection* collection,
    const SerializationConfig& config)
{
    std::vector<PxU8> result;

    if (!m_impl->m_initialized || !collection) {
        m_impl->setError("Not initialized or invalid collection");
        return result;
    }

    PxSerializationRegistry* sr = PxSerialization::createSerializationRegistry(*m_impl->m_physics);
    if (!sr) {
        m_impl->setError("Failed to create serialization registry");
        return result;
    }

    // Create object IDs if requested
    if (config.createObjectIds) {
        PxSerialization::createSerialObjectIds(*collection, config.baseObjectId);
    }

    // Serialize to output stream
    PxDefaultMemoryOutputStream outputStream;

    if (config.format == SerializationFormat::BINARY) {
        PxSerialization::serializeCollectionToBinary(outputStream, *collection, *sr);
    } else {
        PxSerialization::serializeCollectionToXml(outputStream, *collection, *sr);
    }

    // Copy data to result vector
    result.assign(outputStream.getData(), outputStream.getData() + outputStream.getSize());

    sr->release();
    m_impl->clearError();

    return result;
}

DeserializationResult SerializationManager::deserializeCollectionFromFile(
    const std::string& filename)
{
    std::vector<PxU8> data = readDataFromFile(filename);
    if (data.empty()) {
        DeserializationResult result;
        result.success = false;
        result.errorMessage = "Failed to read file: " + filename;
        return result;
    }

    return deserializeCollectionFromMemory(data);
}

DeserializationResult SerializationManager::deserializeCollectionFromMemory(
    const std::vector<PxU8>& data)
{
    DeserializationResult result;

    if (!m_impl->m_initialized) {
        result.success = false;
        result.errorMessage = "Not initialized";
        return result;
    }

    if (data.empty()) {
        result.success = false;
        result.errorMessage = "Empty data";
        return result;
    }

    PxSerializationRegistry* sr = PxSerialization::createSerializationRegistry(*m_impl->m_physics);
    if (!sr) {
        result.success = false;
        result.errorMessage = "Failed to create serialization registry";
        return result;
    }

    // Allocate aligned memory for binary deserialization
    void* alignedBlock = allocateAlignedMemory(data.size());
    if (!alignedBlock) {
        sr->release();
        result.success = false;
        result.errorMessage = "Failed to allocate aligned memory";
        return result;
    }

    std::memcpy(alignedBlock, data.data(), data.size());

    // Deserialize
    result.collection = PxSerialization::createCollectionFromBinary(alignedBlock, *sr);

    if (result.collection) {
        result.objectCount = result.collection->getNbObjects();
        result.success = true;
        m_impl->clearError();
    } else {
        result.success = false;
        result.errorMessage = "Failed to deserialize collection";
        freeAlignedMemory(alignedBlock);
    }

    sr->release();

    return result;
}

// ========================================================================
// Collection Management
// ========================================================================

PxCollection* SerializationManager::createCollectionFromScene(
    PxScene* scene,
    bool includeShared)
{
    if (!scene) return nullptr;

    PxCollection* collection = PxCreateCollection();
    if (!collection) return nullptr;

    // Get all actors
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    if (numActors > 0) {
        std::vector<PxActor*> actors(numActors);
        scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                         actors.data(), numActors);

        for (PxActor* actor : actors) {
            collection->add(*actor);
        }
    }

    // Complete collection (chase pointers)
    PxSerializationRegistry* sr = PxSerialization::createSerializationRegistry(*m_impl->m_physics);
    PxSerialization::complete(*collection, *sr, nullptr, includeShared);
    sr->release();

    return collection;
}

PxCollection* SerializationManager::createCollectionFromActors(
    const std::vector<PxRigidActor*>& actors,
    bool includeShared)
{
    PxCollection* collection = PxCreateCollection();
    if (!collection) return nullptr;

    for (PxRigidActor* actor : actors) {
        if (actor) {
            collection->add(*actor);
        }
    }

    // Complete collection
    PxSerializationRegistry* sr = PxSerialization::createSerializationRegistry(*m_impl->m_physics);
    PxSerialization::complete(*collection, *sr, nullptr, includeShared);
    sr->release();

    return collection;
}

PxU32 SerializationManager::addCollectionToScene(
    PxScene* scene,
    PxCollection* collection,
    const PxTransform* transform)
{
    if (!scene || !collection) return 0;

    PxU32 count = 0;

    // Apply transform if provided
    if (transform) {
        for (PxU32 i = 0; i < collection->getNbObjects(); ++i) {
            PxRigidActor* actor = collection->getObject(i).is<PxRigidActor>();
            if (actor) {
                PxTransform globalPose = actor->getGlobalPose();
                globalPose = globalPose.transform(*transform);
                actor->setGlobalPose(globalPose);
            }
        }
    }

    // Add collection to scene
    scene->addCollection(*collection);
    count = collection->getNbObjects();

    return count;
}

// ========================================================================
// Statistics and Utilities
// ========================================================================

SerializationStats SerializationManager::getCollectionStats(PxCollection* collection) {
    SerializationStats stats;

    if (!collection) return stats;

    stats.totalObjects = collection->getNbObjects();

    for (PxU32 i = 0; i < collection->getNbObjects(); ++i) {
        PxBase& obj = collection->getObject(i);

        if (obj.is<PxRigidActor>()) stats.actorCount++;
        else if (obj.is<PxShape>()) stats.shapeCount++;
        else if (obj.is<PxMaterial>()) stats.materialCount++;
        else if (obj.is<PxConvexMesh>() || obj.is<PxTriangleMesh>()) stats.meshCount++;
        else if (obj.is<PxJoint>()) stats.jointCount++;
    }

    return stats;
}

SerializationStats SerializationManager::getSceneStats(PxScene* scene) {
    SerializationStats stats;

    if (!scene) return stats;

    stats.actorCount = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);

    // Count shapes
    std::vector<PxActor*> actors(stats.actorCount);
    scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                     actors.data(), stats.actorCount);

    for (PxActor* actor : actors) {
        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        if (rigidActor) {
            stats.shapeCount += rigidActor->getNbShapes();
        }
    }

    stats.totalObjects = stats.actorCount + stats.shapeCount;

    return stats;
}

bool SerializationManager::validateSerializedData(const std::vector<PxU8>& data) {
    // Basic validation - check if data is not empty and has minimum size
    if (data.empty() || data.size() < 16) {
        return false;
    }

    // Could add more sophisticated validation (magic numbers, checksums, etc.)
    return true;
}

std::string SerializationManager::getLastError() const {
    return m_impl->m_lastError;
}

// ========================================================================
// Advanced Features
// ========================================================================

PxRigidActor* SerializationManager::cloneActor(
    PxRigidActor* actor,
    const PxTransform& transform,
    PxScene* scene)
{
    if (!actor) return nullptr;

    // Serialize actor
    std::vector<PxU8> data = serializeActor(actor);
    if (data.empty()) return nullptr;

    // Deserialize to new actor
    PxRigidActor* clonedActor = deserializeActor(data, nullptr);
    if (!clonedActor) return nullptr;

    // Apply transform
    clonedActor->setGlobalPose(transform);

    // Add to scene if provided
    if (scene) {
        scene->addActor(*clonedActor);
    }

    return clonedActor;
}

PxU32 SerializationManager::createInstance(
    const std::vector<PxU8>& data,
    PxScene* scene,
    const PxTransform& transform)
{
    if (!scene) return 0;

    DeserializationResult result = deserializeCollectionFromMemory(data);
    if (!result.success) return 0;

    PxU32 count = addCollectionToScene(scene, result.collection, &transform);

    result.collection->release();

    return count;
}

// ========================================================================
// Private Helper Methods
// ========================================================================

void* SerializationManager::allocateAlignedMemory(size_t size) {
    PxU8* baseAddr = static_cast<PxU8*>(malloc(size + PX_SERIAL_FILE_ALIGN - 1));
    if (!baseAddr) return nullptr;

    m_impl->trackAllocation(baseAddr);

    void* alignedBlock = reinterpret_cast<void*>(
        (size_t(baseAddr) + PX_SERIAL_FILE_ALIGN - 1) & ~(PX_SERIAL_FILE_ALIGN - 1));

    return alignedBlock;
}

void SerializationManager::freeAlignedMemory(void* ptr) {
    // Memory is tracked and freed in cleanup
    (void)ptr;
}

bool SerializationManager::writeDataToFile(const std::string& filename, const void* data, size_t size) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        m_impl->setError("Failed to open file for writing: " + filename);
        return false;
    }

    file.write(static_cast<const char*>(data), size);

    if (!file) {
        m_impl->setError("Failed to write data to file: " + filename);
        return false;
    }

    m_impl->clearError();
    return true;
}

std::vector<PxU8> SerializationManager::readDataFromFile(const std::string& filename) {
    std::vector<PxU8> result;

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        m_impl->setError("Failed to open file for reading: " + filename);
        return result;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    result.resize(size);
    file.read(reinterpret_cast<char*>(result.data()), size);

    if (!file) {
        m_impl->setError("Failed to read data from file: " + filename);
        result.clear();
        return result;
    }

    m_impl->clearError();
    return result;
}

} // namespace PhysXWrapper
