#!/bin/bash
# Build the project for WebAssembly using Emscripten.
# Requires emsdk installed and EMSDK environment variable set.
#
# One-time setup:
#   git clone https://github.com/emscripten-core/emsdk.git
#   cd emsdk && ./emsdk install latest && ./emsdk activate latest
#   source emsdk_env.sh   (sets EMSDK, PATH, etc.)

if [ -z "$EMSDK" ]; then
    echo "ERROR: EMSDK not set. Source emsdk_env.sh first."
    exit 1
fi

source "$EMSDK/emsdk_env.sh" 2>/dev/null

PRESET=${1:-wasm-release}

cmake --preset "$PRESET" && cmake --build --preset "$PRESET"
