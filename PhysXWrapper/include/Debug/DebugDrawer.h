/**
 * @file DebugDrawer.h
 * @brief Physics debug visualization system for PhysX
 *
 * This class provides utilities for visualizing physics data:
 * - Draw collision shapes and wireframes
 * - Visualize contacts and joints
 * - Display bounding boxes and centers of mass
 * - Show velocity and force vectors
 * - Render debug information
 *
 * Debug drawing is useful for:
 * - Visual debugging of physics behavior
 * - Understanding collision and contact points
 * - Verifying joint constraints
 * - Performance profiling
 * - Educational purposes
 *
 * @author PhysXWrapper
 * @date 2025-11-05
 */

#pragma once

#include "PxPhysicsAPI.h"
#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace PhysXWrapper {

using namespace physx;

/**
 * @brief Debug color presets
 */
struct DebugColors {
    static constexpr PxU32 RED = 0xFF0000FF;
    static constexpr PxU32 GREEN = 0x00FF00FF;
    static constexpr PxU32 BLUE = 0x0000FFFF;
    static constexpr PxU32 YELLOW = 0xFFFF00FF;
    static constexpr PxU32 CYAN = 0x00FFFFFF;
    static constexpr PxU32 MAGENTA = 0xFF00FFFF;
    static constexpr PxU32 WHITE = 0xFFFFFFFF;
    static constexpr PxU32 BLACK = 0x000000FF;
    static constexpr PxU32 ORANGE = 0xFF8000FF;
    static constexpr PxU32 PURPLE = 0x8000FFFF;
};

/**
 * @brief Debug draw flags
 */
enum class DebugDrawFlag : PxU32 {
    NONE = 0,
    SHAPES = 1 << 0,                    ///< Draw collision shapes
    CONTACTS = 1 << 1,                  ///< Draw contact points
    JOINTS = 1 << 2,                    ///< Draw joints
    AABBS = 1 << 3,                     ///< Draw bounding boxes
    COM = 1 << 4,                       ///< Draw centers of mass
    VELOCITIES = 1 << 5,                ///< Draw velocity vectors
    FORCES = 1 << 6,                    ///< Draw force vectors
    NORMALS = 1 << 7,                   ///< Draw surface normals
    AXES = 1 << 8,                      ///< Draw coordinate axes
    ALL = 0xFFFFFFFF                    ///< Draw everything
};

inline DebugDrawFlag operator|(DebugDrawFlag a, DebugDrawFlag b) {
    return static_cast<DebugDrawFlag>(static_cast<PxU32>(a) | static_cast<PxU32>(b));
}

inline bool operator&(DebugDrawFlag a, DebugDrawFlag b) {
    return (static_cast<PxU32>(a) & static_cast<PxU32>(b)) != 0;
}

/**
 * @brief Line primitive
 */
struct DebugLine {
    PxVec3 start;
    PxVec3 end;
    PxU32 color;
};

/**
 * @brief Triangle primitive
 */
struct DebugTriangle {
    PxVec3 vertices[3];
    PxU32 color;
};

/**
 * @brief Text label
 */
struct DebugText {
    PxVec3 position;
    std::string text;
    PxU32 color;
};

/**
 * @brief Debug draw callback interface
 */
class DebugDrawCallback {
public:
    virtual ~DebugDrawCallback() = default;

    /**
     * @brief Draw line
     * @param start Start point
     * @param end End point
     * @param color Color (RGBA)
     */
    virtual void drawLine(const PxVec3& start, const PxVec3& end, PxU32 color) = 0;

    /**
     * @brief Draw triangle
     * @param v0 Vertex 0
     * @param v1 Vertex 1
     * @param v2 Vertex 2
     * @param color Color (RGBA)
     */
    virtual void drawTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2, PxU32 color) = 0;

    /**
     * @brief Draw text
     * @param position Position
     * @param text Text string
     * @param color Color (RGBA)
     */
    virtual void drawText(const PxVec3& position, const std::string& text, PxU32 color) = 0;
};

/**
 * @brief Debug drawer class
 *
 * This class provides comprehensive debug visualization:
 * - Collision shape wireframes
 * - Contact points and normals
 * - Joint connections and limits
 * - Bounding boxes
 * - Velocity and force vectors
 * - Centers of mass
 * - Coordinate axes
 *
 * @example
 * @code
 * // Create drawer
 * DebugDrawer drawer;
 * drawer.setDrawCallback(myCallback);
 *
 * // Enable various debug visualizations
 * drawer.setFlags(DebugDrawFlag::SHAPES | DebugDrawFlag::CONTACTS);
 *
 * // Draw scene
 * drawer.drawScene(scene);
 *
 * // Or draw individual actors
 * drawer.drawActor(actor);
 * drawer.drawJoint(joint);
 *
 * // Custom shapes
 * drawer.drawBox(center, halfExtents, color);
 * drawer.drawSphere(center, radius, color);
 * drawer.drawArrow(start, direction, length, color);
 * @endcode
 */
class DebugDrawer {
public:
    /**
     * @brief Constructor
     */
    DebugDrawer();

    /**
     * @brief Destructor
     */
    ~DebugDrawer();

    // No copy
    DebugDrawer(const DebugDrawer&) = delete;
    DebugDrawer& operator=(const DebugDrawer&) = delete;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set draw callback
     * @param callback Callback for rendering primitives
     */
    void setDrawCallback(DebugDrawCallback* callback);

    /**
     * @brief Set draw flags
     * @param flags Flags controlling what to draw
     */
    void setFlags(DebugDrawFlag flags);

