# examples/CMakeLists.txt - 示例程序

# 示例 1: PhysX 基础集成
add_executable(example_physx_basic
    example_physx_basic.cpp
)

target_link_libraries(example_physx_basic
    PhysicsRenderCore
    ${PHYSICS_RENDER_LIBRARIES}
)

# 依赖着色器
add_dependencies(example_physx_basic PhysicsRenderShaders)

# 示例 2: Flow 粒子渲染 (如果可用)
if(ENABLE_FLOW)
    add_executable(example_flow_particles
        example_flow_particles.cpp
    )

    target_link_libraries(example_flow_particles
        PhysicsRenderCore
        ${PHYSICS_RENDER_LIBRARIES}
    )

    add_dependencies(example_flow_particles PhysicsRenderShaders)
endif()

# 示例 3: Blast 破坏场景 (如果可用)
if(ENABLE_BLAST)
    add_executable(example_blast_fracture
        example_blast_fracture.cpp
    )

    target_link_libraries(example_blast_fracture
        PhysicsRenderCore
        ${PHYSICS_RENDER_LIBRARIES}
    )

    add_dependencies(example_blast_fracture PhysicsRenderShaders)
endif()

# 示例 4: 综合场景 (如果所有库都可用)
if(ENABLE_FLOW AND ENABLE_BLAST)
    add_executable(example_combined
        example_combined.cpp
    )

    target_link_libraries(example_combined
        PhysicsRenderCore
        ${PHYSICS_RENDER_LIBRARIES}
    )

    add_dependencies(example_combined PhysicsRenderShaders)
endif()

# 安装
install(TARGETS example_physx_basic
    RUNTIME DESTINATION bin
)
