#!/bin/bash

set -e

echo "Building all OctobIR plugins with unified CMake build system..."

echo ""
echo "=== Pre-flight Checks ==="

if ! command -v cmake &> /dev/null
then
  echo "Error: cmake not found. Please install CMake (https://cmake.org/)"
  exit 1
fi
echo "✓ cmake found: $(cmake --version | head -1)"

if ! command -v ninja &> /dev/null && ! command -v make &> /dev/null
then
  echo "Error: Neither ninja nor make found. Please install a build system."
  exit 1
fi
if command -v ninja &> /dev/null
then
  echo "✓ ninja found"
  BUILD_SYSTEM="-G Ninja"
else
  echo "✓ make found"
  BUILD_SYSTEM=""
fi

echo ""
echo "=== Configuring CMake Build ==="
cmake -B build -DCMAKE_BUILD_TYPE=Release $BUILD_SYSTEM

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/lib/platform.sh"
NUM_CORES=$(detect_cpu_cores)

echo ""
echo "=== Building All Plugins ==="
cmake --build build --config Release -j$NUM_CORES

echo ""
echo "✓ All plugins built successfully"
echo ""
echo "JUCE plugin: build/plugins/juce-multiformat/OctobIR_artefacts/"
echo "VCV plugin: plugins/vcv-rack/plugin.dylib (or .so/.dll)"
