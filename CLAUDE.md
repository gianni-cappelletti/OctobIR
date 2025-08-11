## Project Summary

This is a VCV Rack plugin called "VCV IR Loader" that implements an impulse response convolution module.

#### Building

- `make` - Build the plugin
- `make clean` - Clean build artifacts
- `make dist` - Create distribution package
- `make install` - Install plugin to Rack directory

The build system uses the VCV Rack plugin framework (included via `$(RACK_DIR)/plugin.mk`). The `RACK_DIR` defaults to `../..` but can be overridden.

#### Third-Party Dependencies
- `iPlug2/` - Git submodule containing WDL (Winamp Developmental Library)
  - **WDL ConvolutionEngine**: FFT-based convolution with partitioned processing
  - **WDL FFT**: Fast Fourier Transform implementation
- **dr_wav**: WAV file loading (header-only library)

## Development Notes

- Avoid unnecessary or redundant comments. The code should be written with good variable names in a way where the functionality is clear. Only add a comment if it is a complicated operation.
- Adhere to the .clang-format rules when generating code
- Include good logger messages in all branching paths
- Be concise in the code, but do not sacrifice clarity for brevity
- Follow Google's C++ style guide in the code as closely as possible with C++ 11, upon which VCV Rack is built
- For UI components, double check that the colors, fonts, and sizes used adhere to the Web Content Accessibility Guidelines (WCAG) 2.1
- When running `make` to check that things work, run the command `make clean && make install` so it is a clean build that installs as well for testing