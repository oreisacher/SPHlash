cmake_minimum_required(VERSION 3.22)

#set(CMAKE_C_COMPILER /usr/bin/gcc)
#set(CMAKE_CXX_COMPILER /usr/bin/g++)
#set(CMAKE_CUDA_HOST_COMPILER /usr/bin/g++)

project(FluidSimSPH LANGUAGES CXX C CUDA)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_ARCHITECTURES 61)

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
    src/rendering/FluidRenderer.cpp
    src/ui/UIPanel.cpp
    src/ui/UIManager.cpp
    src/ui/SimulationPanel.cpp
    src/rendering/Framebuffer.cpp
    src/rendering/Window.cpp
    src/rendering/Shader.cpp
    src/ui/StatisticsPanel.cpp
    src/utils/Time.cpp
    src/utils/Logger.cpp
    src/ui/SettingsPanel.cpp
    src/simulation/FluidSimulationGPU.cpp
    src/cuda/FluidSimulation.cu
    src/cuda/Kernels.cuh
    src/ui/ParticlesPanel.cpp
    src/utils/InputManager.cpp
    src/utils/Camera.cpp
    src/Application.cpp
    src/ui/InitPanel.cpp
    src/ui/InitPanel.h
    src/ui/ControlPanel.cpp
    src/ui/ControlPanel.h

    # External
    external/glad/src/gl.c
    external/ImGuiFileDialog/ImGuiFileDialog.cpp
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

find_package(CUDAToolkit REQUIRED)

add_subdirectory("external/glfw")
add_subdirectory("external/glm")
add_subdirectory("external/json")
add_subdirectory("external/tinyobjloader")

target_compile_definitions(FluidSimSPH PRIVATE USE_STD_FILESYSTEM)

target_include_directories(FluidSimSPH PUBLIC
    src
    external/glad/include
    external/ImGuiFileDialog
    external/tinyobjloader
)

target_link_libraries(FluidSimSPH
    PRIVATE imgui
    PRIVATE glfw
    PRIVATE glm
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE CUDA::cudart
    PRIVATE tinyobjloader
)

set_target_properties(FluidSimSPH PROPERTIES
    CUDA_SEPARATE_COMPILATION ON
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON
)

add_custom_target(CopyAssets ALL
    COMMENT "Copying assets to build directory..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
)