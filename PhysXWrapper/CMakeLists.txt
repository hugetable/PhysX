## PhysXWrapper - A Modern C++ Wrapper for NVIDIA PhysX
## Copyright (c) 2025. All rights reserved.

cmake_minimum_required(VERSION 3.16)
project(PhysXWrapper VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Options
option(PHYSXWRAPPER_BUILD_EXAMPLES "Build examples" ON)
option(PHYSXWRAPPER_BUILD_TESTS "Build tests" ON)
option(PHYSXWRAPPER_ENABLE_GPU "Enable GPU support" OFF)

# PhysX Root Directory
if(NOT DEFINED PHYSX_ROOT_DIR)
    set(PHYSX_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../physx" CACHE PATH "PhysX root directory")
endif()

if(NOT EXISTS ${PHYSX_ROOT_DIR})
    message(FATAL_ERROR "PHYSX_ROOT_DIR does not exist: ${PHYSX_ROOT_DIR}")
endif()

message(STATUS "PhysX Root: ${PHYSX_ROOT_DIR}")

# PhysX Include Directories
set(PHYSX_INCLUDE_DIRS
    ${PHYSX_ROOT_DIR}/include
    ${PHYSX_ROOT_DIR}/snippets/snippetcommon
    ${PHYSX_ROOT_DIR}/snippets/snippetutils
)

# PhysX Library Directory (adjust based on your build configuration)
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(PHYSX_LIB_DIR "${PHYSX_ROOT_DIR}/bin/linux.clang/debug")
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    set(PHYSX_LIB_DIR "${PHYSX_ROOT_DIR}/bin/linux.clang/release")
else()
    set(PHYSX_LIB_DIR "${PHYSX_ROOT_DIR}/bin/linux.clang/checked")
endif()

# PhysX Libraries
set(PHYSX_LIBRARIES
    PhysX
    PhysXCommon
    PhysXFoundation
    PhysXExtensions
    PhysXCooking
    PhysXPvdSDK
)

# If GPU support is enabled
if(PHYSXWRAPPER_ENABLE_GPU)
    list(APPEND PHYSX_LIBRARIES PhysXGpu)
    add_definitions(-DPHYSXWRAPPER_GPU_ENABLED)
endif()

# PhysXWrapper Library
file(GLOB_RECURSE PHYSXWRAPPER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

file(GLOB_RECURSE PHYSXWRAPPER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# Create library target
add_library(PhysXWrapper STATIC ${PHYSXWRAPPER_SOURCES} ${PHYSXWRAPPER_HEADERS})

# Include directories
target_include_directories(PhysXWrapper
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${PHYSX_INCLUDE_DIRS}
)

# Link PhysX libraries
target_link_directories(PhysXWrapper PRIVATE ${PHYSX_LIB_DIR})
target_link_libraries(PhysXWrapper PRIVATE ${PHYSX_LIBRARIES})

# Platform-specific settings
if(UNIX AND NOT APPLE)
    target_link_libraries(PhysXWrapper PRIVATE pthread dl rt)
elseif(WIN32)
    target_compile_definitions(PhysXWrapper PRIVATE WIN32 _WINDOWS)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(PhysXWrapper PRIVATE /W4)
else()
    target_compile_options(PhysXWrapper PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Examples
if(PHYSXWRAPPER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(PHYSXWRAPPER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install rules
install(TARGETS PhysXWrapper
    EXPORT PhysXWrapperTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Print configuration summary
message(STATUS "=== PhysXWrapper Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Examples: ${PHYSXWRAPPER_BUILD_EXAMPLES}")
message(STATUS "Build Tests: ${PHYSXWRAPPER_BUILD_TESTS}")
message(STATUS "GPU Support: ${PHYSXWRAPPER_ENABLE_GPU}")
message(STATUS "PhysX Library Dir: ${PHYSX_LIB_DIR}")
message(STATUS "================================")
