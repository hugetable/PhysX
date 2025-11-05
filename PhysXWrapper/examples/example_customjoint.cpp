/**
 * @file example_customjoint.cpp
 * @brief Custom Joint Implementation Example
 *
 * This example demonstrates how to create custom joint types in PhysX:
 * - Implementing PxConstraintConnector interface
 * - Custom constraint solver integration
 * - Force and torque computation
 * - Constraint visualization
 * - Breakable constraints with custom logic
 *
 * Based on PhysX Snippet: SnippetCustomJoint
 *
 * Custom Joint Implemented: RopeJoint
 * - Constrains maximum distance between two actors
 * - No minimum distance constraint (unlike distance joint)
 * - Simulates a rope or cable connection
 * - Can be slack (no force) or taut (pulling force)
 *
 * Architecture:
 * 1. RopeJoint - High-level joint interface
 * 2. RopeJointData - Constraint parameters
 * 3. RopeJointConnector - PxConstraintConnector implementation
 * 4. Constraint solver callback - Force computation
 *
 * Key Concepts:
 * - PxConstraint: Low-level constraint object managed by PhysX
 * - PxConstraintConnector: User callback interface for custom behavior
 * - Constraint rows: Linear/angular constraint equations
 * - Constraint solver: Iterative solver for constraint forces
 * - Projection: Error correction for constraint drift
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace PhysXWrapper;

// ============================================================================
// Custom Joint Data Structures
// ============================================================================

/**
 * @brief Data for rope joint constraint
 */
struct RopeJointData {
    PxTransform c2b[2];       // Constraint frames relative to bodies
    PxReal maxDistance;       // Maximum rope length
    PxReal stiffness;         // Rope stiffness (0 = rigid, higher = springy)
    PxReal damping;           // Damping coefficient
    bool broken;              // Is constraint broken?
    PxReal breakForce;        // Force threshold for breaking

    RopeJointData()
        : maxDistance(10.0f)
        , stiffness(100.0f)
        , damping(10.0f)
        , broken(false)
        , breakForce(PX_MAX_F32)
    {
        c2b[0] = PxTransform(PxIdentity);
        c2b[1] = PxTransform(PxIdentity);
    }
};

// Forward declaration
class RopeJoint;

/**
 * @brief Constraint connector for rope joint
 *
 * This class implements the PxConstraintConnector interface to provide
 * custom constraint behavior to PhysX's constraint solver.
 */
class RopeJointConnector : public PxConstraintConnector {
private:
    RopeJointData* mData;
    RopeJoint* mJoint;

public:
    RopeJointConnector(RopeJointData* data, RopeJoint* joint)
        : mData(data)
        , mJoint(joint)
    {}

    // PxConstraintConnector interface implementation

    virtual void* prepareData() override {
        return mData;
    }

    virtual bool updatePvdProperties(physx::pvdsdk::PvdDataStream& pvdConnection,
                                    const PxConstraint* c,
                                    PxPvdUpdateType::Enum updateType) const override {
        // PVD (PhysX Visual Debugger) integration
        // Return true if you want to send data to PVD
        return true;
    }

    virtual void onConstraintRelease() override {
        // Called when constraint is released
        // Clean up if needed
    }

    virtual void onComShift(PxU32 actor) override {
        // Called when center of mass shifts
        // Update constraint frames if needed
    }

    virtual void onOriginShift(const PxVec3& shift) override {
        // Called when world origin shifts
        // Update constraint positions if needed
    }

    virtual void* getExternalReference(PxU32& typeID) override {
        // Return pointer to joint object for serialization
        typeID = PxConstraintExtIDs::eJOINT;
        return mJoint;
    }

    virtual PxBase* getSerializable() override {
        // Return serializable object (nullptr if not serializable)
        return nullptr;
    }

    virtual PxConstraintSolverPrep getPrep() const override {
        // Return solver preparation callback
        return RopeJointSolverPrep;
    }

    virtual const void* getConstantBlock() const override {
        return mData;
    }

