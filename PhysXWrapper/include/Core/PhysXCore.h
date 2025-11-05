/**
 * @file PhysXCore.h
 * @brief Core PhysX wrapper class providing basic physics simulation functionality
 *
 * This class encapsulates the fundamental PhysX initialization, scene management,
 * and simulation loop. Based on SnippetHelloWorld from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <string>
#include <memory>
#include <functional>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Configuration parameters for PhysX initialization
 */
struct PhysXCoreConfig {
    /** Gravity vector (default: -9.81 m/s^2 in Y direction) */
    PxVec3 gravity = PxVec3(0.0f, -9.81f, 0.0f);

    /** Number of CPU threads for simulation (default: 2) */
    PxU32 numThreads = 2;

    /** Enable PhysX Visual Debugger connection (default: false) */
    bool enablePVD = false;

    /** PVD host address (default: "127.0.0.1") */
    std::string pvdHost = "127.0.0.1";

    /** PVD port (default: 5425) */
    PxU32 pvdPort = 5425;

    /** PVD timeout in milliseconds (default: 10) */
    PxU32 pvdTimeout = 10;

    /** Tolerance scale for simulation (default: 1.0) */
    float toleranceScale = 1.0f;

    /** Enable tracking allocations (default: true) */
    bool trackOutstandingAllocations = true;
};

/**
 * @brief Core PhysX wrapper class
 *
 * This class provides a simplified interface for initializing and running
 * PhysX physics simulation. It handles:
 * - Foundation and Physics SDK initialization
 * - Scene creation and configuration
 * - Simulation stepping
 * - Resource cleanup (RAII pattern)
 * - Error reporting
 *
 * @example
 * @code
 * PhysXCoreConfig config;
 * config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
 * config.enablePVD = true;
 *
 * PhysXCore physics;
 * if (physics.initialize(config)) {
 *     // Create actors...
 *     for (int i = 0; i < 100; i++) {
 *         physics.update(1.0f / 60.0f);
 *     }
 * }
 * // Automatic cleanup on destruction
 * @endcode
 */
class PhysXCore {
public:
    /**
     * @brief Constructor
     */
    PhysXCore();

    /**
     * @brief Destructor - automatically cleans up PhysX resources
     */
    ~PhysXCore();

    // Disable copy (PhysX objects shouldn't be copied)
    PhysXCore(const PhysXCore&) = delete;
    PhysXCore& operator=(const PhysXCore&) = delete;

    // Enable move
    PhysXCore(PhysXCore&& other) noexcept;
    PhysXCore& operator=(PhysXCore&& other) noexcept;

    /**
     * @brief Initialize PhysX with given configuration
     * @param config Configuration parameters
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize(const PhysXCoreConfig& config = PhysXCoreConfig());

    /**
     * @brief Cleanup and release all PhysX resources
     * Called automatically by destructor
     */
    void cleanup();

    /**
     * @brief Step the physics simulation
     * @param deltaTime Time step in seconds
     * @return true if simulation step succeeded
     */
    bool update(float deltaTime);

    /**
     * @brief Check if PhysX is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    const std::string& getLastError() const { return m_lastError; }

    /**
     * @brief Get the PhysX Foundation object
     * @return Pointer to Foundation (nullptr if not initialized)
     */
    PxFoundation* getFoundation() { return m_foundation; }

    /**
     * @brief Get the PhysX Physics object
     * @return Pointer to Physics (nullptr if not initialized)
     */
    PxPhysics* getPhysics() { return m_physics; }

    /**
     * @brief Get the PhysX Scene object
     * @return Pointer to Scene (nullptr if not initialized)
     */
    PxScene* getScene() { return m_scene; }

    /**
     * @brief Get the default material
     * @return Pointer to default PxMaterial
     */
    PxMaterial* getDefaultMaterial() { return m_defaultMaterial; }

    /**
     * @brief Create a new material
     * @param staticFriction Static friction coefficient
     * @param dynamicFriction Dynamic friction coefficient
     * @param restitution Restitution (bounciness) coefficient
     * @return Pointer to created material
     */
    PxMaterial* createMaterial(float staticFriction, float dynamicFriction, float restitution);

    /**
     * @brief Create a dynamic rigid body
     * @param transform Initial transform
     * @param geometry Collision geometry
     * @param density Mass density (kg/m^3)
     * @param velocity Initial linear velocity
     * @return Pointer to created dynamic actor (nullptr on failure)
     */
    PxRigidDynamic* createDynamic(
        const PxTransform& transform,
        const PxGeometry& geometry,
        float density = 10.0f,
        const PxVec3& velocity = PxVec3(0)
    );

    /**
     * @brief Create a static rigid body
     * @param transform Transform
     * @param geometry Collision geometry
     * @return Pointer to created static actor (nullptr on failure)
     */
    PxRigidStatic* createStatic(
        const PxTransform& transform,
        const PxGeometry& geometry
    );

    /**
     * @brief Create a ground plane
     * @param normal Plane normal vector
     * @param distance Distance from origin
     * @return Pointer to created ground plane (nullptr on failure)
     */
    PxRigidStatic* createGroundPlane(
        const PxVec3& normal = PxVec3(0, 1, 0),
        float distance = 0.0f
    );

    /**
     * @brief Get current configuration
     * @return Current config
     */
    const PhysXCoreConfig& getConfig() const { return m_config; }

private:
    /** PhysX allocator */
    PxDefaultAllocator m_allocator;

    /** PhysX error callback */
    PxDefaultErrorCallback m_errorCallback;

    /** PhysX Foundation */
    PxFoundation* m_foundation;

    /** PhysX Physics SDK */
    PxPhysics* m_physics;

    /** PhysX Scene */
    PxScene* m_scene;

    /** CPU Dispatcher */
    PxDefaultCpuDispatcher* m_dispatcher;

    /** PhysX Visual Debugger */
    PxPvd* m_pvd;

    /** PVD Transport */
    PxPvdTransport* m_pvdTransport;

    /** Default material */
    PxMaterial* m_defaultMaterial;

    /** Configuration */
    PhysXCoreConfig m_config;

    /** Initialization flag */
    bool m_initialized;

    /** Last error message */
    std::string m_lastError;

    /**
     * @brief Set error message
     */
    void setError(const std::string& error);
};

} // namespace PhysXWrapper
