#!/bin/bash

set -e

echo "Setting up OctobIR development environment..."

echo "Initializing git submodules..."
git submodule update --init --recursive

if [ ! -d "third_party/JUCE" ]; then
    echo "Error: JUCE submodule not found"
    exit 1
fi

if [ ! -d "third_party/iPlug2" ]; then
    echo "Error: iPlug2 submodule not found"
    exit 1
fi

echo "✓ Submodules initialized"

echo "Building octob-ir-core library..."
cmake -B build/core -S libs/octob-ir-core -DCMAKE_BUILD_TYPE=Debug
cmake --build build/core

echo "✓ Core library built"

echo ""
echo "Setup complete! Next steps:"
echo ""
echo "  JUCE Plugin:"
echo "    cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "    cmake --build build --config Release"
echo ""
echo "  VCV Rack Plugin:"
echo "    cd plugins/vcv-rack && make"
echo ""
