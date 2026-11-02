# Agent Guidelines: ryzc

C99 project using SDL3 and OpenGL. Built with CMake and Ninja under `-std=c99 -pedantic -Wall -Wextra`.

## Commands
- Build: `cmake --build build`
- Run: `./run.sh` or `./build/ryzc`
- Clean rebuild: `cmake -B build -G Ninja && cmake --build build`

## Context Pointers
- Deep rationale, ISO WG14 memory models, and formal undefined behavior analysis: [`docs/dependable_coding_style.md`](docs/dependable_coding_style.md).

## Core Philosophy: Dependable C99
Universal portability, decade-long durability, and zero Undefined Behavior (UB).
- **Assumed Absence of UB**: Compilers optimize on the premise that UB never occurs. A code path causing UB allows the compiler to silently eliminate bounds checks, prune dead code, and reorder operations across time. UB is never dependable.
- **Observable Behavior**: Compilers preserve only standard I/O, `volatile` accesses, and process termination. Non-observable intermediate operations may be freely eliminated, reordered, or vectorized.
- **Platform Invariants**: 8-bit `char` (`CHAR_BIT == 8`), 2's complement integers, natural alignment, and little-endian wire/storage formats.

## C99 Feature Discipline: Safe vs Forbidden
Adopt dependable C99 ergonomic improvements; strictly exclude unsafe, unbounded, or non-portable additions.

### Safe C99 Features (Adopted)
- **Fixed-Width Types (`<stdint.h>`, `<inttypes.h>`)**: Use `int32_t`, `uint32_t`, `uint8_t`, `uint64_t`, `size_t`, `uintptr_t` for explicit data representation and serialization.
- **Designated Initializers**: Explicitly initialize structs and arrays by name or index (`struct Point p = { .x = 10, .y = 20 };`). Resilient against member reordering.
- **Scoped Loop Counters**: Use `for (int i = 0; i < count; i++)` to confine loop counter lifetime to the loop header and eliminate variable reuse/leak bugs.
- **Declare at Initialization**: Declare variables at their point of initialization close to first use to eliminate uninitialized variable windows.
- **Bounded Formatting**: Use `snprintf` with destination buffer sizes. `sprintf` is forbidden.
- **`static inline` Functions**: Use `static inline` functions in headers for small performance-critical utilities instead of multi-evaluating preprocessor macros.
- **Variadic Macros (`__VA_ARGS__`)**: Permitted for structured logging and assertions.
- **Compound Literals**: Permitted for temporary value passing (`(struct Point){ .x = 1, .y = 2 }`). Never retain a pointer to a temporary compound literal beyond its enclosing block.
- **Line Comments**: Standard `//` comments are permitted.

### Forbidden C99 & Non-Dependable Features
- **Variable Length Arrays (VLAs)**: Strictly **FORBIDDEN** (`int arr[n];`). Stack overflows from VLAs bypass bounds checks and have no recovery mechanism. Use fixed stack buffers with dynamic heap fallbacks.
- **Jumping Past Declarations**: Never jump across variable declarations with `goto` or `switch` case labels.
- **Unchecked `restrict`**: Do not use `restrict` unless no-aliasing can be statically verified. Aliasing violations with `restrict` cause silent undefined behavior.
- **Variable Shadowing**: Never re-declare an identifier in an inner scope that shadows an outer-scope identifier.
- **Exotic C99 Headers**: Do not include `<complex.h>`, `<tgmath.h>`, or `<fenv.h>`.
- **Annex K Functions**: Do not use `_s` bounds-checking functions (`strcpy_s`, `sprintf_s`); they lack portable implementations.
- **Reserved & Extension Keywords**: Never use leading underscores followed by capital letters or underscores (`_Bool`, `__typeof__`). Do not use `auto`, compiler-specific keywords (`__attribute__`, `__declspec`), or C++ keywords in public headers.

