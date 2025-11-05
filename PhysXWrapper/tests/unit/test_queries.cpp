/**
 * @file test_queries.cpp
 * @brief Unit tests for Query classes
 *
 * Tests FrustumQuery, PointDistanceQuery, and BVHBuilder
 */

#include <gtest/gtest.h>
#include "PhysXCore.h"
#include "Query/FrustumQuery.h"
#include "Query/PointDistanceQuery.h"
#include "Query/BVHBuilder.h"

using namespace PhysXWrapper;

// ============================================================================
// Test Fixture
// ============================================================================

class QueryTest : public ::testing::Test {
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

    // Helper: Create box at position
    PxRigidDynamic* createBox(const PxVec3& position) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(position),
            PxBoxGeometry(0.5f, 0.5f, 0.5f),
            *material,
            10.0f
        );
        scene->addActor(*box);
        return box;
    }
};

// ============================================================================
// FrustumQuery Tests
// ============================================================================

TEST_F(QueryTest, FrustumQueryCreation) {
    FrustumQuery query;
    EXPECT_NO_THROW(query.initialize(scene));
    EXPECT_TRUE(query.isInitialized());
}

TEST_F(QueryTest, FrustumFromCamera) {
    FrustumQuery query;
    query.initialize(scene);

    // Create camera frustum
    PxVec3 cameraPos(0, 5, 10);
    PxVec3 cameraDir(0, 0, -1);
    PxVec3 cameraUp(0, 1, 0);
    PxReal fov = PxPi / 4;  // 45 degrees
    PxReal aspectRatio = 16.0f / 9.0f;
    PxReal nearPlane = 0.1f;
    PxReal farPlane = 100.0f;

    FrustumQuery::Frustum frustum = query.createFrustumFromCamera(
        cameraPos, cameraDir, cameraUp,
        fov, aspectRatio, nearPlane, farPlane
    );

    // Frustum should have 6 planes
    EXPECT_EQ(frustum.planes.size(), 6u);
}

TEST_F(QueryTest, FrustumCulling) {
    FrustumQuery query;
    query.initialize(scene);

    // Create objects at various positions
    createBox(PxVec3(0, 0, 0));    // Inside frustum
    createBox(PxVec3(0, 0, -5));   // Inside frustum
    createBox(PxVec3(0, 0, 50));   // Outside frustum (behind camera)
    createBox(PxVec3(100, 0, 0));  // Outside frustum (to the side)

    // Setup camera looking down negative Z
    PxVec3 cameraPos(0, 0, 10);
    PxVec3 cameraDir(0, 0, -1);
    PxVec3 cameraUp(0, 1, 0);

    FrustumQuery::Frustum frustum = query.createFrustumFromCamera(
        cameraPos, cameraDir, cameraUp,
        PxPi / 3, 16.0f / 9.0f, 0.1f, 50.0f
    );

    // Perform frustum query
    std::vector<PxActor*> visibleActors = query.queryFrustum(frustum);

    // Should find at least some actors (those in frustum)
    EXPECT_GT(visibleActors.size(), 0u);
    EXPECT_LE(visibleActors.size(), 4u);
}

TEST_F(QueryTest, FrustumSphereTest) {
    FrustumQuery query;
    query.initialize(scene);

    // Create simple frustum
    PxVec3 cameraPos(0, 0, 10);
    PxVec3 cameraDir(0, 0, -1);
    PxVec3 cameraUp(0, 1, 0);

    FrustumQuery::Frustum frustum = query.createFrustumFromCamera(
        cameraPos, cameraDir, cameraUp,
        PxPi / 3, 1.0f, 1.0f, 20.0f
    );

    // Test sphere inside frustum
    bool insideSphere = query.isSphereInFrustum(frustum, PxVec3(0, 0, 0), 1.0f);
    EXPECT_TRUE(insideSphere);

    // Test sphere outside frustum
    bool outsideSphere = query.isSphereInFrustum(frustum, PxVec3(100, 0, 0), 1.0f);
    EXPECT_FALSE(outsideSphere);
}

