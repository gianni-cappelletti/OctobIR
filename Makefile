.PHONY: help vcv juce core test clean check release

help:
	@echo "Development build targets (always clean build):"
	@echo "  make vcv         - Clean, build, and install VCV plugin (debug)"
	@echo "  make juce        - Clean and build JUCE plugin (debug)"
	@echo "  make core        - Clean and build core library only"
	@echo "  make test        - Build and run unit tests"
	@echo ""
	@echo "Code quality:"
	@echo "  make check       - Run formatting and static analysis checks"
	@echo "  make format      - Auto-format all code with clang-format"
	@echo ""
	@echo "Other:"
	@echo "  make clean       - Remove all build artifacts"
	@echo "  make release     - Run checks and build all plugins (release)"

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
check:
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

# Release build
release: check
	@cmake -B build/release -DCMAKE_BUILD_TYPE=Release
	@cmake --build build/release --config Release -j$$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@echo "All plugins built in release mode"
	@echo "JUCE plugin: build/release/plugins/juce-multiformat/OctobIR_artefacts/"
	@echo "VCV plugin: plugins/vcv-rack/plugin.dylib"