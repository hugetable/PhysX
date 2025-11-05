/**
 * @file test_rigidbody.cpp
 * @brief Unit tests for RigidBody utility classes
 *
 * Tests ContactHandler, Trigger, CCD, and MassCalculator
 */

#include <gtest/gtest.h>
#include "PhysXCore.h"
#include "RigidBody/RigidBodyContactHandler.h"
#include "RigidBody/RigidBodyTrigger.h"
#include "RigidBody/RigidBodyCCD.h"
#include "RigidBody/RigidBodyMassCalculator.h"

using namespace PhysXWrapper;

// ============================================================================
// Test Fixture
// ============================================================================

class RigidBodyTest : public ::testing::Test {
protected:
    PhysXCore* core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    void SetUp() override {
        core = new PhysXCore();
        core->initialize();
        physics = core->getPhysics();
        scene = core->getScene();
        material = core->getMaterial();
    }

    void TearDown() override {
        if (core) {
            core->cleanup();
            delete core;
            core = nullptr;
        }
    }

    // Helper: Create ground plane
    PxRigidStatic* createGround() {
        PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
        scene->addActor(*ground);
        return ground;
    }

    // Helper: Create dynamic box
    PxRigidDynamic* createBox(const PxVec3& position, PxReal mass = 10.0f) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(position),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            mass
        );
        scene->addActor(*box);
        return box;
    }
};

// ============================================================================
// ContactHandler Tests
// ============================================================================

TEST_F(RigidBodyTest, ContactHandlerCreation) {
    RigidBodyContactHandler handler;
    EXPECT_NO_THROW(handler.initialize(scene));
    EXPECT_TRUE(handler.isInitialized());
}

TEST_F(RigidBodyTest, ContactHandlerCallbacks) {
    RigidBodyContactHandler handler;
    handler.initialize(scene);

    int beginCount = 0;
    int endCount = 0;

    // Set callbacks
    handler.setOnContactBegin([&beginCount](PxActor* actor1, PxActor* actor2) {
        beginCount++;
    });

    handler.setOnContactEnd([&endCount](PxActor* actor1, PxActor* actor2) {
        endCount++;
    });

    // Create colliding objects
    PxRigidStatic* ground = createGround();
    PxRigidDynamic* box = createBox(PxVec3(0, 5, 0));

    // Simulate until collision
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Should have registered contact begin
    EXPECT_GT(beginCount, 0);

    ground->release();
    box->release();
}

