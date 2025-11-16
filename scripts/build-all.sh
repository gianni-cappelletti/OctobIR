#!/bin/bash

set -e

echo "Building all OctobIR plugins..."

echo ""
echo "=== Building JUCE Plugin ==="
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

echo ""
echo "=== Building VCV Rack Plugin ==="
cd plugins/vcv-rack
make clean
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cd ../..

echo ""
echo "✓ All plugins built successfully"
echo ""
echo "JUCE plugin: build/plugins/juce-multiformat/OctobIR_artefacts/"
echo "VCV plugin: plugins/vcv-rack/plugin.dylib (or .so/.dll)"
