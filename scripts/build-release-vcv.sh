#!/usr/bin/env bash
# Build VCV Rack plugin release package locally (mimics CI release workflow)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# VCV Rack SDK version (should match ci.yml)
VCV_SDK_VERSION="2.5.2"

# Detect platform
PLATFORM=$(uname -s)
ARCH=$(uname -m)

case "$PLATFORM" in
  Darwin*)
    if [ "$ARCH" = "arm64" ]
    then
      VCV_PLATFORM="mac-arm64"
    else
      VCV_PLATFORM="mac-x64"
    fi
    ;;
  Linux*)
    VCV_PLATFORM="lin-x64"
    ;;
  MINGW*|MSYS*|CYGWIN*)
    VCV_PLATFORM="win-x64"
    ;;
  *)
    echo "Error: Unsupported platform: $PLATFORM"
    exit 1
    ;;
esac

echo "Building VCV Rack plugin release package..."
echo "==========================================="
echo "Platform: $VCV_PLATFORM"
echo "SDK Version: $VCV_SDK_VERSION"
echo ""

# Check if RACK_DIR is set
if [ -z "$RACK_DIR" ]
then
  RACK_DIR="$PROJECT_DIR/../Rack"
  echo "RACK_DIR not set, using: $RACK_DIR"

  # Download SDK if needed
  if [ ! -d "$RACK_DIR" ]
  then
    echo ""
    echo "VCV Rack SDK not found. Downloading..."
    SDK_URL="https://vcvrack.com/downloads/Rack-SDK-${VCV_SDK_VERSION}-${VCV_PLATFORM}.zip"

    mkdir -p "$PROJECT_DIR/../"
    cd "$PROJECT_DIR/../"

    if command -v wget &> /dev/null
    then
      wget -q --show-progress "$SDK_URL"
    elif command -v curl &> /dev/null
    then
      curl -L -O "$SDK_URL"
    else
      echo "Error: Neither wget nor curl found. Please install one or set RACK_DIR manually."
      exit 1
    fi

    echo "Extracting SDK..."
    unzip -q "Rack-SDK-${VCV_SDK_VERSION}-${VCV_PLATFORM}.zip"
    mv Rack-SDK Rack
    rm "Rack-SDK-${VCV_SDK_VERSION}-${VCV_PLATFORM}.zip"
    echo "✓ SDK downloaded and extracted"
  fi
else
  echo "Using RACK_DIR: $RACK_DIR"
fi

# Verify RACK_DIR exists
if [ ! -d "$RACK_DIR" ]
then
  echo "Error: RACK_DIR not found at: $RACK_DIR"
  exit 1
fi

cd "$PROJECT_DIR/plugins/vcv-rack"

# Clean previous builds
echo ""
echo "Cleaning previous builds..."
make clean 2>/dev/null || true
rm -rf dist

# Build dependencies
echo "Building dependencies..."
export RACK_DIR
make dep

# Build plugin distribution package
echo ""
echo "Building plugin distribution package..."
make dist

# Check results
echo ""
echo "==========================================="
if [ -d "dist" ] && [ -n "$(ls -A dist/*.vcvplugin 2>/dev/null)" ]
then
  echo "✓ VCV plugin release package built successfully!"
  echo ""
  echo "Distribution package:"
  ls -lh dist/*.vcvplugin
  echo ""
  echo "To test: Copy the .vcvplugin file to your VCV Rack plugins directory"
  echo "macOS: ~/Documents/Rack2/plugins-mac-arm64/ (or plugins-mac-x64/)"
  echo "Linux: ~/.Rack2/plugins-lin-x64/"
  echo "Windows: %USERPROFILE%\\Documents\\Rack2\\plugins-win-x64\\"
else
  echo "✗ Build failed: No .vcvplugin file found"
  exit 1
fi
