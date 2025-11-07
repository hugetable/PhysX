/**
 * @file CharacterController.cpp
 * @brief Implementation of CharacterController class
 */

#include "Character/CharacterController.h"
#include <iostream>

namespace PhysXWrapper {

// ============================================================================
// Custom Controller Behavior Callback
// ============================================================================

class ControllerHitCallback : public PxUserControllerHitReport {
public:
    CharacterController::HitCallback callback;

    void onShapeHit(const PxControllerShapeHit& hit) override {
        if (callback) {
            callback(hit);
        }
    }

    void onControllerHit(const PxControllersHit& hit) override {
        // Handle controller-to-controller collisions if needed
    }

    void onObstacleHit(const PxControllerObstacleHit& hit) override {
        // Handle obstacle collisions if needed
    }
};

// ============================================================================
// CharacterController::Impl
// ============================================================================

class CharacterController::Impl {
public:
    PxPhysics* m_physics = nullptr;
    PxScene* m_scene = nullptr;
    PxControllerManager* m_controllerManager = nullptr;
    PxController* m_controller = nullptr;
    PxMaterial* m_defaultMaterial = nullptr;

    MovementConfig m_movementConfig;
    CollisionFlags m_lastCollisionFlags;
    PxReal m_verticalVelocity = 0.0f;
    bool m_isJumping = false;

    ControllerHitCallback m_hitCallback;

