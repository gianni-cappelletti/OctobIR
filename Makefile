.PHONY: help vcv juce core test clean release

help:
	@echo "Development build targets (always clean build):"
	@echo "  make vcv         - Clean, build, and install VCV plugin (debug)"
	@echo "  make juce        - Clean and build JUCE plugin (debug)"
	@echo "  make core        - Clean and build core library only"
	@echo "  make test        - Build and run unit tests"
	@echo "  make clean       - Remove all build artifacts"
	@echo ""
	@echo "Release builds:"
	@echo "  make release     - Build all plugins (release)"

# VCV Plugin Development (always clean build)
vcv:
	@cmake --preset dev-vcv 2>/dev/null || cmake -B build/dev-vcv -DCMAKE_BUILD_TYPE=Debug -DBUILD_JUCE_PLUGIN=OFF
	@cmake --build build/dev-vcv --target vcv-plugin-clean 2>/dev/null || true
	@cmake --build build/dev-vcv --target vcv-plugin -j$$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@cmake --build build/dev-vcv --target vcv-plugin-install
	@echo "VCV plugin cleaned, built, and installed"

# JUCE Plugin Development (always clean build)
juce:
	@rm -rf build/dev-juce
	@cmake --preset dev-juce 2>/dev/null || cmake -B build/dev-juce -DCMAKE_BUILD_TYPE=Debug -DBUILD_VCV_PLUGIN=OFF
	@cmake --build build/dev-juce --target OctobIR -j$$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@echo "JUCE plugin cleaned and built: build/dev-juce/plugins/juce-multiformat/OctobIR_artefacts/"

# Core Library (always clean build)
core:
	@rm -rf build/dev
	@cmake --preset dev 2>/dev/null || cmake -B build/dev -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build/dev --target octobir-core -j$$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@echo "Core library cleaned and built"

# Unit Tests (always clean build)
test:
	@rm -rf build/test
	@cmake -B build/test -DCMAKE_BUILD_TYPE=Debug -DBUILD_JUCE_PLUGIN=OFF -DBUILD_VCV_PLUGIN=OFF -DBUILD_TESTS=ON
	@cmake --build build/test --target octobir-core-tests -j$$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@echo "Running tests..."
	@./build/test/libs/octobir-core/tests/octobir-core-tests

# Clean target
clean:
	@rm -rf build
	@echo "All build artifacts cleaned"

# Release build
release:
	@cmake -B build/release -DCMAKE_BUILD_TYPE=Release
	@cmake --build build/release --config Release -j$$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@echo "All plugins built in release mode"
	@echo "JUCE plugin: build/release/plugins/juce-multiformat/OctobIR_artefacts/"
	@echo "VCV plugin: plugins/vcv-rack/plugin.dylib"