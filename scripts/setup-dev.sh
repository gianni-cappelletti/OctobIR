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
echo "Installing required development tools..."

# Check for clang-format
if ! command -v clang-format &> /dev/null; then
    echo "Installing clang-format..."
    if [[ "$OSTYPE" == "darwin"* ]]; then
        if ! command -v brew &> /dev/null; then
            echo "Error: Homebrew is required but not installed"
            echo "Install from: https://brew.sh"
            exit 1
        fi
        brew install clang-format
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        sudo apt-get update
        sudo apt-get install -y clang-format
    else
        echo "Error: Unsupported platform for automatic installation"
        echo "Please manually install clang-format"
        exit 1
    fi
else
    echo "✓ clang-format already installed"
fi

# Check for clang-tidy
if ! command -v clang-tidy &> /dev/null; then
    echo "Installing clang-tidy..."
    if [[ "$OSTYPE" == "darwin"* ]]; then
        brew install llvm
        echo "Note: You may need to add LLVM to your PATH:"
        echo "  export PATH=\"/opt/homebrew/opt/llvm/bin:\$PATH\""
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        sudo apt-get install -y clang-tidy
    else
        echo "Error: Unsupported platform for automatic installation"
        echo "Please manually install clang-tidy"
        exit 1
    fi
else
    echo "✓ clang-tidy already installed"
fi

# Verify both tools are now available
if ! command -v clang-format &> /dev/null || ! command -v clang-tidy &> /dev/null; then
    echo ""
    echo "Error: Required tools are not available after installation"
    echo "Please install manually:"
    echo "  - clang-format: Code formatting"
    echo "  - clang-tidy: Static analysis"
    exit 1
fi

echo "✓ All development tools installed"

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
