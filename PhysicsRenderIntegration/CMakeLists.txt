# PhysicsRenderIntegration - PhysX + Flow + Blast + OptiX 9.0
# Copyright (c) 2025, Integration Project

cmake_minimum_required(VERSION 3.23)

project(PhysicsRenderIntegration LANGUAGES CXX CUDA)

# C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

# CMake 模块路径
set(CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/../OptiX_Apps/3rdparty/CMake"
    ${CMAKE_MODULE_PATH}
)

message(STATUS "CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}")

# ============================================================================
# 查找依赖库
# ============================================================================

# CUDA Toolkit
find_package(CUDAToolkit 11.0 REQUIRED)
message(STATUS "CUDA Version: ${CUDAToolkit_VERSION}")

# OptiX SDK 9.0
find_package(OptiX90 REQUIRED)
message(STATUS "OptiX 9.0 Include: ${OptiX90_INCLUDE}")

# PhysX 5.x
set(PHYSX_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "PhysX root directory")
find_package(PhysX REQUIRED)

# Flow (可选)
find_package(Flow)
if(Flow_FOUND)
    message(STATUS "Flow library found")
    set(ENABLE_FLOW ON)
else()
    message(STATUS "Flow library not found - Flow integration disabled")
    set(ENABLE_FLOW OFF)
endif()

# Blast (可选)
find_package(Blast)
if(Blast_FOUND)
    message(STATUS "Blast library found")
    set(ENABLE_BLAST ON)
else()
    message(STATUS "Blast library not found - Blast integration disabled")
    set(ENABLE_BLAST OFF)
endif()

# OpenGL & Window Libraries
find_package(OpenGL REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(GLEW REQUIRED)

# Image Library
find_package(DevIL_1_8_0 REQUIRED)

# Assimp (模型加载)
find_package(ASSIMP REQUIRED)

# ============================================================================
# OptiX 着色器编译
# ============================================================================

# 包含 OptiX_Apps 的编译宏
include("${CMAKE_CURRENT_SOURCE_DIR}/../OptiX_Apps/3rdparty/CMake/nvcuda_compile_module.cmake")

# CUDA 架构
if(${CUDAToolkit_VERSION} VERSION_GREATER_EQUAL "13.0")
    set(SM_COMPUTE compute_75)
else()
    set(SM_COMPUTE compute_50)
endif()

message(STATUS "SM_COMPUTE set to ${SM_COMPUTE}")

# OptiX-IR 支持 (OptiX 7.5+ & CUDA 11.7+)
if(${OptiX90_VERSION} VERSION_GREATER_EQUAL "7.5" AND
   ${CUDAToolkit_VERSION} VERSION_GREATER_EQUAL "11.7")
    set(USE_OPTIX_IR ON)
    set(OPTIX_MODULE_EXTENSION ".optixir")
    set(OPTIX_PROGRAM_TARGET "--optixir")
    message(STATUS "Using OptiX-IR")
else()
    set(USE_OPTIX_IR OFF)
    set(OPTIX_MODULE_EXTENSION ".ptx")
    set(OPTIX_PROGRAM_TARGET "--ptx")
    message(STATUS "Using PTX")
endif()

# 模块输出目录
if(WIN32 AND "${CMAKE_GENERATOR}" MATCHES "^(Visual Studio).*")
    set(MODULE_TARGET_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(ConfigurationName)")
else()
    set(MODULE_TARGET_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

# ============================================================================
# 配置头文件
# ============================================================================

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
)

# ============================================================================
# 包含目录
# ============================================================================

set(PHYSICS_RENDER_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../OptiX_Apps/apps/nvlink_shared/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../OptiX_Apps/apps/nvlink_shared/shaders
    ${OptiX90_INCLUDE}
    ${PHYSX_INCLUDE_DIRS}
    ${CUDAToolkit_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${IL_INCLUDE_DIR}
    ${ASSIMP_INCLUDE_DIRS}
)

if(ENABLE_FLOW)
    list(APPEND PHYSICS_RENDER_INCLUDE_DIRS ${FLOW_INCLUDE_DIRS})
endif()

if(ENABLE_BLAST)
    list(APPEND PHYSICS_RENDER_INCLUDE_DIRS ${BLAST_INCLUDE_DIRS})
endif()

include_directories(${PHYSICS_RENDER_INCLUDE_DIRS})

# ============================================================================
# 库目录
# ============================================================================

set(PHYSICS_RENDER_LIBRARIES
    ${PHYSX_LIBRARIES}
    ${CUDAToolkit_LIBRARIES}
    CUDA::cuda_driver
    ${OPENGL_LIBRARIES}
    ${GLFW3_LIBRARY}
    ${GLEW_LIBRARIES}
    ${IL_LIBRARIES}
    ${ILU_LIBRARIES}
    ${ASSIMP_LIBRARIES}
)

if(ENABLE_FLOW)
    list(APPEND PHYSICS_RENDER_LIBRARIES ${FLOW_LIBRARIES})
endif()

if(ENABLE_BLAST)
    list(APPEND PHYSICS_RENDER_LIBRARIES ${BLAST_LIBRARIES})
endif()

# ============================================================================
# 子目录
# ============================================================================

add_subdirectory(src)
add_subdirectory(shaders)
add_subdirectory(examples)

# ============================================================================
# 总结信息
# ============================================================================

message(STATUS "")
message(STATUS "========== PhysicsRenderIntegration Configuration ==========")
message(STATUS "OptiX Version: 9.0.0")
message(STATUS "CUDA Version: ${CUDAToolkit_VERSION}")
message(STATUS "PhysX: ENABLED")
message(STATUS "Flow: ${ENABLE_FLOW}")
message(STATUS "Blast: ${ENABLE_BLAST}")
message(STATUS "OptiX-IR: ${USE_OPTIX_IR}")
message(STATUS "Module Extension: ${OPTIX_MODULE_EXTENSION}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "===========================================================")
message(STATUS "")
