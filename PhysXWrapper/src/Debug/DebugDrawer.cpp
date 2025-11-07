/**
 * @file DebugDrawer.cpp
 * @brief Implementation of DebugDrawer class
 */

#include "Debug/DebugDrawer.h"
#include <cmath>

namespace PhysXWrapper {

// ============================================================================
// DebugDrawer::Impl
// ============================================================================

class DebugDrawer::Impl {
public:
    DebugDrawCallback* m_callback = nullptr;
    DebugDrawFlag m_flags = DebugDrawFlag::ALL;

    std::vector<DebugLine> m_lines;
    std::vector<DebugTriangle> m_triangles;
    std::vector<DebugText> m_textLabels;

    void addLine(const PxVec3& start, const PxVec3& end, PxU32 color) {
        m_lines.push_back({start, end, color});
    }

    void addTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2, PxU32 color) {
        m_triangles.push_back({{v0, v1, v2}, color});
    }

    void addText(const PxVec3& position, const std::string& text, PxU32 color) {
        m_textLabels.push_back({position, text, color});
    }
};

// ============================================================================
// Construction/Destruction
// ============================================================================

DebugDrawer::DebugDrawer()
    : m_impl(std::make_unique<Impl>())
{
}

DebugDrawer::~DebugDrawer() = default;

// ============================================================================
// Configuration
// ============================================================================

void DebugDrawer::setDrawCallback(DebugDrawCallback* callback)
{
    m_impl->m_callback = callback;
}

void DebugDrawer::setFlags(DebugDrawFlag flags)
{
    m_impl->m_flags = flags;
}

DebugDrawFlag DebugDrawer::getFlags() const
{
    return m_impl->m_flags;
}

void DebugDrawer::enableFlag(DebugDrawFlag flag)
{
    m_impl->m_flags = m_impl->m_flags | flag;
}

void DebugDrawer::disableFlag(DebugDrawFlag flag)
{
    PxU32 currentFlags = static_cast<PxU32>(m_impl->m_flags);
    PxU32 flagToRemove = static_cast<PxU32>(flag);
    m_impl->m_flags = static_cast<DebugDrawFlag>(currentFlags & ~flagToRemove);
}

// ============================================================================
// Scene Drawing
// ============================================================================

void DebugDrawer::drawScene(PxScene* scene)
{
    if (!scene) return;

    // Get all actors
    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    if (numActors == 0) return;

    std::vector<PxActor*> actors(numActors);
    scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC,
                     actors.data(), numActors);

    // Draw each actor
    for (PxActor* actor : actors) {
        PxRigidActor* rigidActor = actor->is<PxRigidActor>();
        if (rigidActor) {
            drawActor(rigidActor);
        }
    }

    // Draw contacts if enabled
    if (m_impl->m_flags & DebugDrawFlag::CONTACTS) {
        drawContacts(scene);
    }

    // Draw velocities if enabled
    if (m_impl->m_flags & DebugDrawFlag::VELOCITIES) {
        drawVelocities(scene);
    }

    // Draw centers of mass if enabled
    if (m_impl->m_flags & DebugDrawFlag::COM) {
        drawCentersOfMass(scene);
    }
}

void DebugDrawer::drawActor(PxRigidActor* actor)
{
    if (!actor) return;

    // Get all shapes
    PxU32 numShapes = actor->getNbShapes();
    if (numShapes == 0) return;

    std::vector<PxShape*> shapes(numShapes);
    actor->getShapes(shapes.data(), numShapes);

    // Determine color based on actor type
    PxU32 color = DebugColors::WHITE;
    PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
    if (dynamic) {
        color = dynamic->isSleeping() ? DebugColors::BLUE : DebugColors::GREEN;
    } else {
        color = DebugColors::CYAN; // Static
    }

    // Draw each shape
    for (PxShape* shape : shapes) {
        if (m_impl->m_flags & DebugDrawFlag::SHAPES) {
            drawShape(shape, PxShapeExt::getGlobalPose(*shape, *actor), color);
        }
    }

    // Draw AABB if enabled
    if (m_impl->m_flags & DebugDrawFlag::AABBS) {
        PxBounds3 bounds = actor->getWorldBounds();
        drawBounds(bounds, DebugColors::YELLOW);
    }

    // Draw center of mass if enabled
    if (m_impl->m_flags & DebugDrawFlag::COM) {
        PxRigidBody* body = actor->is<PxRigidBody>();
        if (body) {
            PxTransform comPose = body->getGlobalPose().transform(body->getCMassLocalPose());
            drawAxes(comPose, 0.5f);
        }
    }

    // Draw axes if enabled
    if (m_impl->m_flags & DebugDrawFlag::AXES) {
        drawAxes(actor->getGlobalPose(), 1.0f);
    }
}

