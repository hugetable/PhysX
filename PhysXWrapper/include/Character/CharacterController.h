/**
 * @file CharacterController.h
 * @brief Character controller for kinematic character movement
 *
 * This class provides a character controller for implementing kinematic
 * character movement with collision detection, climbing, sliding, and
 * obstacle handling. It wraps PhysX's PxController API.
 *
 * Based on PhysX SnippetCharacterController
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <vector>
#include <functional>

using namespace physx;

namespace PhysXWrapper {

/**
 * @class CharacterController
 * @brief Character controller for kinematic movement
 *
 * CharacterController provides a high-level interface for character movement
 * with automatic collision detection and response. It supports:
 * - Capsule and box shapes
 * - Gravity and jumping
 * - Climbing slopes
 * - Sliding on walls
 * - Step offset (stair climbing)
 * - Custom filters for collision
 * - Interaction with dynamic objects
 *
 * The controller uses kinematic motion (no physics forces) but provides
 * realistic collision response through the PhysX character controller system.
 *
 * Usage:
 * @code
 * CharacterController controller;
 * controller.initialize(physics, scene);
 *
 * // Create capsule character
 * CharacterController::CapsuleDesc desc;
 * desc.position = PxExtendedVec3(0, 10, 0);
 * desc.radius = 0.5f;
 * desc.height = 1.8f;
 * controller.createCapsule(desc);
 *
 * // Update each frame
 * PxVec3 movement(1, 0, 0);
 * float deltaTime = 0.016f;
 * controller.move(movement, deltaTime);
 *
 * // Get position
 * PxExtendedVec3 pos = controller.getPosition();
 * @endcode
 */
class CharacterController {
public:
    /**
     * @brief Controller shape type
     */
    enum class ShapeType {
        CAPSULE,  ///< Capsule shape (recommended for characters)
        BOX       ///< Box shape
    };

    /**
     * @brief Controller collision flags
     */
    struct CollisionFlags {
        bool collisionDown = false;   ///< Collided below
        bool collisionUp = false;     ///< Collided above
        bool collisionSides = false;  ///< Collided on sides

        void reset() {
            collisionDown = false;
            collisionUp = false;
            collisionSides = false;
        }

        bool isGrounded() const { return collisionDown; }
    };

    /**
     * @brief Capsule controller descriptor
     */
    struct CapsuleDesc {
        PxExtendedVec3 position = PxExtendedVec3(0, 0, 0);  ///< Initial position
        PxReal radius = 0.5f;                                ///< Capsule radius
        PxReal height = 1.8f;                                ///< Capsule height (excluding hemispheres)
        PxReal slopeLimit = 0.707f;                          ///< Slope limit (cos of max angle)
        PxReal stepOffset = 0.5f;                            ///< Step offset for climbing stairs
        PxReal contactOffset = 0.1f;                         ///< Contact offset
        PxReal density = 10.0f;                              ///< Density
        PxMaterial* material = nullptr;                      ///< Material (uses default if null)
        bool nonWalkableMode = false;                        ///< Prevent walking on non-walkable surfaces
        PxVec3 upDirection = PxVec3(0, 1, 0);               ///< Up direction

        CapsuleDesc() = default;
    };

    /**
     * @brief Box controller descriptor
     */
    struct BoxDesc {
        PxExtendedVec3 position = PxExtendedVec3(0, 0, 0);  ///< Initial position
        PxVec3 halfExtents = PxVec3(0.5f, 1.0f, 0.5f);      ///< Half extents
        PxReal slopeLimit = 0.707f;                          ///< Slope limit (cos of max angle)
        PxReal stepOffset = 0.5f;                            ///< Step offset for climbing stairs
        PxReal contactOffset = 0.1f;                         ///< Contact offset
        PxReal density = 10.0f;                              ///< Density
        PxMaterial* material = nullptr;                      ///< Material (uses default if null)
        bool nonWalkableMode = false;                        ///< Prevent walking on non-walkable surfaces
        PxVec3 upDirection = PxVec3(0, 1, 0);               ///< Up direction

        BoxDesc() = default;
    };

    /**
     * @brief Movement configuration
     */
    struct MovementConfig {
        PxReal gravity = 9.8f;                ///< Gravity magnitude
        PxReal jumpSpeed = 8.0f;              ///< Jump initial speed
        PxReal minMoveDistance = 0.0001f;     ///< Minimum move distance
        bool enableGravity = true;            ///< Enable gravity
        bool autoStep = true;                 ///< Auto step up obstacles

        MovementConfig() = default;
    };

    /**
     * @brief Character state
     */
    struct CharacterState {
        PxExtendedVec3 position;        ///< Current position
        PxVec3 velocity;                ///< Current velocity
        CollisionFlags collisionFlags;  ///< Collision flags
        bool isJumping = false;         ///< Is jumping
        PxReal verticalVelocity = 0.0f; ///< Vertical velocity
        ShapeType shapeType;            ///< Shape type

        CharacterState() = default;
    };

    /**
     * @brief Hit callback for character collisions
     */
    using HitCallback = std::function<void(const PxControllerHit& hit)>;

public:
    /**
     * @brief Constructor
     */
    CharacterController();

    /**
     * @brief Destructor
     */
    ~CharacterController();

    // Disable copy
    CharacterController(const CharacterController&) = delete;
    CharacterController& operator=(const CharacterController&) = delete;

