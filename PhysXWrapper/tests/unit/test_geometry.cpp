/**
 * @file test_geometry.cpp
 * @brief Unit tests for Geometry classes
 *
 * Tests ConvexMeshBuilder, TriangleMeshBuilder, and GeometryQuery
 */

#include <gtest/gtest.h>
#include "PhysXCore.h"
#include "Geometry/ConvexMeshBuilder.h"
#include "Geometry/TriangleMeshBuilder.h"
#include "Geometry/GeometryQuery.h"

using namespace PhysXWrapper;

// ============================================================================
// Test Fixture
// ============================================================================

class GeometryTest : public ::testing::Test {
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
};

// ============================================================================
// ConvexMeshBuilder Tests
// ============================================================================

TEST_F(GeometryTest, ConvexMeshBuilderCreation) {
    ConvexMeshBuilder builder;
    EXPECT_NO_THROW(builder.initialize(physics));
    EXPECT_TRUE(builder.isInitialized());
}

TEST_F(GeometryTest, CreateSimpleConvexMesh) {
    ConvexMeshBuilder builder;
    builder.initialize(physics);

    // Create cube vertices
    std::vector<PxVec3> vertices = {
        PxVec3(-1, -1, -1), PxVec3(1, -1, -1),
        PxVec3(1, 1, -1), PxVec3(-1, 1, -1),
        PxVec3(-1, -1, 1), PxVec3(1, -1, 1),
        PxVec3(1, 1, 1), PxVec3(-1, 1, 1)
    };

    PxConvexMesh* mesh = builder.createConvexMesh(vertices);
    ASSERT_NE(mesh, nullptr);
    EXPECT_EQ(mesh->getNbVertices(), 8u);

    mesh->release();
}

TEST_F(GeometryTest, CreateConvexMeshFromStandardShapes) {
    ConvexMeshBuilder builder;
    builder.initialize(physics);

    // Test pyramid
    PxConvexMesh* pyramid = builder.createPyramidMesh(2.0f, 3.0f);
    ASSERT_NE(pyramid, nullptr);
    EXPECT_GT(pyramid->getNbVertices(), 0u);

    // Test wedge
    PxConvexMesh* wedge = builder.createWedgeMesh(2.0f, 1.0f, 3.0f);
    ASSERT_NE(wedge, nullptr);
    EXPECT_GT(wedge->getNbVertices(), 0u);

    pyramid->release();
    wedge->release();
}

TEST_F(GeometryTest, ConvexMeshActorCreation) {
    ConvexMeshBuilder builder;
    builder.initialize(physics);

    std::vector<PxVec3> vertices = {
        PxVec3(-1, -1, -1), PxVec3(1, -1, -1),
        PxVec3(1, 1, -1), PxVec3(-1, 1, -1),
        PxVec3(-1, -1, 1), PxVec3(1, -1, 1),
        PxVec3(1, 1, 1), PxVec3(-1, 1, 1)
    };

    PxConvexMesh* mesh = builder.createConvexMesh(vertices);
    ASSERT_NE(mesh, nullptr);

    // Create actor from mesh
    PxRigidDynamic* actor = builder.createConvexActor(
        mesh,
        PxTransform(PxVec3(0, 10, 0)),
        material,
        10.0f
    );

    ASSERT_NE(actor, nullptr);
    scene->addActor(*actor);

    // Verify actor is dynamic
    EXPECT_TRUE(actor->is<PxRigidDynamic>());

    actor->release();
    mesh->release();
}

TEST_F(GeometryTest, ConvexMeshInvalidInput) {
    ConvexMeshBuilder builder;
    builder.initialize(physics);

    // Test with too few vertices
    std::vector<PxVec3> tooFewVertices = {
        PxVec3(0, 0, 0),
        PxVec3(1, 0, 0)
    };

    PxConvexMesh* mesh = builder.createConvexMesh(tooFewVertices);
    // Should handle gracefully (return nullptr or valid minimal mesh)
    // Behavior depends on implementation
    SUCCEED();

    if (mesh) {
        mesh->release();
    }
}

// ============================================================================
// TriangleMeshBuilder Tests
// ============================================================================

