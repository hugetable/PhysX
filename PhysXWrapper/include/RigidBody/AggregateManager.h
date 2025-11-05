/**
 * @file AggregateManager.h
 * @brief Aggregate manager for grouping actors for performance
 *
 * This class provides utilities for creating and managing aggregates,
 * which are groups of actors that can be processed more efficiently
 * by PhysX's broad-phase collision detection.
 *
 * Based on PhysX PxAggregate API
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <vector>
#include <string>

using namespace physx;

namespace PhysXWrapper {

/**
 * @class AggregateManager
 * @brief Manager for PhysX aggregates
 *
 * AggregateManager provides utilities for creating and managing aggregates,
 * which are collections of actors that share broad-phase bounds for
 * improved collision detection performance.
 *
 * Aggregates are useful for:
 * - Ragdolls (many connected actors)
 * - Debris piles (many small objects)
 * - Vehicles (multiple wheels and chassis)
 * - Destructible objects (fragments after breaking)
 * - Any scene with many related actors
 *
 * Benefits:
 * - Reduced broad-phase overhead
 * - Better cache coherency
 * - Improved performance for groups of related objects
 *
 * Usage:
 * @code
 * AggregateManager manager;
 * manager.initialize(physics, scene);
 *
 * // Create aggregate for ragdoll
 * AggregateManager::AggregateConfig config;
 * config.maxActors = 20;
 * config.enableSelfCollision = false;
 * PxAggregate* ragdollAggregate = manager.createAggregate(config);
 *
 * // Add actors to aggregate
 * manager.addActor(ragdollAggregate, actor1);
 * manager.addActor(ragdollAggregate, actor2);
 *
 * // Add to scene
 * manager.addToScene(ragdollAggregate);
 * @endcode
 */
class AggregateManager {
public:
    /**
     * @brief Configuration for aggregate creation
     */
    struct AggregateConfig {
        PxU32 maxActors = 128;           ///< Maximum number of actors
        bool enableSelfCollision = true;  ///< Enable collision between actors in aggregate
        std::string name;                 ///< Optional name for debugging

        AggregateConfig() = default;
    };

    /**
     * @brief Statistics for an aggregate
     */
    struct AggregateStats {
        PxU32 numActors = 0;              ///< Current number of actors
        PxU32 maxActors = 0;              ///< Maximum number of actors
        PxU32 numShapes = 0;              ///< Total number of shapes
        bool selfCollision = false;       ///< Self-collision enabled
        bool inScene = false;             ///< Added to scene

        void print() const;
    };

    /**
     * @brief Aggregate collection result
     */
    struct AggregateCollection {
        std::vector<PxAggregate*> aggregates;  ///< List of aggregates
        PxU32 totalActors = 0;                 ///< Total actors
        PxU32 totalShapes = 0;                 ///< Total shapes

        void print() const;
    };

public:
    /**
     * @brief Constructor
     */
    AggregateManager();

    /**
     * @brief Destructor
     */
    ~AggregateManager();

    // Disable copy
    AggregateManager(const AggregateManager&) = delete;
    AggregateManager& operator=(const AggregateManager&) = delete;

    /**
     * @brief Initialize the aggregate manager
     * @param physics PhysX physics instance
     * @param scene PhysX scene instance
     * @return true if successful
     */
    bool initialize(PxPhysics* physics, PxScene* scene);

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
    // Aggregate Creation
    // ========================================================================

    /**
     * @brief Create aggregate
     * @param config Aggregate configuration
     * @return Aggregate pointer (or nullptr on failure)
     */
    PxAggregate* createAggregate(const AggregateConfig& config = AggregateConfig());

    /**
     * @brief Create aggregate with specific size
     * @param maxActors Maximum number of actors
     * @param enableSelfCollision Enable self-collision
     * @return Aggregate pointer (or nullptr on failure)
     */
    PxAggregate* createAggregate(PxU32 maxActors, bool enableSelfCollision = true);

    /**
     * @brief Release aggregate
     * @param aggregate Aggregate to release
     * @return true if successful
     */
    bool releaseAggregate(PxAggregate* aggregate);

    /**
     * @brief Release all managed aggregates
     */
    void releaseAllAggregates();

    // ========================================================================
    // Actor Management
    // ========================================================================

    /**
     * @brief Add actor to aggregate
     * @param aggregate Target aggregate
     * @param actor Actor to add
     * @return true if successful
     */
    bool addActor(PxAggregate* aggregate, PxActor* actor);

    /**
     * @brief Remove actor from aggregate
     * @param aggregate Source aggregate
     * @param actor Actor to remove
     * @return true if successful
     */
    bool removeActor(PxAggregate* aggregate, PxActor* actor);

    /**
     * @brief Add multiple actors to aggregate
     * @param aggregate Target aggregate
     * @param actors Vector of actors
     * @return Number of actors successfully added
     */
    PxU32 addActors(PxAggregate* aggregate, const std::vector<PxActor*>& actors);

