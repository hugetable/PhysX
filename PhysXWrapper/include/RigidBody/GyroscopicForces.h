/**
 * @file GyroscopicForces.h
 * @brief Gyroscopic forces demonstration for PhysX
 *
 * This class demonstrates gyroscopic forces and the Dzhanibekov effect
 * (also known as the tennis racket theorem or intermediate axis theorem).
 *
 * The Dzhanibekov effect shows that rotation around the intermediate principal
 * axis of inertia is unstable, causing periodic flipping behavior.
 *
 * Based on PhysX SnippetGyroscopic
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <memory>
#include <vector>

using namespace physx;

namespace PhysXWrapper {

/**
 * @class GyroscopicForces
 * @brief Demonstration and utility for gyroscopic forces
 *
 * Gyroscopic forces arise from the conservation of angular momentum in rotating
 * rigid bodies. When enabled in PhysX, these forces produce more physically
 * accurate behavior for spinning objects, especially those with non-uniform
 * moment of inertia.
 *
 * The Dzhanibekov effect demonstrates that:
 * - Rotation around the maximum moment of inertia axis is stable
 * - Rotation around the minimum moment of inertia axis is stable
 * - Rotation around the intermediate moment of inertia axis is UNSTABLE
 *
 * This instability causes the object to periodically flip 180 degrees,
 * which is dramatically visible with asymmetric objects.
 *
 * Usage:
 * @code
 * GyroscopicForces gyro;
 * gyro.initialize(physics, scene);
 *
 * // Create T-shaped actor with gyroscopic forces enabled
 * PxRigidDynamic* tShape = gyro.createTShape(
 *     PxTransform(PxVec3(0, 5, 0)),
 *     true  // enable gyroscopic
 * );
 *
 * // Observe the flipping behavior during simulation
 * @endcode
 */
class GyroscopicForces {
public:
    /**
     * @brief Configuration for gyroscopic demonstrations
     */
    struct GyroscopicConfig {
        /// Whether to enable gyroscopic forces
        bool enableGyroscopic = true;

        /// Initial angular velocity
        PxVec3 angularVelocity = PxVec3(7.5f, 5.025f, 0.0f);

        /// Angular damping (0 = no damping, shows pure effect)
        PxReal angularDamping = 0.0f;

        /// Density for mass calculation
        PxReal density = 1.0f;

        /// Whether to use zero gravity (cleaner demonstration)
        bool zeroGravity = true;

        GyroscopicConfig() = default;
    };

    /**
     * @brief Shapes that demonstrate gyroscopic effects
     */
    enum class DemoShape {
        T_SHAPE,          ///< T-shaped object (classic demonstration)
        L_SHAPE,          ///< L-shaped object
        HAMMER,           ///< Hammer-shaped object
        DUMBBELL,         ///< Dumbbell (two spheres connected)
        CROSS,            ///< Cross-shaped object
        TENNIS_RACKET     ///< Tennis racket shape
    };

    /**
     * @brief Statistics for gyroscopic object
     */
    struct GyroscopicStats {
        PxVec3 angularVelocity;      ///< Current angular velocity
        PxReal angularSpeed;          ///< Angular speed magnitude
        PxQuat orientation;           ///< Current orientation
        PxVec3 momentOfInertia;       ///< Diagonal moment of inertia
        PxReal kineticEnergy;         ///< Rotational kinetic energy
        int flipCount = 0;            ///< Number of detected flips

        void print() const;
    };

public:
    /**
     * @brief Constructor
     */
    GyroscopicForces();

    /**
     * @brief Destructor
     */
    ~GyroscopicForces();

    // Disable copy
    GyroscopicForces(const GyroscopicForces&) = delete;
    GyroscopicForces& operator=(const GyroscopicForces&) = delete;

    /**
     * @brief Initialize the gyroscopic forces manager
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
    // Actor Creation
    // ========================================================================

    /**
     * @brief Create T-shaped actor (classic Dzhanibekov demonstration)
     * @param transform Initial transform
     * @param config Gyroscopic configuration
     * @return Created actor (or nullptr on failure)
     */
    PxRigidDynamic* createTShape(const PxTransform& transform,
                                  const GyroscopicConfig& config = GyroscopicConfig());

    /**
     * @brief Create L-shaped actor
     * @param transform Initial transform
     * @param config Gyroscopic configuration
     * @return Created actor (or nullptr on failure)
     */
    PxRigidDynamic* createLShape(const PxTransform& transform,
                                  const GyroscopicConfig& config = GyroscopicConfig());

