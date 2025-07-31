# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

- `make` - Build the plugin
- `make clean` - Clean build artifacts
- `make dist` - Create distribution package
- `make install` - Install plugin to Rack directory

The build system uses the VCV Rack plugin framework (included via `$(RACK_DIR)/plugin.mk`). The `RACK_DIR` defaults to `../..` but can be overridden.

## Project Structure

This is a VCV Rack plugin called "VCV IR Loader" that implements an impulse response loader module for audio processing.

### Key Files
- `src/opc-vcv-ir.cpp` - Main module implementation with DSP processing
- `src/plugin.cpp` - Plugin initialization and registration  
- `src/plugin.hpp` - Plugin header with declarations
- `plugin.json` - Plugin metadata and module definitions
- `res/opc-vcv-ir-panel.svg` - Front panel graphics
- `Makefile` - Build configuration using VCV Rack framework

### Architecture
- **Module Class**: `Opc_vcv_ir` extends VCV Rack's `Module` base class
- **Widget Class**: `Opc_vcv_irWidget` extends `ModuleWidget` for UI rendering
- **Parameters**: Input gain and output gain knobs
- **I/O**: Single audio input and output
- **Panel**: Custom SVG graphics with standard VCV Rack components

The module currently has placeholder DSP processing in the `process()` method that needs implementation for actual impulse response convolution functionality.

## Development Notes

- Uses standard VCV Rack module patterns with param/input/output enums
- Panel layout uses millimeter-to-pixel conversion (`mm2px()`)  
- Follows VCV Rack widget conventions for knobs, ports, and screws
- Ready for implementation of convolution-based impulse response processing