void DebugDrawer::drawShape(PxShape* shape, const PxTransform& pose, PxU32 color)
{
    if (!shape) return;

    PxGeometryHolder geometryHolder = shape->getGeometry();
    drawGeometry(geometryHolder.any(), pose, color);
}

void DebugDrawer::drawJoint(PxJoint* joint)
{
    if (!joint) return;

    PxRigidActor* actor0;
    PxRigidActor* actor1;
    joint->getActors(actor0, actor1);

    PxTransform localFrame0 = joint->getLocalPose(PxJointActorIndex::eACTOR0);
    PxTransform localFrame1 = joint->getLocalPose(PxJointActorIndex::eACTOR1);

    PxTransform globalFrame0 = actor0 ? actor0->getGlobalPose().transform(localFrame0) : localFrame0;
    PxTransform globalFrame1 = actor1 ? actor1->getGlobalPose().transform(localFrame1) : localFrame1;

    // Draw connection line
    drawLine(globalFrame0.p, globalFrame1.p, DebugColors::ORANGE);

    // Draw local frames
    drawAxes(globalFrame0, 0.5f);
    drawAxes(globalFrame1, 0.5f);

    // Draw joint-specific visualization based on type
    // PhysX 5.x: getConcreteType() returns PxType, cast to PxJointConcreteType::Enum
    PxJointConcreteType::Enum jointType = static_cast<PxJointConcreteType::Enum>(joint->getConcreteType());

    switch (jointType) {
        case PxJointConcreteType::eSPHERICAL: {
            // Draw sphere at joint location
            drawSphere(globalFrame0.p, 0.1f, DebugColors::ORANGE);
            break;
        }
        case PxJointConcreteType::eREVOLUTE: {
            // Draw rotation axis
            PxVec3 axis = globalFrame0.rotate(PxVec3(1, 0, 0));
            drawArrow(globalFrame0.p, axis, 0.5f, DebugColors::RED);
            break;
        }
        case PxJointConcreteType::ePRISMATIC: {
            // Draw sliding axis
            PxVec3 axis = globalFrame0.rotate(PxVec3(1, 0, 0));
            drawArrow(globalFrame0.p, axis, 0.5f, DebugColors::GREEN);
            break;
        }
        case PxJointConcreteType::eFIXED: {
            // Draw box at joint location
            drawBox(globalFrame0.p, PxVec3(0.1f), PxQuat(PxIdentity), DebugColors::MAGENTA);
            break;
        }
        case PxJointConcreteType::eDISTANCE: {
            // Already drew the line
            break;
        }
        default:
            break;
    }
}

// ============================================================================
// Primitive Drawing
// ============================================================================

void DebugDrawer::drawLine(const PxVec3& start, const PxVec3& end, PxU32 color)
{
    m_impl->addLine(start, end, color);
}

void DebugDrawer::drawBox(const PxVec3& center, const PxVec3& halfExtents,
                          const PxQuat& rotation, PxU32 color)
{
    // Define 8 corners of the box in local space
    PxVec3 corners[8] = {
        PxVec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
        PxVec3( halfExtents.x, -halfExtents.y, -halfExtents.z),
        PxVec3( halfExtents.x,  halfExtents.y, -halfExtents.z),
        PxVec3(-halfExtents.x,  halfExtents.y, -halfExtents.z),
        PxVec3(-halfExtents.x, -halfExtents.y,  halfExtents.z),
        PxVec3( halfExtents.x, -halfExtents.y,  halfExtents.z),
        PxVec3( halfExtents.x,  halfExtents.y,  halfExtents.z),
        PxVec3(-halfExtents.x,  halfExtents.y,  halfExtents.z)
    };

    // Transform corners to world space
    PxTransform pose(center, rotation);
    for (int i = 0; i < 8; i++) {
        corners[i] = pose.transform(corners[i]);
    }

    // Draw 12 edges
    // Bottom face
    drawLine(corners[0], corners[1], color);
    drawLine(corners[1], corners[2], color);
    drawLine(corners[2], corners[3], color);
    drawLine(corners[3], corners[0], color);

    // Top face
    drawLine(corners[4], corners[5], color);
    drawLine(corners[5], corners[6], color);
    drawLine(corners[6], corners[7], color);
    drawLine(corners[7], corners[4], color);

    // Vertical edges
    drawLine(corners[0], corners[4], color);
    drawLine(corners[1], corners[5], color);
    drawLine(corners[2], corners[6], color);
    drawLine(corners[3], corners[7], color);
}

