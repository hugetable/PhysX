/**
 * @file BVHBuilder.h
 * @brief BVH (Bounding Volume Hierarchy) structure builder for PhysX
 *
 * This class provides utilities for building BVH acceleration structures
 * for actors with large numbers of shapes. Using a BVH significantly
 * improves scene query performance for complex actors.
 *
 * Based on PhysX SnippetBVHStructure
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <vector>
#include <string>

using namespace physx;

namespace PhysXWrapper {

/**
 * @class BVHBuilder
 * @brief Builder for BVH acceleration structures
 *
 * BVH (Bounding Volume Hierarchy) structures are spatial acceleration data structures
 * that organize bounding boxes in a tree hierarchy. In PhysX, BVHs can be precomputed
 * for actors with many shapes to significantly improve:
 * - Scene query performance (raycasts, sweeps, overlaps)
 * - Simulation performance (reduced shape bound synchronization)
 * - Build time (shapes not included in scene-wide AABB tree)
 *
 * Usage:
 * @code
 * BVHBuilder builder;
 * builder.initialize(physics);
 *
 * // Option 1: Build BVH from bounds list
 * std::vector<PxBounds3> bounds = {...};
 * PxBVH* bvh = builder.buildFromBounds(bounds);
 *
 * // Option 2: Build BVH from actor shapes
 * PxBVH* bvh = builder.buildFromActor(rigidActor);
 *
 * // Add actor to scene with BVH
 * scene->addActor(*actor, bvh);
 * bvh->release();
 * @endcode
 */
class BVHBuilder {
public:
    /**
     * @brief Configuration for BVH construction
     */
    struct BVHConfig {
        /// Enlargement parameter for BVH nodes (larger values = fewer nodes, faster build, slower queries)
        PxReal enlargement = 0.0f;

        /// Number of primitives per leaf node (typical range: 1-4)
        PxU32 numPrimsPerLeaf = 4;

        /// Whether to print build statistics
        bool printStats = false;

        BVHConfig() = default;
    };

    /**
     * @brief Statistics about a built BVH
     */
    struct BVHStats {
        PxU32 numNodes = 0;              ///< Total number of nodes in the BVH
        PxU32 numLeafNodes = 0;          ///< Number of leaf nodes
        PxU32 numPrimitives = 0;         ///< Number of primitives
        PxU32 maxDepth = 0;              ///< Maximum tree depth
        PxReal totalVolume = 0.0f;       ///< Total bounding volume

        void print() const;
    };

    /**
     * @brief Actor configuration for BVH-optimized actors
     */
    struct ActorWithBVHConfig {
        PxTransform transform = PxTransform(PxIdentity);  ///< Initial transform
        PxReal density = 10.0f;                           ///< Density for mass calculation
        bool isDynamic = true;                            ///< Dynamic vs static
        bool useAggregate = false;                        ///< Create an aggregate
        bool selfCollide = false;                         ///< Enable self-collision (aggregate only)
        PxU32 maxAggregateSize = 256;                     ///< Max shapes in aggregate

        ActorWithBVHConfig() = default;
    };

public:
    /**
     * @brief Constructor
     */
    BVHBuilder();

    /**
     * @brief Destructor
     */
    ~BVHBuilder();

    // Disable copy
    BVHBuilder(const BVHBuilder&) = delete;
    BVHBuilder& operator=(const BVHBuilder&) = delete;

    /**
     * @brief Initialize the BVH builder
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
    // BVH Construction
    // ========================================================================

    /**
     * @brief Build BVH from a list of bounding boxes
     * @param bounds List of bounding boxes
     * @param config BVH configuration
     * @return Created BVH structure (caller must release)
     */
    PxBVH* buildFromBounds(const std::vector<PxBounds3>& bounds,
                           const BVHConfig& config = BVHConfig());

    /**
     * @brief Build BVH from bounding boxes array
     * @param bounds Array of bounding boxes
     * @param numBounds Number of bounds
     * @param config BVH configuration
     * @return Created BVH structure (caller must release)
     */
    PxBVH* buildFromBounds(const PxBounds3* bounds, PxU32 numBounds,
                           const BVHConfig& config = BVHConfig());

    /**
     * @brief Build BVH from actor's shape local bounds
     * @param actor Actor with shapes
     * @param config BVH configuration
     * @return Created BVH structure (caller must release)
     */
    PxBVH* buildFromActor(PxRigidActor* actor,
                          const BVHConfig& config = BVHConfig());

    /**
     * @brief Build BVH from a list of shapes
     * @param shapes List of shapes
     * @param config BVH configuration
     * @return Created BVH structure (caller must release)
     */
    PxBVH* buildFromShapes(const std::vector<PxShape*>& shapes,
                           const BVHConfig& config = BVHConfig());

