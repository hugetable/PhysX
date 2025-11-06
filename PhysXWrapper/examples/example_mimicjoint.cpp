/**
 * @file example_mimicjoint.cpp
 * @brief Mimic Joint Example (Follower Joint)
 *
 * This example demonstrates custom mimic joint implementation:
 * - One joint follows another joint's motion
 * - Configurable multiplier and offset
 * - Relationship: follower = leader × multiplier + offset
 * - Applications: robotic grippers, symmetric mechanisms, coupled motions
 * - Custom constraint implementation using PxConstraintConnector
 *
 * Based on PhysX Snippet: SnippetMimicJoint
 *
 * Mimic Joint Mechanics:
 * - Leader joint moves freely
 * - Follower joint automatically tracks leader
 * - Multiplier: Scale factor (1.0 = same angle, -1.0 = opposite)
 * - Offset: Constant angular/linear offset
 * - Equation: θ_follower = θ_leader × m + offset
 *
 * Applications:
 * - Robot gripper fingers (open/close together)
 * - Symmetric robotic arms
 * - Coupled door/window mechanisms
 * - Pantograph linkages
 * - Parallel manipulators
 *
 * Implementation:
 * Uses custom PxConstraintConnector to create coupling constraint
 * between two revolute joints.
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

using namespace PhysXWrapper;

// ============================================================================
// Mimic Joint Data Structures
// ============================================================================

/**
 * @brief Data for mimic joint constraint
 */
struct MimicJointData {
    PxTransform c2b[2];       // Constraint frames (not used for mimic, but required)
    PxReal multiplier;        // Scale factor
    PxReal offset;            // Angular offset (radians)
    PxReal stiffness;         // Constraint stiffness
    PxReal damping;           // Damping coefficient

    MimicJointData()
        : multiplier(1.0f)
        , offset(0.0f)
        , stiffness(1000.0f)
        , damping(50.0f)
    {
        c2b[0] = PxTransform(PxIdentity);
        c2b[1] = PxTransform(PxIdentity);
    }
};

// Forward declaration
class MimicJoint;

/**
 * @brief Constraint connector for mimic joint
 */
class MimicJointConnector : public PxConstraintConnector {
private:
    MimicJointData* mData;
    MimicJoint* mJoint;
    PxRevoluteJoint* mLeaderJoint;
    PxRevoluteJoint* mFollowerJoint;

public:
    MimicJointConnector(MimicJointData* data, MimicJoint* joint,
                        PxRevoluteJoint* leader, PxRevoluteJoint* follower)
        : mData(data)
        , mJoint(joint)
        , mLeaderJoint(leader)
        , mFollowerJoint(follower)
    {}

    // PxConstraintConnector interface
    virtual void* prepareData() override {
        return mData;
    }

    virtual bool updatePvdProperties(physx::pvdsdk::PvdDataStream& pvdConnection,
                                    const PxConstraint* c,
                                    PxPvdUpdateType::Enum updateType) const override {
        return true;
    }

    virtual void onConstraintRelease() override {}
    virtual void onComShift(PxU32 actor) override {}
    virtual void onOriginShift(const PxVec3& shift) override {}

    virtual void* getExternalReference(PxU32& typeID) override {
        typeID = PxConstraintExtIDs::eJOINT;
        return mJoint;
    }

    virtual PxBase* getSerializable() override {
        return nullptr;
    }

    virtual PxConstraintSolverPrep getPrep() const override {
        return MimicJointSolverPrep;
    }

    virtual const void* getConstantBlock() const override {
        return mData;
    }