    /**
     * @brief Create hammer-shaped actor
     * @param transform Initial transform
     * @param config Gyroscopic configuration
     * @return Created actor (or nullptr on failure)
     */
    PxRigidDynamic* createHammer(const PxTransform& transform,
                                  const GyroscopicConfig& config = GyroscopicConfig());

    /**
     * @brief Create dumbbell-shaped actor
     * @param transform Initial transform
     * @param config Gyroscopic configuration
     * @return Created actor (or nullptr on failure)
     */
    PxRigidDynamic* createDumbbell(const PxTransform& transform,
                                    const GyroscopicConfig& config = GyroscopicConfig());

    /**
     * @brief Create cross-shaped actor
     * @param transform Initial transform
     * @param config Gyroscopic configuration
     * @return Created actor (or nullptr on failure)
     */
    PxRigidDynamic* createCross(const PxTransform& transform,
                                const GyroscopicConfig& config = GyroscopicConfig());

    /**
     * @brief Create tennis racket-shaped actor
     * @param transform Initial transform
     * @param config Gyroscopic configuration
     * @return Created actor (or nullptr on failure)
     */
    PxRigidDynamic* createTennisRacket(const PxTransform& transform,
                                       const GyroscopicConfig& config = GyroscopicConfig());

    /**
     * @brief Create demo shape actor
     * @param shape Shape type to create
     * @param transform Initial transform
     * @param config Gyroscopic configuration
     * @return Created actor (or nullptr on failure)
     */
    PxRigidDynamic* createDemoShape(DemoShape shape,
                                     const PxTransform& transform,
                                     const GyroscopicConfig& config = GyroscopicConfig());

    // ========================================================================
    // Gyroscopic Control
    // ========================================================================

    /**
     * @brief Enable or disable gyroscopic forces for an actor
     * @param actor Actor to modify
     * @param enable Whether to enable gyroscopic forces
     */
    static void setGyroscopicEnabled(PxRigidDynamic* actor, bool enable);

    /**
     * @brief Check if gyroscopic forces are enabled for an actor
     * @param actor Actor to check
     * @return true if gyroscopic forces are enabled
     */
    static bool isGyroscopicEnabled(PxRigidDynamic* actor);

    /**
     * @brief Set angular velocity for an actor
     * @param actor Actor to modify
     * @param angularVelocity Angular velocity vector
     */
    static void setAngularVelocity(PxRigidDynamic* actor, const PxVec3& angularVelocity);

    /**
     * @brief Set angular damping for an actor
     * @param actor Actor to modify
     * @param damping Angular damping value (0 = no damping)
     */
    static void setAngularDamping(PxRigidDynamic* actor, PxReal damping);

    // ========================================================================
    // Analysis and Statistics
    // ========================================================================

    /**
     * @brief Get statistics for a gyroscopic actor
     * @param actor Actor to analyze
     * @return Statistics structure
     */
    static GyroscopicStats getStats(PxRigidDynamic* actor);

    /**
     * @brief Compute moment of inertia for an actor
     * @param actor Actor to analyze
     * @return Diagonal moment of inertia (principal axes)
     */
    static PxVec3 computeMomentOfInertia(PxRigidDynamic* actor);

    /**
     * @brief Compute rotational kinetic energy
     * @param actor Actor to analyze
     * @return Kinetic energy value
     */
    static PxReal computeKineticEnergy(PxRigidDynamic* actor);

    /**
     * @brief Determine if rotation is around intermediate axis
     * @param actor Actor to analyze
     * @return true if rotating around intermediate axis (unstable)
     */
    static bool isIntermediateAxisRotation(PxRigidDynamic* actor);

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /**
     * @brief Create comparison pair (with and without gyroscopic)
     * @param shape Shape type to create
     * @param position1 Position for first actor (with gyroscopic)
     * @param position2 Position for second actor (without gyroscopic)
     * @param config Configuration
     * @return Pair of actors for comparison
     */
    std::pair<PxRigidDynamic*, PxRigidDynamic*> createComparisonPair(
        DemoShape shape,
        const PxVec3& position1,
        const PxVec3& position2,
        const GyroscopicConfig& config = GyroscopicConfig());

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

    // Helper methods
    PxRigidDynamic* createActor(const PxTransform& transform, const GyroscopicConfig& config);
    void configureActor(PxRigidDynamic* actor, const GyroscopicConfig& config);
};

} // namespace PhysXWrapper
