cmake_minimum_required(VERSION 3.20)

project(ryzc LANGUAGES CXX)

# Enforce modern C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically generate compile_commands.json for clangd LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define executable and source files
add_executable(ryzc
    src/main.cpp
    src/greeter.cpp
)

# Tell the compiler to look inside 'include/' for #include <...>
target_include_directories(ryzc PRIVATE
    include
    src
)

# Enable strict warnings
target_compile_options(ryzc PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