void DebugDrawer::drawSphere(const PxVec3& center, PxReal radius, PxU32 color, PxU32 segments)
{
    const PxReal angleStep = PxTwoPi / segments;

    // Draw XY circle
    for (PxU32 i = 0; i < segments; i++) {
        PxReal angle0 = i * angleStep;
        PxReal angle1 = (i + 1) * angleStep;

        PxVec3 p0 = center + PxVec3(PxCos(angle0) * radius, PxSin(angle0) * radius, 0);
        PxVec3 p1 = center + PxVec3(PxCos(angle1) * radius, PxSin(angle1) * radius, 0);
        drawLine(p0, p1, color);
    }

    // Draw XZ circle
    for (PxU32 i = 0; i < segments; i++) {
        PxReal angle0 = i * angleStep;
        PxReal angle1 = (i + 1) * angleStep;

        PxVec3 p0 = center + PxVec3(PxCos(angle0) * radius, 0, PxSin(angle0) * radius);
        PxVec3 p1 = center + PxVec3(PxCos(angle1) * radius, 0, PxSin(angle1) * radius);
        drawLine(p0, p1, color);
    }

    // Draw YZ circle
    for (PxU32 i = 0; i < segments; i++) {
        PxReal angle0 = i * angleStep;
        PxReal angle1 = (i + 1) * angleStep;

        PxVec3 p0 = center + PxVec3(0, PxCos(angle0) * radius, PxSin(angle0) * radius);
        PxVec3 p1 = center + PxVec3(0, PxCos(angle1) * radius, PxSin(angle1) * radius);
        drawLine(p0, p1, color);
    }
}

void DebugDrawer::drawCapsule(const PxVec3& center, PxReal radius, PxReal halfHeight,
                               const PxQuat& rotation, PxU32 color)
{
    PxTransform pose(center, rotation);

    const PxU32 segments = 16;
    const PxReal angleStep = PxTwoPi / segments;

    // Cylinder part (along X axis in local space)
    PxVec3 top = pose.transform(PxVec3(halfHeight, 0, 0));
    PxVec3 bottom = pose.transform(PxVec3(-halfHeight, 0, 0));

    // Draw circles at top and bottom
    for (PxU32 i = 0; i < segments; i++) {
        PxReal angle0 = i * angleStep;
        PxReal angle1 = (i + 1) * angleStep;

        PxVec3 offset0 = pose.rotate(PxVec3(0, PxCos(angle0) * radius, PxSin(angle0) * radius));
        PxVec3 offset1 = pose.rotate(PxVec3(0, PxCos(angle1) * radius, PxSin(angle1) * radius));

        drawLine(top + offset0, top + offset1, color);
        drawLine(bottom + offset0, bottom + offset1, color);
    }

    // Draw vertical lines
    for (PxU32 i = 0; i < 4; i++) {
        PxReal angle = i * PxPi / 2.0f;
        PxVec3 offset = pose.rotate(PxVec3(0, PxCos(angle) * radius, PxSin(angle) * radius));
        drawLine(bottom + offset, top + offset, color);
    }

    // Draw hemisphere arcs
    const PxU32 arcSegments = 8;
    for (PxU32 i = 0; i < arcSegments; i++) {
        PxReal angle0 = i * PxPiDivTwo / arcSegments;
        PxReal angle1 = (i + 1) * PxPiDivTwo / arcSegments;

        PxVec3 p0 = pose.transform(PxVec3(halfHeight + PxSin(angle0) * radius, PxCos(angle0) * radius, 0));
        PxVec3 p1 = pose.transform(PxVec3(halfHeight + PxSin(angle1) * radius, PxCos(angle1) * radius, 0));
        drawLine(p0, p1, color);

        PxVec3 p2 = pose.transform(PxVec3(-halfHeight - PxSin(angle0) * radius, PxCos(angle0) * radius, 0));
        PxVec3 p3 = pose.transform(PxVec3(-halfHeight - PxSin(angle1) * radius, PxCos(angle1) * radius, 0));
        drawLine(p2, p3, color);
    }
}

