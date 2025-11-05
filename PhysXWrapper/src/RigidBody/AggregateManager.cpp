/**
 * @file AggregateManager.cpp
 * @brief Implementation of AggregateManager class
 */

#include "RigidBody/AggregateManager.h"
#include <iostream>
#include <algorithm>

namespace PhysXWrapper {

// ============================================================================
// AggregateStats Implementation
// ============================================================================

void AggregateManager::AggregateStats::print() const
{
    std::cout << "Aggregate Statistics:" << std::endl;
    std::cout << "  Actors: " << numActors << " / " << maxActors << std::endl;
    std::cout << "  Shapes: " << numShapes << std::endl;
    std::cout << "  Self-collision: " << (selfCollision ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  In scene: " << (inScene ? "Yes" : "No") << std::endl;
}

void AggregateManager::AggregateCollection::print() const
{
    std::cout << "Aggregate Collection:" << std::endl;
    std::cout << "  Aggregates: " << aggregates.size() << std::endl;
    std::cout << "  Total Actors: " << totalActors << std::endl;
    std::cout << "  Total Shapes: " << totalShapes << std::endl;
}

// ============================================================================
// AggregateManager::Impl
// ============================================================================

class AggregateManager::Impl {
public:
    PxPhysics* m_physics = nullptr;
    PxScene* m_scene = nullptr;

    std::vector<PxAggregate*> m_aggregates;

    bool m_initialized = false;
};

// ============================================================================
// Construction/Destruction
// ============================================================================

AggregateManager::AggregateManager()
    : m_impl(std::make_unique<Impl>())
{
}

AggregateManager::~AggregateManager()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool AggregateManager::initialize(PxPhysics* physics, PxScene* scene)
{
    if (!physics) {
        std::cerr << "AggregateManager::initialize: physics is null" << std::endl;
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_scene = scene;

    m_impl->m_initialized = true;
    return true;
}

void AggregateManager::cleanup()
{
    releaseAllAggregates();
    m_impl->m_initialized = false;
}

bool AggregateManager::isInitialized() const
{
    return m_impl->m_initialized;
}

// ============================================================================
// Aggregate Creation
// ============================================================================

PxAggregate* AggregateManager::createAggregate(const AggregateConfig& config)
{
    if (!m_impl->m_initialized) {
        std::cerr << "AggregateManager::createAggregate: Not initialized" << std::endl;
        return nullptr;
    }

    PxAggregate* aggregate = m_impl->m_physics->createAggregate(
        config.maxActors,
        config.enableSelfCollision
    );

    if (!aggregate) {
        std::cerr << "AggregateManager::createAggregate: Failed to create aggregate" << std::endl;
        return nullptr;
    }

    m_impl->m_aggregates.push_back(aggregate);
    return aggregate;
}

PxAggregate* AggregateManager::createAggregate(PxU32 maxActors, bool enableSelfCollision)
{
    AggregateConfig config;
    config.maxActors = maxActors;
    config.enableSelfCollision = enableSelfCollision;
    return createAggregate(config);
}

bool AggregateManager::releaseAggregate(PxAggregate* aggregate)
{
    if (!aggregate) {
        return false;
    }

    // Remove from list
    auto it = std::find(m_impl->m_aggregates.begin(), m_impl->m_aggregates.end(), aggregate);
    if (it != m_impl->m_aggregates.end()) {
        m_impl->m_aggregates.erase(it);
    }

    // Remove from scene if needed
    if (m_impl->m_scene && m_impl->m_scene->getAggregate(*aggregate)) {
        m_impl->m_scene->removeAggregate(*aggregate);
    }

    // Release aggregate
    aggregate->release();
    return true;
}

void AggregateManager::releaseAllAggregates()
{
    for (PxAggregate* aggregate : m_impl->m_aggregates) {
        if (aggregate) {
            // Remove from scene if needed
            if (m_impl->m_scene && m_impl->m_scene->getAggregate(*aggregate)) {
                m_impl->m_scene->removeAggregate(*aggregate);
            }
            aggregate->release();
        }
    }
    m_impl->m_aggregates.clear();
}

// ============================================================================
// Actor Management
// ============================================================================

bool AggregateManager::addActor(PxAggregate* aggregate, PxActor* actor)
{
    if (!aggregate || !actor) {
        return false;
    }

    return aggregate->addActor(*actor);
}

bool AggregateManager::removeActor(PxAggregate* aggregate, PxActor* actor)
{
    if (!aggregate || !actor) {
        return false;
    }

    return aggregate->removeActor(*actor);
}

PxU32 AggregateManager::addActors(PxAggregate* aggregate, const std::vector<PxActor*>& actors)
{
    if (!aggregate) {
        return 0;
    }

    PxU32 count = 0;
    for (PxActor* actor : actors) {
        if (addActor(aggregate, actor)) {
            count++;
        }
    }

    return count;
}

PxU32 AggregateManager::removeAllActors(PxAggregate* aggregate)
{
    if (!aggregate) {
        return 0;
    }

    PxU32 numActors = aggregate->getNbActors();
    std::vector<PxActor*> actors(numActors);

    aggregate->getActors(actors.data(), numActors);

    for (PxActor* actor : actors) {
        aggregate->removeActor(*actor);
    }

    return numActors;
}

// ============================================================================
// Scene Integration
// ============================================================================

bool AggregateManager::addToScene(PxAggregate* aggregate)
{
    if (!aggregate || !m_impl->m_scene) {
        return false;
    }

    return m_impl->m_scene->addAggregate(*aggregate);
}

bool AggregateManager::removeFromScene(PxAggregate* aggregate)
{
    if (!aggregate || !m_impl->m_scene) {
        return false;
    }

    return m_impl->m_scene->removeAggregate(*aggregate);
}

bool AggregateManager::isInScene(PxAggregate* aggregate) const
{
    if (!aggregate || !m_impl->m_scene) {
        return false;
    }

    return m_impl->m_scene->getAggregate(*aggregate) != nullptr;
}

// ============================================================================
// Query and Statistics
// ============================================================================

AggregateManager::AggregateStats AggregateManager::getStats(PxAggregate* aggregate)
{
    AggregateStats stats;

    if (!aggregate) {
        return stats;
    }

    stats.numActors = aggregate->getNbActors();
    stats.maxActors = aggregate->getMaxNbActors();
    stats.selfCollision = aggregate->getSelfCollision();

    // Count shapes
    std::vector<PxActor*> actors(stats.numActors);
    aggregate->getActors(actors.data(), stats.numActors);

    for (PxActor* actor : actors) {
        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        if (rigidActor) {
            stats.numShapes += rigidActor->getNbShapes();
        }
    }

    // Check if in scene (requires scene reference)
    PxScene* scene = aggregate->getScene();
    stats.inScene = (scene != nullptr);

    return stats;
}

PxU32 AggregateManager::getActorCount(PxAggregate* aggregate)
{
    if (!aggregate) {
        return 0;
    }

    return aggregate->getNbActors();
}

std::vector<PxActor*> AggregateManager::getActors(PxAggregate* aggregate)
{
    std::vector<PxActor*> actors;

    if (!aggregate) {
        return actors;
    }

    PxU32 numActors = aggregate->getNbActors();
    actors.resize(numActors);
    aggregate->getActors(actors.data(), numActors);

    return actors;
}

PxU32 AggregateManager::getMaxActors(PxAggregate* aggregate)
{
    if (!aggregate) {
        return 0;
    }

    return aggregate->getMaxNbActors();
}

bool AggregateManager::getSelfCollision(PxAggregate* aggregate)
{
    if (!aggregate) {
        return false;
    }

    return aggregate->getSelfCollision();
}

void AggregateManager::printAggregate(PxAggregate* aggregate, bool detailed)
{
    if (!aggregate) {
        std::cout << "Aggregate is null" << std::endl;
        return;
    }

    AggregateStats stats = getStats(aggregate);
    std::cout << "Aggregate " << aggregate << ":" << std::endl;
    stats.print();

    if (detailed) {
        std::cout << "\nActors:" << std::endl;
        std::vector<PxActor*> actors = getActors(aggregate);
        for (size_t i = 0; i < actors.size(); i++) {
            PxActor* actor = actors[i];
            std::cout << "  [" << i << "] " << actor->getConcreteTypeName()
                      << " (Type: " << actor->getType() << ")" << std::endl;
        }
    }
}

// ============================================================================
// Batch Operations
// ============================================================================

PxAggregate* AggregateManager::createAggregateFromActors(const std::vector<PxActor*>& actors,
                                                          bool enableSelfCollision)
{
    if (actors.empty()) {
        return nullptr;
    }

    PxAggregate* aggregate = createAggregate(
        static_cast<PxU32>(actors.size()),
        enableSelfCollision
    );

    if (!aggregate) {
        return nullptr;
    }

    addActors(aggregate, actors);
    return aggregate;
}

std::pair<PxAggregate*, PxAggregate*> AggregateManager::splitAggregate(PxAggregate* aggregate)
{
    if (!aggregate) {
        return {nullptr, nullptr};
    }

    std::vector<PxActor*> allActors = getActors(aggregate);
    std::vector<PxActor*> dynamicActors;
    std::vector<PxActor*> staticActors;

    // Split by type
    for (PxActor* actor : allActors) {
        if (actor->getType() == PxActorType::eRIGID_DYNAMIC) {
            dynamicActors.push_back(actor);
        } else if (actor->getType() == PxActorType::eRIGID_STATIC) {
            staticActors.push_back(actor);
        }
    }

    // Create new aggregates
    PxAggregate* dynamicAggregate = nullptr;
    PxAggregate* staticAggregate = nullptr;

    if (!dynamicActors.empty()) {
        dynamicAggregate = createAggregateFromActors(dynamicActors, getSelfCollision(aggregate));
    }

    if (!staticActors.empty()) {
        staticAggregate = createAggregateFromActors(staticActors, getSelfCollision(aggregate));
    }

    return {dynamicAggregate, staticAggregate};
}

PxAggregate* AggregateManager::mergeAggregates(PxAggregate* aggregate1, PxAggregate* aggregate2)
{
    if (!aggregate1 || !aggregate2) {
        return nullptr;
    }

    // Get all actors
    std::vector<PxActor*> actors1 = getActors(aggregate1);
    std::vector<PxActor*> actors2 = getActors(aggregate2);

    // Merge actor lists
    std::vector<PxActor*> allActors;
    allActors.reserve(actors1.size() + actors2.size());
    allActors.insert(allActors.end(), actors1.begin(), actors1.end());
    allActors.insert(allActors.end(), actors2.begin(), actors2.end());

    // Create merged aggregate
    bool selfCollision = getSelfCollision(aggregate1) || getSelfCollision(aggregate2);
    PxAggregate* merged = createAggregateFromActors(allActors, selfCollision);

    if (merged) {
        // Release original aggregates
        releaseAggregate(aggregate1);
        releaseAggregate(aggregate2);
    }

    return merged;
}

// ============================================================================
// Specialized Aggregate Builders
// ============================================================================

PxAggregate* AggregateManager::createRagdollAggregate(PxU32 bodyCount)
{
    AggregateConfig config;
    config.maxActors = bodyCount;
    config.enableSelfCollision = false; // Ragdoll parts typically don't self-collide
    return createAggregate(config);
}

PxAggregate* AggregateManager::createDebrisAggregate(PxU32 debrisCount)
{
    AggregateConfig config;
    config.maxActors = debrisCount;
    config.enableSelfCollision = true; // Debris pieces collide with each other
    return createAggregate(config);
}

PxAggregate* AggregateManager::createVehicleAggregate(PxU32 wheelCount)
{
    AggregateConfig config;
    config.maxActors = wheelCount + 1; // Wheels + chassis
    config.enableSelfCollision = false; // Vehicle parts don't self-collide
    return createAggregate(config);
}

// ============================================================================
// Management
// ============================================================================

PxU32 AggregateManager::getAggregateCount() const
{
    return static_cast<PxU32>(m_impl->m_aggregates.size());
}

PxAggregate* AggregateManager::getAggregate(PxU32 index) const
{
    if (index >= m_impl->m_aggregates.size()) {
        return nullptr;
    }
    return m_impl->m_aggregates[index];
}

std::vector<PxAggregate*> AggregateManager::getAllAggregates() const
{
    return m_impl->m_aggregates;
}

AggregateManager::AggregateCollection AggregateManager::getCollection() const
{
    AggregateCollection collection;
    collection.aggregates = m_impl->m_aggregates;

    for (PxAggregate* aggregate : m_impl->m_aggregates) {
        AggregateStats stats = getStats(aggregate);
        collection.totalActors += stats.numActors;
        collection.totalShapes += stats.numShapes;
    }

    return collection;
}

void AggregateManager::printAllAggregates() const
{
    std::cout << "Total aggregates: " << m_impl->m_aggregates.size() << std::endl;

    for (size_t i = 0; i < m_impl->m_aggregates.size(); i++) {
        std::cout << "\n[" << i << "] ";
        printAggregate(m_impl->m_aggregates[i]);
    }
}

// ============================================================================
// Configuration
// ============================================================================

PxPhysics* AggregateManager::getPhysics() const
{
    return m_impl->m_physics;
}

PxScene* AggregateManager::getScene() const
{
    return m_impl->m_scene;
}

void AggregateManager::setScene(PxScene* scene)
{
    m_impl->m_scene = scene;
}

} // namespace PhysXWrapper
