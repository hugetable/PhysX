/**
 * @file RigidBodyMassCalculator.h
 * @brief Simplified mass properties calculation
 *
 * This class provides utilities for calculating and setting mass properties
 * for rigid bodies. Based on SnippetMassProperties from PhysX SDK.
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Mass properties data
 */
struct MassProperties {
    PxReal mass = 0.0f;           ///< Total mass
    PxVec3 centerOfMass = PxVec3(0);  ///< Center of mass
    PxMat33 inertiaTensor = PxMat33(PxIdentity);  ///< Inertia tensor
    PxVec3 principalInertia = PxVec3(0);  ///< Principal moments of inertia
};

/**
 * @brief Rigid body mass calculator class
 *
 * This class provides utilities for mass calculations:
 * - Calculate mass from density
 * - Calculate mass from shapes
 * - Set custom center of mass
 * - Calculate inertia tensors
 * - Combine mass properties from multiple shapes
 *
 * @example
 * @code
 * // Calculate mass properties
 * RigidBodyMassCalculator calc;
 *
 * // Set mass from density
 * calc.setMassFromDensity(actor, 1000.0f);  // 1000 kg/m^3
 *
 * // Or set specific mass
 * calc.setMass(actor, 10.0f);
 *
 * // Adjust center of mass
 * calc.setCenterOfMass(actor, PxVec3(0, -0.5f, 0));  // Lower COM
 *
 * // Get mass properties
 * MassProperties props = calc.getMassProperties(actor);
 * std::cout << "Mass: " << props.mass << std::endl;
 * @endcode
 */
class RigidBodyMassCalculator {
public:
    /**
     * @brief Constructor
     */
    RigidBodyMassCalculator();

    /**
     * @brief Destructor
     */
    ~RigidBodyMassCalculator();

    /**
     * @brief Set mass and inertia from density
     * @param actor Dynamic actor to set mass for
     * @param density Material density (kg/m^3)
     * @return True if successful
     */
    bool setMassFromDensity(PxRigidDynamic* actor, PxReal density);

    /**
     * @brief Set uniform mass
     * @param actor Dynamic actor to set mass for
     * @param mass Total mass
     * @return True if successful
     */
    bool setMass(PxRigidDynamic* actor, PxReal mass);

    /**
     * @brief Set mass and inertia from shape densities
     * @param actor Dynamic actor
     * @param shapeDensities Per-shape densities
     * @param massLocalPose Optional mass frame adjustment
     * @return True if successful
     */
    bool setMassFromShapeDensities(
        PxRigidDynamic* actor,
        const std::vector<PxReal>& shapeDensities,
        const PxTransform* massLocalPose = nullptr
    );

    /**
     * @brief Set center of mass
     * @param actor Dynamic actor
     * @param centerOfMass Center of mass in local space
     * @return True if successful
     */
    bool setCenterOfMass(PxRigidDynamic* actor, const PxVec3& centerOfMass);

    /**
     * @brief Get mass properties
     * @param actor Dynamic actor
     * @return Mass properties
     */
    MassProperties getMassProperties(PxRigidDynamic* actor) const;

    /**
     * @brief Calculate mass properties for a geometry
     * @param geometry Geometry to calculate for
     * @param density Material density
     * @return Mass properties
     */
    static MassProperties calculateMassProperties(
        const PxGeometry& geometry,
        PxReal density = 1000.0f
    );

    /**
     * @brief Calculate volume of a geometry
     * @param geometry Geometry to calculate volume for
     * @return Volume in m^3
     */
    static PxReal calculateVolume(const PxGeometry& geometry);

    /**
     * @brief Calculate moment of inertia for common shapes
     * @param geometry Geometry
     * @param mass Mass of the object
     * @return Diagonal inertia tensor (principal moments)
     */
    static PxVec3 calculateInertia(
        const PxGeometry& geometry,
        PxReal mass
    );

    /**
     * @brief Scale inertia tensor
     * @param actor Dynamic actor
     * @param scale Scale factor
     * @return True if successful
     */
    bool scaleInertia(PxRigidDynamic* actor, PxReal scale);

    /**
     * @brief Reset mass properties to defaults
     * @param actor Dynamic actor
     * @param density Density to use
     * @return True if successful
     */
    bool resetMassProperties(PxRigidDynamic* actor, PxReal density = 1000.0f);
};

} // namespace PhysXWrapper
