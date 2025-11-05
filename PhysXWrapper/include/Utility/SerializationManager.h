/**
 * @file SerializationManager.h
 * @brief Scene and object serialization system for PhysX
 *
 * This class provides utilities for serializing and deserializing:
 * - Complete PhysX scenes
 * - Individual actors and objects
 * - Collections of objects
 * - Shared resources (materials, meshes, etc.)
 *
 * Serialization is useful for:
 * - Save/load game states
 * - Asset precomputation
 * - Level streaming
 * - Network transmission
 * - Memory snapshots
 *
 * Based on SnippetSerialization from PhysX SDK.
 *
 * NOTE: XML serialization (RepX) has been DEPRECATED by NVIDIA.
 * This class focuses on binary serialization which is faster and more compact.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Serialization format
 */
enum class SerializationFormat {
    BINARY,         ///< Binary format (recommended, fast and compact)
    XML             ///< XML format (DEPRECATED, for legacy support only)
};

/**
 * @brief Serialization configuration
 */
struct SerializationConfig {
    SerializationFormat format = SerializationFormat::BINARY;  ///< Serialization format
    bool includeSharedObjects = true;                          ///< Include shared resources
    bool createObjectIds = true;                               ///< Create object IDs for references
    PxSerialObjectId baseObjectId = 1;                         ///< Base ID for object references
};

/**
 * @brief Deserialization result
 */
struct DeserializationResult {
    PxCollection* collection = nullptr;                        ///< Deserialized collection
    PxU32 objectCount = 0;                                     ///< Number of objects
    bool success = false;                                      ///< Operation succeeded
    std::string errorMessage;                                  ///< Error message if failed
};

/**
 * @brief Serialization statistics
 */
struct SerializationStats {
    PxU32 actorCount = 0;                                      ///< Number of actors
    PxU32 shapeCount = 0;                                      ///< Number of shapes
    PxU32 materialCount = 0;                                   ///< Number of materials
    PxU32 meshCount = 0;                                       ///< Number of meshes
    PxU32 jointCount = 0;                                      ///< Number of joints
    PxU32 totalObjects = 0;                                    ///< Total object count
    size_t serializedSize = 0;                                 ///< Serialized data size (bytes)
};

/**
 * @brief Serialization manager class
 *
 * This class provides comprehensive serialization capabilities:
 * - Save/load entire scenes
 * - Save/load individual objects
 * - Manage collections of objects
 * - Handle shared resources efficiently
 * - Support binary and XML formats
 *
 * @example
 * @code
 * // Initialize manager
 * SerializationManager serializer;
 * serializer.initialize(physics);
 *
 * // Serialize scene to file
 * bool success = serializer.serializeSceneToFile(scene, "scene.pxs");
 *
 * // Load scene from file
 * PxScene* newScene = physics->createScene(sceneDesc);
 * DeserializationResult result = serializer.deserializeSceneFromFile(
 *     newScene, "scene.pxs");
 *
 * if (result.success) {
 *     std::cout << "Loaded " << result.objectCount << " objects" << std::endl;
 * }
 *
 * // Serialize single actor
 * std::vector<PxU8> data = serializer.serializeActor(actor);
 * // ... save data to disk or network ...
 *
 * // Deserialize actor
 * PxRigidActor* newActor = serializer.deserializeActor(data, scene);
 * @endcode
 */
class SerializationManager {
public:
    /**
     * @brief Constructor
     */
    SerializationManager();

    /**
     * @brief Destructor
     */
    ~SerializationManager();

    // No copy
    SerializationManager(const SerializationManager&) = delete;
    SerializationManager& operator=(const SerializationManager&) = delete;

    // Move allowed
    SerializationManager(SerializationManager&&) noexcept;
    SerializationManager& operator=(SerializationManager&&) noexcept;

    /**
     * @brief Initialize serialization manager
     * @param physics PhysX physics instance
     * @return True if successful
     */
    bool initialize(PxPhysics* physics);

    /**
     * @brief Cleanup and release resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const;

    // ========================================================================
    // Scene Serialization
    // ========================================================================

    /**
     * @brief Serialize scene to file
     * @param scene Scene to serialize
     * @param filename Output filename
     * @param config Serialization configuration
     * @return True if successful
     */
    bool serializeSceneToFile(
        PxScene* scene,
        const std::string& filename,
        const SerializationConfig& config = SerializationConfig());

    /**
     * @brief Serialize scene to memory
     * @param scene Scene to serialize
     * @param config Serialization configuration
     * @return Serialized data
     */
    std::vector<PxU8> serializeSceneToMemory(
        PxScene* scene,
        const SerializationConfig& config = SerializationConfig());

    /**
     * @brief Deserialize scene from file
     * @param scene Target scene (must be empty)
     * @param filename Input filename
     * @return Deserialization result
     */
    DeserializationResult deserializeSceneFromFile(
        PxScene* scene,
        const std::string& filename);

