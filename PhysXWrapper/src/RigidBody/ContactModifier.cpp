/**
 * @file ContactModifier.cpp
 * @brief Implementation of ContactModifier class
 */

#include "RigidBody/ContactModifier.h"
#include <algorithm>
#include <map>
#include <set>

namespace PhysXWrapper {

/**
 * @brief Actor pair key for filtering
 */
struct ActorPairKey {
    PxRigidActor* actor0;
    PxRigidActor* actor1;

    bool operator<(const ActorPairKey& other) const {
        if (actor0 < other.actor0) return true;
        if (actor0 > other.actor0) return false;
        return actor1 < other.actor1;
    }
};

/**
 * @brief Private implementation
 */
class ContactModifier::Impl {
public:
    Impl()
        : m_physics(nullptr)
        , m_scene(nullptr)
        , m_initialized(false)
        , m_enableMassRatioAdjustment(false)
        , m_enableFrictionModification(false)
        , m_enableRestitutionModification(false)
        , m_modifiedContactCount(0)
        , m_nextCallbackID(1)
    {}

    PxPhysics* m_physics;
    PxScene* m_scene;
    bool m_initialized;
    std::string m_lastError;

    // Built-in rules
    bool m_enableMassRatioAdjustment;
    MassRatioAdjustmentConfig m_massRatioConfig;

    bool m_enableFrictionModification;
    FrictionModificationConfig m_frictionConfig;

    bool m_enableRestitutionModification;
    RestitutionModificationConfig m_restitutionConfig;

    // Custom modifications
    std::map<PxU32, CustomModificationCallback> m_customCallbacks;
    PxU32 m_nextCallbackID;

    // Actor pair filtering
    std::set<ActorPairKey> m_enabledPairs;
    bool m_useActorPairFiltering = false;

    // Statistics
    PxU32 m_modifiedContactCount;

    void setError(const std::string& error) {
        m_lastError = error;
    }