void DebugDrawer::drawArrow(const PxVec3& start, const PxVec3& direction,
                            PxReal length, PxU32 color)
{
    PxVec3 normalizedDir = direction;
    normalizedDir.normalize();
    PxVec3 end = start + normalizedDir * length;

    // Draw main line
    drawLine(start, end, color);

    // Draw arrowhead
    PxVec3 perpendicular;
    if (PxAbs(normalizedDir.y) < 0.9f) {
        perpendicular = normalizedDir.cross(PxVec3(0, 1, 0));
    } else {
        perpendicular = normalizedDir.cross(PxVec3(1, 0, 0));
    }
    perpendicular.normalize();

    PxVec3 perpendicular2 = normalizedDir.cross(perpendicular);
    perpendicular2.normalize();

    PxReal arrowSize = length * 0.1f;
    PxVec3 arrowBase = end - normalizedDir * arrowSize * 2.0f;

    PxVec3 arrow1 = arrowBase + perpendicular * arrowSize;
    PxVec3 arrow2 = arrowBase - perpendicular * arrowSize;
    PxVec3 arrow3 = arrowBase + perpendicular2 * arrowSize;
    PxVec3 arrow4 = arrowBase - perpendicular2 * arrowSize;

    drawLine(end, arrow1, color);
    drawLine(end, arrow2, color);
    drawLine(end, arrow3, color);
    drawLine(end, arrow4, color);
}

void DebugDrawer::drawAxes(const PxTransform& pose, PxReal scale)
{
    PxVec3 origin = pose.p;
    PxVec3 xAxis = pose.rotate(PxVec3(scale, 0, 0));
    PxVec3 yAxis = pose.rotate(PxVec3(0, scale, 0));
    PxVec3 zAxis = pose.rotate(PxVec3(0, 0, scale));

    drawArrow(origin, xAxis, 1.0f, DebugColors::RED);
    drawArrow(origin, yAxis, 1.0f, DebugColors::GREEN);
    drawArrow(origin, zAxis, 1.0f, DebugColors::BLUE);
}

void DebugDrawer::drawBounds(const PxBounds3& bounds, PxU32 color)
{
    PxVec3 center = bounds.getCenter();
    PxVec3 extents = bounds.getExtents();
    drawBox(center, extents, PxQuat(PxIdentity), color);
}

void DebugDrawer::drawText(const PxVec3& position, const std::string& text, PxU32 color)
{
    m_impl->addText(position, text, color);
}

// ============================================================================
// Utility Drawing
// ============================================================================

void DebugDrawer::drawContacts(PxScene* scene)
{
    if (!scene) return;

    // Note: PhysX doesn't provide direct access to contact points through public API
    // You would need to use a contact report callback (PxSimulationEventCallback)
    // to collect contact information during simulation.
    // This is a placeholder for visualization of collected contact data.
}

void DebugDrawer::drawVelocities(PxScene* scene)
{
    if (!scene) return;

    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    if (numActors == 0) return;

    std::vector<PxActor*> actors(numActors);
    scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, actors.data(), numActors);

    for (PxActor* actor : actors) {
        PxRigidDynamic* dynamic = actor->is<PxRigidDynamic>();
        if (dynamic) {
            PxVec3 linearVel = dynamic->getLinearVelocity();
            PxVec3 angularVel = dynamic->getAngularVelocity();
            PxVec3 position = dynamic->getGlobalPose().p;

            // Draw linear velocity
            if (linearVel.magnitude() > 0.01f) {
                drawArrow(position, linearVel, 1.0f, DebugColors::CYAN);
            }

            // Draw angular velocity
            if (angularVel.magnitude() > 0.01f) {
                drawArrow(position, angularVel, 0.5f, DebugColors::MAGENTA);
            }
        }
    }
}

void DebugDrawer::drawCentersOfMass(PxScene* scene)
{
    if (!scene) return;

    PxU32 numActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    if (numActors == 0) return;

    std::vector<PxActor*> actors(numActors);
    scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, actors.data(), numActors);

    for (PxActor* actor : actors) {
        PxRigidBody* body = actor->is<PxRigidBody>();
        if (body) {
            PxTransform comPose = body->getGlobalPose().transform(body->getCMassLocalPose());
            drawSphere(comPose.p, 0.1f, DebugColors::YELLOW);
            drawAxes(comPose, 0.3f);
        }
    }
}