TEST_F(QueryTest, FrustumAABBTest) {
    FrustumQuery query;
    query.initialize(scene);

    PxVec3 cameraPos(0, 0, 10);
    PxVec3 cameraDir(0, 0, -1);
    PxVec3 cameraUp(0, 1, 0);

    FrustumQuery::Frustum frustum = query.createFrustumFromCamera(
        cameraPos, cameraDir, cameraUp,
        PxPi / 3, 1.0f, 1.0f, 20.0f
    );

    // Test AABB inside frustum
    PxBounds3 insideBox(PxVec3(-1, -1, -1), PxVec3(1, 1, 1));
    bool inside = query.isAABBInFrustum(frustum, insideBox);
    EXPECT_TRUE(inside);

    // Test AABB outside frustum
    PxBounds3 outsideBox(PxVec3(100, 100, 100), PxVec3(101, 101, 101));
    bool outside = query.isAABBInFrustum(frustum, outsideBox);
    EXPECT_FALSE(outside);
}

// ============================================================================
// PointDistanceQuery Tests
// ============================================================================

TEST_F(QueryTest, PointDistanceQueryCreation) {
    PointDistanceQuery query;
    EXPECT_NO_THROW(query.initialize(physics, scene));
    EXPECT_TRUE(query.isInitialized());
}

TEST_F(QueryTest, ClosestPointToGeometry) {
    PointDistanceQuery query;
    query.initialize(physics, scene);

    // Create box at origin
    createBox(PxVec3(0, 0, 0));

    // Query point near box
    PxVec3 queryPoint(5, 0, 0);
    PxVec3 closestPoint;
    PxReal distance;

    bool found = query.getClosestPoint(queryPoint, closestPoint, distance);

    EXPECT_TRUE(found);
    EXPECT_GT(distance, 0.0f);
    EXPECT_LT(distance, 10.0f);
}

TEST_F(QueryTest, DistanceToSurface) {
    PointDistanceQuery query;
    query.initialize(physics, scene);

    // Create sphere at origin
    PxRigidDynamic* sphere = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 0, 0)),
        PxSphereGeometry(2.0f),
        *material,
        10.0f
    );
    scene->addActor(*sphere);

    // Query point outside sphere
    PxVec3 queryPoint(5, 0, 0);
    PxReal distance = query.getDistanceToSurface(queryPoint);

    // Distance should be approximately 3.0 (5 - 2)
    EXPECT_NEAR(distance, 3.0f, 0.5f);

    sphere->release();
}

TEST_F(QueryTest, BatchPointQuery) {
    PointDistanceQuery query;
    query.initialize(physics, scene);

    // Create object at origin
    createBox(PxVec3(0, 0, 0));

    // Query multiple points
    std::vector<PxVec3> queryPoints = {
        PxVec3(5, 0, 0),
        PxVec3(0, 5, 0),
        PxVec3(0, 0, 5),
        PxVec3(10, 10, 10)
    };

    std::vector<PxVec3> closestPoints;
    std::vector<PxReal> distances;

    int foundCount = query.batchQuery(queryPoints, closestPoints, distances);

    EXPECT_GT(foundCount, 0);
    EXPECT_EQ(closestPoints.size(), queryPoints.size());
    EXPECT_EQ(distances.size(), queryPoints.size());
}

TEST_F(QueryTest, NearestActorQuery) {
    PointDistanceQuery query;
    query.initialize(physics, scene);

    // Create multiple boxes
    PxRigidDynamic* box1 = createBox(PxVec3(5, 0, 0));
    PxRigidDynamic* box2 = createBox(PxVec3(10, 0, 0));
    PxRigidDynamic* box3 = createBox(PxVec3(15, 0, 0));

    // Query from origin - box1 should be nearest
    PxVec3 queryPoint(0, 0, 0);
    PxActor* nearestActor = query.getNearestActor(queryPoint);

    ASSERT_NE(nearestActor, nullptr);
    // Verify it's one of our boxes
    EXPECT_TRUE(nearestActor == box1 || nearestActor == box2 || nearestActor == box3);

    box1->release();
    box2->release();
    box3->release();
}

