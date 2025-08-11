# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

- `make` - Build the plugin
- `make clean` - Clean build artifacts
- `make dist` - Create distribution package
- `make install` - Install plugin to Rack directory

The build system uses the VCV Rack plugin framework (included via `$(RACK_DIR)/plugin.mk`). The `RACK_DIR` defaults to `../..` but can be overridden.

## Project Structure

This is a VCV Rack plugin called "VCV IR Loader" that implements an impulse response convolution module.

### Key Files
- `src/opc-vcv-ir.cpp` - Main module implementation with WDL convolution processing
- `src/plugin.cpp` - Plugin initialization and registration  
- `src/plugin.hpp` - Plugin header with declarations
- `plugin.json` - Plugin metadata and module definitions
- `res/opc-vcv-ir-panel.svg` - Front panel graphics
- `Makefile` - Build configuration with WDL dependencies
- `THIRD_PARTY_NOTICES.txt` - Third-party library attributions

### Third-Party Dependencies
- `iPlug2/` - Git submodule containing WDL (Winamp Developmental Library)
- **WDL ConvolutionEngine**: FFT-based convolution with partitioned processing
- **WDL Resampler**: High-quality sample rate conversion
- **WDL FFT**: Fast Fourier Transform implementation
- **dr_wav**: WAV file loading (header-only library)

### Architecture

#### Core Components
- **Module Class**: `Opc_vcv_ir` extends VCV Rack's `Module` base class
- **Widget Class**: `Opc_vcv_irWidget` extends `ModuleWidget` for UI rendering
- **Convolution Engine**: `WDL_ConvolutionEngine` with `WDL_ImpulseBuffer`
- **Resampling**: VCV Rack `SampleRateConverter` and WDL resampler integration

#### Parameters & I/O
- **Parameters**: Single gain knob (-20dB to +20dB, 0.1dB steps)
- **I/O**: Mono audio input and output (10V peak)

#### DSP Processing Chain
1. **IR Loading**: Loads `~/ir_sample.wav` on startup via dr_wav
2. **Channel Processing**: Converts stereo/multi-channel to mono
3. **Gain Compensation**: Applies -17dB normalization to IR
4. **Sample Rate Conversion**: Resamples IR to match system sample rate if needed
5. **WDL Convolution**: FFT-based convolution with automatic brute force/FFT selection
6. **Real-time Processing**: Per-sample convolution with latency compensation

#### Features
- **Automatic Resampling**: Handles IR files at different sample rates
- **Efficient Convolution**: WDL engine automatically chooses optimal processing method
- **Sample Rate Adaptation**: Reprocesses IR when system sample rate changes
- **Professional Quality**: Uses battle-tested WDL library from audio industry

## Development Notes

- Uses standard VCV Rack module patterns with param/input/output enums
- Panel layout uses millimeter-to-pixel conversion (`mm2px()`)  
- Follows VCV Rack widget conventions for knobs, ports, and screws
- WDL integration provides efficient convolution processing
- Includes proper third-party attribution and licensing compliance
- Build system integrates WDL sources automatically via git submodule

## Performance Characteristics

- **Short IRs**: Uses brute force convolution for minimal latency
- **Long IRs**: Automatic partitioned FFT for efficiency
- **Memory Usage**: Optimized impulse buffer management
- **CPU Usage**: Scales well with IR length due to FFT partitioning
- **Latency**: Minimal latency with WDL's optimized processing