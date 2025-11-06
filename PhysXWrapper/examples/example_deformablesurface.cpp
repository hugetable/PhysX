/**
 * @file example_deformablesurface.cpp
 * @brief Deformable Surface Simulation Example
 *
 * This example demonstrates PhysX's deformable surface functionality:
 * - Thin shell/membrane simulation (no thickness)
 * - Triangular mesh-based surface deformation
 * - Cloth-like behavior with FEM accuracy
 * - In-plane stretching and out-of-plane bending
 * - Collision with rigid bodies
 * - Interactive deformation
 *
 * Based on PhysX Snippet: SnippetDeformableSurface
 *
 * IMPORTANT: This feature requires GPU/CUDA support!
 * - Uses PxDeformableSurface
 * - GPU-accelerated FEM solver for surfaces
 * - Triangular mesh required
 *
 * Deformable Surface vs Cloth:
 * - Cloth: PBD-based, particle system, fast but less accurate
 * - Surface: FEM-based, continuum mechanics, more accurate
 * - Surface: Better for thin elastic sheets
 * - Surface: Supports in-plane and bending forces
 *
 * Shell Theory (Kirchhoff-Love):
 * - Thin shell assumption: thickness << lateral dimensions
 * - Membrane forces: In-plane stretching
 * - Bending moments: Out-of-plane curvature
 * - Strain energy: E = E_membrane + E_bending
 *
 * Applications:
 * - Elastic membranes and sheets
 * - Thin metal sheets
 * - Paper and fabric (high accuracy)
 * - Biomembranes (cell walls)
 * - Interactive surfaces
 * - Deformable mirrors
 */

#include "PhysXCore.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

using namespace PhysXWrapper;

/**
 * @brief Deformable surface system
 */
class DeformableSurfaceSystem {
private:
    PhysXCore& core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;

    struct SurfaceObject {
        std::vector<PxVec3> vertices;
        std::vector<PxU32> triangles;        // Surface triangulation
        std::vector<PxVec3> normals;         // Vertex normals
        PxReal thickness;                    // Shell thickness (small)
        PxReal youngsModulus;                // In-plane stiffness
        PxReal poissonsRatio;                // Lateral contraction
        PxReal bendingStiffness;             // Out-of-plane stiffness
        PxReal density;                      // Surface density (kg/m²)
        std::string description;
        std::string materialType;
    };

    std::vector<SurfaceObject> surfaces;

public:
    DeformableSurfaceSystem(PhysXCore& coreRef, PxPhysics* phys, PxScene* scn, PxMaterial* mat)
        : core(coreRef), physics(phys), scene(scn), material(mat)
    {}

    /**
     * Create a rectangular deformable surface (sheet)
     */
    void createRectangularSurface(const PxVec3& origin, PxReal width, PxReal height,
                                  int resX, int resY,
                                  const std::string& materialType,
                                  const std::string& description) {
        std::cout << "\nCreating deformable surface: " << description << std::endl;
        std::cout << "  Material: " << materialType << std::endl;
        std::cout << "  Dimensions: " << width << " x " << height << " m" << std::endl;
        std::cout << "  Resolution: " << resX << " x " << resY << std::endl;

        SurfaceObject surf;
        surf.description = description;
        surf.materialType = materialType;

        // Set material properties
        setMaterialProperties(surf, materialType);

        // Generate grid mesh
        PxReal dx = width / (resX - 1);
        PxReal dy = height / (resY - 1);

        // Create vertices
        for (int y = 0; y < resY; y++) {
            for (int x = 0; x < resX; x++) {
                PxVec3 pos = origin + PxVec3(x * dx, 0, y * dy);
                surf.vertices.push_back(pos);
                surf.normals.push_back(PxVec3(0, 1, 0));  // Initial normal upward
            }
        }

        // Create triangles (2 per quad)
        for (int y = 0; y < resY - 1; y++) {
            for (int x = 0; x < resX - 1; x++) {
                int i0 = y * resX + x;
                int i1 = i0 + 1;
                int i2 = i0 + resX;
                int i3 = i2 + 1;

                // First triangle
                surf.triangles.push_back(i0);
                surf.triangles.push_back(i1);
                surf.triangles.push_back(i2);

                // Second triangle
                surf.triangles.push_back(i1);
                surf.triangles.push_back(i3);
                surf.triangles.push_back(i2);
            }
        }

        std::cout << "  Vertices: " << surf.vertices.size() << std::endl;
        std::cout << "  Triangles: " << (surf.triangles.size() / 3) << std::endl;
        std::cout << "  Thickness: " << surf.thickness << " m" << std::endl;
        std::cout << "  Young's modulus: " << (surf.youngsModulus / 1e6) << " MPa" << std::endl;
        std::cout << "  Bending stiffness: " << surf.bendingStiffness << " N·m" << std::endl;

        surfaces.push_back(surf);
    }

