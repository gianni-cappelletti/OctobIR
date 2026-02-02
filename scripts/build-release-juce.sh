#!/usr/bin/env bash
# Build JUCE plugin release package locally (mimics CI release workflow)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_DIR"

echo "Building JUCE plugin release package..."
echo "========================================"
echo ""

# Clean previous builds
echo "Cleaning previous builds..."
rm -rf build/release-juce
rm -rf package

# Build
echo "Building JUCE plugin (Release)..."
cmake -B build/release-juce \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_JUCE_PLUGIN=ON \
  -DBUILD_VCV_PLUGIN=OFF \
  -DBUILD_TESTS=OFF

cmake --build build/release-juce --config Release -j

# Create package directory
echo "Creating package directory..."
mkdir -p package/OctobIR

# Copy artifacts
echo "Copying plugin artifacts..."

if [ -d "build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/VST3/OctobIR.vst3" ]
then
  echo "  - VST3 plugin"
  cp -r build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/VST3/OctobIR.vst3 package/OctobIR/
fi

if [ -d "build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/AU/OctobIR.component" ]
then
  echo "  - AU plugin"
  cp -r build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/AU/OctobIR.component package/OctobIR/
fi

if [ -d "build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/Standalone/OctobIR.app" ]
then
  echo "  - Standalone app"
  cp -r build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/Standalone/OctobIR.app package/OctobIR/
fi

if [ -f "build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/Standalone/OctobIR.exe" ]
then
  echo "  - Standalone exe"
  cp build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/Standalone/OctobIR.exe package/OctobIR/
fi

if [ -f "build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/Standalone/OctobIR" ]
then
  echo "  - Standalone binary"
  cp build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release/Standalone/OctobIR package/OctobIR/
fi

# Copy documentation
echo "Copying documentation..."
cp LICENSE package/OctobIR/
cp README.md package/OctobIR/

# Create installer based on platform
echo ""
echo "Creating installer package..."
PLATFORM=$(uname -s)

case "$PLATFORM" in
  Darwin*)
    echo "Platform: macOS - Creating DMG..."
    hdiutil create -volname "OctobIR" -srcfolder package/OctobIR -ov -format UDZO OctobIR-macOS.dmg
    echo "✓ Created: OctobIR-macOS.dmg"
    ;;
  Linux*)
    echo "Platform: Linux - Creating tarball..."
    cd package
    tar -czf ../OctobIR-Linux.tar.gz OctobIR
    cd ..
    echo "✓ Created: OctobIR-Linux.tar.gz"
    ;;
  MINGW*|MSYS*|CYGWIN*)
    echo "Platform: Windows - Creating ZIP..."
    cd package
    if command -v 7z &> /dev/null
    then
      7z a -tzip ../OctobIR-Windows.zip OctobIR
    else
      zip -r ../OctobIR-Windows.zip OctobIR
    fi
    cd ..
    echo "✓ Created: OctobIR-Windows.zip"
    ;;
  *)
    echo "Unknown platform: $PLATFORM"
    echo "Creating tarball as fallback..."
    cd package
    tar -czf ../OctobIR-$PLATFORM.tar.gz OctobIR
    cd ..
    echo "✓ Created: OctobIR-$PLATFORM.tar.gz"
    ;;
esac

echo ""
echo "========================================"
echo "JUCE plugin release package built successfully!"
echo ""
echo "Package contents:"
ls -lh package/OctobIR/
echo ""
echo "Installer files:"
ls -lh OctobIR-* 2>/dev/null || echo "No installer files found"
