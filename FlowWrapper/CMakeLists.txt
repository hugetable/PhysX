## FlowWrapper - A Modern C++ Wrapper for NVIDIA Flow
## Copyright (c) 2025. All rights reserved.

cmake_minimum_required(VERSION 3.16)
project(FlowWrapper VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Options
option(FLOWWRAPPER_BUILD_EXAMPLES "Build examples" ON)
option(FLOWWRAPPER_BUILD_TESTS "Build tests" ON)

# Flow Root Directory
if(NOT DEFINED FLOW_ROOT_DIR)
    set(FLOW_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../flow" CACHE PATH "Flow root directory")
endif()

if(NOT EXISTS ${FLOW_ROOT_DIR})
    message(FATAL_ERROR "FLOW_ROOT_DIR does not exist: ${FLOW_ROOT_DIR}")
endif()

message(STATUS "Flow Root: ${FLOW_ROOT_DIR}")

# Flow Include Directories
set(FLOW_INCLUDE_DIRS
    ${FLOW_ROOT_DIR}/include
    ${FLOW_ROOT_DIR}/include/nvflow
    ${FLOW_ROOT_DIR}/include/nvflowext
)

# Flow Library Directory
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(FLOW_LIB_DIR "${FLOW_ROOT_DIR}/_generated/linux-x86_64/debug")
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    set(FLOW_LIB_DIR "${FLOW_ROOT_DIR}/_generated/linux-x86_64/release")
else()
    set(FLOW_LIB_DIR "${FLOW_ROOT_DIR}/_generated/linux-x86_64/release")
endif()

# Flow Libraries
set(FLOW_LIBRARIES
    nvflow
    nvflowext
)

# FlowWrapper Library
file(GLOB_RECURSE FLOWWRAPPER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

file(GLOB_RECURSE FLOWWRAPPER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# Create library target
add_library(FlowWrapper STATIC ${FLOWWRAPPER_SOURCES} ${FLOWWRAPPER_HEADERS})

# Include directories
target_include_directories(FlowWrapper
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${FLOW_INCLUDE_DIRS}
)

# Link Flow libraries
target_link_directories(FlowWrapper PRIVATE ${FLOW_LIB_DIR})
target_link_libraries(FlowWrapper PRIVATE ${FLOW_LIBRARIES})

# Platform-specific settings
if(UNIX AND NOT APPLE)
    target_link_libraries(FlowWrapper PRIVATE pthread dl rt)
elseif(WIN32)
    target_compile_definitions(FlowWrapper PRIVATE WIN32 _WINDOWS)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(FlowWrapper PRIVATE /W4)
else()
    target_compile_options(FlowWrapper PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Examples
if(FLOWWRAPPER_BUILD_EXAMPLES)
    # add_subdirectory(examples)
endif()

# Tests
if(FLOWWRAPPER_BUILD_TESTS)
    enable_testing()
    # add_subdirectory(tests)
endif()

# Install rules
install(TARGETS FlowWrapper
    EXPORT FlowWrapperTargets
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
message(STATUS "=== FlowWrapper Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Examples: ${FLOWWRAPPER_BUILD_EXAMPLES}")
message(STATUS "Build Tests: ${FLOWWRAPPER_BUILD_TESTS}")
message(STATUS "Flow Library Dir: ${FLOW_LIB_DIR}")
message(STATUS "================================")