    /**
     * @brief Constraint solver preparation callback
     *
     * This function sets up constraint rows for the solver.
     * Called by PhysX before constraint solving.
     */
    static PxU32 RopeJointSolverPrep(Px1DConstraint* constraints,
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
        const RopeJointData* data = static_cast<const RopeJointData*>(constantBlock);

        if (data->broken) {
            return 0;  // No constraints if broken
        }

        // Get constraint frames in world space
        PxTransform cA2w = bA2w * data->c2b[0];
        PxTransform cB2w = bB2w * data->c2b[1];

        // Compute current distance
        PxVec3 direction = cB2w.p - cA2w.p;
        PxReal currentDistance = direction.magnitude();

        // Only apply constraint if distance exceeds maximum
        if (currentDistance <= data->maxDistance) {
            return 0;  // Rope is slack, no constraint needed
        }

        // Normalize direction
        direction = direction.getNormalized();

        // Setup constraint row
        // This is a linear constraint along the rope direction
        Px1DConstraint& c = constraints[0];

        // Constraint linear axis (direction of rope)
        c.linear0 = direction;
        c.linear1 = direction;

        // Angular components (none for this constraint)
        c.angular0 = PxVec3(0);
        c.angular1 = PxVec3(0);

        // Constraint error (how much we exceed max distance)
        PxReal error = currentDistance - data->maxDistance;
        c.geometricError = error;

        // Velocity target (for damping)
        c.velocityTarget = -data->damping * error;

        // Constraint limits
        c.minImpulse = 0.0f;  // Can only pull (not push)
        c.maxImpulse = PX_MAX_F32;

        // Compliance (inverse of stiffness)
        // Higher compliance = softer constraint
        if (data->stiffness > 0.0f) {
            c.mods.spring.stiffness = data->stiffness;
            c.mods.spring.damping = data->damping;
            c.flags |= Px1DConstraintFlag::eSPRING;
        }

        // Output constraint attachment points
        cA2wOut = cA2w.p;
        cB2wOut = cB2w.p;

        // Inverse mass scale (for adjusting mass ratios)
        invMassScale.linear0 = 1.0f;
        invMassScale.linear1 = 1.0f;
        invMassScale.angular0 = 1.0f;
        invMassScale.angular1 = 1.0f;

        return 1;  // One constraint row added
    }
};

// ============================================================================
// Rope Joint Class
// ============================================================================

/**
 * @brief High-level rope joint interface
 */
class RopeJoint {
private:
    PxPhysics* mPhysics;
    PxConstraint* mConstraint;
    RopeJointData mData;
    RopeJointConnector mConnector;
    PxRigidActor* mActor0;
    PxRigidActor* mActor1;

public:
    RopeJoint(PxPhysics* physics,
              PxRigidActor* actor0, const PxTransform& localFrame0,
              PxRigidActor* actor1, const PxTransform& localFrame1,
              PxReal maxDistance)
        : mPhysics(physics)
        , mConstraint(nullptr)
        , mConnector(&mData, this)
        , mActor0(actor0)
        , mActor1(actor1)
    {
        mData.c2b[0] = localFrame0;
        mData.c2b[1] = localFrame1;
        mData.maxDistance = maxDistance;

        // Create constraint
        mConstraint = physics->createConstraint(actor0, actor1, mConnector, mData);

        if (mConstraint) {
            // Enable visualization
            mConstraint->setFlags(PxConstraintFlag::eVISUALIZATION);

            // Enable breakage detection
            mConstraint->setBreakForce(mData.breakForce, mData.breakForce);

            std::cout << "RopeJoint created: maxDistance=" << maxDistance << std::endl;
        }
    }

    ~RopeJoint() {
        if (mConstraint) {
            mConstraint->release();
        }
    }

    void setMaxDistance(PxReal distance) {
        mData.maxDistance = distance;
    }

    PxReal getMaxDistance() const {
        return mData.maxDistance;
    }

    void setStiffness(PxReal stiffness) {
        mData.stiffness = stiffness;
    }

    void setDamping(PxReal damping) {
        mData.damping = damping;
    }

    void setBreakForce(PxReal force) {
        mData.breakForce = force;
        if (mConstraint) {
            mConstraint->setBreakForce(force, force);
        }
    }

    bool isBroken() const {
        return mData.broken || (mConstraint && mConstraint->getFlags() & PxConstraintFlag::eBROKEN);
    }