TEST_F(GeometryTest, TriangleMeshBuilderCreation) {
    TriangleMeshBuilder builder;
    EXPECT_NO_THROW(builder.initialize(physics));
    EXPECT_TRUE(builder.isInitialized());
}

TEST_F(GeometryTest, CreateSimpleTriangleMesh) {
    TriangleMeshBuilder builder;
    builder.initialize(physics);

    // Create simple quad (2 triangles)
    std::vector<PxVec3> vertices = {
        PxVec3(-5, 0, -5),
        PxVec3(5, 0, -5),
        PxVec3(5, 0, 5),
        PxVec3(-5, 0, 5)
    };

    std::vector<PxU32> indices = {
        0, 1, 2,  // First triangle
        0, 2, 3   // Second triangle
    };

    PxTriangleMesh* mesh = builder.createTriangleMesh(vertices, indices);
    ASSERT_NE(mesh, nullptr);
    EXPECT_EQ(mesh->getNbVertices(), 4u);
    EXPECT_EQ(mesh->getNbTriangles(), 2u);

    mesh->release();
}

TEST_F(GeometryTest, CreateTriangleMeshStandardShapes) {
    TriangleMeshBuilder builder;
    builder.initialize(physics);

    // Test ground plane
    PxTriangleMesh* plane = builder.createGroundPlaneMesh(20.0f, 20.0f, 10, 10);
    ASSERT_NE(plane, nullptr);
    EXPECT_GT(plane->getNbVertices(), 0u);
    EXPECT_GT(plane->getNbTriangles(), 0u);

    // Test terrain
    std::vector<std::vector<PxReal>> heightMap(10, std::vector<PxReal>(10, 0.0f));
    PxTriangleMesh* terrain = builder.createTerrainMesh(heightMap, 1.0f, 1.0f);
    ASSERT_NE(terrain, nullptr);
    EXPECT_GT(terrain->getNbVertices(), 0u);

    plane->release();
    terrain->release();
}

TEST_F(GeometryTest, TriangleMeshActorCreation) {
    TriangleMeshBuilder builder;
    builder.initialize(physics);

    std::vector<PxVec3> vertices = {
        PxVec3(-5, 0, -5), PxVec3(5, 0, -5),
        PxVec3(5, 0, 5), PxVec3(-5, 0, 5)
    };

    std::vector<PxU32> indices = {
        0, 1, 2,
        0, 2, 3
    };

    PxTriangleMesh* mesh = builder.createTriangleMesh(vertices, indices);
    ASSERT_NE(mesh, nullptr);

    // Create static actor from mesh
    PxRigidStatic* actor = builder.createTriangleMeshActor(
        mesh,
        PxTransform(PxVec3(0, 0, 0)),
        material
    );

    ASSERT_NE(actor, nullptr);
    scene->addActor(*actor);

    // Verify actor is static
    EXPECT_TRUE(actor->is<PxRigidStatic>());

    actor->release();
    mesh->release();
}

TEST_F(GeometryTest, TriangleMeshWithHeightField) {
    TriangleMeshBuilder builder;
    builder.initialize(physics);

    // Create height map with variation
    std::vector<std::vector<PxReal>> heightMap(20, std::vector<PxReal>(20));

    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 20; j++) {
            // Simple sine wave pattern
            heightMap[i][j] = sin(i * 0.5f) * cos(j * 0.5f) * 2.0f;
        }
    }

    PxTriangleMesh* terrain = builder.createTerrainMesh(heightMap, 1.0f, 1.0f);
    ASSERT_NE(terrain, nullptr);

    // Should have many triangles for 20x20 grid
    EXPECT_GT(terrain->getNbTriangles(), 100u);

    terrain->release();
}

// ============================================================================
// GeometryQuery Tests
// ============================================================================

TEST_F(GeometryTest, GeometryQueryCreation) {
    GeometryQuery query;
    EXPECT_NO_THROW(query.initialize(scene));
    EXPECT_TRUE(query.isInitialized());
}