    /**
     * @brief Get current flags
     */
    DebugDrawFlag getFlags() const;

    /**
     * @brief Enable flag
     */
    void enableFlag(DebugDrawFlag flag);

    /**
     * @brief Disable flag
     */
    void disableFlag(DebugDrawFlag flag);

    // ========================================================================
    // Scene Drawing
    // ========================================================================

    /**
     * @brief Draw entire scene
     * @param scene Scene to visualize
     */
    void drawScene(PxScene* scene);

    /**
     * @brief Draw actor
     * @param actor Actor to visualize
     */
    void drawActor(PxRigidActor* actor);

    /**
     * @brief Draw shape
     * @param shape Shape to visualize
     * @param pose Shape world pose
     * @param color Color
     */
    void drawShape(PxShape* shape, const PxTransform& pose, PxU32 color = DebugColors::WHITE);

    /**
     * @brief Draw joint
     * @param joint Joint to visualize
     */
    void drawJoint(PxJoint* joint);

    // ========================================================================
    // Primitive Drawing
    // ========================================================================

    /**
     * @brief Draw line
     */
    void drawLine(const PxVec3& start, const PxVec3& end, PxU32 color = DebugColors::WHITE);

    /**
     * @brief Draw box wireframe
     */
    void drawBox(const PxVec3& center, const PxVec3& halfExtents,
                 const PxQuat& rotation, PxU32 color = DebugColors::WHITE);

    /**
     * @brief Draw sphere wireframe
     */
    void drawSphere(const PxVec3& center, PxReal radius,
                    PxU32 color = DebugColors::WHITE, PxU32 segments = 16);

    /**
     * @brief Draw capsule wireframe
     */
    void drawCapsule(const PxVec3& center, PxReal radius, PxReal halfHeight,
                     const PxQuat& rotation, PxU32 color = DebugColors::WHITE);

    /**
     * @brief Draw arrow
     */
    void drawArrow(const PxVec3& start, const PxVec3& direction,
                   PxReal length, PxU32 color = DebugColors::WHITE);

    /**
     * @brief Draw coordinate axes
     */
    void drawAxes(const PxTransform& pose, PxReal scale = 1.0f);

    /**
     * @brief Draw bounding box
     */
    void drawBounds(const PxBounds3& bounds, PxU32 color = DebugColors::YELLOW);

    /**
     * @brief Draw text
     */
    void drawText(const PxVec3& position, const std::string& text,
                  PxU32 color = DebugColors::WHITE);

    // ========================================================================
    // Utility Drawing
    // ========================================================================

    /**
     * @brief Draw contact points
     */
    void drawContacts(PxScene* scene);

    /**
     * @brief Draw velocity vectors
     */
    void drawVelocities(PxScene* scene);

    /**
     * @brief Draw centers of mass
     */
    void drawCentersOfMass(PxScene* scene);

    /**
     * @brief Clear all buffered primitives
     */
    void clear();

    /**
     * @brief Flush buffered primitives to callback
     */
    void flush();

    // ========================================================================
    // Batch Collection
    // ========================================================================

    /**
     * @brief Get collected lines
     */
    const std::vector<DebugLine>& getLines() const;

    /**
     * @brief Get collected triangles
     */
    const std::vector<DebugTriangle>& getTriangles() const;

    /**
     * @brief Get collected text labels
     */
    const std::vector<DebugText>& getTextLabels() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    // Helper methods
    void drawGeometry(const PxGeometry& geometry, const PxTransform& pose, PxU32 color);
    void drawBoxGeometry(const PxBoxGeometry& box, const PxTransform& pose, PxU32 color);
    void drawSphereGeometry(const PxSphereGeometry& sphere, const PxTransform& pose, PxU32 color);
    void drawCapsuleGeometry(const PxCapsuleGeometry& capsule, const PxTransform& pose, PxU32 color);
    void drawPlaneGeometry(const PxPlaneGeometry& plane, const PxTransform& pose, PxU32 color);
    void drawConvexMeshGeometry(const PxConvexMeshGeometry& convex, const PxTransform& pose, PxU32 color);
    void drawTriangleMeshGeometry(const PxTriangleMeshGeometry& mesh, const PxTransform& pose, PxU32 color);
};

/**
 * @brief Simple console debug drawer (prints to stdout)
 */
class ConsoleDebugDrawer : public DebugDrawCallback {
public:
    void drawLine(const PxVec3& start, const PxVec3& end, PxU32 color) override;
    void drawTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2, PxU32 color) override;
    void drawText(const PxVec3& position, const std::string& text, PxU32 color) override;
};

/**
 * @brief Buffered debug drawer (collects primitives)
 */
class BufferedDebugDrawer : public DebugDrawCallback {
public:
    void drawLine(const PxVec3& start, const PxVec3& end, PxU32 color) override;
    void drawTriangle(const PxVec3& v0, const PxVec3& v1, const PxVec3& v2, PxU32 color) override;
    void drawText(const PxVec3& position, const std::string& text, PxU32 color) override;

    const std::vector<DebugLine>& getLines() const { return m_lines; }
    const std::vector<DebugTriangle>& getTriangles() const { return m_triangles; }
    const std::vector<DebugText>& getTextLabels() const { return m_textLabels; }

    void clear();

private:
    std::vector<DebugLine> m_lines;
    std::vector<DebugTriangle> m_triangles;
    std::vector<DebugText> m_textLabels;
};

} // namespace PhysXWrapper