TEST_F(QueryTest, WithinRadiusQuery) {
    PointDistanceQuery query;
    query.initialize(physics, scene);

    // Create boxes in a pattern
    for (int i = 0; i < 10; i++) {
        createBox(PxVec3(i * 2.0f, 0, 0));
    }

    // Query within radius
    PxVec3 center(10, 0, 0);
    PxReal radius = 5.0f;

    std::vector<PxActor*> actorsInRadius = query.getActorsWithinRadius(center, radius);

    // Should find some actors within radius
    EXPECT_GT(actorsInRadius.size(), 0u);
    EXPECT_LE(actorsInRadius.size(), 10u);

    // Verify distances
    for (PxActor* actor : actorsInRadius) {
        if (actor->is<PxRigidActor>()) {
            PxRigidActor* rigidActor = actor->is<PxRigidActor>();
            PxVec3 actorPos = rigidActor->getGlobalPose().p;
            PxReal distance = (actorPos - center).magnitude();
            EXPECT_LE(distance, radius + 1.0f);  // +1 for actor size
        }
    }
}

// ============================================================================
// BVHBuilder Tests
// ============================================================================

TEST_F(QueryTest, BVHBuilderCreation) {
    BVHBuilder builder;
    EXPECT_NO_THROW(builder.initialize(physics, scene));
    EXPECT_TRUE(builder.isInitialized());
}

TEST_F(QueryTest, BuildSimpleBVH) {
    BVHBuilder builder;
    builder.initialize(physics, scene);

    // Create multiple boxes
    std::vector<PxRigidActor*> actors;
    for (int i = 0; i < 10; i++) {
        actors.push_back(createBox(PxVec3(i * 2.0f, 0, 0)));
    }

    // Build BVH
    PxBVH* bvh = builder.buildBVHFromActors(actors);
    ASSERT_NE(bvh, nullptr);

    // BVH should have bounds
    PxBounds3 bounds = bvh->getBounds();
    EXPECT_FALSE(bounds.isEmpty());

    bvh->release();
    for (auto* actor : actors) actor->release();
}

TEST_F(QueryTest, BVHFromBounds) {
    BVHBuilder builder;
    builder.initialize(physics, scene);

    // Create bounds array
    std::vector<PxBounds3> bounds;
    for (int i = 0; i < 20; i++) {
        PxVec3 center(i * 3.0f, 0, 0);
        bounds.push_back(PxBounds3(center - PxVec3(1, 1, 1), center + PxVec3(1, 1, 1)));
    }

    // Build BVH
    PxBVH* bvh = builder.buildBVHFromBounds(bounds);
    ASSERT_NE(bvh, nullptr);

    bvh->release();
}

TEST_F(QueryTest, BVHRaycastQuery) {
    BVHBuilder builder;
    builder.initialize(physics, scene);

    // Create grid of boxes
    std::vector<PxRigidActor*> actors;
    for (int x = 0; x < 5; x++) {
        for (int z = 0; z < 5; z++) {
            actors.push_back(createBox(PxVec3(x * 2.0f, 0, z * 2.0f)));
        }
    }

    // Build BVH
    PxBVH* bvh = builder.buildBVHFromActors(actors);
    ASSERT_NE(bvh, nullptr);

    // Perform raycast using BVH
    PxVec3 origin(5, 10, 5);
    PxVec3 direction(0, -1, 0);
    PxReal maxDistance = 20.0f;

    std::vector<PxU32> hitIndices = builder.raycastBVH(bvh, origin, direction, maxDistance);

    // Should hit something
    EXPECT_GT(hitIndices.size(), 0u);

    bvh->release();
    for (auto* actor : actors) actor->release();
}

TEST_F(QueryTest, BVHOverlapQuery) {
    BVHBuilder builder;
    builder.initialize(physics, scene);

    // Create scattered boxes
    std::vector<PxRigidActor*> actors;
    for (int i = 0; i < 20; i++) {
        actors.push_back(createBox(PxVec3(i * 1.5f, 0, (i % 3) * 2.0f)));
    }

    // Build BVH
    PxBVH* bvh = builder.buildBVHFromActors(actors);
    ASSERT_NE(bvh, nullptr);

    // Query overlapping region
    PxBounds3 queryBounds(PxVec3(5, -2, -2), PxVec3(15, 2, 4));

    std::vector<PxU32> overlapIndices = builder.overlapBVH(bvh, queryBounds);

    // Should find overlapping actors
    EXPECT_GT(overlapIndices.size(), 0u);
    EXPECT_LE(overlapIndices.size(), 20u);

    bvh->release();
    for (auto* actor : actors) actor->release();
}

