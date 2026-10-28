#!/usr/bin/env bash
# Builds web/rangka.wasm: the same C++ core as the CLI, compiled to WebAssembly for the web page.
#   scripts/build_wasm.sh [version]
# Needs Zig's bundled clang (pip install ziglang), which ships a wasm32-wasi C++ standard library.
set -euo pipefail
cd "$(dirname "$0")/.."

version=${1:-$(sed -n 's/^project(rangka VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)}
zig=${ZIG:-python3 -m ziglang}

# the template embedder runs natively
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release -DRANGKA_BUILD_TESTS=OFF > /dev/null
cmake --build build/native --target rangka_embed > /dev/null
mkdir -p build/wasm
build/native/rangka_embed templates build/wasm/templates.cpp

# -fno-exceptions: the core never throws (errors are Result values), and wasi's libc++ is built without them
$zig c++ -std=c++20 -Os -target wasm32-wasi -fno-exceptions -fno-rtti -mexec-model=reactor \
  -Isrc -DRANGKA_VERSION="\"$version\"" \
  src/core/catalog.cpp src/core/generator.cpp src/core/names.cpp src/core/template.cpp src/core/zip.cpp \
  build/wasm/templates.cpp src/wasm/exports.cpp \
  -o web/rangka.wasm -s

echo "web/rangka.wasm: $(wc -c < web/rangka.wasm) bytes (version $version)"