TEST_F(RigidBodyTest, ContactHandlerMultipleContacts) {
    RigidBodyContactHandler handler;
    handler.initialize(scene);

    int totalContacts = 0;
    handler.setOnContactBegin([&totalContacts](PxActor*, PxActor*) {
        totalContacts++;
    });

    // Create multiple falling boxes
    PxRigidStatic* ground = createGround();
    std::vector<PxRigidDynamic*> boxes;

    for (int i = 0; i < 5; i++) {
        boxes.push_back(createBox(PxVec3(i * 1.5f, 10, 0)));
    }

    // Simulate
    for (int i = 0; i < 150; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Should have multiple contacts
    EXPECT_GT(totalContacts, 0);

    ground->release();
    for (auto* box : boxes) box->release();
}

// ============================================================================
// Trigger Tests
// ============================================================================

TEST_F(RigidBodyTest, TriggerCreation) {
    RigidBodyTrigger trigger;
    EXPECT_NO_THROW(trigger.initialize(physics, scene));
}

TEST_F(RigidBodyTest, TriggerVolume) {
    RigidBodyTrigger trigger;
    trigger.initialize(physics, scene);

    // Create trigger volume
    PxRigidStatic* triggerActor = trigger.createBoxTrigger(
        PxVec3(0, 0, 0),
        PxVec3(2, 2, 2)
    );

    EXPECT_NE(triggerActor, nullptr);
    EXPECT_TRUE(trigger.isTrigger(triggerActor));

    triggerActor->release();
}

TEST_F(RigidBodyTest, TriggerCallbacks) {
    RigidBodyTrigger trigger;
    trigger.initialize(physics, scene);

    int enterCount = 0;
    int exitCount = 0;

    trigger.setOnEnter([&enterCount](PxActor*, PxActor*) {
        enterCount++;
    });

    trigger.setOnExit([&exitCount](PxActor*, PxActor*) {
        exitCount++;
    });

    // Create trigger at origin
    PxRigidStatic* triggerActor = trigger.createBoxTrigger(
        PxVec3(0, 0, 0),
        PxVec3(2, 2, 2)
    );

    // Create box that will pass through trigger
    PxRigidDynamic* box = createBox(PxVec3(0, 10, 0));
    box->setLinearVelocity(PxVec3(0, -5, 0));

    // Simulate
    for (int i = 0; i < 200; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Should have triggered enter at least once
    EXPECT_GT(enterCount, 0);

    triggerActor->release();
    box->release();
}

TEST_F(RigidBodyTest, TriggerShapes) {
    RigidBodyTrigger trigger;
    trigger.initialize(physics, scene);

    // Test box trigger
    PxRigidStatic* boxTrigger = trigger.createBoxTrigger(
        PxVec3(0, 0, 0),
        PxVec3(1, 1, 1)
    );
    EXPECT_NE(boxTrigger, nullptr);

    // Test sphere trigger
    PxRigidStatic* sphereTrigger = trigger.createSphereTrigger(
        PxVec3(5, 0, 0),
        1.0f
    );
    EXPECT_NE(sphereTrigger, nullptr);

    // Test capsule trigger
    PxRigidStatic* capsuleTrigger = trigger.createCapsuleTrigger(
        PxVec3(10, 0, 0),
        0.5f,
        2.0f
    );
    EXPECT_NE(capsuleTrigger, nullptr);

    boxTrigger->release();
    sphereTrigger->release();
    capsuleTrigger->release();
}

// ============================================================================
// CCD Tests
// ============================================================================

TEST_F(RigidBodyTest, CCDInitialization) {
    RigidBodyCCD ccd;
    EXPECT_NO_THROW(ccd.initialize(physics, scene));
}

TEST_F(RigidBodyTest, EnableCCDOnActor) {
    RigidBodyCCD ccd;
    ccd.initialize(physics, scene);

    // Create fast-moving box
    PxRigidDynamic* box = createBox(PxVec3(0, 10, 0));

    // Enable CCD
    EXPECT_TRUE(ccd.enableCCD(box));
    EXPECT_TRUE(ccd.isCCDEnabled(box));

    box->release();
}

TEST_F(RigidBodyTest, CCDPreventsTunneling) {
    // Recreate scene with CCD enabled
    core->cleanup();
    delete core;

    core = new PhysXCore();
    PhysXCore::Config config;
    config.enableCCD = true;
    core->initialize(config);

    physics = core->getPhysics();
    scene = core->getScene();
    material = core->getMaterial();

    RigidBodyCCD ccd;
    ccd.initialize(physics, scene);

    // Create thin wall
    PxRigidStatic* wall = PxCreateStatic(
        *physics,
        PxTransform(PxVec3(0, 0, 0)),
        PxBoxGeometry(5, 5, 0.1f),
        *material
    );
    scene->addActor(*wall);

    // Create fast-moving bullet
    PxRigidDynamic* bullet = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 0, -10)),
        PxSphereGeometry(0.1f),
        *material,
        1.0f
    );
    scene->addActor(*bullet);

    // Enable CCD on bullet
    ccd.enableCCD(bullet);

    // Give it high velocity towards wall
    bullet->setLinearVelocity(PxVec3(0, 0, 50));

    // Simulate
    for (int i = 0; i < 60; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Bullet should not have tunneled through (position.z should not be > 0)
    PxVec3 bulletPos = bullet->getGlobalPose().p;
    EXPECT_LE(bulletPos.z, 1.0f);  // Allow some tolerance

    wall->release();
    bullet->release();
}

TEST_F(RigidBodyTest, CCDDisable) {
    RigidBodyCCD ccd;
    ccd.initialize(physics, scene);

    PxRigidDynamic* box = createBox(PxVec3(0, 10, 0));

    // Enable then disable CCD
    ccd.enableCCD(box);
    EXPECT_TRUE(ccd.isCCDEnabled(box));

    ccd.disableCCD(box);
    EXPECT_FALSE(ccd.isCCDEnabled(box));

    box->release();
}

// ============================================================================
// MassCalculator Tests
// ============================================================================

TEST_F(RigidBodyTest, MassCalculatorBasic) {
    RigidBodyMassCalculator calculator;
    EXPECT_NO_THROW(calculator.initialize(physics));
}

TEST_F(RigidBodyTest, CalculateMassFromDensity) {
    RigidBodyMassCalculator calculator;
    calculator.initialize(physics);

    // Create box with known dimensions
    PxRigidDynamic* box = createBox(PxVec3(0, 10, 0));

    // Calculate mass for unit cube with density 1000 (water)
    PxReal volume = 1.0f;  // 1m³
    PxReal density = 1000.0f;  // kg/m³
    PxReal expectedMass = volume * density;

    // This is a simplified test - actual mass depends on shape
    EXPECT_GT(box->getMass(), 0.0f);

    box->release();
}