## Memory & Stack Safety
- **Allocators**: Standard `malloc`, `realloc`, `free`. Always assert or handle allocation results before use (`if (!ptr) abort();`).
- **Post-Free Pointer Poisoning**: A freed pointer is dead. Never read, write, or compare freed pointers (`ptr == old_ptr` is UB). Set freed pointers to `NULL` or poison immediately after `free`.
- **Zero-Initialization Hazards**: Avoid `memset(ptr, 0, size)` or `calloc` as a substitute for explicit initialization. Null pointers are not guaranteed to be binary zero on all architectures, and indiscriminate zeroing masks uninitialized read bugs.
- **Debug Sentinel Pattern**: In debug builds, poison freshly allocated and freed memory with invalid patterns (e.g. `0xCD`) to trigger immediate, clean faults on uninitialized reads or use-after-free bugs.
- **Stack Protection**: Replace unbounded stack allocations with a fixed stack buffer paired with a heap fallback:
  ```c
  int stack_buf[256];
  int *buf = (count > 256) ? (int *)malloc(count * sizeof(*buf)) : stack_buf;
  if (!buf) abort();
  /* ... */
  if (buf != stack_buf) free(buf);
  ```
- **Sensitive Memory Scrubbing**: Scrub passwords and secret buffers via `volatile unsigned char *` writes to prevent compiler dead-code elimination.

## Type System, Aliasing & Pointer Rules
- **Strict Aliasing Rule**: Never dereference a memory location through a pointer of an incompatible type. Pointer-casting type punning (`*(int *)&float_val`) is UB.
- **Type Punning**: Pun strictly through a `union` or via `memcpy`.
- **Character Pointer Exception**: Inspect or copy raw object bytes exclusively through `char *` or `unsigned char *`.
- **Pointer Comparisons**: Relational comparisons (`<`, `<=`, `>`, `>=`) are legal ONLY between pointers into the same allocated object or one element past its end. Comparing pointers across distinct allocations is UB.
- **Pointer Arithmetic Bounds**: Never offset pointers beyond `base + count`. Never perform pointer arithmetic between distinct struct or union members.
- **Union Member Pointers**: Never take or store a pointer to an internal union member.
- **C++ Compatible Public Headers**: Public `.h` files contain only declarations and types, wrap C code in `extern "C"` guards, and never use C++ reserved keywords.

## Arithmetic, Bitwise & Flow Control
- **Signed Integer Overflow**: Signed overflow is UB. Use unsigned arithmetic for modular wrapping, or validate bounds before signed operations.
- **Bitwise Shifts**: Mask shift operands to the type's bit-width minus one (`value << (shift & 31)`). Shifting by negative numbers or amounts $\ge$ type bit-width is UB.
- **Floating-Point Equality**: Never compare calculated floating-point numbers with `==` or `!=`. Test NaN using `(x != x)` or `isnan()`.
- **Boolean Discipline**: Evaluate truth with `if (cond)` or `if (!cond)`. Never compare against boolean literals (`cond == true` or `cond == TRUE`) because non-zero values $\ne 1$ fail equality tests.
- **String Termination**: Strings must be terminated with character `'\0'`, never integer `0` or `NULL`. Define buffer sizes with explicit constants.
- **Recursion**: Enforce strict, verified bounds on recursion depth, or replace with an explicit iterative loop.
- **Concurrency**: Favor OS-level mutexes over raw atomics. Wrap POSIX threads or SDL3 thread primitives.

## Verification Checklist
Before proposing or committing changes:
1. Compiles with zero warnings under `cmake --build build` (`-Wall -Wextra -pedantic -std=c99`).
2. No Variable Length Arrays (VLAs) or unchecked stack buffers.
3. Strict aliasing preserved: pointer punning strictly through `union` or `memcpy`.
4. Structs initialized with designated initializers (`.field = val`) or explicit assignment.
5. All dynamic memory allocations validated for non-NULL before dereference.
6. Bitwise shift counts masked and signed arithmetic guarded against overflow.
