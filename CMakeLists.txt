cmake_minimum_required(VERSION 4.1)
project(FluidSimSPH)

set(CMAKE_CXX_STANDARD 20)

# Pull git submodules
find_package(Git QUIET)
if (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    message(STATUS "Submodule update")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(FATAL_ERROR "${GIT_SUBMOD_RESULT}")
    endif()
endif()

add_executable(FluidSimSPH
    src/main.cpp
    src/simulation/FluidSimulation.cpp
    src/simulation/FluidSimulation.h
    src/rendering/FluidRenderer.cpp
    src/rendering/FluidRenderer.h
    src/ui/UIPanel.cpp
    src/ui/UIPanel.h
    src/ui/UIManager.cpp
    src/ui/UIManager.h
    src/ui/SimulationPanel.cpp
    src/ui/SimulationPanel.h
    src/rendering/Framebuffer.cpp
    src/rendering/Framebuffer.h
    src/rendering/Window.cpp
    src/rendering/Window.h
    src/rendering/Shader.cpp
    src/rendering/Shader.h
    src/ui/FPSPanel.cpp
    src/ui/FPSPanel.h
    src/utils/Time.cpp
    src/utils/Time.h
    src/utils/Logger.cpp
    src/utils/Logger.h
    src/ui/SettingsPanel.cpp
    src/ui/SettingsPanel.h

    # External
    external/glad/src/gl.c
    external/ImGuiFileDialog/ImGuiFileDialog.cpp
        src/simulation/FluidSimulationGPU.cpp
        src/simulation/FluidSimulationGPU.h
)

add_library(imgui
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    external/imgui
    external/imgui/backends
)

find_package(OpenMP REQUIRED)

add_subdirectory("external/glfw")
add_subdirectory("external/glm")
add_subdirectory("external/json")

target_compile_definitions(FluidSimSPH PRIVATE USE_STD_FILESYSTEM)

target_include_directories(FluidSimSPH PUBLIC
    src
    external/glad/include
    external/ImGuiFileDialog
)

target_link_libraries(FluidSimSPH
    PRIVATE OpenMP::OpenMP_CXX
    PRIVATE imgui
    PRIVATE glfw
    PRIVATE glm
    PRIVATE nlohmann_json::nlohmann_json
)

add_custom_target(CopyAssets ALL
    COMMENT "Copying assets to build directory..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
)