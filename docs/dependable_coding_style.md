# Dependable C: Coding Rules & Style Guide

> **Based on the "Dependable C" specification by Eskil Steenberg Hald** *(ISO WG14 / C Standards Study Groups on Memory Model and Undefined Behavior)*.

---

## 1. Philosophy & Core Principles

### 1.1 The Goal: Code for the Decades
Dependable C is an architectural discipline designed to produce code that is **universally portable, compliant, and durable for decades**. Software written in Dependable C requires no language dialect extensions, no specific compiler version, and no complex toolchains. It aims to solve a problem once, in C, so that it remains solved indefinitely across all current and future platforms.

### 1.2 "Newscaster C" vs. Dialects
Dependable C is the opposite of a dialect:
- It represents the **broadest common denominator** of C—a neutral, middle-of-the-road subset that compiles and executes reliably across hundreds of C compilers, static analyzers, linters, debuggers, and sanitizers.
- It prioritizes **longevity, safety, and compatibility** over syntactic convenience or modern language ergonomics.
- **Rule of Thumb:** *"Very few people want to write Dependable C, but everyone wishes everyone else wrote their code in Dependable C."*

### 1.3 The "As-If" Rule & Observable Behavior
The C standard defines program behavior via an imaginary **Abstract Machine**. Real hardware and optimizing compilers may transform, reorder, vectorize, or eliminate code freely, provided the **observable behavior** matches what the abstract machine would produce:
- **Observable Behavior** is strictly limited to:
  1. Input/Output operations (e.g., standard library file and stream I/O, `printf`).
  2. Accesses to objects qualified with `volatile`.
  3. Program termination status.
- Operations occurring entirely inside the abstract machine (arithmetic, non-volatile memory assignments, intermediate pointer arithmetic) have **no guaranteed execution ordering or physical persistence**.

### 1.4 Assumed Absence of Undefined Behavior (UB)
Compilers treat undefined behavior not as a runtime error to detect, but as a **contractual promise** made by the programmer:
- **Contract:** The compiler assumes UB *never* occurs in valid code.
- **Consequence:** If a code path would result in UB, the compiler is legally permitted to optimize under the assumption that the condition leading to it is impossible. This leads to:
  - **Dead Code Elimination of Bounds Checks:** `if (x + 1 > x)` or `a[x] = 0; if (x > 4)` will cause the `if` statement to be silently deleted.
  - **"Time-Traveling UB":** Side-effects occurring before the undefined operation can be erased or reordered before the faulting instruction actually executes.
- **Rule:** Never rely on apparent, conventional platform behavior for UB (such as assuming signed integer overflow wraps, dereferencing `NULL` traps, or reading uninitialized memory yields random garbage). **UB is never dependable.**

---

## 2. Platform & Architecture Baseline

### 2.1 Standard Baseline Assumptions (Common Platforms)
Dependable C acknowledges that writing code for completely arbitrary, exotic architectures (e.g., 32-bit `char` DSPs) makes practical programming needlessly difficult. It adopts explicit, defensible baseline assumptions for **common platforms**:
- **8-bit Bytes:** `CHAR_BIT == 8`; `sizeof(char) == 1` represents 8 bits.
- **Natural Alignment:** Types are aligned to multiples of their size.
- **ASCII Source Code:** All `.c` and `.h` files MUST be pure 7-bit ASCII. Unicode in source code is non-portable and creates tooling, rendering, and security pitfalls (homoglyphs, invisible characters, bidirectional overrides).
- **Pointer Homogeneity:** Function pointers have the same size and general representation as data pointers.
- **Integer Sizes:** `int` is at least 32 bits wide.

### 2.2 Assumptions to Avoid (Keep Code Independent)
Do not build structural dependencies on the following assumptions:
- **Do not assume pointers are 64 bits:** 32-bit systems remain prevalent in embedded devices, and 128-bit pointers (e.g., capability-based hardware, NUMA architectures) are emerging.
- **Do not assume fixed sizes for `short` and `int`:** Always check type limits or use fixed-width abstractions where byte layouts matter.
- **Do not assume platform endianness:** Design data storage and network protocols **little-endian first**, and write explicit swizzling functions for big-endian targets.

