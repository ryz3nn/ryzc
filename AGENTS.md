# AGENTS.md

Instructions and context for agents working in `ryzc`.

## Environment & OS (NixOS)

- **NixOS System**: Non-FHS system. Never assume libraries exist in `/usr/lib` or `/usr/include`. All dependencies must be declared in `flake.nix` (`devShells`) and loaded via `nix develop`.
- **GUI & Graphics Stack**: Built with **Dear ImGui**, **SDL3** (window/events backend), and **OpenGL 3** (rendering backend).
- **Required Nix Dependencies**:
  - Tools: `cmake`, `ninja`, `pkg-config`, `llvmPackages.clang-tools`, `cppcheck`
  - Libraries: `sdl3`, `libGL`
- **NixOS OpenGL Gotcha**: If running `./build/ryzc` fails to initialize the OpenGL context on NixOS, ensure GPU drivers are accessible (e.g. set `LD_LIBRARY_PATH=/run/opengl-driver/lib:$LD_LIBRARY_PATH` or configure in `flake.nix` shellHook).

## Key Developer Commands

Run inside `nix develop`:

- **Configure**:
  ```bash
  cmake -B build -G Ninja
  ```
  *(Generates `build/compile_commands.json` for `clangd` and analysis tools).*

- **Build**:
  ```bash
  ninja -C build
  # or: cmake --build build
  ```

- **Run Executable**:
  ```bash
  ./build/ryzc
  ```

- **Lint / Static Analysis**:
  ```bash
  # Clang-Tidy (configured via .clang-tidy)
  clang-tidy src/*.cpp -p build

  # Cppcheck
  cppcheck --project=build/compile_commands.json
  ```

- **Format**:
  ```bash
  clang-format -i src/*.cpp src/*.h
  ```

## Architecture & ImGui Integration

- **Entrypoint**: `src/main.cpp`.
- **Target GUI**: Dear ImGui application using:
  - Backend: `imgui_impl_sdl3.cpp` / `.h` and `imgui_impl_opengl3.cpp` / `.h`
  - Core: `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`, `imgui_demo.cpp`
  - CMake linkage: `SDL3::SDL3` (via `find_package(SDL3 CONFIG REQUIRED)` or `pkg-config`) and `OpenGL::GL` (via `find_package(OpenGL REQUIRED)`).
- **Include Paths**: `CMakeLists.txt` exposes `include/` and `src/` to `#include <...>`.
- **Code Standards**: Modern C++20 (`-Wall -Wextra -Wpedantic`). `.clang-tidy` enforces core guidelines, modernization, and readability, but disables trailing-return-type and magic-number checks.
