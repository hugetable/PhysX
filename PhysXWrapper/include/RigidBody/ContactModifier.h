/**
 * @file ContactModifier.h
 * @brief Runtime contact modification system for PhysX
 *
 * This class provides utilities for modifying contact properties at runtime:
 * - Adjust mass ratios for stability
 * - Modify friction coefficients per contact
 * - Modify restitution (bounciness) per contact
 * - Adjust contact normals and separation
 * - Custom modification rules based on materials/actors
 *
 * Contact modification is useful for:
 * - Stabilizing large mass ratio scenarios
 * - Implementing special surface effects (ice, sticky surfaces)
 * - Custom physics behaviors (one-way platforms, ladders)
 * - Performance optimization (disable contacts conditionally)
 *
 * Based on SnippetContactModification from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Contact modification context
 */
struct ContactModificationContext {
    PxContactModifyPair* pair;          ///< Contact pair being modified
    PxU32 contactIndex;                 ///< Index of current contact
    PxRigidActor* actor0;               ///< First actor
    PxRigidActor* actor1;               ///< Second actor
    PxShape* shape0;                    ///< First shape
    PxShape* shape1;                    ///< Second shape
};

/**
 * @brief Contact modification rule
 */
enum class ContactModificationRule {
    ADJUST_MASS_RATIO,          ///< Adjust mass ratio for stability
    MODIFY_FRICTION,            ///< Modify friction coefficient
    MODIFY_RESTITUTION,         ///< Modify restitution (bounce)
    CUSTOM                       ///< Custom user-defined modification
};

/**
 * @brief Mass ratio adjustment configuration
 */
struct MassRatioAdjustmentConfig {
    PxReal maxMassRatio = 10.0f;        ///< Maximum allowed mass ratio
    bool enableForAllPairs = true;       ///< Enable for all dynamic pairs
    bool scaleInertia = true;            ///< Also scale inertia tensor
};

/**
 * @brief Friction modification configuration
 */
struct FrictionModificationConfig {
    PxReal frictionMultiplier = 1.0f;    ///< Friction multiplier (0-inf)
    bool enableForAllContacts = false;    ///< Enable for all contacts
    PxU32 materialID0 = 0;               ///< Material ID filter (0 = any)
    PxU32 materialID1 = 0;               ///< Material ID filter (0 = any)
};

/**
 * @brief Restitution modification configuration
 */
struct RestitutionModificationConfig {
    PxReal restitutionMultiplier = 1.0f; ///< Restitution multiplier (0-1+)
    bool enableForAllContacts = false;    ///< Enable for all contacts
    PxReal velocityThreshold = 0.0f;     ///< Only apply above this velocity
};

/**
 * @brief Custom modification callback
 */
using CustomModificationCallback = std::function<void(const ContactModificationContext& context)>;

/**
 * @brief Contact modifier class
 *
 * This class provides comprehensive contact modification capabilities:
 * - Automatic mass ratio adjustment for stability
 * - Per-contact friction modification
 * - Per-contact restitution modification
 * - Custom modification rules via callbacks
 * - Material-based modification
 * - Actor-based modification
 *
 * IMPORTANT: The scene must be configured with contact modification enabled
 * in the filter shader (PxPairFlag::eMODIFY_CONTACTS).
 *
 * @example
 * @code
 * // Create modifier
 * ContactModifier modifier;
 * modifier.initialize(physics, scene);
 *
 * // Enable mass ratio adjustment for stability
 * MassRatioAdjustmentConfig massConfig;
 * massConfig.maxMassRatio = 5.0f;  // Limit to 5:1 ratio
 * modifier.enableMassRatioAdjustment(massConfig);
 *
 * // Add custom friction for ice surface
 * modifier.addCustomModification([](const ContactModificationContext& ctx) {
 *     // Reduce friction for objects on "ice" material
 *     if (isIceMaterial(ctx.shape0) || isIceMaterial(ctx.shape1)) {
 *         ctx.pair->contacts.setStaticFriction(0, 0.05f);
 *         ctx.pair->contacts.setDynamicFriction(0, 0.03f);
 *     }
 * });
 *
 * // Scene will automatically call modifications during simulation
 * @endcode
 */