    /**
     * @brief Constraint solver preparation for mimic joint
     *
     * Sets up angular constraint: θ_follower - (θ_leader * mult + offset) = 0
     */
    static PxU32 MimicJointSolverPrep(Px1DConstraint* constraints,
                                      PxVec3p& body0WorldOffset,
                                      PxU32 maxConstraints,
                                      PxConstraintInvMassScale& invMassScale,
                                      const void* constantBlock,
                                      const PxTransform& bA2w,
                                      const PxTransform& bB2w,
                                      bool /*useExtendedLimits*/,
                                      PxVec3p& cA2wOut,
                                      PxVec3p& cB2wOut)
    {
        const MimicJointData* data = static_cast<const MimicJointData*>(constantBlock);

        // We need to get angles from the joints
        // This is tricky because we're in the solver prep callback
        // The constraint enforces angular coupling between the two bodies

        // Setup angular constraint
        // The follower's angular position should match leader * mult + offset
        Px1DConstraint& c = constraints[0];

        // Angular constraint around the rotation axis (Z-axis for our revolute joints)
        PxVec3 axis(0, 0, 1);

        // Transform axis to world space
        PxVec3 worldAxis0 = bA2w.rotate(axis);
        PxVec3 worldAxis1 = bB2w.rotate(axis);

        // Linear components (none for angular constraint)
        c.linear0 = PxVec3(0);
        c.linear1 = PxVec3(0);

        // Angular components
        // For mimic: we want angular_follower = angular_leader * multiplier
        // Constraint: angular_follower - angular_leader * multiplier = 0
        c.angular0 = -worldAxis0 * data->multiplier;  // Leader contribution
        c.angular1 = worldAxis1;                      // Follower contribution

        // Geometric error (offset)
        c.geometricError = data->offset;

        // Velocity target (zero - we want positions to match)
        c.velocityTarget = 0.0f;

        // No limits (bilateral constraint)
        c.minImpulse = -PX_MAX_F32;
        c.maxImpulse = PX_MAX_F32;

        // Spring and damping
        if (data->stiffness > 0.0f) {
            c.mods.spring.stiffness = data->stiffness;
            c.mods.spring.damping = data->damping;
            c.flags |= Px1DConstraintFlag::eSPRING;
        }

        // Output attachment points (midpoint between bodies)
        cA2wOut = bA2w.p;
        cB2wOut = bB2w.p;

        // Inverse mass scale
        invMassScale.linear0 = 1.0f;
        invMassScale.linear1 = 1.0f;
        invMassScale.angular0 = 1.0f;
        invMassScale.angular1 = 1.0f;

        return 1;  // One constraint row
    }
};

// ============================================================================
// Mimic Joint Class
// ============================================================================

/**
 * @brief High-level mimic joint interface
 */
class MimicJoint {
private:
    PxPhysics* mPhysics;
    PxConstraint* mConstraint;
    MimicJointData mData;
    MimicJointConnector mConnector;
    PxRevoluteJoint* mLeaderJoint;
    PxRevoluteJoint* mFollowerJoint;
    PxRigidActor* mLeaderActor;
    PxRigidActor* mFollowerActor;

public:
    MimicJoint(PxPhysics* physics,
               PxRevoluteJoint* leaderJoint,
               PxRevoluteJoint* followerJoint,
               PxRigidActor* leaderActor,
               PxRigidActor* followerActor,
               PxReal multiplier,
               PxReal offset = 0.0f)
        : mPhysics(physics)
        , mConstraint(nullptr)
        , mConnector(&mData, this, leaderJoint, followerJoint)
        , mLeaderJoint(leaderJoint)
        , mFollowerJoint(followerJoint)
        , mLeaderActor(leaderActor)
        , mFollowerActor(followerActor)
    {
        mData.multiplier = multiplier;
        mData.offset = offset;

        // Create constraint linking the two actors
        mConstraint = physics->createConstraint(leaderActor, followerActor,
                                                 mConnector, mData);

        if (mConstraint) {
            mConstraint->setFlags(PxConstraintFlag::eVISUALIZATION);
            std::cout << "MimicJoint created: multiplier=" << multiplier
                      << ", offset=" << (offset * 180.0f / PxPi) << "°" << std::endl;
        }
    }

    ~MimicJoint() {
        if (mConstraint) {
            mConstraint->release();
        }
    }