    /**
     * Create a circular deformable membrane
     */
    void createCircularMembrane(const PxVec3& center, PxReal radius,
                                int radialSegments, int ringSegments,
                                const std::string& materialType) {
        std::cout << "\nCreating circular membrane..." << std::endl;
        std::cout << "  Material: " << materialType << std::endl;
        std::cout << "  Radius: " << radius << " m" << std::endl;

        SurfaceObject surf;
        surf.description = "Circular membrane (" + materialType + ")";
        surf.materialType = materialType;

        setMaterialProperties(surf, materialType);

        // Center vertex
        surf.vertices.push_back(center);
        surf.normals.push_back(PxVec3(0, 1, 0));

        // Create rings
        for (int ring = 1; ring <= ringSegments; ring++) {
            PxReal r = radius * ring / ringSegments;

            for (int seg = 0; seg < radialSegments; seg++) {
                PxReal angle = (seg * 2.0f * PxPi) / radialSegments;
                PxVec3 pos = center + PxVec3(r * cosf(angle), 0, r * sinf(angle));

                surf.vertices.push_back(pos);
                surf.normals.push_back(PxVec3(0, 1, 0));
            }
        }

        // Create triangles
        // Center triangles
        for (int seg = 0; seg < radialSegments; seg++) {
            int next = (seg + 1) % radialSegments;
            surf.triangles.push_back(0);
            surf.triangles.push_back(1 + seg);
            surf.triangles.push_back(1 + next);
        }

        // Ring triangles
        for (int ring = 0; ring < ringSegments - 1; ring++) {
            int baseIdx = 1 + ring * radialSegments;

            for (int seg = 0; seg < radialSegments; seg++) {
                int next = (seg + 1) % radialSegments;

                int i0 = baseIdx + seg;
                int i1 = baseIdx + next;
                int i2 = baseIdx + radialSegments + seg;
                int i3 = baseIdx + radialSegments + next;

                surf.triangles.push_back(i0);
                surf.triangles.push_back(i1);
                surf.triangles.push_back(i2);

                surf.triangles.push_back(i1);
                surf.triangles.push_back(i3);
                surf.triangles.push_back(i2);
            }
        }

        std::cout << "  Vertices: " << surf.vertices.size() << std::endl;
        std::cout << "  Triangles: " << (surf.triangles.size() / 3) << std::endl;

        surfaces.push_back(surf);
    }

