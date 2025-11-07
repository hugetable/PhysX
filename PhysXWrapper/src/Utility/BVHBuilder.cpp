/**
 * @file BVHBuilder.cpp
 * @brief Implementation of BVHBuilder class
 */

#include "Utility/BVHBuilder.h"
#include <iostream>
#include <cmath>

namespace PhysXWrapper {

// ============================================================================
// BVHStats Implementation
// ============================================================================

void BVHBuilder::BVHStats::print() const
{
    std::cout << "BVH Statistics:" << std::endl;
    std::cout << "  Nodes: " << numNodes << std::endl;
    std::cout << "  Leaf nodes: " << numLeafNodes << std::endl;
    std::cout << "  Primitives: " << numPrimitives << std::endl;
    std::cout << "  Max depth: " << maxDepth << std::endl;
    std::cout << "  Total volume: " << totalVolume << std::endl;
}

// ============================================================================
// BVHBuilder::Impl
// ============================================================================

class BVHBuilder::Impl {
public:
    PxPhysics* m_physics = nullptr;
    PxMaterial* m_defaultMaterial = nullptr;
    BVHConfig m_defaultConfig;

    bool m_initialized = false;
};

// ============================================================================
// Construction/Destruction
// ============================================================================

BVHBuilder::BVHBuilder()
    : m_impl(std::make_unique<Impl>())
{
}

BVHBuilder::~BVHBuilder()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool BVHBuilder::initialize(PxPhysics* physics)
{
    if (!physics) {
        std::cerr << "BVHBuilder::initialize: physics is null" << std::endl;
        return false;
    }

    m_impl->m_physics = physics;

    // Create default material
    m_impl->m_defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);
    if (!m_impl->m_defaultMaterial) {
        std::cerr << "BVHBuilder::initialize: Failed to create default material" << std::endl;
        return false;
    }

    m_impl->m_initialized = true;
    return true;
}

void BVHBuilder::cleanup()
{
    if (m_impl->m_defaultMaterial) {
        m_impl->m_defaultMaterial->release();
        m_impl->m_defaultMaterial = nullptr;
    }

    m_impl->m_initialized = false;
}

bool BVHBuilder::isInitialized() const
{
    return m_impl->m_initialized;
}

// ============================================================================
// BVH Construction
// ============================================================================

PxBVH* BVHBuilder::buildFromBounds(const std::vector<PxBounds3>& bounds,
                                    const BVHConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "BVHBuilder::buildFromBounds: Not initialized" << std::endl;
        return nullptr;
    }

    if (bounds.empty()) {
        std::cerr << "BVHBuilder::buildFromBounds: Empty bounds list" << std::endl;
        return nullptr;
    }

    return buildFromBounds(bounds.data(), static_cast<PxU32>(bounds.size()), config);
}

PxBVH* BVHBuilder::buildFromBounds(const PxBounds3* bounds, PxU32 numBounds,
                                    const BVHConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "BVHBuilder::buildFromBounds: Not initialized" << std::endl;
        return nullptr;
    }

    if (!bounds || numBounds == 0) {
        std::cerr << "BVHBuilder::buildFromBounds: Invalid bounds data" << std::endl;
        return nullptr;
    }

    if (config.printStats) {
        std::cout << "Building BVH from " << numBounds << " bounds..." << std::endl;
    }

    // Setup BVH descriptor
    PxBVHDesc bvhDesc;
    bvhDesc.bounds.count = numBounds;
    bvhDesc.bounds.data = bounds;
    bvhDesc.bounds.stride = sizeof(PxBounds3);
    bvhDesc.enlargement = config.enlargement;

    // Cook the BVH
    PxBVH* bvh = PxCreateBVH(bvhDesc, m_impl->m_physics->getPhysicsInsertionCallback());

    if (!bvh) {
        std::cerr << "BVHBuilder::buildFromBounds: Failed to create BVH" << std::endl;
        return nullptr;
    }

    if (config.printStats) {
        BVHStats stats = computeStats(bvh);
        stats.print();
    }

    return bvh;
}