    /**
     * @brief Remove all actors from aggregate
     * @param aggregate Target aggregate
     * @return Number of actors removed
     */
    PxU32 removeAllActors(PxAggregate* aggregate);

    // ========================================================================
    // Scene Integration
    // ========================================================================

    /**
     * @brief Add aggregate to scene
     * @param aggregate Aggregate to add
     * @return true if successful
     */
    bool addToScene(PxAggregate* aggregate);

    /**
     * @brief Remove aggregate from scene
     * @param aggregate Aggregate to remove
     * @return true if successful
     */
    bool removeFromScene(PxAggregate* aggregate);

    /**
     * @brief Check if aggregate is in scene
     * @param aggregate Aggregate to check
     * @return true if in scene
     */
    bool isInScene(PxAggregate* aggregate) const;

    // ========================================================================
    // Query and Statistics
    // ========================================================================

    /**
     * @brief Get aggregate statistics
     * @param aggregate Aggregate to analyze
     * @return Statistics structure
     */
    static AggregateStats getStats(PxAggregate* aggregate);

    /**
     * @brief Get number of actors in aggregate
     * @param aggregate Aggregate to query
     * @return Number of actors
     */
    static PxU32 getActorCount(PxAggregate* aggregate);

    /**
     * @brief Get actors from aggregate
     * @param aggregate Source aggregate
     * @return Vector of actors
     */
    static std::vector<PxActor*> getActors(PxAggregate* aggregate);

    /**
     * @brief Get maximum number of actors
     * @param aggregate Aggregate to query
     * @return Maximum actor count
     */
    static PxU32 getMaxActors(PxAggregate* aggregate);

    /**
     * @brief Check if self-collision is enabled
     * @param aggregate Aggregate to query
     * @return true if self-collision enabled
     */
    static bool getSelfCollision(PxAggregate* aggregate);

    /**
     * @brief Print aggregate info
     * @param aggregate Aggregate to print
     * @param detailed Print detailed information
     */
    static void printAggregate(PxAggregate* aggregate, bool detailed = false);

    // ========================================================================
    // Batch Operations
    // ========================================================================

    /**
     * @brief Create aggregate from existing actors
     * @param actors Actors to group
     * @param enableSelfCollision Enable self-collision
     * @return Aggregate pointer (or nullptr on failure)
     */
    PxAggregate* createAggregateFromActors(const std::vector<PxActor*>& actors,
                                            bool enableSelfCollision = true);

    /**
     * @brief Split aggregate by dynamic/static
     * @param aggregate Source aggregate
     * @return Two aggregates (dynamic, static)
     *
     * This splits actors into two aggregates based on type
     */
    std::pair<PxAggregate*, PxAggregate*> splitAggregate(PxAggregate* aggregate);

    /**
     * @brief Merge two aggregates
     * @param aggregate1 First aggregate
     * @param aggregate2 Second aggregate
     * @return Merged aggregate (or nullptr on failure)
     *
     * Note: Original aggregates are released
     */
    PxAggregate* mergeAggregates(PxAggregate* aggregate1, PxAggregate* aggregate2);

    // ========================================================================
    // Specialized Aggregate Builders
    // ========================================================================

    /**
     * @brief Create aggregate for ragdoll
     * @param bodyCount Number of body parts
     * @return Aggregate configured for ragdoll
     */
    PxAggregate* createRagdollAggregate(PxU32 bodyCount = 20);

    /**
     * @brief Create aggregate for debris
     * @param debrisCount Number of debris pieces
     * @return Aggregate configured for debris
     */
    PxAggregate* createDebrisAggregate(PxU32 debrisCount = 100);

    /**
     * @brief Create aggregate for vehicle
     * @param wheelCount Number of wheels
     * @return Aggregate configured for vehicle
     */
    PxAggregate* createVehicleAggregate(PxU32 wheelCount = 4);

    // ========================================================================
    // Management
    // ========================================================================

    /**
     * @brief Get number of managed aggregates
     * @return Aggregate count
     */
    PxU32 getAggregateCount() const;

    /**
     * @brief Get managed aggregate by index
     * @param index Aggregate index
     * @return Aggregate pointer (or nullptr)
     */
    PxAggregate* getAggregate(PxU32 index) const;

    /**
     * @brief Get all managed aggregates
     * @return Vector of aggregates
     */
    std::vector<PxAggregate*> getAllAggregates() const;

    /**
     * @brief Get aggregate collection statistics
     * @return Collection statistics
     */
    AggregateCollection getCollection() const;

    /**
     * @brief Print all aggregates
     */
    void printAllAggregates() const;

    // ========================================================================
    // Configuration
    // ========================================================================

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

    /**
     * @brief Set scene for future operations
     * @param scene Scene instance
     */
    void setScene(PxScene* scene);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace PhysXWrapper