    void clearError() {
        m_lastError.clear();
    }
};

// ============================================================================
// Construction / Destruction
// ============================================================================

ContactModifier::ContactModifier()
    : m_impl(std::make_unique<Impl>())
{
}

ContactModifier::~ContactModifier() {
    cleanup();
}

ContactModifier::ContactModifier(ContactModifier&& other) noexcept
    : m_impl(std::move(other.m_impl))
{
}

ContactModifier& ContactModifier::operator=(ContactModifier&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

// ============================================================================
// Initialization
// ============================================================================

bool ContactModifier::initialize(PxPhysics* physics, PxScene* scene) {
    if (!physics || !scene) {
        m_impl->setError("Invalid physics or scene instance");
        return false;
    }

    if (m_impl->m_initialized) {
        m_impl->setError("Already initialized");
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_scene = scene;

    // Set this as the scene's contact modify callback
    PxSceneDesc& sceneDesc = const_cast<PxSceneDesc&>(scene->getSceneDesc());
    sceneDesc.contactModifyCallback = this;

    m_impl->m_initialized = true;
    m_impl->clearError();

    return true;
}

void ContactModifier::cleanup() {
    if (!m_impl->m_initialized) return;

    // Remove callback from scene
    if (m_impl->m_scene) {
        PxSceneDesc& sceneDesc = const_cast<PxSceneDesc&>(m_impl->m_scene->getSceneDesc());
        if (sceneDesc.contactModifyCallback == this) {
            sceneDesc.contactModifyCallback = nullptr;
        }
    }

    m_impl->m_customCallbacks.clear();
    m_impl->m_enabledPairs.clear();

    m_impl->m_physics = nullptr;
    m_impl->m_scene = nullptr;
    m_impl->m_initialized = false;
    m_impl->clearError();
}

bool ContactModifier::isInitialized() const {
    return m_impl->m_initialized;
}

// ============================================================================
// Built-in Modification Rules
// ============================================================================

void ContactModifier::enableMassRatioAdjustment(const MassRatioAdjustmentConfig& config) {
    m_impl->m_enableMassRatioAdjustment = true;
    m_impl->m_massRatioConfig = config;
}

void ContactModifier::disableMassRatioAdjustment() {
    m_impl->m_enableMassRatioAdjustment = false;
}

void ContactModifier::enableFrictionModification(const FrictionModificationConfig& config) {
    m_impl->m_enableFrictionModification = true;
    m_impl->m_frictionConfig = config;
}

void ContactModifier::disableFrictionModification() {
    m_impl->m_enableFrictionModification = false;
}

void ContactModifier::enableRestitutionModification(const RestitutionModificationConfig& config) {
    m_impl->m_enableRestitutionModification = true;
    m_impl->m_restitutionConfig = config;
}

void ContactModifier::disableRestitutionModification() {
    m_impl->m_enableRestitutionModification = false;
}

// ============================================================================
// Custom Modifications
// ============================================================================

PxU32 ContactModifier::addCustomModification(CustomModificationCallback callback) {
    PxU32 id = m_impl->m_nextCallbackID++;
    m_impl->m_customCallbacks[id] = callback;
    return id;
}

void ContactModifier::removeCustomModification(PxU32 callbackID) {
    m_impl->m_customCallbacks.erase(callbackID);
}

void ContactModifier::clearCustomModifications() {
    m_impl->m_customCallbacks.clear();
}

// ============================================================================
// Actor/Shape Filtering
// ============================================================================

void ContactModifier::enableForActorPair(PxRigidActor* actor0, PxRigidActor* actor1) {
    ActorPairKey key{actor0, actor1};
    m_impl->m_enabledPairs.insert(key);
    m_impl->m_useActorPairFiltering = true;
}

void ContactModifier::disableForActorPair(PxRigidActor* actor0, PxRigidActor* actor1) {
    ActorPairKey key{actor0, actor1};
    m_impl->m_enabledPairs.erase(key);
}

bool ContactModifier::isEnabledForActorPair(PxRigidActor* actor0, PxRigidActor* actor1) const {
    if (!m_impl->m_useActorPairFiltering) return true;

    ActorPairKey key{actor0, actor1};
    return m_impl->m_enabledPairs.find(key) != m_impl->m_enabledPairs.end();
}

// ============================================================================
// Statistics
// ============================================================================

PxU32 ContactModifier::getModifiedContactCount() const {
    return m_impl->m_modifiedContactCount;
}

void ContactModifier::resetStatistics() {
    m_impl->m_modifiedContactCount = 0;
}

std::string ContactModifier::getLastError() const {
    return m_impl->m_lastError;
}

// ============================================================================
// Contact Modification Callback
// ============================================================================

void ContactModifier::onContactModify(PxContactModifyPair* const pairs, PxU32 count) {
    m_impl->m_modifiedContactCount = 0;

    for (PxU32 i = 0; i < count; i++) {
        PxContactModifyPair& pair = pairs[i];

        // Check actor pair filtering
        if (m_impl->m_useActorPairFiltering) {
            if (!isEnabledForActorPair(pair.actor[0], pair.actor[1])) {
                continue;
            }
        }

        // Apply built-in modifications
        if (m_impl->m_enableMassRatioAdjustment) {
            applyMassRatioAdjustment(pair);
        }

        if (m_impl->m_enableFrictionModification) {
            applyFrictionModification(pair);
        }

        if (m_impl->m_enableRestitutionModification) {
            applyRestitutionModification(pair);
        }

        // Apply custom modifications
        if (!m_impl->m_customCallbacks.empty()) {
            applyCustomModifications(pair);
        }

        m_impl->m_modifiedContactCount++;
    }
}

// ============================================================================
// Private Helpers
// ============================================================================

void ContactModifier::applyMassRatioAdjustment(PxContactModifyPair& pair) {
    const PxRigidDynamic* dynamic0 = pair.actor[0]->is<PxRigidDynamic>();
    const PxRigidDynamic* dynamic1 = pair.actor[1]->is<PxRigidDynamic>();

    if (!dynamic0 || !dynamic1) return;

    if (!m_impl->m_massRatioConfig.enableForAllPairs) return;

    PxReal mass0 = dynamic0->getMass();
    PxReal mass1 = dynamic1->getMass();

    const PxReal maxMassRatio = m_impl->m_massRatioConfig.maxMassRatio;

    if (mass0 > mass1) {
        // dynamic0 is heavier, adjust dynamic1
        PxReal ratio = mass0 / mass1;
        if (ratio > maxMassRatio) {
            PxReal invMassScale = maxMassRatio / ratio;
            pair.contacts.setInvMassScale1(invMassScale);

            if (m_impl->m_massRatioConfig.scaleInertia) {
                pair.contacts.setInvInertiaScale1(invMassScale);
            }
        }
    } else if (mass1 > mass0) {
        // dynamic1 is heavier, adjust dynamic0
        PxReal ratio = mass1 / mass0;
        if (ratio > maxMassRatio) {
            PxReal invMassScale = maxMassRatio / ratio;
            pair.contacts.setInvMassScale0(invMassScale);

            if (m_impl->m_massRatioConfig.scaleInertia) {
                pair.contacts.setInvInertiaScale0(invMassScale);
            }
        }
    }
}

void ContactModifier::applyFrictionModification(PxContactModifyPair& pair) {
    if (!m_impl->m_frictionConfig.enableForAllContacts) return;

    const PxU32 contactCount = pair.contacts.size();

    for (PxU32 i = 0; i < contactCount; i++) {
        // Get current friction values
        PxReal staticFriction = pair.contacts.getStaticFriction(i);
        PxReal dynamicFriction = pair.contacts.getDynamicFriction(i);

        // Apply multiplier
        staticFriction *= m_impl->m_frictionConfig.frictionMultiplier;
        dynamicFriction *= m_impl->m_frictionConfig.frictionMultiplier;

        // Clamp to valid range
        staticFriction = PxMax(0.0f, staticFriction);
        dynamicFriction = PxMax(0.0f, dynamicFriction);

        // Set modified values
        pair.contacts.setStaticFriction(i, staticFriction);
        pair.contacts.setDynamicFriction(i, dynamicFriction);
    }
}

void ContactModifier::applyRestitutionModification(PxContactModifyPair& pair) {
    if (!m_impl->m_restitutionConfig.enableForAllContacts) return;

    const PxU32 contactCount = pair.contacts.size();

    for (PxU32 i = 0; i < contactCount; i++) {
        // Get current restitution
        PxReal restitution = pair.contacts.getRestitution(i);

        // Apply multiplier
        restitution *= m_impl->m_restitutionConfig.restitutionMultiplier;

        // Clamp to valid range [0, 1]
        restitution = PxClamp(restitution, 0.0f, 1.0f);

        // Set modified value
        pair.contacts.setRestitution(i, restitution);
    }
}

void ContactModifier::applyCustomModifications(PxContactModifyPair& pair) {
    const PxU32 contactCount = pair.contacts.size();

    for (PxU32 contactIdx = 0; contactIdx < contactCount; contactIdx++) {
        // Create context
        ContactModificationContext context;
        context.pair = &pair;
        context.contactIndex = contactIdx;
        context.actor0 = pair.actor[0];
        context.actor1 = pair.actor[1];
        context.shape0 = pair.shape[0];
        context.shape1 = pair.shape[1];

        // Call all custom callbacks
        for (const auto& callbackPair : m_impl->m_customCallbacks) {
            callbackPair.second(context);
        }
    }
}

// ============================================================================
// Filter Shader Helper
// ============================================================================

PxFilterFlags ContactModificationFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    PX_UNUSED(attributes0);
    PX_UNUSED(attributes1);
    PX_UNUSED(filterData0);
    PX_UNUSED(filterData1);
    PX_UNUSED(constantBlock);
    PX_UNUSED(constantBlockSize);

    // Enable contact solving and modification
    pairFlags = PxPairFlag::eSOLVE_CONTACT
              | PxPairFlag::eDETECT_DISCRETE_CONTACT
              | PxPairFlag::eMODIFY_CONTACTS;

    // Optionally enable contact reports
    // pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_CONTACT_POINTS;

    return PxFilterFlag::eDEFAULT;
}

} // namespace PhysXWrapper
