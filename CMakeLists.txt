cmake_minimum_required(VERSION 3.20)

project(ryzc LANGUAGES CXX)

# Enforce modern C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically generate compile_commands.json for clangd LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# 1. External Dependencies
# -----------------------------------------------------------------------------
find_package(SDL3 REQUIRED CONFIG)
find_package(OpenGL REQUIRED)

# -----------------------------------------------------------------------------
# 2. Dear ImGui Library Target
# -----------------------------------------------------------------------------
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# PUBLIC so anything linking to 'imgui' automatically inherits these include paths
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# ImGui backends need SDL3 and OpenGL
target_link_libraries(imgui PUBLIC
    SDL3::SDL3
    OpenGL::GL
)

# Suppress compiler warnings inside third-party ImGui code
target_compile_options(imgui PRIVATE -w)

# -----------------------------------------------------------------------------
# 3. Main Application Target (ryzc)
# -----------------------------------------------------------------------------
add_executable(ryzc
    src/main.cpp
    src/greeter.cpp
)

target_include_directories(ryzc PRIVATE
    include
    src
)

# Link your app against ImGui (SDL3 and OpenGL are inherited automatically)
target_link_libraries(ryzc PRIVATE
    imgui
)

# Strict warnings enforced ONLY on your own code
target_compile_options(ryzc PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
