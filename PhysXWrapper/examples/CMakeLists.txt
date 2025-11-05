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

message(STATUS "PhysXWrapper examples configured successfully")
