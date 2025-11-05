/**
 * @file VehicleManager.cpp
 * @brief Implementation of VehicleManager class
 */

#include "Vehicle/VehicleManager.h"
#include <iostream>
#include <algorithm>

namespace PhysXWrapper {

// ============================================================================
// VehicleManager::Impl
// ============================================================================

class VehicleManager::Impl {
public:
    PxPhysics* m_physics = nullptr;
    PxScene* m_scene = nullptr;
    PxMaterial* m_defaultMaterial = nullptr;

    std::vector<Vehicle*> m_vehicles;
    PxU32 m_nextVehicleId = 0;

    bool m_initialized = false;
};

// ============================================================================
// Construction/Destruction
// ============================================================================

VehicleManager::VehicleManager()
    : m_impl(std::make_unique<Impl>())
{
}

VehicleManager::~VehicleManager()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool VehicleManager::initialize(PxPhysics* physics, PxScene* scene)
{
    if (!physics || !scene) {
        std::cerr << "VehicleManager::initialize: physics or scene is null" << std::endl;
        return false;
    }

    m_impl->m_physics = physics;
    m_impl->m_scene = scene;

    // Create default material
    m_impl->m_defaultMaterial = physics->createMaterial(0.8f, 0.8f, 0.1f);
    if (!m_impl->m_defaultMaterial) {
        std::cerr << "VehicleManager::initialize: Failed to create default material" << std::endl;
        return false;
    }

    m_impl->m_initialized = true;
    return true;
}

void VehicleManager::cleanup()
{
    releaseAllVehicles();

    if (m_impl->m_defaultMaterial) {
        m_impl->m_defaultMaterial->release();
        m_impl->m_defaultMaterial = nullptr;
    }

    m_impl->m_initialized = false;
}

bool VehicleManager::isInitialized() const
{
    return m_impl->m_initialized;
}

// ============================================================================
// Vehicle Creation
// ============================================================================

VehicleManager::Vehicle* VehicleManager::createVehicle(const VehicleDesc& desc)
{
    if (!m_impl->m_initialized) {
        std::cerr << "VehicleManager::createVehicle: Not initialized" << std::endl;
        return nullptr;
    }

    Vehicle* vehicle = new Vehicle();
    vehicle->desc = desc;
    vehicle->id = m_impl->m_nextVehicleId++;

    // Create chassis
    PxBoxGeometry chassisGeom(desc.chassisDims * 0.5f);
    vehicle->chassisShape = m_impl->m_physics->createShape(chassisGeom, *m_impl->m_defaultMaterial, true);

    PxTransform chassisTransform = desc.initialTransform;
    vehicle->chassis = m_impl->m_physics->createRigidDynamic(chassisTransform);
    vehicle->chassis->attachShape(*vehicle->chassisShape);

    // Set chassis mass and center of mass
    PxRigidBodyExt::setMassAndUpdateInertia(*vehicle->chassis, desc.chassisMass);
    vehicle->chassis->setCMassLocalPose(PxTransform(desc.chassisCenterOfMass));

    // Set damping to reduce excessive motion
    vehicle->chassis->setLinearDamping(0.1f);
    vehicle->chassis->setAngularDamping(0.5f);

    // Create wheels
    for (int i = 0; i < 4; i++) {
        PxVec3 wheelPos = chassisTransform.p + desc.wheelCenterOffsets[i];

        // Create wheel shape (cylinder approximation using capsule)
        PxCapsuleGeometry wheelGeom(desc.wheelConfig.width * 0.5f, desc.wheelConfig.radius);
        vehicle->wheelShapes[i] = m_impl->m_physics->createShape(wheelGeom, *m_impl->m_defaultMaterial, true);

        // Rotate wheel to be horizontal (capsule is vertical by default)
        PxQuat wheelRot = PxQuat(PxHalfPi, PxVec3(0, 0, 1));
        vehicle->wheelShapes[i]->setLocalPose(PxTransform(wheelRot));

        vehicle->wheels[i] = m_impl->m_physics->createRigidDynamic(PxTransform(wheelPos));
        vehicle->wheels[i]->attachShape(*vehicle->wheelShapes[i]);

        PxRigidBodyExt::setMassAndUpdateInertia(*vehicle->wheels[i], desc.wheelConfig.mass);

        // Increase angular damping for wheels to prevent excessive spinning
        vehicle->wheels[i]->setAngularDamping(0.5f);

        m_impl->m_scene->addActor(*vehicle->wheels[i]);
    }

    // Add chassis to scene
    m_impl->m_scene->addActor(*vehicle->chassis);

    m_impl->m_vehicles.push_back(vehicle);

    return vehicle;
}

VehicleManager::Vehicle* VehicleManager::createSimpleVehicle(const PxVec3& position)
{
    VehicleDesc desc;
    desc.initialTransform = PxTransform(position);
    return createVehicle(desc);
}

bool VehicleManager::releaseVehicle(Vehicle* vehicle)
{
    if (!vehicle) {
        return false;
    }

    // Remove from list
    auto it = std::find(m_impl->m_vehicles.begin(), m_impl->m_vehicles.end(), vehicle);
    if (it != m_impl->m_vehicles.end()) {
        m_impl->m_vehicles.erase(it);
    }

    // Release wheels
    for (int i = 0; i < 4; i++) {
        if (vehicle->wheels[i]) {
            if (m_impl->m_scene) {
                m_impl->m_scene->removeActor(*vehicle->wheels[i]);
            }
            vehicle->wheels[i]->release();
            vehicle->wheels[i] = nullptr;
        }
        if (vehicle->wheelShapes[i]) {
            vehicle->wheelShapes[i]->release();
            vehicle->wheelShapes[i] = nullptr;
        }
    }

    // Release chassis
    if (vehicle->chassis) {
        if (m_impl->m_scene) {
            m_impl->m_scene->removeActor(*vehicle->chassis);
        }
        vehicle->chassis->release();
        vehicle->chassis = nullptr;
    }

    if (vehicle->chassisShape) {
        vehicle->chassisShape->release();
        vehicle->chassisShape = nullptr;
    }

    delete vehicle;
    return true;
}

void VehicleManager::releaseAllVehicles()
{
    while (!m_impl->m_vehicles.empty()) {
        releaseVehicle(m_impl->m_vehicles.back());
    }
}

// ============================================================================
// Vehicle Control
// ============================================================================

void VehicleManager::setThrottle(Vehicle* vehicle, PxReal throttle)
{
    if (vehicle) {
        vehicle->controls.throttle = PxClamp(throttle, 0.0f, 1.0f);
    }
}

void VehicleManager::setBrake(Vehicle* vehicle, PxReal brake)
{
    if (vehicle) {
        vehicle->controls.brake = PxClamp(brake, 0.0f, 1.0f);
    }
}

void VehicleManager::setSteering(Vehicle* vehicle, PxReal steering)
{
    if (vehicle) {
        vehicle->controls.steering = PxClamp(steering, -1.0f, 1.0f);
    }
}

void VehicleManager::setHandbrake(Vehicle* vehicle, PxReal handbrake)
{
    if (vehicle) {
        vehicle->controls.handbrake = PxClamp(handbrake, 0.0f, 1.0f);
    }
}

void VehicleManager::setControls(Vehicle* vehicle, const VehicleControls& controls)
{
    if (vehicle) {
        vehicle->controls = controls;
        vehicle->controls.throttle = PxClamp(controls.throttle, 0.0f, 1.0f);
        vehicle->controls.brake = PxClamp(controls.brake, 0.0f, 1.0f);
        vehicle->controls.steering = PxClamp(controls.steering, -1.0f, 1.0f);
        vehicle->controls.handbrake = PxClamp(controls.handbrake, 0.0f, 1.0f);
    }
}

void VehicleManager::resetControls(Vehicle* vehicle)
{
    if (vehicle) {
        vehicle->controls.reset();
    }
}

// ============================================================================
// Vehicle State
// ============================================================================

VehicleManager::VehicleState VehicleManager::getState(Vehicle* vehicle) const
{
    VehicleState state;

    if (!vehicle || !vehicle->chassis) {
        return state;
    }

    state.position = vehicle->chassis->getGlobalPose().p;
    state.rotation = vehicle->chassis->getGlobalPose().q;
    state.linearVelocity = vehicle->chassis->getLinearVelocity();
    state.angularVelocity = vehicle->chassis->getAngularVelocity();
    state.speed = state.linearVelocity.magnitude();

    // Check if on ground (simple raycast down from chassis)
    PxVec3 origin = state.position;
    PxVec3 direction(0, -1, 0);
    PxReal distance = 2.0f;

    PxRaycastBuffer hit;
    if (m_impl->m_scene->raycast(origin, direction, distance, hit)) {
        state.isOnGround = true;
    }

    // Get wheel speeds
    for (int i = 0; i < 4; i++) {
        if (vehicle->wheels[i]) {
            PxVec3 angVel = vehicle->wheels[i]->getAngularVelocity();
            state.wheelRotationSpeeds[i] = angVel.magnitude();
        }
    }

    return state;
}

PxVec3 VehicleManager::getPosition(Vehicle* vehicle) const
{
    if (!vehicle || !vehicle->chassis) {
        return PxVec3(0, 0, 0);
    }
    return vehicle->chassis->getGlobalPose().p;
}

PxQuat VehicleManager::getRotation(Vehicle* vehicle) const
{
    if (!vehicle || !vehicle->chassis) {
        return PxQuat(PxIdentity);
    }
    return vehicle->chassis->getGlobalPose().q;
}

PxReal VehicleManager::getSpeed(Vehicle* vehicle) const
{
    if (!vehicle || !vehicle->chassis) {
        return 0.0f;
    }
    return vehicle->chassis->getLinearVelocity().magnitude();
}

bool VehicleManager::isOnGround(Vehicle* vehicle) const
{
    if (!vehicle || !vehicle->chassis) {
        return false;
    }

    VehicleState state = getState(vehicle);
    return state.isOnGround;
}

// ============================================================================
// Update
// ============================================================================

void VehicleManager::update(PxReal deltaTime)
{
    for (Vehicle* vehicle : m_impl->m_vehicles) {
        updateVehicle(vehicle, deltaTime);
    }
}

void VehicleManager::updateVehicle(Vehicle* vehicle, PxReal deltaTime)
{
    if (!vehicle || !vehicle->chassis) {
        return;
    }

    const VehicleControls& controls = vehicle->controls;
    const VehicleDesc& desc = vehicle->desc;

    // Get chassis transform
    PxTransform chassisTransform = vehicle->chassis->getGlobalPose();
    PxVec3 chassisForward = chassisTransform.q.rotate(PxVec3(0, 0, 1));
    PxVec3 chassisRight = chassisTransform.q.rotate(PxVec3(1, 0, 0));

    // Apply suspension forces (simple spring-damper for each wheel)
    for (int i = 0; i < 4; i++) {
        if (!vehicle->wheels[i]) continue;

        PxVec3 wheelPos = vehicle->wheels[i]->getGlobalPose().p;
        PxVec3 chassisWheelPos = chassisTransform.p + chassisTransform.q.rotate(desc.wheelCenterOffsets[i]);

        // Raycast down from chassis wheel position
        PxVec3 rayOrigin = chassisWheelPos + PxVec3(0, 0.5f, 0);
        PxVec3 rayDir(0, -1, 0);
        PxReal rayLength = desc.suspensionConfig.maxCompression + desc.suspensionConfig.maxDroop + 1.0f;

        PxRaycastBuffer hit;
        if (m_impl->m_scene->raycast(rayOrigin, rayDir, rayLength, hit)) {
            // Calculate suspension force
            PxReal compression = hit.block.distance - 0.5f;
            PxReal compressionRatio = PxClamp(compression / desc.suspensionConfig.maxCompression, 0.0f, 1.0f);

            // Spring force
            PxReal springForce = desc.suspensionConfig.springStrength * (1.0f - compressionRatio);

            // Damping force
            PxVec3 wheelVel = vehicle->wheels[i]->getLinearVelocity();
            PxReal damperForce = desc.suspensionConfig.springDamping * wheelVel.y;

            PxReal totalForce = springForce - damperForce;
            PxVec3 suspensionForce = PxVec3(0, totalForce, 0);

            // Apply suspension force to chassis
            vehicle->chassis->addForceAtPos(suspensionForce, chassisWheelPos);

            // Keep wheel at ground level
            PxVec3 targetWheelPos = hit.block.position + PxVec3(0, desc.wheelConfig.radius, 0);
            vehicle->wheels[i]->setGlobalPose(PxTransform(targetWheelPos, vehicle->wheels[i]->getGlobalPose().q));
        }
    }

    // Apply throttle (engine force)
    if (controls.throttle > 0.0f) {
        PxReal engineForce = desc.engineConfig.peakTorque * controls.throttle * 10.0f;

        // Determine which wheels to drive based on drive type
        bool driveWheel[4] = {false};
        switch (desc.driveType) {
            case DriveType::FOUR_WHEEL_DRIVE:
                driveWheel[0] = driveWheel[1] = driveWheel[2] = driveWheel[3] = true;
                break;
            case DriveType::FRONT_WHEEL_DRIVE:
                driveWheel[0] = driveWheel[1] = true;
                break;
            case DriveType::REAR_WHEEL_DRIVE:
                driveWheel[2] = driveWheel[3] = true;
                break;
        }

        // Apply force to driven wheels
        int numDrivenWheels = 0;
        for (int i = 0; i < 4; i++) {
            if (driveWheel[i]) numDrivenWheels++;
        }

        PxReal forcePerWheel = engineForce / numDrivenWheels;
        for (int i = 0; i < 4; i++) {
            if (driveWheel[i] && vehicle->wheels[i]) {
                PxVec3 driveForce = chassisForward * forcePerWheel;
                vehicle->chassis->addForceAtPos(driveForce, vehicle->wheels[i]->getGlobalPose().p);
            }
        }
    }

    // Apply brake
    if (controls.brake > 0.0f) {
        PxVec3 velocity = vehicle->chassis->getLinearVelocity();
        PxReal speed = velocity.magnitude();

        if (speed > 0.1f) {
            PxReal brakeForce = desc.wheelConfig.maxBrakeTorque * controls.brake;
            PxVec3 brakeDirection = -velocity.getNormalized();
            vehicle->chassis->addForce(brakeDirection * brakeForce);
        }
    }

    // Apply steering (front wheels only)
    if (PxAbs(controls.steering) > 0.001f) {
        PxReal steerAngle = controls.steering * desc.wheelConfig.maxSteerAngle;
        PxReal turnRadius = desc.chassisDims.z / PxTan(PxAbs(steerAngle));

        PxReal angularVelocity = getSpeed(vehicle) / turnRadius;
        if (controls.steering < 0.0f) angularVelocity = -angularVelocity;

        // Apply torque to turn
        PxReal turnTorque = angularVelocity * desc.chassisMass * 5.0f;
        vehicle->chassis->addTorque(PxVec3(0, turnTorque, 0));
    }

    // Apply handbrake (rear wheels only)
    if (controls.handbrake > 0.0f) {
        PxVec3 velocity = vehicle->chassis->getLinearVelocity();
        PxReal speed = velocity.magnitude();

        if (speed > 0.1f) {
            PxReal handbrakeForce = desc.wheelConfig.maxHandBrakeTorque * controls.handbrake;
            PxVec3 brakeDirection = -velocity.getNormalized();
            vehicle->chassis->addForce(brakeDirection * handbrakeForce);

            // Also reduce angular velocity for sharper stops
            PxVec3 angVel = vehicle->chassis->getAngularVelocity();
            vehicle->chassis->setAngularVelocity(angVel * (1.0f - controls.handbrake * 0.5f));
        }
    }
}

// ============================================================================
// Management
// ============================================================================

PxU32 VehicleManager::getVehicleCount() const
{
    return static_cast<PxU32>(m_impl->m_vehicles.size());
}

VehicleManager::Vehicle* VehicleManager::getVehicle(PxU32 index) const
{
    if (index >= m_impl->m_vehicles.size()) {
        return nullptr;
    }
    return m_impl->m_vehicles[index];
}

std::vector<VehicleManager::Vehicle*> VehicleManager::getAllVehicles() const
{
    return m_impl->m_vehicles;
}

void VehicleManager::printVehicleInfo(Vehicle* vehicle) const
{
    if (!vehicle) {
        std::cout << "Vehicle is null" << std::endl;
        return;
    }

    VehicleState state = getState(vehicle);

    std::cout << "Vehicle " << vehicle->id << ":" << std::endl;
    std::cout << "  Position: (" << state.position.x << ", " << state.position.y << ", " << state.position.z << ")" << std::endl;
    std::cout << "  Speed: " << state.speed << " m/s (" << (state.speed * 3.6f) << " km/h)" << std::endl;
    std::cout << "  On Ground: " << (state.isOnGround ? "Yes" : "No") << std::endl;
    std::cout << "  Controls:" << std::endl;
    std::cout << "    Throttle: " << vehicle->controls.throttle << std::endl;
    std::cout << "    Brake: " << vehicle->controls.brake << std::endl;
    std::cout << "    Steering: " << vehicle->controls.steering << std::endl;
    std::cout << "    Handbrake: " << vehicle->controls.handbrake << std::endl;
}

void VehicleManager::printAllVehicles() const
{
    std::cout << "Total vehicles: " << m_impl->m_vehicles.size() << std::endl;

    for (size_t i = 0; i < m_impl->m_vehicles.size(); i++) {
        std::cout << "\n[" << i << "] ";
        printVehicleInfo(m_impl->m_vehicles[i]);
    }
}

// ============================================================================
// Configuration
// ============================================================================

PxPhysics* VehicleManager::getPhysics() const
{
    return m_impl->m_physics;
}

PxScene* VehicleManager::getScene() const
{
    return m_impl->m_scene;
}

} // namespace PhysXWrapper