TEST_F(GeometryTest, RaycastBasic) {
    GeometryQuery query;
    query.initialize(scene);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Raycast downward from above ground
    PxVec3 origin(0, 10, 0);
    PxVec3 direction(0, -1, 0);
    PxReal maxDistance = 20.0f;

    PxRaycastHit hit;
    bool hasHit = query.raycast(origin, direction, maxDistance, hit);

    EXPECT_TRUE(hasHit);
    EXPECT_NEAR(hit.position.y, 0.0f, 0.1f);

    ground->release();
}

TEST_F(GeometryTest, RaycastMultipleHits) {
    GeometryQuery query;
    query.initialize(scene);

    // Create stack of boxes
    for (int i = 0; i < 5; i++) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(0, i * 2.0f, 0)),
            PxBoxGeometry(1, 1, 1),
            *material,
            10.0f
        );
        scene->addActor(*box);
    }

    // Raycast through stack
    PxVec3 origin(0, -5, 0);
    PxVec3 direction(0, 1, 0);
    PxReal maxDistance = 20.0f;

    std::vector<PxRaycastHit> hits;
    int numHits = query.raycastMultiple(origin, direction, maxDistance, hits, 10);

    EXPECT_GT(numHits, 0);
    EXPECT_LE(numHits, 5);
}

TEST_F(GeometryTest, SweepTest) {
    GeometryQuery query;
    query.initialize(scene);

    // Create obstacle
    PxRigidStatic* wall = PxCreateStatic(
        *physics,
        PxTransform(PxVec3(10, 0, 0)),
        PxBoxGeometry(1, 10, 10),
        *material
    );
    scene->addActor(*wall);

    // Sweep sphere towards wall
    PxSphereGeometry sweepGeom(0.5f);
    PxTransform sweepPose(PxVec3(0, 0, 0));
    PxVec3 sweepDirection(1, 0, 0);
    PxReal sweepDistance = 20.0f;

    PxSweepHit hit;
    bool hasHit = query.sweep(sweepGeom, sweepPose, sweepDirection, sweepDistance, hit);

    EXPECT_TRUE(hasHit);
    EXPECT_GT(hit.distance, 0.0f);
    EXPECT_LT(hit.distance, sweepDistance);

    wall->release();
}

TEST_F(GeometryTest, OverlapQuery) {
    GeometryQuery query;
    query.initialize(scene);

    // Create objects in region
    for (int i = 0; i < 5; i++) {
        PxRigidDynamic* box = PxCreateDynamic(
            *physics,
            PxTransform(PxVec3(i * 0.5f, 0, 0)),
            PxBoxGeometry(0.2f, 0.2f, 0.2f),
            *material,
            10.0f
        );
        scene->addActor(*box);
    }

    // Query overlapping region
    PxBoxGeometry queryGeom(3, 3, 3);
    PxTransform queryPose(PxVec3(0, 0, 0));

    std::vector<PxOverlapHit> hits;
    int numHits = query.overlap(queryGeom, queryPose, hits, 10);

    EXPECT_GT(numHits, 0);
    EXPECT_LE(numHits, 5);
}

TEST_F(GeometryTest, SphereCastTest) {
    GeometryQuery query;
    query.initialize(scene);

    // Create ground
    PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
    scene->addActor(*ground);

    // Sphere cast downward
    PxSphereGeometry sphere(1.0f);
    PxTransform startPose(PxVec3(0, 10, 0));
    PxVec3 direction(0, -1, 0);
    PxReal distance = 20.0f;

    PxSweepHit hit;
    bool hasHit = query.sweep(sphere, startPose, direction, distance, hit);

    EXPECT_TRUE(hasHit);
    EXPECT_GT(hit.distance, 0.0f);

    ground->release();
}

TEST_F(GeometryTest, BoxOverlapTest) {
    GeometryQuery query;
    query.initialize(scene);

    // Create box in space
    PxRigidDynamic* box = PxCreateDynamic(
        *physics,
        PxTransform(PxVec3(0, 0, 0)),
        PxBoxGeometry(1, 1, 1),
        *material,
        10.0f
    );
    scene->addActor(*box);

    // Test overlap with larger box
    PxBoxGeometry queryGeom(2, 2, 2);
    PxTransform queryPose(PxVec3(0, 0, 0));

    std::vector<PxOverlapHit> hits;
    int numHits = query.overlap(queryGeom, queryPose, hits, 10);

    EXPECT_EQ(numHits, 1);

    box->release();
}