PxBVH* BVHBuilder::buildFromActor(PxRigidActor* actor, const BVHConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "BVHBuilder::buildFromActor: Not initialized" << std::endl;
        return nullptr;
    }

    if (!actor) {
        std::cerr << "BVHBuilder::buildFromActor: actor is null" << std::endl;
        return nullptr;
    }

    // Get shape local bounds from the actor
    PxU32 numBounds = 0;
    PxBounds3* bounds = PxRigidActorExt::getRigidActorShapeLocalBoundsList(*actor, numBounds);

    if (!bounds || numBounds == 0) {
        std::cerr << "BVHBuilder::buildFromActor: No bounds retrieved" << std::endl;
        return nullptr;
    }

    if (config.printStats) {
        std::cout << "Building BVH for actor with " << numBounds << " shapes..." << std::endl;
    }

    // Build BVH from bounds
    PxBVH* bvh = buildFromBounds(bounds, numBounds, config);

    // Release temporary bounds memory
    PxGetFoundation().getAllocatorCallback().deallocate(bounds);

    return bvh;
}

PxBVH* BVHBuilder::buildFromShapes(const std::vector<PxShape*>& shapes,
                                    const BVHConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "BVHBuilder::buildFromShapes: Not initialized" << std::endl;
        return nullptr;
    }

    if (shapes.empty()) {
        std::cerr << "BVHBuilder::buildFromShapes: Empty shapes list" << std::endl;
        return nullptr;
    }

    // Extract bounds from shapes
    std::vector<PxBounds3> bounds;
    extractShapeBounds(shapes, bounds);

    return buildFromBounds(bounds, config);
}

// ============================================================================
// Actor Integration
// ============================================================================

bool BVHBuilder::addActorWithBVH(PxScene* scene, PxRigidActor* actor, PxBVH* bvh)
{
    if (!scene) {
        std::cerr << "BVHBuilder::addActorWithBVH: scene is null" << std::endl;
        return false;
    }

    if (!actor) {
        std::cerr << "BVHBuilder::addActorWithBVH: actor is null" << std::endl;
        return false;
    }

    if (!bvh) {
        std::cerr << "BVHBuilder::addActorWithBVH: bvh is null" << std::endl;
        return false;
    }

    scene->addActor(*actor, bvh);
    return true;
}

bool BVHBuilder::addActorToAggregate(PxAggregate* aggregate, PxRigidActor* actor, PxBVH* bvh)
{
    if (!aggregate) {
        std::cerr << "BVHBuilder::addActorToAggregate: aggregate is null" << std::endl;
        return false;
    }

    if (!actor) {
        std::cerr << "BVHBuilder::addActorToAggregate: actor is null" << std::endl;
        return false;
    }

    if (!bvh) {
        std::cerr << "BVHBuilder::addActorToAggregate: bvh is null" << std::endl;
        return false;
    }

    return aggregate->addActor(*actor, bvh);
}

PxRigidActor* BVHBuilder::createActorWithBVH(PxScene* scene,
                                              const std::vector<PxShape*>& shapes,
                                              const ActorWithBVHConfig& config,
                                              PxMaterial* material)
{
    if (!m_impl->m_initialized) {
        std::cerr << "BVHBuilder::createActorWithBVH: Not initialized" << std::endl;
        return nullptr;
    }

    if (!scene) {
        std::cerr << "BVHBuilder::createActorWithBVH: scene is null" << std::endl;
        return nullptr;
    }

    if (shapes.empty()) {
        std::cerr << "BVHBuilder::createActorWithBVH: Empty shapes list" << std::endl;
        return nullptr;
    }

    // Use provided material or default
    PxMaterial* useMaterial = material ? material : m_impl->m_defaultMaterial;

    // Create actor
    PxRigidActor* actor = nullptr;
    if (config.isDynamic) {
        actor = m_impl->m_physics->createRigidDynamic(config.transform);
    } else {
        actor = m_impl->m_physics->createRigidStatic(config.transform);
    }

    if (!actor) {
        std::cerr << "BVHBuilder::createActorWithBVH: Failed to create actor" << std::endl;
        return nullptr;
    }

    // Attach shapes
    for (PxShape* shape : shapes) {
        actor->attachShape(*shape);
    }

    // Update mass for dynamic actors
    if (config.isDynamic) {
        PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>();
        if (dynamicActor) {
            PxRigidBodyExt::updateMassAndInertia(*dynamicActor, config.density);
        }
    }

    // Build BVH
    PxBVH* bvh = buildFromActor(actor);
    if (!bvh) {
        std::cerr << "BVHBuilder::createActorWithBVH: Failed to build BVH" << std::endl;
        actor->release();
        return nullptr;
    }

    // Add to scene
    if (config.useAggregate) {
        PxAggregate* aggregate = m_impl->m_physics->createAggregate(
            1,
            config.maxAggregateSize,
            config.selfCollide
        );

        if (aggregate) {
            aggregate->addActor(*actor, bvh);
            scene->addAggregate(*aggregate);
        } else {
            std::cerr << "BVHBuilder::createActorWithBVH: Failed to create aggregate, adding actor directly" << std::endl;
            scene->addActor(*actor, bvh);
        }
    } else {
        scene->addActor(*actor, bvh);
    }

    bvh->release();

    return actor;
}