    bool m_initialized = false;
};

// ============================================================================
// Construction/Destruction
// ============================================================================

CharacterController::CharacterController()
    : m_impl(std::make_unique<Impl>())
{
}

CharacterController::~CharacterController()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool CharacterController::initialize(PxPhysics* physics, PxScene* scene)
{
    if (!physics || !scene) {
        std::cerr << "CharacterController::initialize: physics or scene is null" << std::endl;
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_scene = scene;

    // Create controller manager
    m_impl->m_controllerManager = PxCreateControllerManager(*scene);
    if (!m_impl->m_controllerManager) {
        std::cerr << "CharacterController::initialize: Failed to create controller manager" << std::endl;
        return false;
    }

    // Create default material
    m_impl->m_defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.1f);
    if (!m_impl->m_defaultMaterial) {
        std::cerr << "CharacterController::initialize: Failed to create default material" << std::endl;
        m_impl->m_controllerManager->release();
        m_impl->m_controllerManager = nullptr;
        return false;
    }

    m_impl->m_initialized = true;
    return true;
}

void CharacterController::cleanup()
{
    release();

    if (m_impl->m_defaultMaterial) {
        m_impl->m_defaultMaterial->release();
        m_impl->m_defaultMaterial = nullptr;
    }

    if (m_impl->m_controllerManager) {
        m_impl->m_controllerManager->release();
        m_impl->m_controllerManager = nullptr;
    }

    m_impl->m_initialized = false;
}

bool CharacterController::isInitialized() const
{
    return m_impl->m_initialized;
}

// ============================================================================
// Controller Creation
// ============================================================================

bool CharacterController::createCapsule(const CapsuleDesc& desc)
{
    if (!m_impl->m_initialized) {
        std::cerr << "CharacterController::createCapsule: Not initialized" << std::endl;
        return false;
    }

    // Release existing controller
    release();

    // Setup capsule descriptor
    PxCapsuleControllerDesc capsuleDesc;
    capsuleDesc.position = desc.position;
    capsuleDesc.radius = desc.radius;
    capsuleDesc.height = desc.height;
    capsuleDesc.slopeLimit = desc.slopeLimit;
    capsuleDesc.stepOffset = desc.stepOffset;
    capsuleDesc.contactOffset = desc.contactOffset;
    capsuleDesc.density = desc.density;
    capsuleDesc.material = desc.material ? desc.material : m_impl->m_defaultMaterial;
    capsuleDesc.nonWalkableMode = desc.nonWalkableMode ?
        PxControllerNonWalkableMode::ePREVENT_CLIMBING :
        PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
    capsuleDesc.upDirection = desc.upDirection;
    capsuleDesc.reportCallback = &m_impl->m_hitCallback;

    // Validate descriptor
    if (!capsuleDesc.isValid()) {
        std::cerr << "CharacterController::createCapsule: Invalid descriptor" << std::endl;
        return false;
    }

    // Create controller
    m_impl->m_controller = m_impl->m_controllerManager->createController(capsuleDesc);
    if (!m_impl->m_controller) {
        std::cerr << "CharacterController::createCapsule: Failed to create controller" << std::endl;
        return false;
    }

    // Reset state
    m_impl->m_verticalVelocity = 0.0f;
    m_impl->m_isJumping = false;
    m_impl->m_lastCollisionFlags.reset();

    return true;
}

bool CharacterController::createBox(const BoxDesc& desc)
{
    if (!m_impl->m_initialized) {
        std::cerr << "CharacterController::createBox: Not initialized" << std::endl;
        return false;
    }

    // Release existing controller
    release();

    // Setup box descriptor
    PxBoxControllerDesc boxDesc;
    boxDesc.position = desc.position;
    boxDesc.halfHeight = desc.halfExtents.y;
    boxDesc.halfSideExtent = desc.halfExtents.x;
    boxDesc.halfForwardExtent = desc.halfExtents.z;
    boxDesc.slopeLimit = desc.slopeLimit;
    boxDesc.stepOffset = desc.stepOffset;
    boxDesc.contactOffset = desc.contactOffset;
    boxDesc.density = desc.density;
    boxDesc.material = desc.material ? desc.material : m_impl->m_defaultMaterial;
    boxDesc.nonWalkableMode = desc.nonWalkableMode ?
        PxControllerNonWalkableMode::ePREVENT_CLIMBING :
        PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
    boxDesc.upDirection = desc.upDirection;
    boxDesc.reportCallback = &m_impl->m_hitCallback;

    // Validate descriptor
    if (!boxDesc.isValid()) {
        std::cerr << "CharacterController::createBox: Invalid descriptor" << std::endl;
        return false;
    }

    // Create controller
    m_impl->m_controller = m_impl->m_controllerManager->createController(boxDesc);
    if (!m_impl->m_controller) {
        std::cerr << "CharacterController::createBox: Failed to create controller" << std::endl;
        return false;
    }

    // Reset state
    m_impl->m_verticalVelocity = 0.0f;
    m_impl->m_isJumping = false;
    m_impl->m_lastCollisionFlags.reset();

    return true;
}

void CharacterController::release()
{
    if (m_impl->m_controller) {
        m_impl->m_controller->release();
        m_impl->m_controller = nullptr;
    }
}

bool CharacterController::hasController() const
{
    return m_impl->m_controller != nullptr;
}

// ============================================================================
// Movement
// ============================================================================

CharacterController::CollisionFlags CharacterController::move(const PxVec3& displacement, PxReal deltaTime)
{
    m_impl->m_lastCollisionFlags.reset();

    if (!m_impl->m_controller) {
        return m_impl->m_lastCollisionFlags;
    }

    // Perform move
    PxControllerFilters filters;
    PxControllerCollisionFlags collisionFlags = m_impl->m_controller->move(
        displacement,
        m_impl->m_movementConfig.minMoveDistance,
        deltaTime,
        filters
    );

    // Update collision flags (PhysX 5.x: use isSet() for PxFlags)
    m_impl->m_lastCollisionFlags.collisionDown = collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);
    m_impl->m_lastCollisionFlags.collisionUp = collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_UP);
    m_impl->m_lastCollisionFlags.collisionSides = collisionFlags.isSet(PxControllerCollisionFlag::eCOLLISION_SIDES);

    return m_impl->m_lastCollisionFlags;
}

CharacterController::CollisionFlags CharacterController::moveWithGravity(const PxVec3& horizontalDisplacement, PxReal deltaTime)
{
    if (!m_impl->m_controller) {
        m_impl->m_lastCollisionFlags.reset();
        return m_impl->m_lastCollisionFlags;
    }

    PxVec3 displacement = horizontalDisplacement;

    // Apply gravity
    if (m_impl->m_movementConfig.enableGravity) {
        m_impl->m_verticalVelocity -= m_impl->m_movementConfig.gravity * deltaTime;
        displacement += PxVec3(0, m_impl->m_verticalVelocity * deltaTime, 0);
    }

    // Perform move
    CollisionFlags flags = move(displacement, deltaTime);

    // Handle landing
    if (flags.collisionDown && m_impl->m_verticalVelocity < 0.0f) {
        m_impl->m_verticalVelocity = 0.0f;
        m_impl->m_isJumping = false;
    }

    // Handle ceiling hit
    if (flags.collisionUp && m_impl->m_verticalVelocity > 0.0f) {
        m_impl->m_verticalVelocity = 0.0f;
    }

    return flags;
}