    /**
     * @brief Deserialize scene from memory
     * @param scene Target scene
     * @param data Serialized data
     * @return Deserialization result
     */
    DeserializationResult deserializeSceneFromMemory(
        PxScene* scene,
        const std::vector<PxU8>& data);

    // ========================================================================
    // Object Serialization
    // ========================================================================

    /**
     * @brief Serialize actor to memory
     * @param actor Actor to serialize
     * @param config Serialization configuration
     * @return Serialized data
     */
    std::vector<PxU8> serializeActor(
        PxRigidActor* actor,
        const SerializationConfig& config = SerializationConfig());

    /**
     * @brief Deserialize actor from memory
     * @param data Serialized data
     * @param scene Target scene (can be null)
     * @return Deserialized actor
     */
    PxRigidActor* deserializeActor(
        const std::vector<PxU8>& data,
        PxScene* scene = nullptr);

    /**
     * @brief Serialize collection to file
     * @param collection Collection to serialize
     * @param filename Output filename
     * @param config Serialization configuration
     * @return True if successful
     */
    bool serializeCollectionToFile(
        PxCollection* collection,
        const std::string& filename,
        const SerializationConfig& config = SerializationConfig());

    /**
     * @brief Serialize collection to memory
     * @param collection Collection to serialize
     * @param config Serialization configuration
     * @return Serialized data
     */
    std::vector<PxU8> serializeCollectionToMemory(
        PxCollection* collection,
        const SerializationConfig& config = SerializationConfig());

    /**
     * @brief Deserialize collection from file
     * @param filename Input filename
     * @return Deserialization result
     */
    DeserializationResult deserializeCollectionFromFile(
        const std::string& filename);

    /**
     * @brief Deserialize collection from memory
     * @param data Serialized data
     * @return Deserialization result
     */
    DeserializationResult deserializeCollectionFromMemory(
        const std::vector<PxU8>& data);

    // ========================================================================
    // Collection Management
    // ========================================================================

    /**
     * @brief Create collection from scene
     * @param scene Source scene
     * @param includeShared Include shared objects (materials, meshes)
     * @return Created collection
     */
    PxCollection* createCollectionFromScene(
        PxScene* scene,
        bool includeShared = true);

    /**
     * @brief Create collection from actors
     * @param actors Actors to include
     * @param includeShared Include shared objects
     * @return Created collection
     */
    PxCollection* createCollectionFromActors(
        const std::vector<PxRigidActor*>& actors,
        bool includeShared = true);

    /**
     * @brief Add collection to scene
     * @param scene Target scene
     * @param collection Collection to add
     * @param transform Optional transform to apply
     * @return Number of objects added
     */
    PxU32 addCollectionToScene(
        PxScene* scene,
        PxCollection* collection,
        const PxTransform* transform = nullptr);

    // ========================================================================
    // Statistics and Utilities
    // ========================================================================

    /**
     * @brief Get statistics for collection
     * @param collection Collection to analyze
     * @return Statistics
     */
    static SerializationStats getCollectionStats(PxCollection* collection);

    /**
     * @brief Get statistics for scene
     * @param scene Scene to analyze
     * @return Statistics
     */
    static SerializationStats getSceneStats(PxScene* scene);

    /**
     * @brief Validate serialized data
     * @param data Serialized data
     * @return True if data appears valid
     */
    static bool validateSerializedData(const std::vector<PxU8>& data);

    /**
     * @brief Get last error message
     * @return Error message or empty string
     */
    std::string getLastError() const;

    // ========================================================================
    // Advanced Features
    // ========================================================================

    /**
     * @brief Clone actor (serialize then deserialize)
     * @param actor Actor to clone
     * @param transform Transform for clone
     * @param scene Target scene
     * @return Cloned actor
     */
    PxRigidActor* cloneActor(
        PxRigidActor* actor,
        const PxTransform& transform,
        PxScene* scene = nullptr);

    /**
     * @brief Create instance of serialized data
     * @param data Serialized collection data
     * @param scene Target scene
     * @param transform Transform for instance
     * @return Number of objects created
     */
    PxU32 createInstance(
        const std::vector<PxU8>& data,
        PxScene* scene,
        const PxTransform& transform);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    // Helper methods
    void* allocateAlignedMemory(size_t size);
    void freeAlignedMemory(void* ptr);
    bool writeDataToFile(const std::string& filename, const void* data, size_t size);
    std::vector<PxU8> readDataFromFile(const std::string& filename);
};

/**
 * @brief Helper class for managing serialization memory
 */
class SerializationMemoryManager {
public:
    /**
     * @brief Allocate aligned memory for serialization
     * @param size Size in bytes
     * @return Aligned memory pointer
     */
    static void* allocate(size_t size);

    /**
     * @brief Free aligned memory
     * @param ptr Memory pointer
     */
    static void free(void* ptr);

    /**
     * @brief Check if pointer is properly aligned
     * @param ptr Pointer to check
     * @return True if aligned
     */
    static bool isAligned(const void* ptr);
};

} // namespace PhysXWrapper