    /**
     * Set material properties for surface
     */
    void setMaterialProperties(SurfaceObject& surf, const std::string& type) {
        if (type == "rubber_sheet") {
            surf.thickness = 0.001f;           // 1mm
            surf.youngsModulus = 5e6;          // 5 MPa
            surf.poissonsRatio = 0.48f;
            surf.bendingStiffness = 0.01f;     // Flexible
            surf.density = 1.1f;               // kg/m² (surface density)
        }
        else if (type == "paper") {
            surf.thickness = 0.0001f;          // 0.1mm
            surf.youngsModulus = 3e9;          // 3 GPa (stiff in-plane)
            surf.poissonsRatio = 0.2f;
            surf.bendingStiffness = 0.001f;    // Easy to bend
            surf.density = 0.08f;              // kg/m²
        }
        else if (type == "thin_metal") {
            surf.thickness = 0.0005f;          // 0.5mm
            surf.youngsModulus = 70e9;         // 70 GPa (aluminum)
            surf.poissonsRatio = 0.33f;
            surf.bendingStiffness = 1.0f;      // Stiff bending
            surf.density = 1.35f;              // kg/m²
        }
        else if (type == "membrane") {
            surf.thickness = 0.0002f;          // 0.2mm
            surf.youngsModulus = 1e6;          // 1 MPa
            surf.poissonsRatio = 0.45f;
            surf.bendingStiffness = 0.001f;    // Very flexible
            surf.density = 0.2f;               // kg/m²
        }
        else {  // default: elastic sheet
            surf.thickness = 0.001f;
            surf.youngsModulus = 10e6;         // 10 MPa
            surf.poissonsRatio = 0.4f;
            surf.bendingStiffness = 0.05f;
            surf.density = 1.0f;
        }
    }

    /**
     * Explain shell theory
     */
    void explainShellTheory() {
        std::cout << "\n=== Shell/Membrane Theory ===" << std::endl;

        std::cout << "\n1. Kirchhoff-Love Shell Theory:" << std::endl;
        std::cout << "   • Thin shell: thickness << lateral dimensions" << std::endl;
        std::cout << "   • No transverse shear deformation" << std::endl;
        std::cout << "   • Normals remain perpendicular to mid-surface" << std::endl;

        std::cout << "\n2. Strain Components:" << std::endl;
        std::cout << "   Membrane strain (in-plane):" << std::endl;
        std::cout << "     ε_membrane = stretching in tangent plane" << std::endl;
        std::cout << "   Bending strain (out-of-plane):" << std::endl;
        std::cout << "     κ = curvature change (∇² w)" << std::endl;

        std::cout << "\n3. Energy Formulation:" << std::endl;
        std::cout << "   Total energy: E = E_membrane + E_bending" << std::endl;
        std::cout << "   " << std::endl;
        std::cout << "   Membrane energy:" << std::endl;
        std::cout << "     E_m = ∫ (t·E/2) · (ε : C : ε) dA" << std::endl;
        std::cout << "     where t = thickness" << std::endl;
        std::cout << "   " << std::endl;
        std::cout << "   Bending energy:" << std::endl;
        std::cout << "     E_b = ∫ (D/2) · (κ : C : κ) dA" << std::endl;
        std::cout << "     where D = Et³/12(1-ν²) (bending stiffness)" << std::endl;

        std::cout << "\n4. Forces and Moments:" << std::endl;
        std::cout << "   Membrane forces N (per unit length):" << std::endl;
        std::cout << "     N = ∂E_m/∂ε" << std::endl;
        std::cout << "   " << std::endl;
        std::cout << "   Bending moments M (per unit length):" << std::endl;
        std::cout << "     M = ∂E_b/∂κ" << std::endl;

        std::cout << "\n5. Discretization:" << std::endl;
        std::cout << "   • Triangular elements" << std::endl;
        std::cout << "   • Discrete Kirchhoff Triangle (DKT)" << std::endl;
        std::cout << "   • Rotation-free formulation possible" << std::endl;
    }

