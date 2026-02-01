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
echo "Checking development tools..."

MISSING_TOOLS=()

if ! command -v clang-format &> /dev/null; then
    echo "⚠ clang-format not found"
    MISSING_TOOLS+=("clang-format")
else
    echo "✓ clang-format installed"
fi

if ! command -v clang-tidy &> /dev/null; then
    echo "⚠ clang-tidy not found"
    MISSING_TOOLS+=("clang-tidy")
else
    echo "✓ clang-tidy installed"
fi

if [ ${#MISSING_TOOLS[@]} -gt 0 ]; then
    echo ""
    echo "Optional development tools are missing. Install them with:"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "  brew install llvm"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "  sudo apt-get install clang-format clang-tidy"
    fi
    echo ""
    echo "These tools enable:"
    echo "  - clang-format: Automatic code formatting"
    echo "  - clang-tidy: Static analysis and linting"
fi

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