---

## 3. C Standards Version & Keyword Discipline

### 3.1 Strict C89 Subset Baseline
- The dependable core of C is the **C89 / ANSI C subset**.
- Code must compile cleanly across all standard modes: C89, C99, C11, C17, and C23.
- When modern features are required, access them strictly through isolated preprocessor compatibility macros.

### 3.2 Forbidden & Reserved Identifiers
1. **Never use leading underscores (`_`):** Identifiers beginning with an underscore followed by an uppercase letter or another underscore (e.g., `_Bool`, `__typeof__`) are reserved for compiler implementations and the standard library.
2. **Never use `auto`:**
   - In C89/C99, `auto` meant automatic storage duration (redundant for local variables).
   - In C23, `auto` was repurposed for type deduction (`auto x = 0.0;`).
   - Using `auto` creates silent semantic changes across different compiler revisions. **Never use it.**
3. **Do not use newer standard keywords as identifiers:**
   Avoid using names reserved by C99, C11, and C23:
   ```c
   /* FORBIDDEN as variable or function identifiers: */
   true, false, bool, null, nullptr,
   alignas, alignof, constexpr, inline,
   static_assert, thread_local, typeof, type_unequal
   ```
4. **Avoid compiler extension keywords:**
   Avoid `asm`, `fortran`, `__attribute__`, `__declspec`, etc., unless wrapped in strict platform/compiler feature checks.

### 3.3 C++ Compatibility & Public Headers
- Public header files (`.h`) must be 100% C++ compatible.
- Public headers must contain **only declarations and type definitions**—never function definitions or static inline logic.
- Do not use C++ keywords as identifiers in headers or public APIs:
  ```c
  /* FORBIDDEN in public headers: */
  new, delete, class, template, this, friend,
  operator, virtual, private, protected, public,
  catch, throw, try, namespace, explicit,
  and, and_eq, bitand, bitor, compl, not, not_eq, or, or_eq, xor
  ```

---

## 4. Scoping & Flow Control Rules

### 4.1 Variable Declarations: Function-Scope Only (C89 Style)
- **Rule:** Declare all local variables **only at the very beginning of the function scope**.
- **Rule:** Never declare variables inside loop headers, `if` statements, or nested inner blocks:
  ```c
  /* FORBIDDEN: C99-style inline loop declaration */
  for (int i = 0; i < 10; i++) { ... }

  /* FORBIDDEN: Declaration inside switch or if condition */
  if (int x = calculate()) { ... }

  /* CORRECT: C89 function-top declaration */
  void process_items(void)
  {
      int i;
      int count;

      count = 10;
      for (i = 0; i < count; i++) {
          /* ... */
      }
  }
  ```
- **Rationale:** 
  1. Guarantees universal compiler compatibility (C89-compliant).
  2. Eliminates variable shadowing and accidental re-declaration across nested scopes.
  3. Avoids uninitialized variable bypasses when jumping with `goto` or across `switch` case labels.

### 4.2 Flow Control Constructs
- Safe and dependable constructs: `if`, `else`, `for`, `while`, `do ... while`, `switch`, `case`, `default`, `break`, `continue`, `return`, `goto`.
- **Recursion:** Strongly discouraged. When unavoidable, enforce strict, empirically tested bounds on maximum recursion depth. Stack sizes are fixed and small (often 1 MB on Windows, 8 MB on Linux, and tiny on embedded devices).

---

## 5. Type System & Representations

### 5.1 Booleans: Avoid `bool` and `_Bool`
- **Rule:** Do not include `<stdbool.h>` and do not use `_Bool` or `bool`.
- **Rule:** Use standard integer types:
  - Use `int` for return values, boolean function arguments, and local flags.
  - Use `unsigned char` (or `char`) for boolean struct members when memory footprint is critical.
  - Use bitmasks within unsigned integers for serialization, networking, and disk storage.