    void breakJoint() {
        mData.broken = true;
        std::cout << "RopeJoint broken!" << std::endl;
    }

    PxReal getCurrentDistance() const {
        if (!mActor0 || !mActor1) return 0.0f;

        PxTransform pose0 = mActor0->getGlobalPose();
        PxTransform pose1 = mActor1->getGlobalPose();

        PxVec3 worldPos0 = pose0.transform(mData.c2b[0].p);
        PxVec3 worldPos1 = pose1.transform(mData.c2b[1].p);

        return (worldPos1 - worldPos0).magnitude();
    }

    PxReal getCurrentForce() const {
        if (!mConstraint) return 0.0f;

        PxVec3 linearForce, angularForce;
        mConstraint->getForce(linearForce, angularForce);

        return linearForce.magnitude();
    }

    void printStatus() const {
        PxReal currentDist = getCurrentDistance();
        PxReal currentForce = getCurrentForce();
        bool taut = currentDist > mData.maxDistance;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Distance: " << currentDist << " / " << mData.maxDistance;
        std::cout << "  Force: " << currentForce << " N";
        std::cout << "  Status: " << (isBroken() ? "BROKEN" : (taut ? "TAUT" : "SLACK"));
        std::cout << std::endl;
    }
};

// ============================================================================
// Example Application
// ============================================================================

class CustomJointExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    std::vector<RopeJoint*> ropeJoints;
    std::vector<PxRigidDynamic*> actors;