void DebugDrawer::clear()
{
    m_impl->m_lines.clear();
    m_impl->m_triangles.clear();
    m_impl->m_textLabels.clear();
}

void DebugDrawer::flush()
{
    if (!m_impl->m_callback) return;

    // Flush lines
    for (const DebugLine& line : m_impl->m_lines) {
        m_impl->m_callback->drawLine(line.start, line.end, line.color);
    }

    // Flush triangles
    for (const DebugTriangle& tri : m_impl->m_triangles) {
        m_impl->m_callback->drawTriangle(tri.vertices[0], tri.vertices[1], tri.vertices[2], tri.color);
    }

    // Flush text
    for (const DebugText& text : m_impl->m_textLabels) {
        m_impl->m_callback->drawText(text.position, text.text, text.color);
    }

    clear();
}

// ============================================================================
// Batch Collection
// ============================================================================

const std::vector<DebugLine>& DebugDrawer::getLines() const
{
    return m_impl->m_lines;
}

const std::vector<DebugTriangle>& DebugDrawer::getTriangles() const
{
    return m_impl->m_triangles;
}

const std::vector<DebugText>& DebugDrawer::getTextLabels() const
{
    return m_impl->m_textLabels;
}

// ============================================================================
// Helper Methods - Geometry Drawing
// ============================================================================

void DebugDrawer::drawGeometry(const PxGeometry& geometry, const PxTransform& pose, PxU32 color)
{
    switch (geometry.getType()) {
        case PxGeometryType::eBOX:
            drawBoxGeometry(static_cast<const PxBoxGeometry&>(geometry), pose, color);
            break;
        case PxGeometryType::eSPHERE:
            drawSphereGeometry(static_cast<const PxSphereGeometry&>(geometry), pose, color);
            break;
        case PxGeometryType::eCAPSULE:
            drawCapsuleGeometry(static_cast<const PxCapsuleGeometry&>(geometry), pose, color);
            break;
        case PxGeometryType::ePLANE:
            drawPlaneGeometry(static_cast<const PxPlaneGeometry&>(geometry), pose, color);
            break;
        case PxGeometryType::eCONVEXMESH:
            drawConvexMeshGeometry(static_cast<const PxConvexMeshGeometry&>(geometry), pose, color);
            break;
        case PxGeometryType::eTRIANGLEMESH:
            drawTriangleMeshGeometry(static_cast<const PxTriangleMeshGeometry&>(geometry), pose, color);
            break;
        default:
            break;
    }
}

void DebugDrawer::drawBoxGeometry(const PxBoxGeometry& box, const PxTransform& pose, PxU32 color)
{
    drawBox(pose.p, box.halfExtents, pose.q, color);
}

void DebugDrawer::drawSphereGeometry(const PxSphereGeometry& sphere, const PxTransform& pose, PxU32 color)
{
    drawSphere(pose.p, sphere.radius, color);
}

void DebugDrawer::drawCapsuleGeometry(const PxCapsuleGeometry& capsule, const PxTransform& pose, PxU32 color)
{
    drawCapsule(pose.p, capsule.radius, capsule.halfHeight, pose.q, color);
}

void DebugDrawer::drawPlaneGeometry(const PxPlaneGeometry& plane, const PxTransform& pose, PxU32 color)
{
    // Draw a grid on the plane
    const PxReal gridSize = 10.0f;
    const PxU32 gridLines = 20;
    const PxReal step = gridSize * 2.0f / gridLines;

    PxVec3 right = pose.rotate(PxVec3(1, 0, 0));
    PxVec3 forward = pose.rotate(PxVec3(0, 0, 1));
    PxVec3 center = pose.p;

    // Draw grid lines
    for (PxU32 i = 0; i <= gridLines; i++) {
        PxReal offset = -gridSize + i * step;

        // Lines along right direction
        PxVec3 start1 = center + forward * offset - right * gridSize;
        PxVec3 end1 = center + forward * offset + right * gridSize;
        drawLine(start1, end1, color);

        // Lines along forward direction
        PxVec3 start2 = center + right * offset - forward * gridSize;
        PxVec3 end2 = center + right * offset + forward * gridSize;
        drawLine(start2, end2, color);
    }
}

