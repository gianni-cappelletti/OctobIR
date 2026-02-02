.PHONY: help vcv juce core test clean tidy format release installers

help:
	@echo "Development build targets (always clean build):"
	@echo "  make vcv         - Clean, build, and install VCV plugin (debug)"
	@echo "  make juce        - Clean and build JUCE plugin (debug)"
	@echo "  make core        - Clean and build core library only"
	@echo "  make test        - Build and run unit tests"
	@echo ""
	@echo "Code quality:"
	@echo "  make tidy        - Run formatting and static analysis checks"
	@echo "  make format      - Auto-format all code with clang-format"
	@echo ""
	@echo "Release builds:"
	@echo "  make release     - Build and install release versions locally"
	@echo "  make installers  - Build JUCE distributable installers (DMG/ZIP/tarball)"
	@echo ""
	@echo "Other:"
	@echo "  make clean       - Remove all build artifacts"

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

# Code quality checks
tidy:
	@echo "Checking code formatting..."
	@if ! command -v clang-format &> /dev/null; then \
		echo "Error: clang-format not installed. Run ./scripts/setup-dev.sh"; \
		exit 1; \
	fi
	@find libs plugins -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | \
		xargs clang-format --dry-run --Werror --style=file || \
		(echo "Error: Code is not properly formatted. Run 'make format' to fix." && exit 1)
	@echo "✓ Code formatting verified"
	@echo ""
	@echo "Running static analysis..."
	@if ! command -v clang-tidy &> /dev/null; then \
		echo "Error: clang-tidy not installed. Run ./scripts/setup-dev.sh"; \
		exit 1; \
	fi
	@find libs/octobir-core/src -name "*.cpp" | \
		xargs -I {} clang-tidy {} -- \
		-I./libs/octobir-core/include \
		-I./third_party/WDL/WDL \
		-I./third_party \
		-std=c++11 || \
		(echo "Error: Static analysis found issues" && exit 1)
	@if [ -n "$$RACK_DIR" ] && [ -d "$$RACK_DIR" ]; then \
		find plugins/vcv-rack/src -name "*.cpp" | \
			xargs -I {} clang-tidy {} -- \
			-I./libs/octobir-core/include \
			-I./third_party/WDL/WDL \
			-I./third_party \
			-I$$RACK_DIR/include \
			-I$$RACK_DIR/dep/include \
			-std=c++11 || \
			(echo "Error: Static analysis found issues in VCV plugin" && exit 1); \
	else \
		echo "  Skipping VCV plugin analysis (RACK_DIR not set or not found)"; \
	fi
	@echo "✓ Static analysis passed"
	@echo ""
	@echo "All code quality checks passed"

# Auto-format code
format:
	@echo "Formatting code..."
	@if ! command -v clang-format &> /dev/null; then \
		echo "Error: clang-format not installed. Run ./scripts/setup-dev.sh"; \
		exit 1; \
	fi
	@find libs plugins -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | \
		xargs clang-format -i --style=file
	@echo "Code formatted"

# Build and install release versions locally (for testing)
release:
	@echo "Building release versions of all plugins..."
	@echo ""
	@echo "Building JUCE plugin (Release)..."
	@cmake -B build/release -DCMAKE_BUILD_TYPE=Release
	@cmake --build build/release --config Release -j$$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@echo "✓ JUCE plugin built"
	@echo "  VST3: ~/Library/Audio/Plug-Ins/VST3/OctobIR.vst3"
	@echo "  AU:   ~/Library/Audio/Plug-Ins/Components/OctobIR.component"
	@echo ""
	@if [ -n "$$RACK_DIR" ] || [ -d "../Rack" ]; then \
		echo "Building VCV plugin (Release)..."; \
		cd plugins/vcv-rack && \
		export RACK_DIR=$${RACK_DIR:-../../../Rack} && \
		make clean && make -j$$(sysctl -n hw.ncpu 2>/dev/null || echo 4) && \
		make install && \
		echo "✓ VCV plugin built and installed"; \
	else \
		echo "⚠ Skipping VCV plugin (RACK_DIR not set)"; \
		echo "  Set RACK_DIR or run: ./scripts/build-release-vcv.sh"; \
	fi
	@echo ""
	@echo "✓ Release builds complete"

# Build JUCE distributable installers (DMG/ZIP/tarball)
installers:
	@./scripts/build-release-juce.sh
	@echo ""
	@echo "Note: VCV Rack plugin is distributed via VCV Library (source submission)."
	@echo "To test VCV build locally: ./scripts/build-release-vcv.sh"