    /**
     * @brief Initialize the character controller system
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
    // Controller Creation
    // ========================================================================

    /**
     * @brief Create capsule controller
     * @param desc Capsule descriptor
     * @return true if successful
     */
    bool createCapsule(const CapsuleDesc& desc);

    /**
     * @brief Create box controller
     * @param desc Box descriptor
     * @return true if successful
     */
    bool createBox(const BoxDesc& desc);

    /**
     * @brief Release current controller
     */
    void release();

    /**
     * @brief Check if controller exists
     * @return true if controller exists
     */
    bool hasController() const;

    // ========================================================================
    // Movement
    // ========================================================================

    /**
     * @brief Move the character
     * @param displacement Desired displacement
     * @param deltaTime Time step
     * @return Collision flags
     */
    CollisionFlags move(const PxVec3& displacement, PxReal deltaTime);

    /**
     * @brief Move with gravity
     * @param horizontalDisplacement Horizontal movement
     * @param deltaTime Time step
     * @return Collision flags
     *
     * This applies gravity and handles jumping automatically
     */
    CollisionFlags moveWithGravity(const PxVec3& horizontalDisplacement, PxReal deltaTime);

    /**
     * @brief Jump
     * @return true if jump initiated (only works when grounded)
     */
    bool jump();

    /**
     * @brief Teleport to position
     * @param position New position
     * @return true if successful
     */
    bool setPosition(const PxExtendedVec3& position);

    /**
     * @brief Reset vertical velocity (useful after landing)
     */
    void resetVerticalVelocity();

    // ========================================================================
    // State Query
    // ========================================================================

    /**
     * @brief Get current position
     * @return Position
     */
    PxExtendedVec3 getPosition() const;

    /**
     * @brief Get current position as float vector
     * @return Position as PxVec3
     */
    PxVec3 getPositionVec3() const;

    /**
     * @brief Get foot position
     * @return Foot position
     */
    PxExtendedVec3 getFootPosition() const;

    /**
     * @brief Get current state
     * @return Character state
     */
    CharacterState getState() const;

    /**
     * @brief Get last collision flags
     * @return Collision flags
     */
    CollisionFlags getCollisionFlags() const;

    /**
     * @brief Check if grounded
     * @return true if on ground
     */
    bool isGrounded() const;

    /**
     * @brief Get controller type
     * @return Controller type
     */
    PxControllerShapeType::Enum getType() const;

    /**
     * @brief Get underlying controller
     * @return PhysX controller (or nullptr)
     */
    PxController* getController() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set movement configuration
     * @param config Movement config
     */
    void setMovementConfig(const MovementConfig& config);

    /**
     * @brief Get movement configuration
     * @return Movement config
     */
    const MovementConfig& getMovementConfig() const;

    /**
     * @brief Set slope limit
     * @param slopeLimit Slope limit (cosine of max angle)
     */
    void setSlopeLimit(PxReal slopeLimit);

    /**
     * @brief Get slope limit
     * @return Slope limit
     */
    PxReal getSlopeLimit() const;

    /**
     * @brief Set step offset
     * @param stepOffset Step offset for climbing
     */
    void setStepOffset(PxReal stepOffset);

    /**
     * @brief Get step offset
     * @return Step offset
     */
    PxReal getStepOffset() const;

    /**
     * @brief Set up direction
     * @param upDirection Up direction vector
     */
    void setUpDirection(const PxVec3& upDirection);

    /**
     * @brief Get up direction
     * @return Up direction
     */
    PxVec3 getUpDirection() const;

    /**
     * @brief Set hit callback
     * @param callback Callback function for hits
     */
    void setHitCallback(HitCallback callback);

    // ========================================================================
    // Shape Properties (Capsule)
    // ========================================================================

    /**
     * @brief Set capsule radius
     * @param radius New radius
     * @return true if successful (only for capsule controllers)
     */
    bool setCapsuleRadius(PxReal radius);

    /**
     * @brief Get capsule radius
     * @return Radius (0 if not a capsule)
     */
    PxReal getCapsuleRadius() const;

    /**
     * @brief Set capsule height
     * @param height New height (excluding hemispheres)
     * @return true if successful (only for capsule controllers)
     */
    bool setCapsuleHeight(PxReal height);

    /**
     * @brief Get capsule height
     * @return Height (0 if not a capsule)
     */
    PxReal getCapsuleHeight() const;

    // ========================================================================
    // Shape Properties (Box)
    // ========================================================================

    /**
     * @brief Set box half extents
     * @param halfExtents New half extents
     * @return true if successful (only for box controllers)
     */
    bool setBoxHalfExtents(const PxVec3& halfExtents);

    /**
     * @brief Get box half extents
     * @return Half extents (zero if not a box)
     */
    PxVec3 getBoxHalfExtents() const;

    // ========================================================================
    // Physics Interaction
    // ========================================================================

    /**
     * @brief Get underlying actor
     * @return Actor (or nullptr)
     */
    PxRigidDynamic* getActor() const;

    /**
     * @brief Invalidate cache (call after teleporting or modifying obstacles)
     */
    void invalidateCache();

    /**
     * @brief Resize controller
     * @param height New height
     * @return true if successful
     */
    bool resize(PxReal height);

    // ========================================================================
    // Debug
    // ========================================================================

    /**
     * @brief Print controller info
     */
    void printInfo() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace PhysXWrapper
