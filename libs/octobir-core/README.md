# octobir-core

Platform-agnostic impulse response convolution DSP library.

## Features

- Load WAV impulse responses (mono/stereo)
- Automatic resampling to target sample rate
- FFT-based convolution via WDL ConvolutionEngine
- Zero VCV/JUCE dependencies

## Usage

```cpp
#include <octobir-core/IRProcessor.hpp>

octob::IRProcessor processor;
processor.setSampleRate(48000.0);

std::string error;
if (processor.loadImpulseResponse("/path/to/ir.wav", error)) {
    float inL[512], inR[512], outL[512], outR[512];
    processor.processMono(inL, outL, 512);
} else {
    // Handle error
    std::cerr << "Failed to load IR: " << error << std::endl;
}
```

## API Reference

### IRProcessor

Main convolution processor class.

#### Methods

- `bool loadImpulseResponse(const std::string& filepath, std::string& errorMessage)`
  - Load an impulse response from a WAV file
  - Returns: `true` on success, `false` on failure
  - `errorMessage` will contain error details on failure

- `void setSampleRate(SampleRate sampleRate)`
  - Set the processing sample rate
  - Automatically resamples loaded IR if needed

- `void processMono(const Sample* input, Sample* output, FrameCount numFrames)`
  - Process mono audio through convolution
  - Input and output can point to the same buffer (in-place processing)

- `bool isIRLoaded() const`
  - Check if an IR is currently loaded

- `std::string getCurrentIRPath() const`
  - Get path of currently loaded IR

- `void reset()`
  - Reset the convolution engine state

### IRLoader

Handles WAV file loading and resampling.

#### Methods

- `IRLoadResult loadFromFile(const std::string& filepath)`
  - Load WAV file
  - Returns struct with success status, error message, and IR info

- `bool resampleAndInitialize(WDL_ImpulseBuffer& impulseBuffer, SampleRate targetSampleRate)`
  - Resample loaded IR to target rate
  - Initialize WDL impulse buffer

### AudioBuffer

Simple audio buffer wrapper.

#### Methods

- `void resize(FrameCount numFrames)` - Resize buffer
- `void clear()` - Zero all samples
- `Sample* getData()` - Get raw data pointer
- `FrameCount getNumFrames() const` - Get buffer size

## Dependencies

- **WDL** (Winamp Developmental Library from Cockos)
  - Source: https://github.com/justinfrankel/WDL
  - ConvolutionEngine (`convoengine.cpp`) - FFT-based convolution
  - FFT (`fft.c`) - Fast Fourier Transform
  - Resampler (`resample.cpp`) - Sample rate conversion
  - License: zlib-style (permissive, GPL-compatible)
- **dr_wav** - WAV file loading (header-only)
  - License: Public Domain / MIT No Attribution

## License

GNU General Public License v3.0 (GPL-3.0)

This core library is part of the OctobIR project and is licensed under GPL-3.0 to ensure compatibility with JUCE (GPL-3.0) and VCV Rack (GPL-3.0+) requirements.
