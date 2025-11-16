## Project Summary

This is a monorepo containing the OctobIR impulse response convolution processor, available in multiple formats:
- **VCV Rack Plugin**: "VCV IR Loader" module for VCV Rack
- **JUCE Plugin**: VST3/AU/Standalone multi-format plugin
- **Core Library**: Shared DSP library (`octob-ir-core`) used by both plugin formats

#### Building

The project uses a unified CMake build system for all components.

**Quick Start:**
```bash
# Initial setup
./scripts/setup-dev.sh

# Build all plugins
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Or use the convenience script
./scripts/build-all.sh
```

**Build Specific Targets:**
- `cmake --build build --target octob-ir-core` - Core library only
- `cmake --build build --target OctobIR` - JUCE plugin only
- `cmake --build build --target vcv-plugin` - VCV Rack plugin only

**VCV-Specific Targets:**
- `cmake --build build --target vcv-plugin-install` - Install VCV plugin to Rack directory
- `cmake --build build --target vcv-plugin-dist` - Create VCV distribution package
- `cmake --build build --target vcv-plugin-clean` - Clean VCV build artifacts
- `cmake --build build --target vcv-plugin-format` - Format VCV plugin code
- `cmake --build build --target vcv-plugin-format-check` - Check VCV plugin code formatting

**Build Options:**
- `-DBUILD_CORE_LIB=ON/OFF` - Build core library (default: ON)
- `-DBUILD_JUCE_PLUGIN=ON/OFF` - Build JUCE plugin (default: ON)
- `-DBUILD_VCV_PLUGIN=ON/OFF` - Build VCV Rack plugin (default: ON)

**Note:** The VCV Rack plugin internally uses the VCV Rack plugin framework (via `$(RACK_DIR)/plugin.mk`), but is wrapped in CMake for unified builds. The `RACK_DIR` defaults to `../..` but can be overridden via environment variable.

#### Third-Party Dependencies
- `JUCE/` - Git submodule containing JUCE audio framework (for JUCE plugin)
- `iPlug2/` - Git submodule containing WDL (Winamp Developmental Library)
  - **WDL ConvolutionEngine**: FFT-based convolution with partitioned processing
  - **WDL FFT**: Fast Fourier Transform implementation
- **dr_wav**: WAV file loading (header-only library)

# Development Rules

- Avoid unnecessary or redundant comments. The code should be written with good variable names in a way where the functionality is clear. Only add a comment if it is a complicated operation.
- Adhere to the .clang-format rules when generating code
- Include good logger messages in all branching paths
- Be concise in the code, but do not sacrifice clarity for brevity
- Follow Google's C++ style guide in the code as closely as possible with C++ 11
- For UI components, double check that the colors, fonts, and sizes used adhere to the Web Content Accessibility Guidelines (WCAG) 2.1
- When testing builds, always perform a clean build: `rm -rf build && cmake -B build && cmake --build build`
- For VCV plugin testing specifically, use: `cmake --build build --target vcv-plugin-clean && cmake --build build --target vcv-plugin && cmake --build build --target vcv-plugin-install`