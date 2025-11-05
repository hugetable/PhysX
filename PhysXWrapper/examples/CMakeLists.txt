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

message(STATUS "PhysXWrapper examples configured successfully (10 examples)")
