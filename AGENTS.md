# AGENTS.md

Instructions and context for agents working in `ryzc`.

## Environment & Tooling (NixOS)

- **NixOS Environment**: Non-FHS system. Never assume global library or header paths. Run build and analysis commands inside `nix develop`.
- **GPU & Drivers**: Requires GPU driver access for OpenGL. `flake.nix` sets `LD_LIBRARY_PATH=/run/opengl-driver/lib:$LD_LIBRARY_PATH`. Software rendering fallback (`LIBGL_ALWAYS_SOFTWARE=1`) is handled automatically by `build.sh`.
- **Core Dependencies**: `pkg-config`, `clang` / `clang++`, `clang-tools` (`clangd`, `clang-format`), `SDL3`, `libGL`, Dear ImGui (bundled in `external/imgui`).

## Developer Workflow

Run inside `nix develop`:

- **Build (Debug)**: `./build.sh`
- **Build & Run**: `./build.sh run` (or release: `./build.sh release run`)
- **Clean Build Artifacts**: `./build.sh clean`
- **Format Code**: `clang-format -i src/*.cpp src/*.h`

## Architecture & Code Boundaries

- **Entrypoint & Frame Loop**: `src/main.cpp` drives the application lifecycle and frame loop directly.
- **Third-Party Code Boundary**: ImGui code in `external/imgui` is third-party. Do not modify external ImGui headers or source unless explicitly instructed.
- **Includes**: Search roots are `src/`, `include/`, `external/imgui`, and `external/imgui/backends`.
- **Detailed Reference**: For extended portable C++ rationale and freestanding details, consult `docs/orthodox_cpp_guideline.md`.

## Orthodox C++ Code Rules

Code must be readable to anyone familiar with the C language. If code does not require more complexity, do not add C++ abstractions. The end of design rationale in Orthodox C++ is: *"Quite simple, and it is usable. EOF."*

### 1. Memory Management & Lifecycles (No RAII)
- **Procedural Lifecycles**: Use explicit lifecycle functions (`init()`, `shutdown()`, `reset()`). Never rely on constructor/destructor side effects, cascading destructions, or RAII wrappers for system resources.
- **No Smart Pointers or `new`/`delete`**: Never use `std::unique_ptr`, `std::shared_ptr`, `new`, or `delete` (`operator new` links exception machinery even with `std::nothrow`). Use Arena allocators (`src/arena.h`) or `malloc`/`free`.
- **Raw Pointers**: Pointers (`T*`) and references (`T&`) are non-owning, borrowed views. Verify pointers against `nullptr` when optional.
- **Bulk Allocation & Memory Mapping**: For transient tasks, loading archives, or unpacking files (e.g. PAK files), allocate from a linear Arena or use `mmap`. Avoid repeated `seek` + `fread` loops and scattered heap allocations.
- **Fail Fast**: If memory allocation fails on critical paths, call `abort()`. Never build complex unwinding or recovery cascades for heap exhaustion.

### 2. Class Design & Data Structures
- **Rule of Zero & POD Structs**: Prefer plain `struct`s with public data. Do not write copy/move constructors, assignment operators, or custom destructor hierarchies.
- **Free Functions Over Methods**: Prefer standalone free functions over member methods with hidden `this` pointers unless state is strictly coupled.
- **No Virtual Hierarchies or RTTI**: Ban `virtual` methods, C++ vtables, `dynamic_cast`, and `typeid`. Use simple structs, function pointers, or explicit enum type tags if polymorphism is required.
- **No Private Fields**: Do not hide members behind `private` when passive public access keeps the code simple and directly testable.

### 3. Error Handling & Runtime Features
- **No Exceptions**: Never throw, catch, or specify exceptions. Use return error codes, status enums, or boolean flags (`bool init(...)`). Do not use `std::expected` to emulate exceptions.
- **No C++ Modules**: Use standard header files and translation units.

### 4. Standard Library & Includes
- **C Runtime Headers**: Include C runtime headers directly (`<stdio.h>`, `<math.h>`, `<stdint.h>`, `<string.h>`). Do not use C++ wrappers (`<cstdio>`, `<cmath>`, `<cstdint>`).
- **No Streams or Formatters**: Never use `<iostream>`, `std::cout`, `std::cin`, `<stringstream>`, `<format>`, or `std::format`. Use `printf`, `snprintf`, and `fprintf(stderr, ...)` for all text output.
- **No Locale Dependencies**: Avoid `<cctype>` (it is locale-dependent, slow, and thread-unsafe). Write simple ASCII checks (e.g. `c >= 'A' && c <= 'Z'`) directly.
- **No Allocating STL Containers**: Do not use `std::vector`, `std::string`, `std::map`, `<ranges>`, or other heap-allocating STL containers. Use fixed buffers, string slices (`std::string_view`), spans (`std::span`), Dear ImGui's `ImVector`, or custom Arena-backed arrays.
- **No `std::function`**: Use plain C function pointers (`void (*fn)(void* user_data)`) with an explicit context pointer instead of `std::function` or heavy closures.

### 5. Types & Integers
- **Index with `size_t`**: Use `size_t` as the default for indexing, buffers, and sizes. Avoid signed `int` for memory offsets.
- **Explicit Fixed-Width Integers**: Use fixed-width types from `<stdint.h>` (`uint32_t`, `int32_t`, `uint64_t`). Avoid `intmax_t` and `uintmax_t` due to ABI stability issues.