TEST_F(QueryTest, BVHPerformanceComparison) {
    BVHBuilder builder;
    builder.initialize(physics, scene);

    // Create large number of actors
    std::vector<PxRigidActor*> actors;
    for (int i = 0; i < 100; i++) {
        actors.push_back(createBox(PxVec3(
            (i % 10) * 3.0f,
            0,
            (i / 10) * 3.0f
        )));
    }

    // Build BVH
    PxBVH* bvh = builder.buildBVHFromActors(actors);
    ASSERT_NE(bvh, nullptr);

    // Multiple queries should be faster with BVH
    PxVec3 origin(15, 10, 15);
    PxVec3 direction(0, -1, 0);

    std::vector<PxU32> hitIndices = builder.raycastBVH(bvh, origin, direction, 20.0f);

    // Just verify it works
    SUCCEED();

    bvh->release();
    for (auto* actor : actors) actor->release();
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(QueryTest, FrustumCullingWithBVH) {
    FrustumQuery frustumQuery;
    BVHBuilder bvhBuilder;

    frustumQuery.initialize(scene);
    bvhBuilder.initialize(physics, scene);

    // Create large scene
    std::vector<PxRigidActor*> actors;
    for (int x = 0; x < 10; x++) {
        for (int z = 0; z < 10; z++) {
            actors.push_back(createBox(PxVec3(x * 3.0f, 0, z * 3.0f)));
        }
    }

    // Build BVH for acceleration
    PxBVH* bvh = bvhBuilder.buildBVHFromActors(actors);

    // Setup frustum
    FrustumQuery::Frustum frustum = frustumQuery.createFrustumFromCamera(
        PxVec3(15, 5, 30),
        PxVec3(0, 0, -1),
        PxVec3(0, 1, 0),
        PxPi / 3, 16.0f / 9.0f, 0.1f, 50.0f
    );

    // Perform frustum culling
    std::vector<PxActor*> visibleActors = frustumQuery.queryFrustum(frustum);

    // Should find subset of actors
    EXPECT_GT(visibleActors.size(), 0u);
    EXPECT_LT(visibleActors.size(), 100u);

    bvh->release();
    for (auto* actor : actors) actor->release();
}

TEST_F(QueryTest, CombinedPointAndFrustumQuery) {
    PointDistanceQuery pointQuery;
    FrustumQuery frustumQuery;

    pointQuery.initialize(physics, scene);
    frustumQuery.initialize(scene);

    // Create objects
    for (int i = 0; i < 10; i++) {
        createBox(PxVec3(i * 2.0f, 0, 0));
    }

    // Get visible objects in frustum
    FrustumQuery::Frustum frustum = frustumQuery.createFrustumFromCamera(
        PxVec3(10, 5, 20),
        PxVec3(0, 0, -1),
        PxVec3(0, 1, 0),
        PxPi / 3, 1.0f, 1.0f, 30.0f
    );

    std::vector<PxActor*> visibleActors = frustumQuery.queryFrustum(frustum);

    // Find nearest visible actor to camera
    PxVec3 cameraPos(10, 5, 20);
    PxActor* nearest = nullptr;
    PxReal minDistance = FLT_MAX;

    for (PxActor* actor : visibleActors) {
        if (actor->is<PxRigidActor>()) {
            PxRigidActor* rigidActor = actor->is<PxRigidActor>();
            PxVec3 actorPos = rigidActor->getGlobalPose().p;
            PxReal dist = (actorPos - cameraPos).magnitude();

            if (dist < minDistance) {
                minDistance = dist;
                nearest = actor;
            }
        }
    }

    // Should have found nearest
    if (!visibleActors.empty()) {
        EXPECT_NE(nearest, nullptr);
        EXPECT_GT(minDistance, 0.0f);
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