- **Evaluating Booleans:**
  ```c
  #define TRUE  1
  #define FALSE 0

  /* CORRECT: Idiomatic truth test */
  if (is_valid) { ... }

  /* CORRECT: Explicit inequality check */
  if (is_valid != FALSE) { ... }

  /* CORRECT: Double negation normalization */
  if (!!is_valid == TRUE) { ... }

  /* DANGEROUS / FORBIDDEN: Any non-zero value is true, not just 1 */
  if (is_valid == TRUE) { ... } /* FAILS if is_valid is 2 or -1 */
  ```

### 5.2 Floating-Point Arithmetic
- `float` (single precision) and `double` (double precision) are dependable IEEE 754 types, but have platform-dependent rounding characteristics and compiler folding variations.
- **Rule 1:** Never use floating-point types for lockstep network synchronization (identical code with identical input produces divergent results across architectures).
- **Rule 2:** **Never compare computed floating-point numbers using `==` or `!=`**:
  ```c
  /* FORBIDDEN: Comparing calculated floats */
  x = 6.0f / 2.0f;
  if (x == 3.0f) { ... } /* NOT dependable across optimization levels */

  /* PERMITTED: Directly assigned constants */
  x = 6.0f;
  if (x == 6.0f) { ... }
  ```
- **Rule 3 (NaN Checking):** All IEEE 754 NaN values compare unequal to themselves:
  ```c
  /* Safe NaN detection */
  int dependable_isnanf(float x)
  {
      return (x != x);
  }
  ```
  *(Note: Avoid `#define isnan(x) ((x) != (x))` because macro arguments evaluated twice trigger severe bugs if passed function calls with side-effects).*

### 5.3 Endianness Discipline: "Little-Endian First"
- Modern client, server, and mobile computing has converged on little-endian architecture.
- **Rule:** Design all on-disk binary formats, cache files, and network communication payloads to be **little-endian**.
- Big-endian platforms must explicitly convert using documented bitshift/mask routines:
  ```c
  unsigned int swizzle_uint32(unsigned int val)
  {
      return ((val << 24) & 0xFF000000) |
             ((val <<  8) & 0x00FF0000) |
             ((val >>  8) & 0x0000FF00) |
             ((val >> 24) & 0x000000FF);
  }
  ```

---

## 6. Initialization Discipline & Sentinels

### 6.1 Explicit Member Assignment over Braces
- **Rule:** Explicitly assign structure members individually rather than relying on positional brace initialization:
  ```c
  /* FRAGILE: Positional brace initialization */
  struct Point p = {10, 20, NULL}; /* Breaks silently if struct fields are reordered */

  /* CORRECT & DEPENDABLE: Explicit field assignment */
  struct Point p;
  p.x = 10;
  p.y = 20;
  p.name = (void *)0;
  ```
- **Rule:** Never use C23 empty braces (`struct Point p = {};`). On pre-C23 compilers, this produces uninitialized memory with zero compiler warnings or errors.

### 6.2 Zero-Initialization is Harmful (Why `memset(0)` and `calloc` are Dangerous)
1. **NULL is Not Guaranteed to be Binary Zero:** While address zero is common, the C standard defines `NULL` as an abstract null pointer constant (`(void *)0`). On platforms where null pointers use non-zero representations, `memset(ptr, 0, sizeof ptr)` yields an invalid pointer.
2. **Zero Hides Latent Bugs:**
   - Setting uninitialized memory or pointers to `0` / `NULL` allows buggy code to fail silently or be quietly bypassed by defensive `if (ptr != NULL)` checks.
   - The bug only surfaces on reuse (the "second-use bug"), where it is immensely harder to diagnose.
3. **The Sentinel Pattern (Fail-Fast Debugging):**
   - In debug builds, fill all newly allocated memory with an invalid sentinel pattern (e.g., `0xCD` or `0xDEADBEEF`).
   - A pointer dereferencing `0xCDCDCDCD` crashes immediately, cleanly, and visibly at the root point of failure.

### 6.3 Array Initialization & Null Terminators
- **Rule 1:** Explicitly size arrays using constant `#define` values:
  ```c
  #define MAX_BUFFER 64
  int items[MAX_BUFFER];
  ```