// ============================================================================
// Utility Functions
// ============================================================================

PxU32 BVHBuilder::extractActorShapeBounds(PxRigidActor* actor,
                                          std::vector<PxBounds3>& outBounds)
{
    if (!actor) {
        return 0;
    }

    PxU32 numBounds = 0;
    PxBounds3* bounds = PxRigidActorExt::getRigidActorShapeLocalBoundsList(*actor, numBounds);

    if (bounds && numBounds > 0) {
        outBounds.assign(bounds, bounds + numBounds);
        PxGetFoundation().getAllocatorCallback().deallocate(bounds);
    }

    return numBounds;
}

void BVHBuilder::extractShapeBounds(const std::vector<PxShape*>& shapes,
                                    std::vector<PxBounds3>& outBounds)
{
    outBounds.clear();
    outBounds.reserve(shapes.size());

    for (PxShape* shape : shapes) {
        if (shape) {
            PxGeometryHolder geom = shape->getGeometry();
            PxBounds3 bounds;
            // PhysX 5.x: use computeGeomBounds instead of getWorldBounds
            PxGeometryQuery::computeGeomBounds(
                bounds,
                geom.any(),
                shape->getLocalPose(),
                0.0f,  // offset
                1.0f   // inflation (scale)
            );
            outBounds.push_back(bounds);
        }
    }
}

BVHBuilder::BVHStats BVHBuilder::computeStats(PxBVH* bvh)
{
    BVHStats stats;

    if (!bvh) {
        return stats;
    }

    // Get basic counts
    stats.numNodes = bvh->getNbBounds();

    // Note: PhysX doesn't provide direct access to internal BVH structure,
    // so we can only get basic information
    stats.numPrimitives = bvh->getNbBounds();

    // Estimate leaf nodes (typically half of total nodes in a balanced tree)
    stats.numLeafNodes = stats.numNodes / 2;

    // Estimate max depth (log2 of leaf nodes)
    if (stats.numLeafNodes > 0) {
        stats.maxDepth = static_cast<PxU32>(std::log2(stats.numLeafNodes)) + 1;
    }

    return stats;
}

PxReal BVHBuilder::computeTotalVolume(const std::vector<PxBounds3>& bounds)
{
    PxReal totalVolume = 0.0f;

    for (const PxBounds3& bound : bounds) {
        if (bound.isValid()) {
            PxVec3 extents = bound.getExtents();
            PxReal volume = extents.x * extents.y * extents.z * 8.0f; // Full box volume
            totalVolume += volume;
        }
    }

    return totalVolume;
}

PxBounds3 BVHBuilder::computeAABB(const std::vector<PxBounds3>& bounds)
{
    if (bounds.empty()) {
        return PxBounds3::empty();
    }

    PxBounds3 aabb = bounds[0];

    for (size_t i = 1; i < bounds.size(); i++) {
        if (bounds[i].isValid()) {
            aabb.include(bounds[i]);
        }
    }

    return aabb;
}

// ============================================================================
// Advanced Features
// ============================================================================

