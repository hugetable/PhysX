/**
 * @file RigidBodyCCD.h
 * @brief Simplified Continuous Collision Detection (CCD) interface
 *
 * This class provides a simplified interface for using CCD to prevent fast-moving
 * objects from tunneling through geometry. Based on SnippetCCD from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <string>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief CCD algorithm type
 */
enum class CCDAlgorithm {
    LINEAR,      ///< Linear CCD (prevents tunneling for translating objects)
    SPECULATIVE, ///< Speculative/Angular CCD (prevents tunneling for rotating objects)
    FULL,        ///< Full CCD (linear + speculative)
    RAYCAST,     ///< Raycast CCD (extension-based, most accurate)
    NONE         ///< No CCD (default PhysX behavior)
};

/**
 * @brief Configuration for CCD
 */
struct CCDConfig {
    /** CCD algorithm to use */
    CCDAlgorithm algorithm = CCDAlgorithm::LINEAR;

    /** Maximum CCD passes (default: 1) */
    PxU32 maxCCDPasses = 1;

    /** CCD threshold (minimum relative velocity to trigger CCD) */
    PxReal ccdThreshold = 0.0f;

    /** Max bias coefficient for contact points (0-1, default: 0.08) */
    PxReal maxBiasCoefficient = 0.08f;

    /** Enable CCD for all dynamic objects by default */
    bool enableForAllDynamics = false;
};

/**
 * @brief CCD statistics
 */
struct CCDStats {
    PxU32 ccdPairs = 0;          ///< Number of CCD pairs processed
    PxU32 ccdContacts = 0;       ///< Number of CCD contacts generated
    float ccdTime = 0.0f;        ///< Time spent in CCD (ms)
};

/**
 * @brief Rigid body CCD manager class
 *
 * This class simplifies working with Continuous Collision Detection in PhysX:
 * - Easy CCD enabling for rigid bodies
 * - Multiple CCD algorithms
 * - Scene configuration for CCD
 * - Per-shape CCD threshold configuration
 * - Raycast CCD integration
 *
 * @example
 * @code
 * // Create CCD manager
 * RigidBodyCCD ccdManager;
 *
 * // Configure scene for CCD
 * CCDConfig config;
 * config.algorithm = CCDAlgorithm::FULL;
 * PxSceneDesc sceneDesc = ccdManager.createSceneDesc(scale, config);
 * scene = physics->createScene(sceneDesc);
 *
 * // Create fast-moving object with CCD
 * PxRigidDynamic* bullet = physics->createRigidDynamic(transform);
 * bullet->attachShape(*shape);
 * ccdManager.enableCCD(bullet, config);
 * bullet->setLinearVelocity(PxVec3(0, 0, -1000));  // Very fast!
 * scene->addActor(*bullet);
 * @endcode
 */
class RigidBodyCCD {
public:
    /**
     * @brief Constructor
     */
    RigidBodyCCD();

    /**
     * @brief Destructor
     */
    ~RigidBodyCCD();

    // Disable copy
    RigidBodyCCD(const RigidBodyCCD&) = delete;
    RigidBodyCCD& operator=(const RigidBodyCCD&) = delete;

    /**
     * @brief Create scene descriptor configured for CCD
     * @param scale Tolerance scale
     * @param config CCD configuration
     * @param gravity Gravity vector
     * @return Configured scene descriptor
     */
    PxSceneDesc createSceneDesc(
        const PxTolerancesScale& scale,
        const CCDConfig& config,
        const PxVec3& gravity = PxVec3(0, -9.81f, 0)
    );

    /**
     * @brief Enable CCD for a rigid body
     * @param actor Dynamic actor to enable CCD for
     * @param config CCD configuration (uses current if not specified)
     * @return True if successful
     */
    bool enableCCD(
        PxRigidDynamic* actor,
        const CCDConfig& config = CCDConfig()
    );

    /**
     * @brief Disable CCD for a rigid body
     * @param actor Dynamic actor to disable CCD for
     * @return True if successful
     */
    bool disableCCD(PxRigidDynamic* actor);

    /**
     * @brief Check if CCD is enabled for an actor
     * @param actor Actor to check
     * @return True if CCD is enabled
     */
    bool isCCDEnabled(PxRigidDynamic* actor) const;

    /**
     * @brief Set CCD threshold for a shape
     * @param shape Shape to set threshold for
     * @param threshold CCD threshold (0 = always use CCD)
     */
    void setShapeCCDThreshold(PxShape* shape, PxReal threshold);

    /**
     * @brief Get CCD threshold for a shape
     * @param shape Shape to get threshold from
     * @return CCD threshold
     */
    PxReal getShapeCCDThreshold(PxShape* shape) const;

    /**
     * @brief Enable CCD for all dynamic actors in a scene
     * @param scene Scene to process
     * @param config CCD configuration
     * @return Number of actors modified
     */
    PxU32 enableCCDForAllDynamics(
        PxScene* scene,
        const CCDConfig& config = CCDConfig()
    );

    /**
     * @brief Create a fast-moving dynamic actor with CCD enabled
     * @param physics PxPhysics instance
     * @param scene PxScene instance
     * @param transform Initial transform
     * @param geometry Geometry for the actor
     * @param material Material for the shape
     * @param velocity Initial velocity
     * @param density Density for mass calculation
     * @param config CCD configuration
     * @return Created actor (owned by scene)
     */
    PxRigidDynamic* createFastMovingDynamic(
        PxPhysics* physics,
        PxScene* scene,
        const PxTransform& transform,
        const PxGeometry& geometry,
        PxMaterial* material,
        const PxVec3& velocity,
        PxReal density = 10.0f,
        const CCDConfig& config = CCDConfig()
    );

    /**
     * @brief Get CCD statistics from a scene
     * @param scene Scene to get statistics from
     * @return CCD statistics
     */
    CCDStats getCCDStats(PxScene* scene) const;

    /**
     * @brief Get filter shader for CCD
     * @return Filter shader function pointer
     */
    static PxSimulationFilterShader getCCDFilterShader();

    /**
     * @brief Calculate recommended CCD threshold for an object
     * @param velocity Linear velocity
     * @param size Characteristic size of object
     * @param timeStep Simulation time step
     * @return Recommended threshold
     */
    static PxReal calculateCCDThreshold(
        const PxVec3& velocity,
        PxReal size,
        PxReal timeStep = 1.0f / 60.0f
    );

    /**
     * @brief Get current configuration
     * @return Current CCD configuration
     */
    const CCDConfig& getConfig() const;

    /**
     * @brief Set configuration
     * @param config New CCD configuration
     */
    void setConfig(const CCDConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace PhysXWrapper
