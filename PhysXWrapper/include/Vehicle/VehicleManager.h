/**
 * @file VehicleManager.h
 * @brief Vehicle physics manager for wheeled vehicles
 *
 * This class provides utilities for creating and managing wheeled vehicles
 * with realistic physics including suspension, wheels, engine, transmission,
 * and tire friction models.
 *
 * Based on PhysX Vehicle SDK
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <vector>
#include <string>

using namespace physx;

namespace PhysXWrapper {

/**
 * @class VehicleManager
 * @brief Manager for PhysX wheeled vehicles
 *
 * VehicleManager provides a simplified interface for creating and controlling
 * wheeled vehicles with realistic physics simulation. It supports:
 * - 4-wheel vehicles (cars, trucks)
 * - Suspension system (springs and dampers)
 * - Wheel and tire physics
 * - Engine and transmission
 * - Steering, throttle, and braking
 * - Tire friction models
 * - Drive models (4WD, FWD, RWD)
 *
 * The vehicle system uses PhysX's built-in vehicle dynamics which includes:
 * - Longitudinal tire force (acceleration/braking)
 * - Lateral tire force (cornering)
 * - Suspension travel and forces
 * - Weight transfer during acceleration/braking
 * - Ackermann steering geometry
 *
 * Usage:
 * @code
 * VehicleManager manager;
 * manager.initialize(physics, scene);
 *
 * // Create simple vehicle
 * VehicleManager::VehicleDesc desc;
 * desc.chassisMass = 1500.0f;
 * desc.chassisDims = PxVec3(2.0f, 1.0f, 4.5f);
 * desc.wheelRadius = 0.5f;
 *
 * VehicleManager::Vehicle* vehicle = manager.createVehicle(desc);
 *
 * // Control vehicle
 * manager.setThrottle(vehicle, 0.8f);
 * manager.setSteering(vehicle, 0.3f);
 *
 * // Update each frame
 * manager.update(deltaTime);
 * @endcode
 */
class VehicleManager {
public:
    /**
     * @brief Drive type
     */
    enum class DriveType {
        FOUR_WHEEL_DRIVE,   ///< 4WD - all wheels driven
        FRONT_WHEEL_DRIVE,  ///< FWD - front wheels driven
        REAR_WHEEL_DRIVE    ///< RWD - rear wheels driven
    };

    /**
     * @brief Wheel position
     */
    enum WheelPosition {
        FRONT_LEFT = 0,
        FRONT_RIGHT = 1,
        REAR_LEFT = 2,
        REAR_RIGHT = 3
    };

    /**
     * @brief Wheel configuration
     */
    struct WheelConfig {
        PxReal radius = 0.5f;           ///< Wheel radius
        PxReal width = 0.3f;            ///< Wheel width
        PxReal mass = 20.0f;            ///< Wheel mass
        PxReal moi = 0.5f;              ///< Moment of inertia
        PxReal maxBrakeTorque = 1500.0f; ///< Maximum brake torque
        PxReal maxHandBrakeTorque = 4000.0f; ///< Maximum handbrake torque
        PxReal maxSteerAngle = 0.6f;    ///< Maximum steer angle (radians)

        WheelConfig() = default;

        static const WheelConfig& defaultConfig() {
            static const WheelConfig config;
            return config;
        }
    };

    /**
     * @brief Suspension configuration
     */
    struct SuspensionConfig {
        PxReal springStrength = 35000.0f;  ///< Spring strength
        PxReal springDamping = 4500.0f;    ///< Spring damping
        PxReal maxCompression = 0.3f;      ///< Max compression distance
        PxReal maxDroop = 0.1f;            ///< Max droop distance
        PxVec3 forceAppPoint = PxVec3(0, -0.3f, 0); ///< Force application point

        SuspensionConfig() = default;

        static const SuspensionConfig& defaultConfig() {
            static const SuspensionConfig config;
            return config;
        }
    };

    /**
     * @brief Engine configuration
     */
    struct EngineConfig {
        PxReal peakTorque = 500.0f;        ///< Peak engine torque (Nm)
        PxReal maxOmega = 600.0f;          ///< Maximum angular velocity (rad/s)
        PxReal dampingRateFullThrottle = 0.15f;  ///< Damping at full throttle
        PxReal dampingRateZeroThrottle = 2.0f;   ///< Damping at zero throttle

        EngineConfig() = default;

        static const EngineConfig& defaultConfig() {
            static const EngineConfig config;
            return config;
        }
    };

