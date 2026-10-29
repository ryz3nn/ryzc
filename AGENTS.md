# AGENTS.md

Instructions and context for agents working in `ryzc`.

## Environment & Tooling (NixOS)

- **NixOS Environment**: Non-FHS system. Never assume global library or header paths like `/usr/lib` or `/usr/include`. Run build and analysis commands inside `nix develop`.
- **Graphics & Driver Hook**: Requires GPU driver access for OpenGL. `flake.nix` sets `LD_LIBRARY_PATH=/run/opengl-driver/lib:$LD_LIBRARY_PATH`. For systems with OpenGL < 3.0, enable software rendering via `LIBGL_ALWAYS_SOFTWARE=1` (handled automatically by `./run.sh`).
- **Core Dependencies**:
  - Build & Analysis: `cmake`, `ninja`, `pkg-config`, `clang-tools` (`clangd`, `clang-format`, `clang-tidy`), `cppcheck`
  - Libraries: `SDL3`, `OpenGL` (`libGL`), Dear ImGui (bundled in `external/imgui`)

## Developer Workflow

Run inside `nix develop`:

- **Configure**:
  ```bash
  cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  ```
- **Build**:
  ```bash
  ninja -C build
  # or: cmake --build build --parallel
  ```
- **Run Executable**:
  ```bash
  ./build/ryzc
  # or helper script: ./run.sh [debug|release]
  ```
- **Static Analysis & Linting** (must pass with zero warnings):
  ```bash
  # Clang-Tidy (checks cppcoreguidelines-*, modernize-*, bugprone-*, readability-*)
  clang-tidy src/*.cpp -p build

  # Cppcheck
  cppcheck --project=build/compile_commands.json --enable=all --suppress=missingIncludeSystem
  ```
- **Format**:
  ```bash
  clang-format -i src/*.cpp src/*.h
  ```

## Architecture & Project Structure

- **Entrypoint**: `src/main.cpp`.
- **Target GUI**: Dear ImGui application using:
  - Backends: `imgui_impl_sdl3` and `imgui_impl_opengl3`.
  - Core: Static library compiled from `external/imgui`.
- **Include Paths**: `include/` and `src/` are exposed as search roots via `CMakeLists.txt` (`#include <greeter.h>` or `#include "greeter.h"`).
- **Third-Party Code Boundary**: ImGui code in `external/imgui` is third-party. Do not modify external ImGui headers or source unless explicitly instructed. Third-party warnings are suppressed in `CMakeLists.txt`; strict warnings apply to application code.

## C++ Core Guidelines & Code Style

The project targets **Modern C++20** with `-Wall -Wextra -Wpedantic`. All code must adhere to the **C++ Core Guidelines**.

### 1. Resource Management & Ownership (RAII)
- **RAII everywhere**: Encapsulate every system resource (SDL windows, GL contexts, GPU textures, file descriptors, memory) inside an RAII owner.
- **Smart pointers for ownership**:
  - Use `std::unique_ptr` for single/exclusive ownership (allocate via `std::make_unique`).
  - Use `std::shared_ptr` only when ownership is genuinely shared (allocate via `std::make_shared`).
  - Never use raw `new` or `delete`.
- **Raw pointers and references are non-owning**:
  - Treat raw pointers (`T*`) and references (`T&`) strictly as non-owning, borrowed references.
  - Assume references and raw pointers point to valid objects that outlive the caller; verify raw pointers against `nullptr` if optional.
- **C API RAII wrappers**: Wrap SDL3 and OpenGL handles using custom deleters with `std::unique_ptr` or dedicated wrapper structs:
  ```cpp
  struct SDLWindowDeleter { void operator()(SDL_Window* w) const { SDL_DestroyWindow(w); } };
  using WindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;
  ```

### 2. Interfaces & Function Design
- **Parameter passing**:
  - Pass cheap-to-copy types (primitives, small `struct` <= 16 bytes) by value.
  - Pass read-only strings as `std::string_view`.
  - Pass read-only contiguous sequences as `std::span<const T>`.
  - Pass expensive-to-copy read-only types by `const T&`.
  - Pass sink parameters (values consumed or moved into storage) by value and `std::move`.
- **Return values**:
  - Return by value and rely on RVO / move semantics.
  - Avoid output parameters (`T& out`); return a `struct`, `std::pair`, or `std::tuple` and unpack with structured bindings (`auto [x, y] = ...`).
  - Use `std::optional<T>` for functions that may fail or return no value.
- **Const-correctness**:
  - Mark member functions `const` whenever they do not alter observable object state.
  - Mark immutable local variables and references `const`.
- **Noexcept specification**: Mark destructors, swap functions, move constructors, and move assignment operators `noexcept`.

### 3. Class Design & Types
- **Rule of Zero / Five**:
  - Prefer the **Rule of Zero**: design classes so default compiler-generated special member functions suffice by using RAII member types.
  - If a custom destructor is required (Rule of Five), declare or explicitly delete all five special member functions (destructor, copy constructor, copy assignment, move constructor, move assignment).
- **Constructors**:
  - Mark all single-argument constructors `explicit` to prevent accidental implicit conversions.
  - Initialize member variables at the point of declaration (in-class member initializers).
- **Enums**: Use scoped `enum class` instead of plain `enum`.
- **Structs vs Classes**:
  - Use `struct` for passive data holders with public members and no class invariants.
  - Use `class` when maintaining an invariant that requires encapsulation and private data.

### 4. Expressions, Statements & Modern C++20
- **Initialization**: Always initialize variables at declaration. Prefer `auto` when the concrete type is repeated on the right-hand side (`auto win = std::make_unique<...>()`).
- **Algorithms over raw loops**: Prefer standard library algorithms (`<algorithm>`, `std::ranges`) over manual index-based loops.
- **Narrowing conversions**: Avoid implicit narrowing conversions; use explicit `static_cast` or brace-initialization `{}` to prevent truncation.
- **Magic numbers**: Keep numbers with domain meaning in named `constexpr` constants (note: `.clang-tidy` allows magic numbers for quick GUI dimensions, but use named constants for core logic and physics).
- **Clang-Tidy conformance**: Keep code clean of `cppcoreguidelines-*`, `modernize-*`, `bugprone-*`, and `readability-*` warnings as configured in `.clang-tidy`.