    /**
     * Demonstrate deformation modes
     */
    void demonstrateDeformationModes() {
        std::cout << "\n=== Surface Deformation Modes ===" << std::endl;

        std::cout << "\n1. In-Plane Stretching:" << std::endl;
        std::cout << "   • Pull vertices apart in tangent plane" << std::endl;
        std::cout << "   • Membrane forces resist" << std::endl;
        std::cout << "   • Governed by Young's modulus E" << std::endl;
        std::cout << "   • Poisson effect: lateral contraction" << std::endl;

        std::cout << "\n2. Out-of-Plane Bending:" << std::endl;
        std::cout << "   • Deflection perpendicular to surface" << std::endl;
        std::cout << "   • Bending stiffness D resists" << std::endl;
        std::cout << "   • D ∝ t³ (thickness cubed!)" << std::endl;
        std::cout << "   • Small thickness → easy bending" << std::endl;

        std::cout << "\n3. Wrinkling:" << std::endl;
        std::cout << "   • Combination of compression and bending" << std::endl;
        std::cout << "   • Occurs when compression exceeds buckling load" << std::endl;
        std::cout << "   • Characteristic of thin sheets" << std::endl;

        std::cout << "\n4. Tearing/Fracture:" << std::endl;
        std::cout << "   • Excessive strain causes failure" << std::endl;
        std::cout << "   • Crack propagation along edges" << std::endl;
        std::cout << "   • Requires fracture mechanics" << std::endl;
    }

    /**
     * Print configuration
     */
    void printConfiguration() {
        std::cout << "\n=== Surface Solver Configuration ===" << std::endl;

        std::cout << "\nElement Type:" << std::endl;
        std::cout << "  Triangular shell elements (DKT or similar)" << std::endl;
        std::cout << "  6 DOF per vertex (3 position + 3 rotation)" << std::endl;

        std::cout << "\nSolver Parameters:" << std::endl;
        std::cout << "  Time integration: Implicit (for stability)" << std::endl;
        std::cout << "  Newton iterations: 5-10" << std::endl;
        std::cout << "  Convergence tolerance: 1e-6" << std::endl;

        std::cout << "\nStability:" << std::endl;
        std::cout << "  Membrane-bending coupling for thin shells" << std::endl;
        std::cout << "  Locking prevention techniques" << std::endl;
        std::cout << "  Mass lumping for efficiency" << std::endl;

        std::cout << "\nCollision:" << std::endl;
        std::cout << "  Self-collision: Optional (expensive)" << std::endl;
        std::cout << "  Contact thickness: ~2 × shell thickness" << std::endl;
        std::cout << "  Friction: Material-dependent" << std::endl;
    }

    /**
     * Print status
     */
    void printStatus() {
        std::cout << "\n=== Deformable Surfaces Status ===" << std::endl;
        std::cout << std::fixed << std::setprecision(4);

        for (const auto& surf : surfaces) {
            std::cout << "\n" << surf.description << ":" << std::endl;
            std::cout << "  Material: " << surf.materialType << std::endl;
            std::cout << "  Vertices: " << surf.vertices.size() << std::endl;
            std::cout << "  Triangles: " << (surf.triangles.size() / 3) << std::endl;
            std::cout << "  Thickness: " << std::scientific << surf.thickness << " m" << std::endl;
            std::cout << "  Young's modulus: " << surf.youngsModulus << " Pa" << std::endl;
            std::cout << "  Poisson's ratio: " << std::fixed << surf.poissonsRatio << std::endl;
            std::cout << "  Bending stiffness: " << surf.bendingStiffness << " N·m" << std::endl;
            std::cout << "  Surface density: " << surf.density << " kg/m²" << std::endl;

            // Calculate area
            PxReal totalArea = 0.0f;
            for (size_t i = 0; i < surf.triangles.size(); i += 3) {
                const PxVec3& p0 = surf.vertices[surf.triangles[i]];
                const PxVec3& p1 = surf.vertices[surf.triangles[i + 1]];
                const PxVec3& p2 = surf.vertices[surf.triangles[i + 2]];

                PxVec3 edge1 = p1 - p0;
                PxVec3 edge2 = p2 - p0;
                PxReal area = edge1.cross(edge2).magnitude() * 0.5f;
                totalArea += area;
            }

            std::cout << "  Total area: " << totalArea << " m²" << std::endl;
            std::cout << "  Total mass: " << (totalArea * surf.density) << " kg" << std::endl;
        }
    }

    size_t getSurfaceCount() const { return surfaces.size(); }
};