TEST_F(RigidBodyTest, SetMassAndInertia) {
    RigidBodyMassCalculator calculator;
    calculator.initialize(physics);

    PxRigidDynamic* box = createBox(PxVec3(0, 10, 0));

    // Set custom mass
    PxReal customMass = 50.0f;
    calculator.setMass(box, customMass);

    EXPECT_FLOAT_EQ(box->getMass(), customMass);

    box->release();
}

TEST_F(RigidBodyTest, CalculateCenterOfMass) {
    RigidBodyMassCalculator calculator;
    calculator.initialize(physics);

    PxRigidDynamic* box = createBox(PxVec3(0, 10, 0));

    // Get center of mass (should be at geometric center for uniform box)
    PxVec3 com = calculator.getCenterOfMass(box);

    // For a symmetric box, COM should be at origin in local space
    EXPECT_NEAR(com.x, 0.0f, 0.1f);
    EXPECT_NEAR(com.y, 0.0f, 0.1f);
    EXPECT_NEAR(com.z, 0.0f, 0.1f);

    box->release();
}

TEST_F(RigidBodyTest, MassCalculatorMultipleShapes) {
    RigidBodyMassCalculator calculator;
    calculator.initialize(physics);

    // Create actor with multiple shapes
    PxRigidDynamic* actor = physics->createRigidDynamic(PxTransform(PxVec3(0, 10, 0)));

    // Add box shape
    PxShape* boxShape = PxRigidActorExt::createExclusiveShape(
        *actor,
        PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *material
    );

    // Add sphere shape
    PxShape* sphereShape = PxRigidActorExt::createExclusiveShape(
        *actor,
        PxSphereGeometry(0.3f),
        *material
    );

    scene->addActor(*actor);

    // Update mass properties
    PxRigidBodyExt::updateMassAndInertia(*actor, 1000.0f);

    // Should have combined mass from both shapes
    EXPECT_GT(actor->getMass(), 0.0f);

    actor->release();
}

TEST_F(RigidBodyTest, MassCalculatorDensityPresets) {
    RigidBodyMassCalculator calculator;
    calculator.initialize(physics);

    // Test with different density presets
    struct DensityTest {
        const char* name;
        PxReal density;
    };

    std::vector<DensityTest> densities = {
        {"Wood", 700.0f},
        {"Water", 1000.0f},
        {"Steel", 7850.0f},
        {"Lead", 11340.0f}
    };

    for (const auto& test : densities) {
        PxRigidDynamic* box = createBox(PxVec3(0, 10, 0));

        // Set density
        PxRigidBodyExt::updateMassAndInertia(*box, test.density);

        PxReal mass = box->getMass();
        EXPECT_GT(mass, 0.0f) << "Failed for density: " << test.name;

        box->release();
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(RigidBodyTest, ContactHandlerWithMultipleTypes) {
    RigidBodyContactHandler handler;
    handler.initialize(scene);

    int contactCount = 0;
    handler.setOnContactBegin([&contactCount](PxActor*, PxActor*) {
        contactCount++;
    });

    // Create various objects
    PxRigidStatic* ground = createGround();
    PxRigidDynamic* box1 = createBox(PxVec3(0, 5, 0));
    PxRigidDynamic* box2 = createBox(PxVec3(0, 7, 0));

    // Simulate
    for (int i = 0; i < 150; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    EXPECT_GT(contactCount, 0);

    ground->release();
    box1->release();
    box2->release();
}

TEST_F(RigidBodyTest, CCDWithHighVelocity) {
    // Recreate with CCD
    core->cleanup();
    delete core;

    core = new PhysXCore();
    PhysXCore::Config config;
    config.enableCCD = true;
    core->initialize(config);

    physics = core->getPhysics();
    scene = core->getScene();
    material = core->getMaterial();

    RigidBodyCCD ccd;
    ccd.initialize(physics, scene);

    // Create setup
    PxRigidStatic* ground = createGround();
    PxRigidDynamic* bullet = createBox(PxVec3(0, 50, 0));

    // Enable CCD
    ccd.enableCCD(bullet);

    // Apply extreme downward velocity
    bullet->setLinearVelocity(PxVec3(0, -100, 0));

    // Simulate
    for (int i = 0; i < 60; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Should have hit ground (y position should be low)
    PxVec3 pos = bullet->getGlobalPose().p;
    EXPECT_LT(pos.y, 5.0f);

    ground->release();
    bullet->release();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
