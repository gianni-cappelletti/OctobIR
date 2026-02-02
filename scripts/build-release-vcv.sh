#!/usr/bin/env bash
# Build VCV Rack plugin release package locally (mimics CI release workflow)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Load shared utilities
source "$SCRIPT_DIR/lib/platform.sh"
source "$SCRIPT_DIR/vcv-sdk-config.sh"

# Detect platform
VCV_PLATFORM=$(detect_vcv_platform)
if [ $? -ne 0 ]; then
  exit 1
fi

echo "Building VCV Rack plugin release package..."
echo "==========================================="
echo "Platform: $VCV_PLATFORM"
echo "SDK Version: $VCV_SDK_VERSION"
echo ""

echo "Pre-flight checks..."
if ! command -v make &> /dev/null
then
  echo "Error: make not found. Please install build tools."
  exit 1
fi
echo "✓ make found"

if ! command -v unzip &> /dev/null
then
  echo "Error: unzip not found. Please install unzip utility."
  exit 1
fi
echo "✓ unzip found"

echo ""
echo "Syncing plugin version from VERSION file..."
"$SCRIPT_DIR/sync-vcv-version.sh"

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
    SDK_FILE="Rack-SDK-${VCV_SDK_VERSION}-${VCV_PLATFORM}.zip"

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

    echo "Verifying SDK integrity..."
    if command -v sha256sum &> /dev/null
    then
      echo "${VCV_SDK_SHA256}  ${SDK_FILE}" | sha256sum -c -
    elif command -v shasum &> /dev/null
    then
      echo "${VCV_SDK_SHA256}  ${SDK_FILE}" | shasum -a 256 -c -
    else
      echo "Warning: Neither sha256sum nor shasum found. Skipping integrity check."
    fi

    echo "Extracting SDK..."
    unzip -q "$SDK_FILE"
    mv Rack-SDK Rack
    rm "$SDK_FILE"
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
