/**
 * @file CollectionLoader.h
 * @brief Collection loading utilities for PhysX
 *
 * This class provides utilities for loading serialized collections and
 * instantiating objects in scenes. It works with collections created
 * by SerializationManager.
 *
 * Based on PhysX SnippetLoadCollection
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <vector>
#include <string>

using namespace physx;

namespace PhysXWrapper {

/**
 * @class CollectionLoader
 * @brief Loader for serialized PhysX collections
 *
 * CollectionLoader provides utilities for loading serialized collections
 * (both XML and binary formats) and instantiating the contained objects
 * into a PhysX scene. This is useful for:
 * - Loading pre-authored physics content
 * - Level streaming and asset management
 * - Importing physics data from external tools
 * - Batch creation of physics objects
 *
 * Collections can contain:
 * - Materials
 * - Shapes
 * - Rigid bodies (static and dynamic)
 * - Joints
 * - Articulations
 * - Shared resources (meshes, materials)
 *
 * Usage:
 * @code
 * CollectionLoader loader;
 * loader.initialize(physics, scene);
 *
 * // Load from file
 * PxCollection* collection = loader.loadFromFile("scene.xml");
 *
 * // Load multiple collections with dependencies
 * std::vector<std::string> files = {"shared.xml", "level1.xml"};
 * auto collections = loader.loadMultipleFiles(files);
 *
 * // Add to scene
 * loader.addCollectionToScene(collection);
 * @endcode
 */
class CollectionLoader {
public:
    /**
     * @brief Load result information
     */
    struct LoadResult {
        PxCollection* collection = nullptr;  ///< Loaded collection
        PxU32 objectCount = 0;               ///< Number of objects
        bool success = false;                ///< Load success flag
        std::string errorMessage;            ///< Error message if failed
        float loadTimeMs = 0.0f;             ///< Load time in milliseconds

        LoadResult() = default;
    };

    /**
     * @brief Configuration for loading collections
     */
    struct LoadConfig {
        /// Whether to automatically add to scene after loading
        bool autoAddToScene = false;

        /// Whether to print load statistics
        bool printStats = true;

        /// Whether to validate loaded objects
        bool validate = true;

        LoadConfig() = default;

        /// Static default configuration instance for use as default parameter
        static const LoadConfig& defaultConfig() {
            static const LoadConfig config;
            return config;
        }
    };

    /**
     * @brief Statistics for a loaded collection
     */
    struct CollectionStats {
        PxU32 numActors = 0;
        PxU32 numStaticActors = 0;
        PxU32 numDynamicActors = 0;
        PxU32 numShapes = 0;
        PxU32 numMaterials = 0;
        PxU32 numJoints = 0;
        PxU32 numArticulations = 0;
        PxU32 numAggregates = 0;

        void print() const;
    };

public:
    /**
     * @brief Constructor
     */
    CollectionLoader();

    /**
     * @brief Destructor
     */
    ~CollectionLoader();

    // Disable copy
    CollectionLoader(const CollectionLoader&) = delete;
    CollectionLoader& operator=(const CollectionLoader&) = delete;

    /**
     * @brief Initialize the collection loader
     * @param physics PhysX physics instance
     * @param scene PhysX scene instance (optional, can be set later)
     * @return true if successful
     */
    bool initialize(PxPhysics* physics, PxScene* scene = nullptr);

    /**
     * @brief Cleanup resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     * @return true if initialized
     */
    bool isInitialized() const;

    /**
     * @brief Set target scene
     * @param scene Target scene for loading
     */
    void setScene(PxScene* scene);

    // ========================================================================
    // File Loading
    // ========================================================================

    /**
     * @brief Load collection from file (XML or binary)
     * @param filename File path
     * @param config Load configuration
     * @return Load result with collection
     */
    LoadResult loadFromFile(const std::string& filename,
                            const LoadConfig& config = LoadConfig::defaultConfig());

    /**
     * @brief Load collection from binary file
     * @param filename File path
     * @param config Load configuration
     * @return Load result with collection
     */
    LoadResult loadFromBinaryFile(const std::string& filename,
                                   const LoadConfig& config = LoadConfig::defaultConfig());

