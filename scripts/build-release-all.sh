#!/usr/bin/env bash
# DEPRECATED: Use 'make installers' instead
# This script is kept for backwards compatibility

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "========================================"
echo "⚠ DEPRECATED SCRIPT"
echo "========================================"
echo ""
echo "This script has been deprecated."
echo "Please use the following commands instead:"
echo ""
echo "  make installers  - Build JUCE distributable installers"
echo "  make release     - Build and install release versions locally"
echo ""
echo "For VCV Rack testing:"
echo "  ./scripts/build-release-vcv.sh"
echo ""
echo "Redirecting to: make installers"
echo ""

cd "$SCRIPT_DIR/.."
make installers