- **Rule 2:** Avoid `sizeof(array) / sizeof(array[0])` wherever possible. If the array decays to a pointer during refactoring, this pattern silently computes `sizeof(pointer) / sizeof(type)` without warning.
- **Rule 3:** The string null-terminator is **`'\0'`**, NEVER integer `0` and NEVER `NULL`:
  ```c
  /* CORRECT */
  char str[32];
  str[0] = '\0';

  /* FORBIDDEN: Slower, fills entire buffer */
  char str[32] = {'\0'}; 

  /* FORBIDDEN: Type errors and non-portable semantics */
  char str[] = {'H', 'i', NULL}; 
  char str[] = {'H', 'i', 0};
  ```

---

## 7. Memory Management & Stack Safety

### 7.1 In Defense of Standard `malloc`, `realloc`, and `free`
- Standard `malloc` is one of the most rigorously tested and heavily optimized algorithms in computing.
- It interfaces directly with hardware MMUs and OS virtual memory paging, returning physical pages to the operating system when possible.
- Avoid custom arena/pool allocators unless strictly necessary. Custom pools require manual capacity guesswork and cannot return unused pages to other system processes.

### 7.2 Variable Length Arrays (VLAs) are Strictly Forbidden
- **Rule:** **NEVER use Variable Length Arrays (`int arr[n];`).**
- **Rationale:**
  1. VLAs have no mechanism to report out-of-memory errors.
  2. When allocation exceeds stack capacity, behavior is completely undefined (instant silent stack corruption or kernel panic).
  3. Stack space is severely constrained (1 MB on Windows by default).

### 7.3 Safe Alternatives for Dynamic Scratch Memory
Use a fixed stack buffer with a dynamic heap fallback:
```c
void process_data(size_t count)
{
    int stack_buffer[1024];
    int *buffer;

    if (count > 1024) {
        buffer = (int *)malloc(count * sizeof(*buffer));
        if (buffer == (void *)0) {
            abort(); /* Handle allocation failure */
        }
    } else {
        buffer = stack_buffer;
    }

    /* Perform work on buffer ... */

    if (buffer != stack_buffer) {
        free(buffer);
    }
}
```

### 7.4 Sensitive Memory Erasing (Volatile Scrubbing)
Optimizing compilers will dead-code eliminate standard `memset(password, 0, size)` if the buffer is not read before exiting scope. When scrubbing secrets:
```c
void dependable_memzero_secure(void *ptr, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) {
        *p++ = 0;
    }
}
```

---

## 8. Aliasing, Effective Type & Pointer Rules

### 8.1 Effective Type Rules in a Nutshell
- **Declared Objects:** Retain their declared type permanently throughout their lifetime. It is strictly UB to read or write a declared `float` through an `int *` lvalue.
- **Allocated Memory (`malloc`):** Has no effective type upon allocation.
  - The first non-character write gives the memory that effective type.
  - Any subsequent read must use a compatible type.
  - Writing to allocated memory with a new type updates its effective type.
- **Character Type Exception:** Any object (declared or allocated) can be safely read or written via character pointers (`char *`, `unsigned char *`).
- **`memcpy` / `memmove` Exception:** Copying bytes via `memcpy` or `memmove` into allocated memory also transfers the source object's effective type.

### 8.2 Safe Type Punning: Always Use Unions or `memcpy`
- **FORBIDDEN:** Direct pointer casting:
  ```c
  float f = 3.14f;
  int i = *(int *)&f; /* STRICT ALIASING VIOLATION (Undefined Behavior) */
  ```
- **CORRECT:** Type punning through a `union`:
  ```c
  union FloatIntPun {
      float f;
      int i;
  };

  int float_to_bits(float val)
  {
      union FloatIntPun pun;
      pun.f = val;
      return pun.i; /* Well-defined in C */
  }
  ```
- **CORRECT:** Type punning through `memcpy`:
  ```c
  int float_to_bits_memcpy(float val)
  {
      int bits;
      memcpy(&bits, &val, sizeof(bits));
      return bits;
  }
  ```

