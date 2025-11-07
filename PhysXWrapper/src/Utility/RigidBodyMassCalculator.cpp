/**
 * @file RigidBodyMassCalculator.cpp
 * @brief Implementation of RigidBodyMassCalculator class
 */

#include "Utility/RigidBodyMassCalculator.h"
#include <cmath>

namespace PhysXWrapper {

RigidBodyMassCalculator::RigidBodyMassCalculator() {
}

RigidBodyMassCalculator::~RigidBodyMassCalculator() = default;

bool RigidBodyMassCalculator::setMassFromDensity(PxRigidDynamic* actor, PxReal density) {
    if (!actor || density <= 0.0f) return false;

    return PxRigidBodyExt::updateMassAndInertia(*actor, density);
}

bool RigidBodyMassCalculator::setMass(PxRigidDynamic* actor, PxReal mass) {
    if (!actor || mass <= 0.0f) return false;

    actor->setMass(mass);
    return true;
}

bool RigidBodyMassCalculator::setMassFromShapeDensities(
    PxRigidDynamic* actor,
    const std::vector<PxReal>& shapeDensities,
    const PxTransform* massLocalPose)
{
    if (!actor) return false;

    PxU32 numShapes = actor->getNbShapes();
    if (numShapes == 0 || shapeDensities.size() != numShapes) return false;

    // PhysX 5.x: updateMassAndInertia uses PxVec3* instead of PxTransform*
    const PxVec3* massPos = massLocalPose ? &massLocalPose->p : nullptr;
    return PxRigidBodyExt::updateMassAndInertia(*actor, shapeDensities.data(), numShapes, massPos);
}

bool RigidBodyMassCalculator::setCenterOfMass(PxRigidDynamic* actor, const PxVec3& centerOfMass) {
    if (!actor) return false;

    actor->setCMassLocalPose(PxTransform(centerOfMass));
    return true;
}

MassProperties RigidBodyMassCalculator::getMassProperties(PxRigidDynamic* actor) const {
    MassProperties props;

    if (!actor) return props;

    props.mass = actor->getMass();
    props.centerOfMass = actor->getCMassLocalPose().p;
    // PhysX 5.x: PxMat33 constructor from PxVec3 creates diagonal matrix
    PxVec3 inertia = actor->getMassSpaceInertiaTensor();
    props.inertiaTensor = PxMat33::createDiagonal(inertia);
    props.principalInertia = inertia;

    return props;
}

MassProperties RigidBodyMassCalculator::calculateMassProperties(
    const PxGeometry& geometry,
    PxReal density)
{
    MassProperties props;

    PxReal volume = calculateVolume(geometry);
    props.mass = volume * density;

    if (props.mass > 0.0f) {
        props.principalInertia = calculateInertia(geometry, props.mass);
    }

    return props;
}

PxReal RigidBodyMassCalculator::calculateVolume(const PxGeometry& geometry) {
    switch (geometry.getType()) {
        case PxGeometryType::eSPHERE: {
            const PxSphereGeometry& sphere = static_cast<const PxSphereGeometry&>(geometry);
            return (4.0f / 3.0f) * PxPi * sphere.radius * sphere.radius * sphere.radius;
        }
        case PxGeometryType::eBOX: {
            const PxBoxGeometry& box = static_cast<const PxBoxGeometry&>(geometry);
            return 8.0f * box.halfExtents.x * box.halfExtents.y * box.halfExtents.z;
        }
        case PxGeometryType::eCAPSULE: {
            const PxCapsuleGeometry& capsule = static_cast<const PxCapsuleGeometry&>(geometry);
            PxReal r = capsule.radius;
            PxReal h = capsule.halfHeight * 2.0f;
            return PxPi * r * r * (h + (4.0f / 3.0f) * r);
        }
        default:
            return 0.0f;
    }
}

PxVec3 RigidBodyMassCalculator::calculateInertia(
    const PxGeometry& geometry,
    PxReal mass)
{
    switch (geometry.getType()) {
        case PxGeometryType::eSPHERE: {
            const PxSphereGeometry& sphere = static_cast<const PxSphereGeometry&>(geometry);
            PxReal I = (2.0f / 5.0f) * mass * sphere.radius * sphere.radius;
            return PxVec3(I, I, I);
        }
        case PxGeometryType::eBOX: {
            const PxBoxGeometry& box = static_cast<const PxBoxGeometry&>(geometry);
            PxReal x = box.halfExtents.x * 2.0f;
            PxReal y = box.halfExtents.y * 2.0f;
            PxReal z = box.halfExtents.z * 2.0f;
            return PxVec3(
                (mass / 12.0f) * (y * y + z * z),
                (mass / 12.0f) * (x * x + z * z),
                (mass / 12.0f) * (x * x + y * y)
            );
        }
        case PxGeometryType::eCAPSULE: {
            const PxCapsuleGeometry& capsule = static_cast<const PxCapsuleGeometry&>(geometry);
            PxReal r = capsule.radius;
            PxReal h = capsule.halfHeight;

            PxReal Ix = mass * ((3.0f / 80.0f) * (4.0f * h + 3.0f * r) * r + (h * h) / 12.0f);
            PxReal Iy = (mass * r * r) / 2.0f;

            return PxVec3(Ix, Iy, Ix);
        }
        default:
            return PxVec3(1.0f, 1.0f, 1.0f);
    }
}

bool RigidBodyMassCalculator::scaleInertia(PxRigidDynamic* actor, PxReal scale) {
    if (!actor || scale <= 0.0f) return false;

    PxVec3 inertia = actor->getMassSpaceInertiaTensor();
    actor->setMassSpaceInertiaTensor(inertia * scale);

    return true;
}

bool RigidBodyMassCalculator::resetMassProperties(PxRigidDynamic* actor, PxReal density) {
    if (!actor) return false;

    return setMassFromDensity(actor, density);
}

} // namespace PhysXWrapper