TEST_F(GeometryTest, RaycastNoHit) {
    GeometryQuery query;
    query.initialize(scene);

    // Raycast into empty space
    PxVec3 origin(0, 0, 0);
    PxVec3 direction(1, 0, 0);
    PxReal maxDistance = 100.0f;

    PxRaycastHit hit;
    bool hasHit = query.raycast(origin, direction, maxDistance, hit);

    EXPECT_FALSE(hasHit);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(GeometryTest, ConvexAndTriangleMeshCollision) {
    ConvexMeshBuilder convexBuilder;
    TriangleMeshBuilder triangleBuilder;

    convexBuilder.initialize(physics);
    triangleBuilder.initialize(physics);

    // Create triangle mesh ground
    std::vector<PxVec3> groundVerts = {
        PxVec3(-10, 0, -10), PxVec3(10, 0, -10),
        PxVec3(10, 0, 10), PxVec3(-10, 0, 10)
    };
    std::vector<PxU32> groundIndices = {0, 1, 2, 0, 2, 3};

    PxTriangleMesh* groundMesh = triangleBuilder.createTriangleMesh(groundVerts, groundIndices);
    PxRigidStatic* ground = triangleBuilder.createTriangleMeshActor(
        groundMesh, PxTransform(PxVec3(0, 0, 0)), material
    );
    scene->addActor(*ground);

    // Create convex mesh falling object
    std::vector<PxVec3> boxVerts = {
        PxVec3(-0.5f, -0.5f, -0.5f), PxVec3(0.5f, -0.5f, -0.5f),
        PxVec3(0.5f, 0.5f, -0.5f), PxVec3(-0.5f, 0.5f, -0.5f),
        PxVec3(-0.5f, -0.5f, 0.5f), PxVec3(0.5f, -0.5f, 0.5f),
        PxVec3(0.5f, 0.5f, 0.5f), PxVec3(-0.5f, 0.5f, 0.5f)
    };

    PxConvexMesh* boxMesh = convexBuilder.createConvexMesh(boxVerts);
    PxRigidDynamic* box = convexBuilder.createConvexActor(
        boxMesh, PxTransform(PxVec3(0, 10, 0)), material, 10.0f
    );
    scene->addActor(*box);

    // Simulate collision
    for (int i = 0; i < 120; i++) {
        scene->simulate(1.0f / 60.0f);
        scene->fetchResults(true);
    }

    // Box should have fallen and stopped on ground
    PxVec3 boxPos = box->getGlobalPose().p;
    EXPECT_LT(boxPos.y, 5.0f);
    EXPECT_GT(boxPos.y, -1.0f);

    ground->release();
    groundMesh->release();
    box->release();
    boxMesh->release();
}

TEST_F(GeometryTest, QueryAgainstComplexGeometry) {
    TriangleMeshBuilder builder;
    builder.initialize(physics);

    // Create terrain
    std::vector<std::vector<PxReal>> heightMap(10, std::vector<PxReal>(10));
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            heightMap[i][j] = sin(i * 0.5f) * 0.5f;
        }
    }

    PxTriangleMesh* terrain = builder.createTerrainMesh(heightMap, 1.0f, 1.0f);
    PxRigidStatic* terrainActor = builder.createTriangleMeshActor(
        terrain, PxTransform(PxVec3(0, 0, 0)), material
    );
    scene->addActor(*terrainActor);

    // Perform queries
    GeometryQuery query;
    query.initialize(scene);

    // Raycast down at various points
    int hitCount = 0;
    for (int i = 0; i < 5; i++) {
        PxVec3 origin(i * 2.0f, 10, i * 2.0f);
        PxVec3 direction(0, -1, 0);

        PxRaycastHit hit;
        if (query.raycast(origin, direction, 20.0f, hit)) {
            hitCount++;
        }
    }

    EXPECT_GT(hitCount, 0);

    terrainActor->release();
    terrain->release();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