    /**
     * @brief Transmission configuration
     */
    struct TransmissionConfig {
        PxU32 numGears = 6;                ///< Number of forward gears
        PxReal gearSwitchTime = 0.5f;      ///< Time to switch gears
        PxReal clutchStrength = 10.0f;     ///< Clutch strength

        TransmissionConfig() = default;

        static const TransmissionConfig& defaultConfig() {
            static const TransmissionConfig config;
            return config;
        }
    };

    /**
     * @brief Tire friction configuration
     */
    struct TireFrictionConfig {
        PxReal frictionMultiplier = 1.0f;  ///< Friction multiplier
        PxReal longitudinalStiffness = 1000.0f; ///< Longitudinal stiffness
        PxReal lateralStiffness = 1000.0f;      ///< Lateral stiffness

        TireFrictionConfig() = default;

        static const TireFrictionConfig& defaultConfig() {
            static const TireFrictionConfig config;
            return config;
        }
    };

    /**
     * @brief Vehicle descriptor
     */
    struct VehicleDesc {
        // Chassis
        PxVec3 chassisDims = PxVec3(2.0f, 1.0f, 4.5f);  ///< Chassis dimensions
        PxReal chassisMass = 1500.0f;                    ///< Chassis mass (kg)
        PxVec3 chassisCenterOfMass = PxVec3(0, -0.5f, 0); ///< Center of mass offset

        // Wheels
        WheelConfig wheelConfig;
        PxVec3 wheelCenterOffsets[4] = {
            PxVec3(-1.0f, -0.3f, 1.5f),   // Front left
            PxVec3(1.0f, -0.3f, 1.5f),    // Front right
            PxVec3(-1.0f, -0.3f, -1.5f),  // Rear left
            PxVec3(1.0f, -0.3f, -1.5f)    // Rear right
        };

        // Suspension
        SuspensionConfig suspensionConfig;

        // Engine
        EngineConfig engineConfig;

        // Transmission
        TransmissionConfig transmissionConfig;

        // Tire friction
        TireFrictionConfig tireFrictionConfig;

        // Drive type
        DriveType driveType = DriveType::FOUR_WHEEL_DRIVE;

        // Initial position
        PxTransform initialTransform = PxTransform(PxVec3(0, 2, 0));

        VehicleDesc() = default;

        static const VehicleDesc& defaultDesc() {
            static const VehicleDesc desc;
            return desc;
        }
    };

    /**
     * @brief Vehicle state
     */
    struct VehicleState {
        PxVec3 position;                ///< Position
        PxQuat rotation;                ///< Rotation
        PxVec3 linearVelocity;          ///< Linear velocity
        PxVec3 angularVelocity;         ///< Angular velocity
        PxReal speed = 0.0f;            ///< Speed (m/s)
        PxReal rpm = 0.0f;              ///< Engine RPM
        PxU32 currentGear = 0;          ///< Current gear
        bool isOnGround = false;        ///< Is on ground
        PxReal wheelRotationSpeeds[4];  ///< Wheel rotation speeds

        VehicleState() {
            for (int i = 0; i < 4; i++) wheelRotationSpeeds[i] = 0.0f;
        }
    };

    /**
     * @brief Vehicle control inputs
     */
    struct VehicleControls {
        PxReal throttle = 0.0f;     ///< Throttle [0, 1]
        PxReal brake = 0.0f;        ///< Brake [0, 1]
        PxReal steering = 0.0f;     ///< Steering [-1, 1]
        PxReal handbrake = 0.0f;    ///< Handbrake [0, 1]
        bool autoGear = true;       ///< Automatic gear shifting

        void reset() {
            throttle = 0.0f;
            brake = 0.0f;
            steering = 0.0f;
            handbrake = 0.0f;
        }
    };

    /**
     * @brief Simple vehicle representation
     */
    struct Vehicle {
        PxRigidDynamic* chassis = nullptr;           ///< Chassis actor
        PxShape* chassisShape = nullptr;             ///< Chassis shape
        PxRigidDynamic* wheels[4] = {nullptr};       ///< Wheel actors
        PxShape* wheelShapes[4] = {nullptr};         ///< Wheel shapes
        VehicleControls controls;                    ///< Control inputs
        VehicleDesc desc;                            ///< Vehicle descriptor
        PxU32 id = 0;                               ///< Vehicle ID

        bool isValid() const {
            return chassis != nullptr;
        }
    };

public:
    /**
     * @brief Constructor
     */
    VehicleManager();

