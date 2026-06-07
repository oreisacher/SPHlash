cmake_minimum_required(VERSION 3.22)

project(SPHlash LANGUAGES CXX C CUDA)

option(ENABLE_CUDA_DEBUG "Enable Cuda debug checks for kernel calls" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_ARCHITECTURES 61)

# --- Update submodules ---
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

# --- Get Version from Commit ---
execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(
    ${CMAKE_SOURCE_DIR}/src/Version.h.in
    ${CMAKE_BINARY_DIR}/generated/Version.h
    @ONLY
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.cu"
)

# --- Build Application ---
add_executable(SPHlash
    ${SOURCES}

    # External
    external/glad/src/gl.c
    external/ImGuiFileDialog/ImGuiFileDialog.cpp
        src/rendering/FluidSurfaceScreenSpace.cpp
        src/rendering/FluidSurfaceScreenSpace.h
)

add_library(imgui
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/imgui/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imgui PUBLIC
    external/imgui
    external/imgui/backends
    external/imgui/misc
)

find_package(CUDAToolkit REQUIRED)

add_subdirectory("external/glfw")
add_subdirectory("external/glm")
add_subdirectory("external/json")
add_subdirectory("external/tinyobjloader")
add_subdirectory("external/ImGuizmo")
target_link_libraries(imguizmo PUBLIC imgui)

target_compile_definitions(SPHlash PRIVATE USE_STD_FILESYSTEM)

target_include_directories(SPHlash PUBLIC
    src
    ${CMAKE_BINARY_DIR}/generated
    external/glad/include
    external/ImGuiFileDialog
    external/tinyobjloader
    external/ImGuizmo/src
    external/stb
)

target_link_libraries(SPHlash
    PRIVATE imgui
    PRIVATE glfw
    PRIVATE glm
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE CUDA::cudart
    PRIVATE tinyobjloader
    PRIVATE imguizmo
)

set_target_properties(SPHlash PROPERTIES
    CUDA_SEPARATE_COMPILATION ON
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON
)

if (ENABLE_CUDA_DEBUG)
    target_compile_definitions(SPHlash PRIVATE DEBUG_CUDA)
endif()

# --- Copy assets to build directory ---
add_custom_command(
    TARGET SPHlash POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying assets to build directory..."
)

add_custom_target(CopyAssets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            ${CMAKE_BINARY_DIR}/assets
)

add_dependencies(SPHlash CopyAssets)