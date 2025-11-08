# src/CMakeLists.txt - 源代码构建

# 收集所有源文件
set(PHYSICS_RENDER_SOURCES
    # Core
    core/PhysicsRenderApp.cpp

    # Physics
    physics/PhysicsSimulator.cpp

    # Rendering
    rendering/PhysicsRenderer.cpp
    rendering/PhysicsRenderer_AS.cpp
    rendering/CameraManager.cpp
    rendering/LightManager.cpp
    rendering/MaterialSystemExtended.cpp
    rendering/PerformanceOptimizer.cpp

    # Sync
    sync/SyncManager.cpp
)

# 从 OptiX_Apps 复用的源文件
set(OPTIX_APPS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../OptiX_Apps/apps/nvlink_shared/src/Camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../OptiX_Apps/apps/nvlink_shared/src/Options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../OptiX_Apps/apps/nvlink_shared/src/Picture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../OptiX_Apps/apps/nvlink_shared/src/Texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../OptiX_Apps/apps/nvlink_shared/src/Timer.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/../../OptiX_Apps/apps/nvlink_shared/src/SceneGraph.cpp
)

# ImGui 源文件
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../OptiX_Apps/3rdparty/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_impl_glfw_gl3.cpp
)

# 创建静态库
add_library(PhysicsRenderCore STATIC
    ${PHYSICS_RENDER_SOURCES}
    ${OPTIX_APPS_SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(PhysicsRenderCore PUBLIC
    ${PHYSICS_RENDER_INCLUDE_DIRS}
    ${IMGUI_DIR}
)

target_link_libraries(PhysicsRenderCore
    ${PHYSICS_RENDER_LIBRARIES}
)

# 设置编译选项
if(UNIX)
    target_compile_options(PhysicsRenderCore PRIVATE
        -Wall -Wno-unused-function
    )
endif()

# 安装
install(TARGETS PhysicsRenderCore
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
