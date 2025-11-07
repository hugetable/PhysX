# shaders/CMakeLists.txt - OptiX 着色器编译

# 着色器源文件
set(SHADER_SOURCES
    raygeneration.cu
    miss.cu
    closesthit_physics.cu
    anyhit.cu
    exception.cu
)

# 为每个着色器创建编译规则
foreach(SHADER_SOURCE ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME_WE)

    # 使用 OptiX_Apps 的编译宏
    nvcuda_compile_module(
        SOURCES ${SHADER_SOURCE}
        DEPENDENCIES
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/config.h
            ${CMAKE_CURRENT_SOURCE_DIR}/physics_material_definition.h
            ${CMAKE_CURRENT_SOURCE_DIR}/physics_system_data.h
        TARGET_PATH ${MODULE_TARGET_DIR}/${SHADER_NAME}_core${OPTIX_MODULE_EXTENSION}
        EXTENSION ${OPTIX_MODULE_EXTENSION}
        GENERATED_FILES GENERATED
        NVCC_OPTIONS
            ${OPTIX_PROGRAM_TARGET}
            -use_fast_math
            --machine=64
            -I${CMAKE_CURRENT_SOURCE_DIR}/../include
            -I${CMAKE_CURRENT_SOURCE_DIR}/../../OptiX_Apps/apps/nvlink_shared/shaders
            -I${OptiX90_INCLUDE}
            ${SM_COMPUTE}
    )

    list(APPEND COMPILED_SHADERS ${GENERATED})
endforeach()

# 创建一个自定义目标来跟踪所有着色器
add_custom_target(PhysicsRenderShaders ALL
    DEPENDS ${COMPILED_SHADERS}
)

message(STATUS "Shader modules will be compiled to: ${MODULE_TARGET_DIR}")
