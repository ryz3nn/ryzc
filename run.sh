#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-debug}"
[[ $# -gt 0 ]] && shift

BUILD_DIR="build/${MODE,,}"
BUILD_TYPE="${MODE^}"

# Check host OpenGL version
if command -v glxinfo &>/dev/null; then
    GL_VER=$(glxinfo -B 2>/dev/null |
        awk -F': ' '/OpenGL (core profile )?version string/ {print $2; exit}' |
        grep -oE '^[0-9]+\.[0-9]+' || true)

    echo "==> Host OpenGL: ${GL_VER:-unknown}"

    # Enable software rendering if OpenGL is lower than 3.0
    if [[ -n "$GL_VER" ]]; then
        if awk -v ver="$GL_VER" 'BEGIN {
            split(ver, v, ".")
            exit !((v[1] + 0) < 3 || ((v[1] + 0) == 3 && (v[2] + 0) < 0))
        }'; then
            export LIBGL_ALWAYS_SOFTWARE=1
            echo "==> OpenGL < 3.0: enabled software rendering"
        fi
    else
        echo "==> Warning: Could not detect OpenGL version"
    fi
else
    echo "==> Warning: glxinfo not found; skipping OpenGL check"
fi

echo "==> Building ${BUILD_TYPE}..."
cmake -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build "$BUILD_DIR" --parallel

echo "==> Running ryzc..."
exec "$BUILD_DIR/ryzc" "$@"
