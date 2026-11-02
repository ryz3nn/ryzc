cmake_minimum_required(VERSION 3.20)

project (ryzc LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)


add_executable(ryzc 
  src/main.c
)

target_include_directories(ryzc PRIVATE 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(ryzc PRIVATE 
    -Wall 
    -Wextra 
    -pedantic
)

target_link_libraries(ryzc PRIVATE
  SDL3::SDL3
  OpenGL::GL
)