public:
    CustomJointExample()
        : physics(nullptr)
        , scene(nullptr)
        , material(nullptr)
    {}

    ~CustomJointExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "===================================================" << std::endl;
        std::cout << "PhysX Custom Joint Example - Rope Joint" << std::endl;
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

        // Enable joint visualization
        scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
        scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);

        std::cout << "\nRope Joint Characteristics:" << std::endl;
        std::cout << "  • Maximum distance constraint (rope length)" << std::endl;
        std::cout << "  • Slack when distance < max (no force)" << std::endl;
        std::cout << "  • Taut when distance >= max (pulling force)" << std::endl;
        std::cout << "  • Can only pull, never push" << std::endl;
        std::cout << "  • Configurable stiffness and damping" << std::endl;

        return true;
    }

    void createRopeChain() {
        std::cout << "\n=== Creating Rope Chain ===" << std::endl;

        // Create anchor (static)
        PxRigidStatic* anchor = physics->createRigidStatic(PxTransform(PxVec3(0, 20, 0)));
        PxBoxGeometry anchorGeom(0.5f, 0.5f, 0.5f);
        PxRigidActorExt::createExclusiveShape(*anchor, anchorGeom, *material);
        scene->addActor(*anchor);

        // Create chain of boxes connected by rope joints
        PxRigidActor* prevActor = anchor;
        PxVec3 prevPos(0, 20, 0);

        for (int i = 0; i < 5; i++) {
            // Create box
            PxVec3 pos = prevPos + PxVec3(0, -3, 0);
            PxBoxGeometry boxGeom(0.5f, 0.5f, 0.5f);
            PxRigidDynamic* box = PxCreateDynamic(*physics, PxTransform(pos), boxGeom, *material, 1.0f);
            scene->addActor(*box);
            actors.push_back(box);

            // Create rope joint
            PxTransform localFrame0(PxVec3(0, -0.5f, 0));  // Bottom of previous
            PxTransform localFrame1(PxVec3(0, 0.5f, 0));   // Top of current

            RopeJoint* rope = new RopeJoint(physics, prevActor, localFrame0, box, localFrame1, 2.5f);
            rope->setStiffness(100.0f);
            rope->setDamping(10.0f);
            ropeJoints.push_back(rope);

            prevActor = box;
            prevPos = pos;
        }

        std::cout << "Created chain with " << actors.size() << " boxes and "
                  << ropeJoints.size() << " rope joints" << std::endl;
    }

    void createBreakableRope() {
        std::cout << "\n=== Creating Breakable Rope ===" << std::endl;

        // Create two heavy boxes
        PxVec3 pos1(5, 15, 0);
        PxVec3 pos2(5, 10, 0);

        PxBoxGeometry boxGeom(1.0f, 1.0f, 1.0f);
        PxRigidDynamic* box1 = PxCreateDynamic(*physics, PxTransform(pos1), boxGeom, *material, 10.0f);
        PxRigidDynamic* box2 = PxCreateDynamic(*physics, PxTransform(pos2), boxGeom, *material, 50.0f);  // Heavy!

        scene->addActor(*box1);
        scene->addActor(*box2);
        actors.push_back(box1);
        actors.push_back(box2);

        // Create breakable rope (will break under weight of heavy box)
        RopeJoint* rope = new RopeJoint(physics, box1, PxTransform(PxVec3(0, -1, 0)),
                                        box2, PxTransform(PxVec3(0, 1, 0)), 4.0f);
        rope->setBreakForce(300.0f);  // Break at 300N
        rope->setStiffness(200.0f);
        ropeJoints.push_back(rope);

        std::cout << "Created breakable rope (break force: 300N)" << std::endl;
        std::cout << "Heavy box will cause rope to break!" << std::endl;
    }

    void createElasticRope() {
        std::cout << "\n=== Creating Elastic Rope ===" << std::endl;

        // Create bouncing ball
        PxVec3 pos(-5, 15, 0);
        PxSphereGeometry sphereGeom(0.5f);
        PxRigidDynamic* ball = PxCreateDynamic(*physics, PxTransform(pos), sphereGeom, *material, 5.0f);

        // Give it initial velocity
        ball->setLinearVelocity(PxVec3(0, -10, 0));

        scene->addActor(*ball);
        actors.push_back(ball);

        // Create elastic rope (low stiffness = springy)
        PxRigidStatic* anchor = physics->createRigidStatic(PxTransform(PxVec3(-5, 20, 0)));
        scene->addActor(*anchor);

        RopeJoint* rope = new RopeJoint(physics, anchor, PxTransform(PxVec3(0, 0, 0)),
                                        ball, PxTransform(PxVec3(0, 0, 0)), 8.0f);
        rope->setStiffness(50.0f);   // Low stiffness = elastic
        rope->setDamping(5.0f);      // Low damping = bouncy
        ropeJoints.push_back(rope);

        std::cout << "Created elastic rope (low stiffness, bouncy behavior)" << std::endl;
    }

    void simulate(PxReal dt) {
        scene->simulate(dt);
        scene->fetchResults(true);
    }

    void printStatus() {
        std::cout << "\nRope Status:" << std::endl;
        for (size_t i = 0; i < ropeJoints.size(); i++) {
            std::cout << "Rope " << i << ": ";
            ropeJoints[i]->printStatus();
        }
    }

    void run() {
        std::cout << "\n=== Setting Up Scenarios ===" << std::endl;

        createRopeChain();
        createBreakableRope();
        createElasticRope();

        std::cout << "\n=== Starting Simulation ===" << std::endl;

        const PxReal dt = 1.0f / 60.0f;
        const int totalFrames = 600;  // 10 seconds

        for (int frame = 0; frame < totalFrames; frame++) {
            simulate(dt);

            // Print status every 60 frames (1 second)
            if (frame % 60 == 0) {
                std::cout << "\n=== Frame " << frame << " (t=" << (frame * dt) << "s) ===" << std::endl;
                printStatus();
            }
        }

        std::cout << "\n=== Simulation Complete ===" << std::endl;
        printStatus();

        std::cout << "\nKey Features Demonstrated:" << std::endl;
        std::cout << "  ✓ Custom constraint connector implementation" << std::endl;
        std::cout << "  ✓ Unilateral constraint (pull only, no push)" << std::endl;
        std::cout << "  ✓ Slack and taut states" << std::endl;
        std::cout << "  ✓ Spring/damping parameters" << std::endl;
        std::cout << "  ✓ Breakable constraints" << std::endl;
        std::cout << "  ✓ Constraint force monitoring" << std::endl;
    }

    void cleanup() {
        for (RopeJoint* rope : ropeJoints) {
            delete rope;
        }
        ropeJoints.clear();

        if (material) material->release();
        core.cleanup();
    }
};

int main() {
    CustomJointExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