bool CharacterController::jump()
{
    if (!m_impl->m_controller) {
        return false;
    }

    // Only jump if grounded
    if (!isGrounded()) {
        return false;
    }

    m_impl->m_verticalVelocity = m_impl->m_movementConfig.jumpSpeed;
    m_impl->m_isJumping = true;

    return true;
}

bool CharacterController::setPosition(const PxExtendedVec3& position)
{
    if (!m_impl->m_controller) {
        return false;
    }

    return m_impl->m_controller->setPosition(position);
}

void CharacterController::resetVerticalVelocity()
{
    m_impl->m_verticalVelocity = 0.0f;
    m_impl->m_isJumping = false;
}

// ============================================================================
// State Query
// ============================================================================

PxExtendedVec3 CharacterController::getPosition() const
{
    if (!m_impl->m_controller) {
        return PxExtendedVec3(0, 0, 0);
    }

    return m_impl->m_controller->getPosition();
}

PxVec3 CharacterController::getPositionVec3() const
{
    PxExtendedVec3 pos = getPosition();
    return PxVec3(static_cast<PxReal>(pos.x),
                  static_cast<PxReal>(pos.y),
                  static_cast<PxReal>(pos.z));
}

PxExtendedVec3 CharacterController::getFootPosition() const
{
    if (!m_impl->m_controller) {
        return PxExtendedVec3(0, 0, 0);
    }

    return m_impl->m_controller->getFootPosition();
}

CharacterController::CharacterState CharacterController::getState() const
{
    CharacterState state;

    if (m_impl->m_controller) {
        state.position = getPosition();
        state.velocity = PxVec3(0, m_impl->m_verticalVelocity, 0);
        state.collisionFlags = m_impl->m_lastCollisionFlags;
        state.isJumping = m_impl->m_isJumping;
        state.verticalVelocity = m_impl->m_verticalVelocity;

        PxControllerShapeType::Enum type = m_impl->m_controller->getType();
        state.shapeType = (type == PxControllerShapeType::eCAPSULE) ?
            ShapeType::CAPSULE : ShapeType::BOX;
    }

    return state;
}

CharacterController::CollisionFlags CharacterController::getCollisionFlags() const
{
    return m_impl->m_lastCollisionFlags;
}

bool CharacterController::isGrounded() const
{
    return m_impl->m_lastCollisionFlags.isGrounded();
}

PxControllerShapeType::Enum CharacterController::getType() const
{
    if (!m_impl->m_controller) {
        return PxControllerShapeType::eFORCE_DWORD;
    }

    return m_impl->m_controller->getType();
}

PxController* CharacterController::getController() const
{
    return m_impl->m_controller;
}

// ============================================================================
// Configuration
// ============================================================================

void CharacterController::setMovementConfig(const MovementConfig& config)
{
    m_impl->m_movementConfig = config;
}

const CharacterController::MovementConfig& CharacterController::getMovementConfig() const
{
    return m_impl->m_movementConfig;
}

void CharacterController::setSlopeLimit(PxReal slopeLimit)
{
    if (m_impl->m_controller) {
        m_impl->m_controller->setSlopeLimit(slopeLimit);
    }
}

PxReal CharacterController::getSlopeLimit() const
{
    if (!m_impl->m_controller) {
        return 0.0f;
    }

    return m_impl->m_controller->getSlopeLimit();
}

void CharacterController::setStepOffset(PxReal stepOffset)
{
    if (m_impl->m_controller) {
        m_impl->m_controller->setStepOffset(stepOffset);
    }
}

PxReal CharacterController::getStepOffset() const
{
    if (!m_impl->m_controller) {
        return 0.0f;
    }

    return m_impl->m_controller->getStepOffset();
}

void CharacterController::setUpDirection(const PxVec3& upDirection)
{
    if (m_impl->m_controller) {
        m_impl->m_controller->setUpDirection(upDirection);
    }
}

PxVec3 CharacterController::getUpDirection() const
{
    if (!m_impl->m_controller) {
        return PxVec3(0, 1, 0);
    }

    return m_impl->m_controller->getUpDirection();
}

void CharacterController::setHitCallback(HitCallback callback)
{
    m_impl->m_hitCallback.callback = callback;
}

// ============================================================================
// Shape Properties (Capsule)
// ============================================================================

