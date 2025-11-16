#!/bin/bash

set -e

echo "Setting up OctobIR development environment..."

echo "Initializing git submodules..."
git submodule update --init --recursive

if [ ! -d "third_party/JUCE" ]; then
    echo "Error: JUCE submodule not found"
    exit 1
fi

if [ ! -d "third_party/WDL" ]; then
    echo "Error: WDL submodule not found"
    exit 1
fi

echo "✓ Submodules initialized"

echo ""
echo "Installing git hooks..."
if [ -f "scripts/hooks/pre-commit" ]; then
    cp scripts/hooks/pre-commit .git/hooks/pre-commit
    chmod +x .git/hooks/pre-commit
    echo "✓ Pre-commit hook installed"
else
    echo "Warning: pre-commit hook not found in scripts/hooks/"
fi

echo ""
echo "Setup complete! Next steps:"
echo ""
echo "  Build all plugins (unified CMake system):"
echo "    cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "    cmake --build build --config Release"
echo ""
echo "  Or use the build script:"
echo "    ./scripts/build-all.sh"
echo ""
echo "  Build specific targets:"
echo "    cmake --build build --target octobir-core      # Core library only"
echo "    cmake --build build --target OctobIR            # JUCE plugin only"
echo "    cmake --build build --target vcv-plugin         # VCV plugin only"
echo ""
echo "  VCV-specific targets:"
echo "    cmake --build build --target vcv-plugin-install # Install VCV plugin"
echo "    cmake --build build --target vcv-plugin-dist    # Create VCV distribution"
echo "    cmake --build build --target vcv-plugin-clean   # Clean VCV build"
echo ""