    void setMultiplier(PxReal mult) {
        mData.multiplier = mult;
    }

    void setOffset(PxReal off) {
        mData.offset = off;
    }

    void setStiffness(PxReal stiff) {
        mData.stiffness = stiff;
    }

    void setDamping(PxReal damp) {
        mData.damping = damp;
    }

    PxReal getLeaderAngle() const {
        return mLeaderJoint ? mLeaderJoint->getAngle() : 0.0f;
    }

    PxReal getFollowerAngle() const {
        return mFollowerJoint ? mFollowerJoint->getAngle() : 0.0f;
    }

    PxReal getExpectedFollowerAngle() const {
        return getLeaderAngle() * mData.multiplier + mData.offset;
    }

    PxReal getError() const {
        return std::abs(getFollowerAngle() - getExpectedFollowerAngle());
    }

    void printStatus() const {
        PxReal leaderAngle = getLeaderAngle();
        PxReal followerAngle = getFollowerAngle();
        PxReal expected = getExpectedFollowerAngle();
        PxReal error = getError();

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "  Leader: " << (leaderAngle * 180.0f / PxPi) << "°  ";
        std::cout << "Follower: " << (followerAngle * 180.0f / PxPi) << "°  ";
        std::cout << "Expected: " << (expected * 180.0f / PxPi) << "°  ";
        std::cout << "Error: " << (error * 180.0f / PxPi) << "°";

        if (error < 0.1f) {
            std::cout << " ✓";
        }
        std::cout << std::endl;
    }
};

// ============================================================================
// Mimic Joint System
// ============================================================================

/**
 * @brief Helper class for creating mimic joint systems
 */
class MimicJointSystem {
private:
    PhysXCore& core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    struct MimicPair {
        PxRigidDynamic* leaderBody;
        PxRigidDynamic* followerBody;
        PxRevoluteJoint* leaderJoint;
        PxRevoluteJoint* followerJoint;
        MimicJoint* mimicJoint;
        std::string description;
    };

    std::vector<MimicPair> pairs;

public:
    MimicJointSystem(PhysXCore& coreRef, PxPhysics* phys, PxScene* scn, PxMaterial* mat)
        : core(coreRef), physics(phys), scene(scn), material(mat)
    {}

    ~MimicJointSystem() {
        for (auto& pair : pairs) {
            delete pair.mimicJoint;
        }
    }

