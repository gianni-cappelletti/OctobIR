#!/bin/bash

set -e

echo "Building all OctobIR plugins with unified CMake build system..."

echo ""
echo "=== Configuring CMake Build ==="
cmake -B build -DCMAKE_BUILD_TYPE=Release

echo ""
echo "=== Building All Plugins ==="
cmake --build build --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "✓ All plugins built successfully"
echo ""
echo "JUCE plugin: build/plugins/juce-multiformat/OctobIR_artefacts/"
echo "VCV plugin: plugins/vcv-rack/plugin.dylib (or .so/.dll)"