bool CharacterController::setCapsuleRadius(PxReal radius)
{
    if (!m_impl->m_controller || getType() != PxControllerShapeType::eCAPSULE) {
        return false;
    }

    PxCapsuleController* capsule = static_cast<PxCapsuleController*>(m_impl->m_controller);
    return capsule->setRadius(radius);
}

PxReal CharacterController::getCapsuleRadius() const
{
    if (!m_impl->m_controller || getType() != PxControllerShapeType::eCAPSULE) {
        return 0.0f;
    }

    PxCapsuleController* capsule = static_cast<PxCapsuleController*>(m_impl->m_controller);
    return capsule->getRadius();
}

bool CharacterController::setCapsuleHeight(PxReal height)
{
    if (!m_impl->m_controller || getType() != PxControllerShapeType::eCAPSULE) {
        return false;
    }

    PxCapsuleController* capsule = static_cast<PxCapsuleController*>(m_impl->m_controller);
    return capsule->setHeight(height);
}

PxReal CharacterController::getCapsuleHeight() const
{
    if (!m_impl->m_controller || getType() != PxControllerShapeType::eCAPSULE) {
        return 0.0f;
    }

    PxCapsuleController* capsule = static_cast<PxCapsuleController*>(m_impl->m_controller);
    return capsule->getHeight();
}

// ============================================================================
// Shape Properties (Box)
// ============================================================================

bool CharacterController::setBoxHalfExtents(const PxVec3& halfExtents)
{
    if (!m_impl->m_controller || getType() != PxControllerShapeType::eBOX) {
        return false;
    }

    PxBoxController* box = static_cast<PxBoxController*>(m_impl->m_controller);
    bool success = true;
    success &= box->setHalfHeight(halfExtents.y);
    success &= box->setHalfSideExtent(halfExtents.x);
    success &= box->setHalfForwardExtent(halfExtents.z);
    return success;
}

PxVec3 CharacterController::getBoxHalfExtents() const
{
    if (!m_impl->m_controller || getType() != PxControllerShapeType::eBOX) {
        return PxVec3(0, 0, 0);
    }

    PxBoxController* box = static_cast<PxBoxController*>(m_impl->m_controller);
    return PxVec3(
        box->getHalfSideExtent(),
        box->getHalfHeight(),
        box->getHalfForwardExtent()
    );
}

// ============================================================================
// Physics Interaction
// ============================================================================

PxRigidDynamic* CharacterController::getActor() const
{
    if (!m_impl->m_controller) {
        return nullptr;
    }

    return m_impl->m_controller->getActor();
}

void CharacterController::invalidateCache()
{
    if (m_impl->m_controller) {
        m_impl->m_controller->invalidateCache();
    }
}

bool CharacterController::resize(PxReal height)
{
    if (!m_impl->m_controller) {
        return false;
    }

    // PhysX 5.x: resize() returns void instead of bool
    m_impl->m_controller->resize(height);
    return true;
}

// ============================================================================
// Debug
// ============================================================================

void CharacterController::printInfo() const
{
    if (!m_impl->m_controller) {
        std::cout << "CharacterController: No controller" << std::endl;
        return;
    }

    std::cout << "CharacterController Info:" << std::endl;

    PxControllerShapeType::Enum type = getType();
    if (type == PxControllerShapeType::eCAPSULE) {
        std::cout << "  Type: Capsule" << std::endl;
        std::cout << "  Radius: " << getCapsuleRadius() << std::endl;
        std::cout << "  Height: " << getCapsuleHeight() << std::endl;
    } else if (type == PxControllerShapeType::eBOX) {
        std::cout << "  Type: Box" << std::endl;
        PxVec3 extents = getBoxHalfExtents();
        std::cout << "  Half Extents: (" << extents.x << ", " << extents.y << ", " << extents.z << ")" << std::endl;
    }

    PxExtendedVec3 pos = getPosition();
    std::cout << "  Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    std::cout << "  Grounded: " << (isGrounded() ? "Yes" : "No") << std::endl;
    std::cout << "  Vertical Velocity: " << m_impl->m_verticalVelocity << std::endl;
    std::cout << "  Slope Limit: " << getSlopeLimit() << std::endl;
    std::cout << "  Step Offset: " << getStepOffset() << std::endl;

    PxVec3 upDir = getUpDirection();
    std::cout << "  Up Direction: (" << upDir.x << ", " << upDir.y << ", " << upDir.z << ")" << std::endl;
}

} // namespace PhysXWrapper