    /**
     * Create a simple mimic pair
     */
    void createMimicPair(const PxVec3& basePos, PxReal multiplier, PxReal offset,
                         const std::string& description) {
        std::cout << "\nCreating: " << description << std::endl;

        // Create base
        PxRigidStatic* base = physics->createRigidStatic(PxTransform(basePos));
        PxBoxGeometry baseGeom(0.3f, 0.3f, 0.3f);
        PxRigidActorExt::createExclusiveShape(*base, baseGeom, *material);
        scene->addActor(*base);

        // Create leader arm
        PxVec3 leaderPos = basePos + PxVec3(-1.5f, 0, 0);
        PxBoxGeometry armGeom(1.0f, 0.2f, 0.2f);
        PxRigidDynamic* leaderArm = PxCreateDynamic(*physics, PxTransform(leaderPos),
                                                     armGeom, *material, 10.0f);
        scene->addActor(*leaderArm);

        // Create leader revolute joint
        PxVec3 hingeAxis(0, 0, 1);
        PxTransform leaderFrame0(PxVec3(-1.0f, 0, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxTransform leaderFrame1(PxVec3(1.0f, 0, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxRevoluteJoint* leaderJoint = PxRevoluteJointCreate(*physics, base, leaderFrame0,
                                                              leaderArm, leaderFrame1);
        leaderJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Create follower arm
        PxVec3 followerPos = basePos + PxVec3(1.5f, 0, 0);
        PxRigidDynamic* followerArm = PxCreateDynamic(*physics, PxTransform(followerPos),
                                                       armGeom, *material, 10.0f);
        scene->addActor(*followerArm);

        // Create follower revolute joint
        PxTransform followerFrame0(PxVec3(1.0f, 0, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxTransform followerFrame1(PxVec3(-1.0f, 0, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxRevoluteJoint* followerJoint = PxRevoluteJointCreate(*physics, base, followerFrame0,
                                                                followerArm, followerFrame1);
        followerJoint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Create mimic joint
        MimicJoint* mimic = new MimicJoint(physics, leaderJoint, followerJoint,
                                           leaderArm, followerArm,
                                           multiplier, offset);
        mimic->setStiffness(500.0f);
        mimic->setDamping(50.0f);

        // Apply torque to leader to start motion
        leaderArm->addTorque(PxVec3(0, 0, 20.0f));

        // Store
        MimicPair pair;
        pair.leaderBody = leaderArm;
        pair.followerBody = followerArm;
        pair.leaderJoint = leaderJoint;
        pair.followerJoint = followerJoint;
        pair.mimicJoint = mimic;
        pair.description = description;
        pairs.push_back(pair);
    }

    /**
     * Create a robotic gripper (two fingers mimic each other)
     */
    void createGripper(const PxVec3& basePos) {
        std::cout << "\nCreating robotic gripper..." << std::endl;

        // Create gripper base
        PxRigidStatic* base = physics->createRigidStatic(PxTransform(basePos));
        PxBoxGeometry baseGeom(0.5f, 0.3f, 0.3f);
        PxRigidActorExt::createExclusiveShape(*base, baseGeom, *material);
        scene->addActor(*base);

        // Create finger 1
        PxVec3 finger1Pos = basePos + PxVec3(-0.5f, -0.8f, 0);
        PxBoxGeometry fingerGeom(0.15f, 0.6f, 0.15f);
        PxRigidDynamic* finger1 = PxCreateDynamic(*physics, PxTransform(finger1Pos),
                                                   fingerGeom, *material, 5.0f);
        scene->addActor(*finger1);

        // Finger 1 joint (rotates to close)
        PxTransform f1Frame0(PxVec3(-0.5f, -0.3f, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxTransform f1Frame1(PxVec3(0, 0.6f, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxRevoluteJoint* finger1Joint = PxRevoluteJointCreate(*physics, base, f1Frame0,
                                                               finger1, f1Frame1);
        finger1Joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Set limits (0 to 60 degrees)
        finger1Joint->setLimit(PxJointAngularLimitPair(0.0f, PxPi / 3.0f, 0.1f));
        finger1Joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);

        // Create finger 2 (mirror)
        PxVec3 finger2Pos = basePos + PxVec3(0.5f, -0.8f, 0);
        PxRigidDynamic* finger2 = PxCreateDynamic(*physics, PxTransform(finger2Pos),
                                                   fingerGeom, *material, 5.0f);
        scene->addActor(*finger2);

        // Finger 2 joint
        PxTransform f2Frame0(PxVec3(0.5f, -0.3f, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxTransform f2Frame1(PxVec3(0, 0.6f, 0), PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
        PxRevoluteJoint* finger2Joint = PxRevoluteJointCreate(*physics, base, f2Frame0,
                                                               finger2, f2Frame1);
        finger2Joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        // Set limits
        finger2Joint->setLimit(PxJointAngularLimitPair(-PxPi / 3.0f, 0.0f, 0.1f));
        finger2Joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);

        // Create mimic joint (finger 2 mirrors finger 1)
        MimicJoint* mimic = new MimicJoint(physics, finger1Joint, finger2Joint,
                                           finger1, finger2,
                                           -1.0f, 0.0f);  // Opposite direction
        mimic->setStiffness(1000.0f);
        mimic->setDamping(100.0f);

        // Apply torque to finger 1 to close gripper
        finger1->addTorque(PxVec3(0, 0, 15.0f));

        std::cout << "  Gripper created: 2 fingers with mimic constraint" << std::endl;
        std::cout << "  Multiplier: -1.0 (mirror motion)" << std::endl;

        // Store
        MimicPair pair;
        pair.leaderBody = finger1;
        pair.followerBody = finger2;
        pair.leaderJoint = finger1Joint;
        pair.followerJoint = finger2Joint;
        pair.mimicJoint = mimic;
        pair.description = "Robotic gripper";
        pairs.push_back(pair);
    }

    void printStatus() {
        std::cout << "\n=== Mimic Joint Status ===" << std::endl;
        for (const auto& pair : pairs) {
            std::cout << "\n" << pair.description << ":";
            std::cout << std::endl;
            pair.mimicJoint->printStatus();
        }
    }

    size_t getPairCount() const { return pairs.size(); }
};

// ============================================================================
// Main Example
// ============================================================================

class MimicJointExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;
    MimicJointSystem* system;

public:
    MimicJointExample() : physics(nullptr), scene(nullptr), material(nullptr), system(nullptr) {}

    ~MimicJointExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "===================================================" << std::endl;
        std::cout << "PhysX Mimic Joint Example" << std::endl;
        std::cout << "===================================================" << std::endl;

        PhysXCore::Config config;
        config.gravity = PxVec3(0.0f, -9.81f, 0.0f);
        config.numThreads = 2;

        if (!core.initialize(config)) {
            std::cerr << "Failed to initialize PhysX" << std::endl;
            return false;
        }

        physics = core.getPhysics();
        scene = core.getScene();
        material = physics->createMaterial(0.5f, 0.5f, 0.3f);

        scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);

        system = new MimicJointSystem(core, physics, scene, material);

        std::cout << "\nMimic Joint Mechanics:" << std::endl;
        std::cout << "  • Follower tracks leader motion" << std::endl;
        std::cout << "  • follower = leader × multiplier + offset" << std::endl;
        std::cout << "  • multiplier = 1.0: same direction" << std::endl;
        std::cout << "  • multiplier = -1.0: opposite direction (mirror)" << std::endl;
        std::cout << "  • offset: constant angular difference" << std::endl;

        return true;
    }

    void createScenarios() {
        std::cout << "\n=== Creating Mimic Scenarios ===" << std::endl;

        // Scenario 1: 1:1 same direction
        system->createMimicPair(PxVec3(-10, 5, 0), 1.0f, 0.0f,
                                "1:1 same direction");

        // Scenario 2: 1:1 opposite (mirror)
        system->createMimicPair(PxVec3(-5, 5, 0), -1.0f, 0.0f,
                                "1:1 mirror");

        // Scenario 3: 2:1 (follower moves twice as fast)
        system->createMimicPair(PxVec3(0, 5, 0), 2.0f, 0.0f,
                                "2:1 amplified");

        // Scenario 4: With offset
        system->createMimicPair(PxVec3(5, 5, 0), 1.0f, PxPi / 4.0f,
                                "1:1 with 45° offset");

        // Scenario 5: Gripper
        system->createGripper(PxVec3(10, 5, 0));

        std::cout << "\nTotal pairs created: " << system->getPairCount() << std::endl;
    }

    void simulate(PxReal dt) {
        scene->simulate(dt);
        scene->fetchResults(true);
    }

    void run() {
        createScenarios();

        std::cout << "\n=== Starting Simulation ===" << std::endl;

        const PxReal dt = 1.0f / 60.0f;
        const int totalFrames = 600;

        for (int frame = 0; frame < totalFrames; frame++) {
            simulate(dt);

            if (frame % 120 == 0) {
                std::cout << "\n=== Frame " << frame << " (t=" << (frame * dt) << "s) ===";
                system->printStatus();
            }
        }

        std::cout << "\n\n=== Final Status ===";
        system->printStatus();

        std::cout << "\n\n=== Simulation Complete ===" << std::endl;
    }

    void cleanup() {
        if (system) delete system;
        if (material) material->release();
        core.cleanup();
    }
};

int main() {
    MimicJointExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
