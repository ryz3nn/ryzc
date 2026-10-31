cmake_minimum_required(VERSION 3.20)

project(ryzc LANGUAGES CXX)

# Enforce C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# 0. Build Options & Linker Configuration
# -----------------------------------------------------------------------------
option(ENABLE_UNITY "Enable Unity Build" OFF)

# Automatically link with mold or lld if present on the system
find_program(MOLD_PATH mold)
find_program(LLD_PATH ld.lld)
if(MOLD_PATH)
  add_link_options("-fuse-ld=mold")
  message(STATUS "==> Linker: mold")
elseif(LLD_PATH)
  add_link_options("-fuse-ld=lld")
  message(STATUS "==> Linker: lld")
endif()

# Dead code elimination across both compiler and linker
add_compile_options(-ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections)

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

target_include_directories(imgui PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
  SDL3::SDL3
  OpenGL::GL
)

# ImGui: Orthodox constraints, suppress warnings, optimize even in Debug
target_compile_options(imgui PRIVATE
  -w
  -fno-exceptions
  -fno-rtti
  $<$<CONFIG:Debug>:-O2 -g> # Keep UI snappy and stack traces intact in debug mode
)

# -----------------------------------------------------------------------------
# 3. Main Application Target (ryzc)
# -----------------------------------------------------------------------------
add_executable(ryzc
  src/main.cpp
  src/app.cpp
)

target_include_directories(ryzc PRIVATE
  include
  src
)

target_link_libraries(ryzc PRIVATE
  imgui
)

target_compile_options(ryzc PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -fno-exceptions
  -fno-rtti
  # NixOS fix: suppress _FORTIFY_SOURCE warning on -O0 Debug builds
  $<$<CONFIG:Debug>:-U_FORTIFY_SOURCE>
)

# -----------------------------------------------------------------------------
# 4. Unity Build Configuration
# -----------------------------------------------------------------------------
if(ENABLE_UNITY)
  set_target_properties(ryzc PROPERTIES
    UNITY_BUILD ON
    UNITY_BUILD_MODE BATCH
    UNITY_BUILD_BATCH_SIZE 0
  )
  message(STATUS "==> Target 'ryzc' configured with Unity Build")
else()
  message(STATUS "==> Target 'ryzc' configured with Separate Compilation")
endif()

# -----------------------------------------------------------------------------
# 5. Clangd Integration: Symlink compile_commands.json to root
# -----------------------------------------------------------------------------
add_custom_target(copy_compile_commands ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  "${CMAKE_BINARY_DIR}/compile_commands.json"
  "${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json"
  COMMENT "Symlinking compile_commands.json to source root for clangd"
)