    // ========================================================================
    // Actor Integration
    // ========================================================================

    /**
     * @brief Add actor to scene with precomputed BVH
     * @param scene Scene to add actor to
     * @param actor Actor to add
     * @param bvh Precomputed BVH structure
     * @return true if successful
     */
    bool addActorWithBVH(PxScene* scene, PxRigidActor* actor, PxBVH* bvh);

    /**
     * @brief Add actor to aggregate with precomputed BVH
     * @param aggregate Aggregate to add actor to
     * @param actor Actor to add
     * @param bvh Precomputed BVH structure
     * @return true if successful
     */
    bool addActorToAggregate(PxAggregate* aggregate, PxRigidActor* actor, PxBVH* bvh);

    /**
     * @brief Create actor with shapes and BVH, then add to scene
     * @param scene Scene to add actor to
     * @param shapes Shapes to attach
     * @param config Actor configuration
     * @param material Material for shapes (if null, uses default)
     * @return Created actor (or nullptr on failure)
     */
    PxRigidActor* createActorWithBVH(PxScene* scene,
                                     const std::vector<PxShape*>& shapes,
                                     const ActorWithBVHConfig& config = ActorWithBVHConfig(),
                                     PxMaterial* material = nullptr);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /**
     * @brief Extract local bounds from actor shapes
     * @param actor Actor to extract bounds from
     * @param[out] outBounds Output bounds array
     * @return Number of bounds extracted
     */
    static PxU32 extractActorShapeBounds(PxRigidActor* actor,
                                         std::vector<PxBounds3>& outBounds);

    /**
     * @brief Extract local bounds from shape list
     * @param shapes List of shapes
     * @param[out] outBounds Output bounds array
     */
    static void extractShapeBounds(const std::vector<PxShape*>& shapes,
                                   std::vector<PxBounds3>& outBounds);

    /**
     * @brief Compute statistics for a BVH
     * @param bvh BVH to analyze
     * @return Statistics structure
     */
    static BVHStats computeStats(PxBVH* bvh);

    /**
     * @brief Get total volume of bounding boxes
     * @param bounds List of bounding boxes
     * @return Total volume
     */
    static PxReal computeTotalVolume(const std::vector<PxBounds3>& bounds);

    /**
     * @brief Compute AABB of all bounding boxes
     * @param bounds List of bounding boxes
     * @return Combined AABB
     */
    static PxBounds3 computeAABB(const std::vector<PxBounds3>& bounds);

    // ========================================================================
    // Advanced Features
    // ========================================================================

    /**
     * @brief Create a large compound sphere actor (test utility)
     * @param scene Scene to add actor to
     * @param transform Initial transform
     * @param density Number of small spheres per dimension
     * @param largeRadius Radius of the large sphere
     * @param smallRadius Radius of each small sphere
     * @param useAggregate Whether to use aggregate
     * @return Created actor
     */
    PxRigidDynamic* createCompoundSphere(PxScene* scene,
                                         const PxTransform& transform,
                                         PxU32 density,
                                         PxReal largeRadius,
                                         PxReal smallRadius,
                                         bool useAggregate = false);

    /**
     * @brief Create a grid of boxes as a single actor with BVH
     * @param scene Scene to add actor to
     * @param transform Initial transform
     * @param gridSize Number of boxes per dimension
     * @param boxHalfExtents Half extents of each box
     * @param spacing Spacing between boxes
     * @param isDynamic Whether the actor is dynamic
     * @return Created actor
     */
    PxRigidActor* createBoxGrid(PxScene* scene,
                                const PxTransform& transform,
                                PxU32 gridSize,
                                const PxVec3& boxHalfExtents,
                                PxReal spacing,
                                bool isDynamic = true);

    /**
     * @brief Refit BVH (update bounds without rebuilding tree)
     * Note: PhysX does not support refitting, this is a placeholder
     * @param bvh BVH to refit
     * @param newBounds Updated bounding boxes
     * @return Always returns false (not supported)
     */
    bool refitBVH(PxBVH* bvh, const std::vector<PxBounds3>& newBounds);

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set default BVH configuration
     * @param config Configuration to use for future builds
     */
    void setDefaultConfig(const BVHConfig& config);

    /**
     * @brief Get default BVH configuration
     * @return Current default configuration
     */
    const BVHConfig& getDefaultConfig() const;

    /**
     * @brief Get PhysX physics instance
     * @return Physics instance
     */
    PxPhysics* getPhysics() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace PhysXWrapper