class ContactModifier : public PxContactModifyCallback {
public:
    /**
     * @brief Constructor
     */
    ContactModifier();

    /**
     * @brief Destructor
     */
    ~ContactModifier();

    // No copy
    ContactModifier(const ContactModifier&) = delete;
    ContactModifier& operator=(const ContactModifier&) = delete;

    // Move allowed
    ContactModifier(ContactModifier&&) noexcept;
    ContactModifier& operator=(ContactModifier&&) noexcept;

    /**
     * @brief Initialize contact modifier
     * @param physics PhysX physics instance
     * @param scene Scene to attach to
     * @return True if successful
     */
    bool initialize(PxPhysics* physics, PxScene* scene);

    /**
     * @brief Cleanup and release resources
     */
    void cleanup();

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const;

    // ========================================================================
    // Built-in Modification Rules
    // ========================================================================

    /**
     * @brief Enable mass ratio adjustment
     * Automatically adjusts local mass properties to prevent instability
     * from large mass differences
     * @param config Mass ratio adjustment configuration
     */
    void enableMassRatioAdjustment(const MassRatioAdjustmentConfig& config = MassRatioAdjustmentConfig());

    /**
     * @brief Disable mass ratio adjustment
     */
    void disableMassRatioAdjustment();

    /**
     * @brief Enable friction modification
     * @param config Friction modification configuration
     */
    void enableFrictionModification(const FrictionModificationConfig& config);

    /**
     * @brief Disable friction modification
     */
    void disableFrictionModification();

    /**
     * @brief Enable restitution modification
     * @param config Restitution modification configuration
     */
    void enableRestitutionModification(const RestitutionModificationConfig& config);

    /**
     * @brief Disable restitution modification
     */
    void disableRestitutionModification();

    // ========================================================================
    // Custom Modifications
    // ========================================================================

    /**
     * @brief Add custom modification callback
     * @param callback Custom modification function
     * @return Callback ID for later removal
     */
    PxU32 addCustomModification(CustomModificationCallback callback);

    /**
     * @brief Remove custom modification
     * @param callbackID ID returned from addCustomModification
     */
    void removeCustomModification(PxU32 callbackID);

    /**
     * @brief Clear all custom modifications
     */
    void clearCustomModifications();

    // ========================================================================
    // Actor/Shape Filtering
    // ========================================================================

    /**
     * @brief Enable modification for specific actor pair
     * @param actor0 First actor
     * @param actor1 Second actor
     */
    void enableForActorPair(PxRigidActor* actor0, PxRigidActor* actor1);

    /**
     * @brief Disable modification for specific actor pair
     * @param actor0 First actor
     * @param actor1 Second actor
     */
    void disableForActorPair(PxRigidActor* actor0, PxRigidActor* actor1);

    /**
     * @brief Check if modification is enabled for actor pair
     * @param actor0 First actor
     * @param actor1 Second actor
     * @return True if enabled
     */
    bool isEnabledForActorPair(PxRigidActor* actor0, PxRigidActor* actor1) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Get number of contacts modified in last frame
     * @return Contact count
     */
    PxU32 getModifiedContactCount() const;

    /**
     * @brief Reset statistics
     */
    void resetStatistics();

    /**
     * @brief Get last error message
     * @return Error message or empty string
     */
    std::string getLastError() const;

    // ========================================================================
    // PxContactModifyCallback Implementation
    // ========================================================================

    /**
     * @brief Called by PhysX to modify contacts
     * @param pairs Contact pairs to modify
     * @param count Number of pairs
     */
    virtual void onContactModify(PxContactModifyPair* const pairs, PxU32 count) override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    void applyMassRatioAdjustment(PxContactModifyPair& pair);
    void applyFrictionModification(PxContactModifyPair& pair);
    void applyRestitutionModification(PxContactModifyPair& pair);
    void applyCustomModifications(PxContactModifyPair& pair);
};

/**
 * @brief Contact modification filter shader helper
 *
 * Use this function as your scene's filter shader to enable contact modification
 * for all pairs, or customize it for specific pairs.
 */
PxFilterFlags ContactModificationFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize
);

} // namespace PhysXWrapper
