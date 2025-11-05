## PhysXWrapper Tests

# Find Google Test (if available)
find_package(GTest QUIET)

if(GTest_FOUND)
    # Macro to create test executables
    macro(add_physxwrapper_test TEST_NAME)
        add_executable(${TEST_NAME} ${ARGN})
        target_link_libraries(${TEST_NAME} PRIVATE
            PhysXWrapper
            GTest::GTest
            GTest::Main
        )
        target_include_directories(${TEST_NAME} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${PHYSX_INCLUDE_DIRS}
        )

        # Link PhysX libraries
        target_link_directories(${TEST_NAME} PRIVATE ${PHYSX_LIB_DIR})
        target_link_libraries(${TEST_NAME} PRIVATE ${PHYSX_LIBRARIES})

        if(UNIX AND NOT APPLE)
            target_link_libraries(${TEST_NAME} PRIVATE pthread dl rt)
        endif()

        gtest_discover_tests(${TEST_NAME})
    endmacro()

    # ========================================================================
    # Unit Tests
    # ========================================================================

    # Core functionality tests
    add_physxwrapper_test(test_physxcore unit/test_physxcore.cpp)

    # RigidBody tests
    add_physxwrapper_test(test_rigidbody unit/test_rigidbody.cpp)

    # Geometry tests
    add_physxwrapper_test(test_geometry unit/test_geometry.cpp)

    # Joint tests
    add_physxwrapper_test(test_joints unit/test_joints.cpp)

    # Query tests
    add_physxwrapper_test(test_queries unit/test_queries.cpp)

    # Utility tests
    add_physxwrapper_test(test_utilities unit/test_utilities.cpp)

    # ========================================================================
    # Integration Tests
    # ========================================================================

    # Basic integration tests
    add_physxwrapper_test(test_integration_basic integration/test_integration_basic.cpp)

    # Performance integration tests
    add_physxwrapper_test(test_integration_performance integration/test_integration_performance.cpp)

    message(STATUS "Google Test found. Configured 8 test suites with comprehensive coverage")
else()
    message(STATUS "Google Test not found. Tests disabled. Install GTest to enable testing.")
endif()