    /**
     * @brief Destructor
     */
    ~VehicleManager();

    // Disable copy
    VehicleManager(const VehicleManager&) = delete;
    VehicleManager& operator=(const VehicleManager&) = delete;

    /**
     * @brief Initialize vehicle manager
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
    // Vehicle Creation
    // ========================================================================

    /**
     * @brief Create vehicle
     * @param desc Vehicle descriptor
     * @return Vehicle pointer (or nullptr on failure)
     */
    Vehicle* createVehicle(const VehicleDesc& desc = VehicleDesc::defaultDesc());

    /**
     * @brief Create simple 4-wheel vehicle
     * @param position Initial position
     * @return Vehicle pointer (or nullptr on failure)
     */
    Vehicle* createSimpleVehicle(const PxVec3& position = PxVec3(0, 2, 0));

    /**
     * @brief Release vehicle
     * @param vehicle Vehicle to release
     * @return true if successful
     */
    bool releaseVehicle(Vehicle* vehicle);

    /**
     * @brief Release all vehicles
     */
    void releaseAllVehicles();

    // ========================================================================
    // Vehicle Control
    // ========================================================================

    /**
     * @brief Set throttle
     * @param vehicle Target vehicle
     * @param throttle Throttle value [0, 1]
     */
    void setThrottle(Vehicle* vehicle, PxReal throttle);

    /**
     * @brief Set brake
     * @param vehicle Target vehicle
     * @param brake Brake value [0, 1]
     */
    void setBrake(Vehicle* vehicle, PxReal brake);

    /**
     * @brief Set steering
     * @param vehicle Target vehicle
     * @param steering Steering value [-1, 1]
     */
    void setSteering(Vehicle* vehicle, PxReal steering);

    /**
     * @brief Set handbrake
     * @param vehicle Target vehicle
     * @param handbrake Handbrake value [0, 1]
     */
    void setHandbrake(Vehicle* vehicle, PxReal handbrake);

    /**
     * @brief Set all controls
     * @param vehicle Target vehicle
     * @param controls Control inputs
     */
    void setControls(Vehicle* vehicle, const VehicleControls& controls);

    /**
     * @brief Reset controls to zero
     * @param vehicle Target vehicle
     */
    void resetControls(Vehicle* vehicle);

    // ========================================================================
    // Vehicle State
    // ========================================================================

    /**
     * @brief Get vehicle state
     * @param vehicle Target vehicle
     * @return Vehicle state
     */
    VehicleState getState(Vehicle* vehicle) const;

    /**
     * @brief Get vehicle position
     * @param vehicle Target vehicle
     * @return Position
     */
    PxVec3 getPosition(Vehicle* vehicle) const;

    /**
     * @brief Get vehicle rotation
     * @param vehicle Target vehicle
     * @return Rotation
     */
    PxQuat getRotation(Vehicle* vehicle) const;

    /**
     * @brief Get vehicle speed
     * @param vehicle Target vehicle
     * @return Speed in m/s
     */
    PxReal getSpeed(Vehicle* vehicle) const;

    /**
     * @brief Check if vehicle is on ground
     * @param vehicle Target vehicle
     * @return true if on ground
     */
    bool isOnGround(Vehicle* vehicle) const;

    // ========================================================================
    // Update
    // ========================================================================

    /**
     * @brief Update all vehicles
     * @param deltaTime Time step
     *
     * This applies forces based on control inputs
     */
    void update(PxReal deltaTime);

    /**
     * @brief Update specific vehicle
     * @param vehicle Target vehicle
     * @param deltaTime Time step
     */
    void updateVehicle(Vehicle* vehicle, PxReal deltaTime);

    // ========================================================================
    // Management
    // ========================================================================

    /**
     * @brief Get number of vehicles
     * @return Vehicle count
     */
    PxU32 getVehicleCount() const;

    /**
     * @brief Get vehicle by index
     * @param index Vehicle index
     * @return Vehicle pointer (or nullptr)
     */
    Vehicle* getVehicle(PxU32 index) const;

    /**
     * @brief Get all vehicles
     * @return Vector of vehicles
     */
    std::vector<Vehicle*> getAllVehicles() const;

    /**
     * @brief Print vehicle info
     * @param vehicle Target vehicle
     */
    void printVehicleInfo(Vehicle* vehicle) const;

    /**
     * @brief Print all vehicles
     */
    void printAllVehicles() const;

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

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace PhysXWrapper