### 8.3 The "Union Condom" Pattern
When writing low-level routines that must read unknown memory types without violating aliasing:
```c
int read_as_int(void *p)
{
    union AllTypes {
        char c;
        short s;
        int i;
        long l;
        float f;
        double d;
        void *ptr;
    } *u;

    u = (union AllTypes *)p;
    return u->i; /* Access via union containing compatible type */
}
```

### 8.4 Pointer Comparison & Provenance Rules
1. **Relational Comparisons (`<`, `<=`, `>`, `>=`):** ONLY legal between pointers pointing into or one element past the end of the **same array or object**. Comparing pointers from two separate allocations is UB.
2. **Equality Comparisons (`==`, `!=`):** Valid between any two pointers.
3. **Pointers to Freed Memory:** NEVER use a pointer to freed memory for *anything*—not even for relational or equality comparisons (`ptr == old_ptr` is UB). If ABA tracking is necessary, cast the pointer to `uintptr_t` or `size_t` *before* freeing.
4. **Pointer Boundaries:** Never calculate or create a pointer that points beyond the base allocation plus one element (`&(arr[size])` is the absolute limit).

### 8.5 Struct & Union Member Pointer Rules
- **Rule:** A pointer to a member of a structure must **never** be used to access neighboring members via pointer arithmetic.
- **Rule:** Taking a pointer to a member of a union is non-dependable and forbidden.
- **Polymorphism Rule:** A pointer to a structure is guaranteed to be equal to a pointer to its first member (widely dependable for C polymorphism):
  ```c
  struct Base { int type; };
  struct Widget { struct Base base; int widget_id; };

  /* Safe and dependable */
  struct Widget w;
  struct Base *b = (struct Base *)&w;
  ```

---

## 9. Arithmetic & Bitwise Operations

### 9.1 Signed Integer Overflow
- Signed integer overflow is **Undefined Behavior**.
- Never write tests that rely on signed overflow wrapping:
  ```c
  /* COMPILER DELETES THIS: Assumed absence of UB */
  if (x + 1 > x) { ... }
  ```
- Always use unsigned arithmetic when wrapping is intended, or validate operands *before* executing the operation.

### 9.2 Bit Shifts
- Left-shifting (`<<`) or right-shifting (`>>`) by a negative number or by an amount greater than or equal to the bit-width of the type is **UB**.
- Right-shifting a negative signed integer is **implementation-defined** (arithmetic vs. logical shift varies).
- **Rule:** Always mask the shift count:
  ```c
  /* Defensive shift pattern */
  unsigned int shift_safe(unsigned int value, unsigned int shift_count)
  {
      return value << (shift_count & 31);
  }
  ```

---

## 10. Concurrency & Threading

### 10.1 Prefer Mutexes over Atomics
- **Rule:** Favor coarse- or fine-grained **Mutexes** over atomic operations for general multithreading.
- **Rationale:**
  - Mutexes provide implicit, foolproof acquire/release memory barriers (full fences).
  - Mutexes integrate with the operating system scheduler to sleep blocked threads cleanly.
  - C11 atomics (`_Atomic`) flawed the design by making atomicity a type property rather than an operation, obscuring explicit memory order semantics.

### 10.2 Threading Libraries
- Avoid C11 `<threads.h>` (poor compiler and platform adoption).
- Depend on **POSIX Threads (`pthreads`)** or native OS threading primitives wrapped behind clean, minimal project abstraction functions.

---

## 11. Standard Library Usage Matrix

