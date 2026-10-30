#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="Debug"
UNITY="ON"
BUILD_DIR="build"

show_help() {
  cat <<EOF
Usage: ./run.sh [command] [options]

Commands:
  r, run         Build and run the executable (default)
  b, build       Build only
  bench          Run benchmark suite (cold, incremental, zero-op)
  c, clean       Remove build directories and symlinks

Options:
  debug          Set CMAKE_BUILD_TYPE=Debug (default)
  release        Set CMAKE_BUILD_TYPE=Release
  unity          Enable Unity build (default: ON)
  nounity        Disable Unity build (separate compilation)

Examples:
  ./run.sh                     # Build (Debug + Unity) and run
  ./run.sh r release           # Build Release and run
  ./run.sh b nounity           # Build Debug with separate compilation
  ./run.sh bench               # Compare standard vs unity with hyperfine
EOF
}

# 1. Parse Arguments
CMD="run"
if [ $# -gt 0 ]; then
  case "$1" in
    r|run)       CMD="run"; shift ;;
    b|build)     CMD="build"; shift ;;
    bench)       CMD="bench"; shift ;;
    c|clean)     CMD="clean"; shift ;;
    -h|--help)   show_help; exit 0 ;;
  esac
fi

for arg in "$@"; do
  case "$arg" in
    debug)       BUILD_TYPE="Debug" ;;
    release)     BUILD_TYPE="Release" ;;
    unity)       UNITY="ON" ;;
    nounity)     UNITY="OFF" ;;
    -h|--help)   show_help; exit 0 ;;
    *)
      echo "Unknown option: $arg"
      show_help
      exit 1
      ;;
  esac
done

# Adjust build dir name if testing non-default unity mode
if [ "$UNITY" = "OFF" ]; then
  BUILD_DIR="build_standard"
else
  BUILD_DIR="build"
fi

# 2. Command: Clean
if [ "$CMD" = "clean" ]; then
  echo "==> Cleaning build directories..."
  rm -rf build build_standard build_unity compile_commands.json
  echo "==> Done."
  exit 0
fi

# 3. Command: Benchmark Suite
if [ "$CMD" = "bench" ]; then
  if ! command -v hyperfine &>/dev/null; then
    echo "Error: 'hyperfine' is not installed or not in PATH." >&2
    exit 1
  fi

  echo "==> Configuring build_standard (Separate Compilation)..."
  cmake -B build_standard -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITY=OFF > /dev/null
  echo "==> Configuring build_unity (Unity Build)..."
  cmake -B build_unity -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITY=ON > /dev/null

  echo "==> Pre-warming both builds..."
  cmake --build build_standard > /dev/null
  cmake --build build_unity > /dev/null

  echo -e "\n=== 1. ZERO-OP BUILD BENCHMARK (No changes) ==="
  hyperfine -S \
    --warmup 3 \
    --runs 20 \
    'cmake --build build_standard' \
    'cmake --build build_unity'

  echo -e "\n=== 2. INCREMENTAL BUILD BENCHMARK (touch src/main.cpp) ==="
  hyperfine -S \
    --warmup 2 \
    --runs 15 \
    --prepare 'touch src/main.cpp' \
    'cmake --build build_standard' \
    'cmake --build build_unity'

  echo -e "\n=== 3. HEADER CHANGE BENCHMARK (touch src/app.h) ==="
  hyperfine -S \
    --warmup 2 \
    --runs 15 \
    --prepare 'touch src/app.h' \
    'cmake --build build_standard' \
    'cmake --build build_unity'

  echo -e "\n=== 4. COLD BUILD BENCHMARK (Full Scratch Rebuild) ==="
  hyperfine -S \
    --warmup 1 \
    --runs 5 \
    --prepare 'rm -rf build_standard build_unity' \
    'cmake -B build_standard -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITY=OFF > /dev/null && cmake --build build_standard' \
    'cmake -B build_unity -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_UNITY=ON > /dev/null && cmake --build build_unity'

  exit 0
fi

# 4. Command: Build & Run
# Configure if build directory or Ninja files don't exist
if [ ! -f "$BUILD_DIR/build.ninja" ]; then
  echo "==> Configuring $BUILD_DIR (Type: $BUILD_TYPE, Unity: $UNITY)..."
  cmake -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DENABLE_UNITY="$UNITY"
fi

echo "==> Building target 'ryzc' ($BUILD_TYPE)..."
cmake --build "$BUILD_DIR"

if [ "$CMD" = "run" ]; then
  TARGET_BIN="$BUILD_DIR/ryzc"
  if [ ! -f "$TARGET_BIN" ]; then
    echo "Error: Binary not found at $TARGET_BIN" >&2
    exit 1
  fi
  echo "==> Executing $TARGET_BIN..."
  exec "$TARGET_BIN"
fi