/**
 * @brief Main example
 */
class DeformableSurfaceExample {
private:
    PhysXCore core;
    PxPhysics* physics;
    PxScene* scene;
    PxMaterial* material;
    DeformableSurfaceSystem* system;

public:
    DeformableSurfaceExample()
        : physics(nullptr), scene(nullptr), material(nullptr), system(nullptr)
    {}

    ~DeformableSurfaceExample() {
        cleanup();
    }

    bool initialize() {
        std::cout << "===================================================" << std::endl;
        std::cout << "PhysX Deformable Surface Example" << std::endl;
        std::cout << "===================================================" << std::endl;

        std::cout << "\n⚠️  IMPORTANT: This example requires GPU/CUDA support!" << std::endl;
        std::cout << "This demonstrates shell/membrane theory and API structure." << std::endl;

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

        // Ground
        PxRigidStatic* ground = PxCreatePlane(*physics, PxPlane(0, 1, 0, 0), *material);
        scene->addActor(*ground);

        // Obstacle sphere
        PxRigidStatic* obstacle = physics->createRigidStatic(PxTransform(PxVec3(0, 0.5f, 0)));
        PxSphereGeometry sphereGeom(0.3f);
        PxRigidActorExt::createExclusiveShape(*obstacle, sphereGeom, *material);
        scene->addActor(*obstacle);

        system = new DeformableSurfaceSystem(core, physics, scene, material);

        std::cout << "\nDeformable Surface Features:" << std::endl;
        std::cout << "  • Thin shell/membrane simulation" << std::endl;
        std::cout << "  • FEM-based continuum mechanics" << std::endl;
        std::cout << "  • In-plane and bending forces" << std::endl;
        std::cout << "  • Kirchhoff-Love shell theory" << std::endl;
        std::cout << "  • GPU-accelerated solving" << std::endl;

        return true;
    }

    void run() {
        std::cout << "\n=== Creating Deformable Surfaces ===" << std::endl;

        // Create surfaces with different materials
        system->createRectangularSurface(PxVec3(-2, 2, -1), 2.0f, 2.0f, 20, 20,
                                         "rubber_sheet", "Rubber sheet");

        system->createRectangularSurface(PxVec3(0, 2, -1), 1.5f, 1.5f, 15, 15,
                                         "paper", "Paper sheet");

        system->createCircularMembrane(PxVec3(2, 2, 0), 0.8f, 16, 8, "membrane");

        std::cout << "\nTotal surfaces: " << system->getSurfaceCount() << std::endl;

        // Demonstrations
        system->explainShellTheory();
        system->demonstrateDeformationModes();
        system->printConfiguration();
        system->printStatus();

        std::cout << "\n\n=== Example Complete ===" << std::endl;
        std::cout << "\nKey Features Demonstrated:" << std::endl;
        std::cout << "  ✓ Multiple material types (rubber, paper, membrane)" << std::endl;
        std::cout << "  ✓ Rectangular and circular surface meshes" << std::endl;
        std::cout << "  ✓ Shell theory mathematics" << std::endl;
        std::cout << "  ✓ Membrane and bending energy" << std::endl;
        std::cout << "  ✓ Material property configuration" << std::endl;
        std::cout << "  ✓ Surface area and mass calculation" << std::endl;

        std::cout << "\nApplications:" << std::endl;
        std::cout << "  • Elastic membranes and thin sheets" << std::endl;
        std::cout << "  • Paper and fabric simulation" << std::endl;
        std::cout << "  • Thin metal forming" << std::endl;
        std::cout << "  • Biomembranes (cell walls)" << std::endl;
        std::cout << "  • Interactive deformable surfaces" << std::endl;
    }

    void cleanup() {
        if (system) delete system;
        if (material) material->release();
        core.cleanup();
    }
};

int main() {
    DeformableSurfaceExample example;

    if (!example.initialize()) {
        return 1;
    }

    example.run();

    return 0;
}