void DebugDrawer::drawConvexMeshGeometry(const PxConvexMeshGeometry& convex, const PxTransform& pose, PxU32 color)
{
    PxConvexMesh* mesh = convex.convexMesh;
    if (!mesh) return;

    const PxU8* indices = mesh->getIndexBuffer();
    const PxVec3* vertices = mesh->getVertices();
    PxU32 nbPolygons = mesh->getNbPolygons();

    // Draw each polygon's edges
    for (PxU32 i = 0; i < nbPolygons; i++) {
        PxHullPolygon polygon;
        mesh->getPolygonData(i, polygon);

        // Draw edges of this polygon
        for (PxU32 j = 0; j < polygon.mNbVerts; j++) {
            PxU32 idx0 = indices[polygon.mIndexBase + j];
            PxU32 idx1 = indices[polygon.mIndexBase + (j + 1) % polygon.mNbVerts];

            PxVec3 v0 = pose.transform(convex.scale.transform(vertices[idx0]));
            PxVec3 v1 = pose.transform(convex.scale.transform(vertices[idx1]));

            drawLine(v0, v1, color);
        }
    }
}

void DebugDrawer::drawTriangleMeshGeometry(const PxTriangleMeshGeometry& mesh, const PxTransform& pose, PxU32 color)
{
    PxTriangleMesh* triangleMesh = mesh.triangleMesh;
    if (!triangleMesh) return;

    const PxVec3* vertices = triangleMesh->getVertices();
    PxU32 nbTriangles = triangleMesh->getNbTriangles();

    // Determine if mesh uses 16-bit or 32-bit indices
    bool has16BitIndices = triangleMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES;

    if (has16BitIndices) {
        const PxU16* indices = reinterpret_cast<const PxU16*>(triangleMesh->getTriangles());

        for (PxU32 i = 0; i < nbTriangles; i++) {
            PxU32 idx0 = indices[i * 3 + 0];
            PxU32 idx1 = indices[i * 3 + 1];
            PxU32 idx2 = indices[i * 3 + 2];

            PxVec3 v0 = pose.transform(mesh.scale.transform(vertices[idx0]));
            PxVec3 v1 = pose.transform(mesh.scale.transform(vertices[idx1]));
            PxVec3 v2 = pose.transform(mesh.scale.transform(vertices[idx2]));

            drawLine(v0, v1, color);
            drawLine(v1, v2, color);
            drawLine(v2, v0, color);
        }
    } else {
        const PxU32* indices = reinterpret_cast<const PxU32*>(triangleMesh->getTriangles());

        for (PxU32 i = 0; i < nbTriangles; i++) {
            PxU32 idx0 = indices[i * 3 + 0];
            PxU32 idx1 = indices[i * 3 + 1];
            PxU32 idx2 = indices[i * 3 + 2];

            PxVec3 v0 = pose.transform(mesh.scale.transform(vertices[idx0]));
            PxVec3 v1 = pose.transform(mesh.scale.transform(vertices[idx1]));
            PxVec3 v2 = pose.transform(mesh.scale.transform(vertices[idx2]));

            drawLine(v0, v1, color);
            drawLine(v1, v2, color);
            drawLine(v2, v0, color);
        }
    }
}

// ============================================================================
// Helper Implementations - ConsoleDebugDrawer
// ============================================================================

void ConsoleDebugDrawer::drawLine(const PxVec3& start, const PxVec3& end, PxU32 color)
{
    printf("Line: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f) [Color: 0x%08X]\n",
           start.x, start.y, start.z, end.x, end.y, end.z, color);
}

void ConsoleDebugDrawer::drawTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2, PxU32 color)
{
    printf("Triangle: (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f) [Color: 0x%08X]\n",
           v0.x, v0.y, v0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, color);
}

void ConsoleDebugDrawer::drawText(const PxVec3& position, const std::string& text, PxU32 color)
{
    printf("Text at (%.2f, %.2f, %.2f): \"%s\" [Color: 0x%08X]\n",
           position.x, position.y, position.z, text.c_str(), color);
}

// ============================================================================
// Helper Implementations - BufferedDebugDrawer
// ============================================================================

void BufferedDebugDrawer::drawLine(const PxVec3& start, const PxVec3& end, PxU32 color)
{
    m_lines.push_back({start, end, color});
}

void BufferedDebugDrawer::drawTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2, PxU32 color)
{
    m_triangles.push_back({{v0, v1, v2}, color});
}

void BufferedDebugDrawer::drawText(const PxVec3& position, const std::string& text, PxU32 color)
{
    m_textLabels.push_back({position, text, color});
}

void BufferedDebugDrawer::clear()
{
    m_lines.clear();
    m_triangles.clear();
    m_textLabels.clear();
}

} // namespace PhysXWrapper