PxRigidDynamic* BVHBuilder::createCompoundSphere(PxScene* scene,
                                                  const PxTransform& transform,
                                                  PxU32 density,
                                                  PxReal largeRadius,
                                                  PxReal smallRadius,
                                                  bool useAggregate)
{
    if (!m_impl->m_initialized) {
        std::cerr << "BVHBuilder::createCompoundSphere: Not initialized" << std::endl;
        return nullptr;
    }

    if (!scene) {
        std::cerr << "BVHBuilder::createCompoundSphere: scene is null" << std::endl;
        return nullptr;
    }

    // Create dynamic actor
    PxRigidDynamic* body = m_impl->m_physics->createRigidDynamic(transform);
    if (!body) {
        return nullptr;
    }

    // Generate sphere shapes in a spherical distribution
    const float gStep = PxPi / float(density);
    const float tStep = 2.0f * PxPi / float(density);

    for (PxU32 i = 0; i < density; i++) {
        for (PxU32 j = 0; j < density; j++) {
            const float sinG = PxSin(gStep * i);
            const float cosG = PxCos(gStep * i);
            const float sinT = PxSin(tStep * j);
            const float cosT = PxCos(tStep * j);

            PxTransform localTm(PxVec3(
                largeRadius * sinG * cosT,
                largeRadius * sinG * sinT,
                largeRadius * cosG
            ));

            PxShape* shape = m_impl->m_physics->createShape(
                PxSphereGeometry(smallRadius),
                *m_impl->m_defaultMaterial
            );
            shape->setLocalPose(localTm);
            body->attachShape(*shape);
            shape->release();
        }
    }

    // Update mass and inertia
    PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);

    // Build BVH
    PxBVH* bvh = buildFromActor(body);
    if (!bvh) {
        body->release();
        return nullptr;
    }

    // Add to scene
    if (useAggregate) {
        PxAggregate* aggregate = m_impl->m_physics->createAggregate(
            1,
            body->getNbShapes(),
            false
        );

        if (aggregate) {
            aggregate->addActor(*body, bvh);
            scene->addAggregate(*aggregate);
        } else {
            scene->addActor(*body, bvh);
        }
    } else {
        scene->addActor(*body, bvh);
    }

    bvh->release();

    return body;
}

PxRigidActor* BVHBuilder::createBoxGrid(PxScene* scene,
                                        const PxTransform& transform,
                                        PxU32 gridSize,
                                        const PxVec3& boxHalfExtents,
                                        PxReal spacing,
                                        bool isDynamic)
{
    if (!m_impl->m_initialized) {
        std::cerr << "BVHBuilder::createBoxGrid: Not initialized" << std::endl;
        return nullptr;
    }

    if (!scene) {
        std::cerr << "BVHBuilder::createBoxGrid: scene is null" << std::endl;
        return nullptr;
    }

    // Create actor
    PxRigidActor* actor = nullptr;
    if (isDynamic) {
        actor = m_impl->m_physics->createRigidDynamic(transform);
    } else {
        actor = m_impl->m_physics->createRigidStatic(transform);
    }

    if (!actor) {
        return nullptr;
    }

    // Create grid of boxes
    PxReal offset = -(gridSize - 1) * spacing * 0.5f;

    for (PxU32 x = 0; x < gridSize; x++) {
        for (PxU32 y = 0; y < gridSize; y++) {
            for (PxU32 z = 0; z < gridSize; z++) {
                PxTransform localTm(PxVec3(
                    offset + x * spacing,
                    offset + y * spacing,
                    offset + z * spacing
                ));

                PxShape* shape = m_impl->m_physics->createShape(
                    PxBoxGeometry(boxHalfExtents),
                    *m_impl->m_defaultMaterial
                );
                shape->setLocalPose(localTm);
                actor->attachShape(*shape);
                shape->release();
            }
        }
    }

    // Update mass for dynamic actors
    if (isDynamic) {
        PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>();
        if (dynamicActor) {
            PxRigidBodyExt::updateMassAndInertia(*dynamicActor, 10.0f);
        }
    }

    // Build BVH
    PxBVH* bvh = buildFromActor(actor);
    if (!bvh) {
        actor->release();
        return nullptr;
    }

    // Add to scene
    scene->addActor(*actor, bvh);
    bvh->release();

    return actor;
}

bool BVHBuilder::refitBVH(PxBVH* bvh, const std::vector<PxBounds3>& newBounds)
{
    // PhysX does not support refitting BVH structures
    // Would need to rebuild from scratch
    return false;
}

// ========================================================================
// Configuration
// ========================================================================

void BVHBuilder::setDefaultConfig(const BVHConfig& config)
{
    m_impl->m_defaultConfig = config;
}

const BVHBuilder::BVHConfig& BVHBuilder::getDefaultConfig() const
{
    return m_impl->m_defaultConfig;
}

PxPhysics* BVHBuilder::getPhysics() const
{
    return m_impl->m_physics;
}

} // namespace PhysXWrapper
