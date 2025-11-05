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

    # Tests will be added after implementing wrapper classes
    # add_physxwrapper_test(test_physxcore test_physxcore.cpp)

    message(STATUS "Google Test found. Tests configured (to be added after implementing wrapper classes)")
else()
    message(STATUS "Google Test not found. Tests disabled. Install GTest to enable testing.")
endif()
