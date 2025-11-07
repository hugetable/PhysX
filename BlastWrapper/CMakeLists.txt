## BlastWrapper - A Modern C++ Wrapper for NVIDIA Blast
## Copyright (c) 2025. All rights reserved.

cmake_minimum_required(VERSION 3.16)
project(BlastWrapper VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Options
option(BLASTWRAPPER_BUILD_EXAMPLES "Build examples" ON)
option(BLASTWRAPPER_BUILD_TESTS "Build tests" ON)

# Blast Root Directory
if(NOT DEFINED BLAST_ROOT_DIR)
    set(BLAST_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../blast" CACHE PATH "Blast root directory")
endif()

if(NOT EXISTS ${BLAST_ROOT_DIR})
    message(FATAL_ERROR "BLAST_ROOT_DIR does not exist: ${BLAST_ROOT_DIR}")
endif()

message(STATUS "Blast Root: ${BLAST_ROOT_DIR}")

# Blast Include Directories
set(BLAST_INCLUDE_DIRS
    ${BLAST_ROOT_DIR}/include
    ${BLAST_ROOT_DIR}/include/lowlevel
    ${BLAST_ROOT_DIR}/include/toolkit
    ${BLAST_ROOT_DIR}/include/globals
    ${BLAST_ROOT_DIR}/include/extensions
    ${BLAST_ROOT_DIR}/include/shared
)

# Blast Library Directory
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(BLAST_LIB_DIR "${BLAST_ROOT_DIR}/_repo/linux-x86_64/debug/lib")
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    set(BLAST_LIB_DIR "${BLAST_ROOT_DIR}/_repo/linux-x86_64/release/lib")
else()
    set(BLAST_LIB_DIR "${BLAST_ROOT_DIR}/_repo/linux-x86_64/release/lib")
endif()

# Blast Libraries
set(BLAST_LIBRARIES
    NvBlast
    NvBlastTk
    NvBlastGlobals
    NvBlastExtAuthoring
    NvBlastExtSerialization
)

# BlastWrapper Library
file(GLOB_RECURSE BLASTWRAPPER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

file(GLOB_RECURSE BLASTWRAPPER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

# Create library target
add_library(BlastWrapper STATIC ${BLASTWRAPPER_SOURCES} ${BLASTWRAPPER_HEADERS})

# Include directories
target_include_directories(BlastWrapper
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${BLAST_INCLUDE_DIRS}
)

# Link Blast libraries
target_link_directories(BlastWrapper PRIVATE ${BLAST_LIB_DIR})
target_link_libraries(BlastWrapper PRIVATE ${BLAST_LIBRARIES})

# Platform-specific settings
if(UNIX AND NOT APPLE)
    target_link_libraries(BlastWrapper PRIVATE pthread dl rt)
elseif(WIN32)
    target_compile_definitions(BlastWrapper PRIVATE WIN32 _WINDOWS)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(BlastWrapper PRIVATE /W4)
else()
    target_compile_options(BlastWrapper PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Examples
if(BLASTWRAPPER_BUILD_EXAMPLES)
    # add_subdirectory(examples)
endif()

# Tests
if(BLASTWRAPPER_BUILD_TESTS)
    enable_testing()
    # add_subdirectory(tests)
endif()

# Install rules
install(TARGETS BlastWrapper
    EXPORT BlastWrapperTargets
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
message(STATUS "=== BlastWrapper Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Examples: ${BLASTWRAPPER_BUILD_EXAMPLES}")
message(STATUS "Build Tests: ${BLASTWRAPPER_BUILD_TESTS}")
message(STATUS "Blast Library Dir: ${BLAST_LIB_DIR}")
message(STATUS "================================")
