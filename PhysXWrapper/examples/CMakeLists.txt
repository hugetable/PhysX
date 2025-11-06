## PhysXWrapper Examples

# Macro to create example executables
macro(add_physxwrapper_example EXAMPLE_NAME)
    add_executable(${EXAMPLE_NAME} ${ARGN})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE PhysXWrapper)
    target_include_directories(${EXAMPLE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${PHYSX_INCLUDE_DIRS}
    )

    # Link PhysX libraries directly for examples
    target_link_directories(${EXAMPLE_NAME} PRIVATE ${PHYSX_LIB_DIR})
    target_link_libraries(${EXAMPLE_NAME} PRIVATE ${PHYSX_LIBRARIES})

    if(UNIX AND NOT APPLE)
        target_link_libraries(${EXAMPLE_NAME} PRIVATE pthread dl rt)
    endif()
endmacro()

# Example 1: Hello World
add_physxwrapper_example(example_helloworld example_helloworld.cpp)

# Example 2: Contact Report
add_physxwrapper_example(example_contactreport example_contactreport.cpp)

# Example 3: Geometry Query
add_physxwrapper_example(example_geometryquery example_geometryquery.cpp)

# Example 4: Convex Mesh Builder
add_physxwrapper_example(example_convexmesh example_convexmesh.cpp)

# Example 5: Triangle Mesh Builder
add_physxwrapper_example(example_trianglemesh example_trianglemesh.cpp)

# Example 6: Trigger Volumes
add_physxwrapper_example(example_trigger example_trigger.cpp)

# Example 7: Continuous Collision Detection (CCD)
add_physxwrapper_example(example_ccd example_ccd.cpp)

# Example 8: Joint System
add_physxwrapper_example(example_joint example_joint.cpp)

# Example 9: Articulation System
add_physxwrapper_example(example_articulation example_articulation.cpp)

# Example 10: Deformable Volume (Soft Body) - Requires GPU/CUDA
add_physxwrapper_example(example_deformable example_deformable.cpp)

# Example 11: Contact Modifier (Runtime Contact Modification)
add_physxwrapper_example(example_contactmodifier example_contactmodifier.cpp)

# Example 12: PBD Fluid Simulation - Requires GPU/CUDA
add_physxwrapper_example(example_pbdfluid example_pbdfluid.cpp)

# Example 13: Frustum Query (View-Frustum Culling)
add_physxwrapper_example(example_frustum example_frustum.cpp)

# Example 14: Point Distance Query
add_physxwrapper_example(example_pointdistance example_pointdistance.cpp)

# Example 15: Debug Drawer (Physics Visualization)
add_physxwrapper_example(example_debug example_debug.cpp)

# Example 16: BVH Builder (Spatial Acceleration)
add_physxwrapper_example(example_bvh example_bvh.cpp)

# Example 17: Gyroscopic Forces (Dzhanibekov Effect)
add_physxwrapper_example(example_gyroscopic example_gyroscopic.cpp)

# Example 18: Collection Loader (Batch Loading)
add_physxwrapper_example(example_collection example_collection.cpp)

# Example 19: Character Controller (Kinematic Character Movement)
add_physxwrapper_example(example_character example_character.cpp)

# Example 20: Aggregate Manager (Actor Grouping for Performance)
add_physxwrapper_example(example_aggregate example_aggregate.cpp)

# Example 21: Vehicle Manager (Wheeled Vehicle Physics)
add_physxwrapper_example(example_vehicle example_vehicle.cpp)

# Example 22: Material Library (Predefined Physical Materials)
add_physxwrapper_example(example_material example_material.cpp)

# Example 23: Physics Recorder (Recording and Playback)
add_physxwrapper_example(example_recorder example_recorder.cpp)

# Example 24: Performance Profiler (Performance Analysis and Monitoring)
add_physxwrapper_example(example_profiler example_profiler.cpp)

# Example 25: Joint Drive (D6 Joint Drive System with Velocity/Position Control)
add_physxwrapper_example(example_jointdrive example_jointdrive.cpp)

# Example 26: Tolerance Scale (Unit System Configuration - meters vs centimeters)
add_physxwrapper_example(example_tolerancescale example_tolerancescale.cpp)

# Example 27: Stepper (Custom Time Stepping and Substepping for Accuracy)
add_physxwrapper_example(example_stepper example_stepper.cpp)

# Example 28: Mass Properties (Mass Calculation, Inertia Tensors, CoM)
add_physxwrapper_example(example_massproperties example_massproperties.cpp)

# Example 29: Multithreading (Thread Pool, Performance Scaling, Parallel Queries)
add_physxwrapper_example(example_multithreading example_multithreading.cpp)

# Example 30: Custom Joint (PxConstraintConnector Implementation - Rope Joint)
add_physxwrapper_example(example_customjoint example_customjoint.cpp)

# Example 31: Gear Joint (Gear Ratio Constraint, Gear Trains, Mechanical Transmissions)
add_physxwrapper_example(example_gearjoint example_gearjoint.cpp)

# Example 32: Rack and Pinion Joint (Rotation to Linear Motion, Steering Systems)
add_physxwrapper_example(example_rackjoint example_rackjoint.cpp)

# Example 33: Mimic Joint (Follower Joint, Robotic Grippers, Coupled Motions)
add_physxwrapper_example(example_mimicjoint example_mimicjoint.cpp)

# Example 34: Fixed Tendon (Articulation Tendon Coupling, Cable-Driven Mechanisms)
add_physxwrapper_example(example_fixedtendon example_fixedtendon.cpp)

# Example 35: Spatial Tendon (3D Cable Routing, Pulley Systems, Tendon-Driven Robots)
add_physxwrapper_example(example_spatialtendon example_spatialtendon.cpp)

# Example 36: Immediate Mode Articulation (Direct FK/ID Computation, Predictive Control)
add_physxwrapper_example(example_immediatearticulation example_immediatearticulation.cpp)

# Example 37: PBD Cloth (Position-Based Dynamics Cloth, GPU-Accelerated)
add_physxwrapper_example(example_pbdcloth example_pbdcloth.cpp)

# Example 38: PBD Inflatable (Volume Constraints, Pressure Simulation, Balloons)
add_physxwrapper_example(example_pbdinflatable example_pbdinflatable.cpp)

# Example 39: Deformable Mesh (FEM Soft Bodies, Tetrahedral Mesh, Elastic Materials)
add_physxwrapper_example(example_deformablemesh example_deformablemesh.cpp)

message(STATUS "PhysXWrapper examples configured successfully (39 examples)")