| Standard Library Feature | Status | Notes / Instructions |
| :--- | :--- | :--- |
| `malloc`, `realloc`, `free` | **DEPENDABLE** | Core memory functions. Pair with debug tracking. |
| `calloc` | **USE WITH CARE** | Zeroes memory; prefer explicit member initialization. |
| `memcpy`, `memmove`, `memset` | **DEPENDABLE** | Often lowered to high-performance compiler intrinsics. |
| `exit`, `abort`, `assert` | **DEPENDABLE** | Standard flow and panic termination. |
| `fopen`, `fclose`, `fread`, `fwrite` | **DEPENDABLE** | Universal, OS-agnostic stream I/O. |
| `printf`, `fprintf`, `sprintf` | **DEPENDABLE** | Stick to standard C89 format specifiers. |
| `<math.h>` (`sin`, `cos`, `sqrt`, etc.) | **DEPENDABLE** | Hardware-accelerated intrinsics on most compilers. |
| `_Bool` / `<stdbool.h>` | **FORBIDDEN** | Non-portable, causes namespace collisions. Use `int`. |
| VLAs (Variable Length Arrays) | **FORBIDDEN** | Unchecked stack overflows, effectively UB. |
| Annex K (Bounds-checking `_s` API) | **FORBIDDEN** | `strcpy_s`, `sprintf_s`: No portable implementations exist. |
| `<threads.h>` (C11 Threads) | **AVOID** | Use POSIX Threads (`pthreads`) or native wrappers. |
| `quick_exit` / `at_quick_exit` | **AVOID** | Inconsistent support across legacy platforms. |

---

## 12. Defensive Diagnostics & Memory Debugging Patterns

### 12.1 Warning Management
- **Rule:** Treat warnings seriously, but avoid blind reliance on `-Werror` in production releases (new compiler versions introduce new diagnostic warnings that break previously clean builds).
- Selectively enable strict warnings during development (`-Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes`).
- Elevate warnings to errors during local development, but release with clean standard flags.

### 12.2 Strict Aliasing Compiler Setting
- In large projects with legacy code or varying team experience levels, explicitly pass `-fno-strict-aliasing` (GCC/Clang) to prevent aggressive, catastrophic aliasing optimizations.

### 12.3 The Sentinel Memory Debugger Pattern (The "Forge" Pattern)
During development and testing, wrap standard allocation functions with custom debugging macros:
1. **Source Tracking:** Capture `__FILE__` and `__LINE__` on every `malloc`, `realloc`, and `free`.
2. **Pre- and Post-Allocation Guard Padding (Canaries):** Allocate padding around every heap block filled with a canary byte (e.g., `0xCF`). Check guards on every allocation or boundary-check sweep to instantly flag heap buffer overflows.
3. **Poisoning on Allocation:** Fill all newly allocated buffers with sentinel `0xCD`.
4. **Poisoning on Deallocation:** Scrub freed memory with sentinel `0xCD` immediately to catch use-after-free reads.
5. **Freed Registry:** Maintain a hash table of freed pointers to immediately catch double-free bugs.

---

## 13. The 17 Golden Rules Checklist

1. **Memory as Buffers:** Never use declared variables as arbitrary memory buffers outside of `memcpy` and `memmove`.
2. **Uninitialized Memory:** Never read uninitialized memory for any reason (it is not a source of entropy; compilers can erase such paths).
3. **Initialization:** Do not use `calloc` or `memset(0)` for pointer or struct initialization. Explicitly assign members.
4. **Union Member Pointers:** Never take a pointer to a member of a union.
5. **Struct Member Pointers:** Never use a pointer to a struct member to access other struct members or out-of-bound memory.
6. **Type Punning:** Always type-pun through a `union` or `memcpy`; never cast pointers directly.
7. **Integer-to-Pointer Casts:** Never convert an integer into a pointer.
8. **Consistent Types:** Never read allocated memory using a type incompatible with how it was written.
9. **Memory Reuse:** When reusing allocated memory for different data, re-initialize it with the new type before reading.
10. **Standard I/O Reads:** Read memory filled by `fread` using the expected data types or a union.
11. **Union Condom:** When reading memory of uncertain effective type, route the access through an all-types union.
12. **Pointer Arithmetic Limit:** Never construct a pointer more than one element past the allocated boundary.
13. **Freed Pointers:** Never use a pointer to freed memory for *anything* (including `==` comparisons).
14. **Pointer-to-Integer Casts:** Never convert a pointer to an integer except for ABA detection or debug logging.
15. **Object Relationships:** Never calculate offsets or perform relational comparisons (`<`, `>`) between pointers to different allocations.
16. **No VLAs or Unbounded Recursion:** Never use Variable Length Arrays, and strictly bound recursion.
17. **No "It Works On My Machine":** Never assume you can break the C specification because your current compiler seems to permit it. Compilers optimize aggressively against UB when least expected.