    /**
     * @brief Load collection from XML file
     * @param filename File path
     * @param config Load configuration
     * @return Load result with collection
     */
    LoadResult loadFromXMLFile(const std::string& filename,
                                const LoadConfig& config = LoadConfig::defaultConfig());

    /**
     * @brief Load multiple files (with dependencies)
     * @param filenames List of file paths
     * @param config Load configuration
     * @return Vector of load results
     *
     * @note The first file should be complete. Subsequent files
     * may contain references to objects in the first file.
     */
    std::vector<LoadResult> loadMultipleFiles(const std::vector<std::string>& filenames,
                                               const LoadConfig& config = LoadConfig::defaultConfig());

    // ========================================================================
    // Memory Loading
    // ========================================================================

    /**
     * @brief Load collection from memory buffer (binary)
     * @param data Memory buffer
     * @param size Buffer size
     * @param config Load configuration
     * @return Load result with collection
     */
    LoadResult loadFromMemory(const void* data, PxU32 size,
                              const LoadConfig& config = LoadConfig::defaultConfig());

    /**
     * @brief Load collection from vector buffer
     * @param buffer Binary buffer
     * @param config Load configuration
     * @return Load result with collection
     */
    LoadResult loadFromBuffer(const std::vector<PxU8>& buffer,
                              const LoadConfig& config = LoadConfig::defaultConfig());

    // ========================================================================
    // Scene Integration
    // ========================================================================

    /**
     * @brief Add collection to scene
     * @param collection Collection to add
     * @return true if successful
     */
    bool addCollectionToScene(PxCollection* collection);

    /**
     * @brief Remove collection from scene
     * @param collection Collection to remove
     * @return true if successful
     */
    bool removeCollectionFromScene(PxCollection* collection);

    /**
     * @brief Add all loaded collections to scene
     * @return Number of collections added
     */
    PxU32 addAllCollectionsToScene();

    // ========================================================================
    // Collection Management
    // ========================================================================

    /**
     * @brief Get number of managed collections
     * @return Collection count
     */
    PxU32 getCollectionCount() const;

    /**
     * @brief Get collection by index
     * @param index Collection index
     * @return Collection pointer (or nullptr if out of range)
     */
    PxCollection* getCollection(PxU32 index) const;

    /**
     * @brief Release a specific collection
     * @param collection Collection to release
     */
    void releaseCollection(PxCollection* collection);

    /**
     * @brief Release all managed collections
     */
    void releaseAllCollections();

    // ========================================================================
    // Analysis and Statistics
    // ========================================================================

    /**
     * @brief Get statistics for a collection
     * @param collection Collection to analyze
     * @return Statistics structure
     */
    static CollectionStats getStats(PxCollection* collection);

    /**
     * @brief Print collection contents
     * @param collection Collection to print
     * @param detailed Whether to print detailed information
     */
    static void printCollection(PxCollection* collection, bool detailed = false);

    /**
     * @brief Validate collection
     * @param collection Collection to validate
     * @param errorMessages Output vector for error messages
     * @return true if valid
     */
    static bool validateCollection(PxCollection* collection,
                                   std::vector<std::string>* errorMessages = nullptr);

    /**
     * @brief Check if file is XML format
     * @param filename File path
     * @return true if XML format
     */
    static bool isXMLFile(const std::string& filename);

    /**
     * @brief Check if file is binary format
     * @param filename File path
     * @return true if binary format
     */
    static bool isBinaryFile(const std::string& filename);

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set default load configuration
     * @param config Configuration to use for future loads
     */
    void setDefaultConfig(const LoadConfig& config);

    /**
     * @brief Get default load configuration
     * @return Current default configuration
     */
    const LoadConfig& getDefaultConfig() const;

    /**
     * @brief Get PhysX physics instance
     * @return Physics instance
     */
    PxPhysics* getPhysics() const;

    /**
     * @brief Get PhysX scene instance
     * @return Scene instance
     */
    PxScene* getScene() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    // Helper methods
    LoadResult loadFromSerializedData(PxInputStream& stream, const LoadConfig& config);
    void completeCollection(PxCollection* collection);
};

} // namespace PhysXWrapper
