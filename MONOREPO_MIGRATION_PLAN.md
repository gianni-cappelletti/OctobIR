# OctobIR Monorepo Migration Plan

## Overview

Transform the VCV Rack-centric OctobIR repository into a monorepo supporting multiple plugin formats (VCV Rack, VST3, AU, AAX) using JUCE framework with shared DSP code.

## Current State

### Repository Structure
```
OctobIR/
├── src/
│   ├── opc-vcv-ir.cpp          # 343 lines - ALL logic (DSP + UI mixed)
│   ├── plugin.cpp              # 8 lines - VCV entry point
│   └── plugin.hpp              # 7 lines - VCV declarations
├── res/
│   ├── OpcVcvIr.svg           # Panel design
│   └── fonts/ShareTechMono-Regular.ttf
├── iPlug2/                     # Git submodule (WDL only currently)
├── Makefile                    # VCV Rack build system
└── plugin.json                 # VCV Rack metadata
```

### Code Analysis (opc-vcv-ir.cpp)

**Shareable DSP Code (~180 lines, 50%):**
- `LoadIR()` function - WAV loading with dr_wav, resampling logic
- `ResampleAndInitialize()` - Sample rate conversion + WDL engine setup
- `process()` audio loop - Convolution processing core
- State variables: `impulseBuffer`, `engine`, `sampleRate`, `irSampleRate`, etc.

**VCV Rack-Specific UI (~140 lines, 39%):**
- `IrFileDisplay` widget - Custom text display widget
- `OpcVcvIrWidget` - Panel layout with file browser button
- VCV Module integration - I/O configuration

**VCV Framework Integration (~38 lines, 11%):**
- Module registration, model creation

### Dependencies
- **WDL ConvolutionEngine** (via iPlug2 submodule) - FFT-based convolution
- **WDL FFT** (via iPlug2 submodule) - Fast Fourier Transform
- **dr_wav** (header-only) - WAV file loading
- **VCV Rack SDK** - VCV-specific (stays in VCV plugin only)

## Target Monorepo Structure

```
OctobIR/
├── libs/
│   ├── octob-ir-core/                    # Shared DSP library
│   │   ├── include/
│   │   │   └── octob-ir-core/
│   │   │       ├── IRProcessor.hpp       # Main convolution processor class
│   │   │       ├── IRLoader.hpp          # WAV loading + resampling
│   │   │       ├── AudioBuffer.hpp       # Platform-agnostic buffer wrapper
│   │   │       └── Types.hpp             # Common types, constants
│   │   ├── src/
│   │   │   ├── IRProcessor.cpp
│   │   │   ├── IRLoader.cpp
│   │   │   └── AudioBuffer.cpp
│   │   ├── CMakeLists.txt                # Build config for shared lib
│   │   ├── LICENSE                       # MIT (most permissive)
│   │   └── README.md                     # API documentation
│   │
│   └── platform-abstraction/             # Optional: Platform services interface
│       ├── include/
│       │   └── platform-abstraction/
│       │       ├── FileDialog.hpp        # File browser abstraction
│       │       └── Logger.hpp            # Logging abstraction
│       └── src/
│           ├── FileDialog_VCV.cpp
│           ├── FileDialog_JUCE.cpp
│           └── Logger.cpp
│
├── plugins/
│   ├── vcv-rack/                         # VCV Rack plugin (refactored)
│   │   ├── src/
│   │   │   ├── OpcVcvIr.cpp             # VCV wrapper using octob-ir-core
│   │   │   ├── OpcVcvIrWidget.cpp       # UI code (extracted from main)
│   │   │   ├── plugin.cpp
│   │   │   └── plugin.hpp
│   │   ├── res/
│   │   │   ├── OpcVcvIr.svg
│   │   │   └── fonts/ShareTechMono-Regular.ttf
│   │   ├── Makefile                      # Updated to link octob-ir-core
│   │   ├── plugin.json
│   │   └── LICENSE                       # GPL-3.0+ (VCV requirement)
│   │
│   └── juce-multiformat/                 # JUCE plugin (VST3/AU/AAX/Standalone)
│       ├── Source/
│       │   ├── PluginProcessor.h         # JUCE AudioProcessor interface
│       │   ├── PluginProcessor.cpp       # Wraps octob-ir-core
│       │   ├── PluginEditor.h            # UI stub (implement later)
│       │   └── PluginEditor.cpp
│       ├── CMakeLists.txt                # JUCE plugin config
│       ├── LICENSE                       # MIT or AGPLv3
│       └── README.md
│
├── third_party/
│   ├── iPlug2/                           # Existing submodule (keep for WDL)
│   ├── JUCE/                             # New submodule
│   └── dr_wav/                           # Header-only (vendor in or submodule)
│
├── scripts/
│   ├── setup-dev.sh                      # Clone submodules, setup environment
│   └── build-all.sh                      # Build all plugins
│
├── .github/
│   └── workflows/
│       ├── build-vcv.yml                 # VCV Rack CI
│       ├── build-juce.yml                # JUCE plugin CI (later: Pamplejuce)
│       └── validation.yml                # Code formatting, pluginval
│
├── CMakeLists.txt                        # Root CMake (orchestrates builds)
├── .clang-format                         # Existing
├── .gitignore                            # Updated for monorepo
├── .gitmodules                           # Git submodules config
├── LICENSE                               # MIT for overall project
├── README.md                             # Updated monorepo documentation
└── MONOREPO_MIGRATION_PLAN.md           # This document
```

## Migration Steps

### Phase 1: Repository Restructuring

#### 1.1 Create New Directory Structure
```bash
# Create new directories (preserve git history by moving, not creating new)
mkdir -p libs/octob-ir-core/{include/octob-ir-core,src}
mkdir -p libs/platform-abstraction/{include/platform-abstraction,src}
mkdir -p plugins/vcv-rack
mkdir -p plugins/juce-multiformat/Source
mkdir -p third_party
mkdir -p scripts
mkdir -p .github/workflows

# Move existing VCV code
git mv src plugins/vcv-rack/src
git mv res plugins/vcv-rack/res
git mv Makefile plugins/vcv-rack/Makefile
git mv plugin.json plugins/vcv-rack/plugin.json

# Move iPlug2 submodule reference to third_party
git mv iPlug2 third_party/iPlug2
```

#### 1.2 Update .gitignore
Add monorepo-specific ignores:
```gitignore
# Existing VCV ignores
/dist
/plugin.dylib
/plugin.so
/plugin.dll

# CMake build directories
build/
cmake-build-*/

# JUCE
plugins/juce-multiformat/Builds/
*.jucer
JuceLibraryCode/

# IDE
.vscode/
.idea/
*.xcworkspace/
*.xcuserdata/

# Platform
.DS_Store
Thumbs.db
```

#### 1.3 Update .gitmodules
```ini
[submodule "third_party/iPlug2"]
    path = third_party/iPlug2
    url = https://github.com/iPlug2/iPlug2.git

[submodule "third_party/JUCE"]
    path = third_party/JUCE
    url = https://github.com/juce-framework/JUCE.git
    branch = master
```

#### 1.4 Add JUCE Submodule
```bash
git submodule add https://github.com/juce-framework/JUCE.git third_party/JUCE
git submodule update --init --recursive
```

### Phase 2: Extract Shared DSP Library

#### 2.1 Create Core Library Headers

**File: `libs/octob-ir-core/include/octob-ir-core/Types.hpp`**
```cpp
#pragma once

#include <cstddef>

namespace octob {

// Sample rate type
using SampleRate = double;

// Audio sample type (matching VCV's float)
using Sample = float;

// Frame count
using FrameCount = size_t;

// Constants
constexpr int MAX_IR_LENGTH_SECONDS = 10;
constexpr int MIN_SAMPLE_RATE = 8000;
constexpr int MAX_SAMPLE_RATE = 192000;

} // namespace octob
```

**File: `libs/octob-ir-core/include/octob-ir-core/AudioBuffer.hpp`**
```cpp
#pragma once

#include "Types.hpp"
#include <vector>

namespace octob {

// Simple stereo audio buffer
class AudioBuffer {
public:
    AudioBuffer() = default;
    AudioBuffer(FrameCount numFrames);

    void resize(FrameCount numFrames);
    void clear();

    Sample* getLeftChannel() { return left_.data(); }
    Sample* getRightChannel() { return right_.data(); }
    const Sample* getLeftChannel() const { return left_.data(); }
    const Sample* getRightChannel() const { return right_.data(); }

    FrameCount getNumFrames() const { return left_.size(); }

private:
    std::vector<Sample> left_;
    std::vector<Sample> right_;
};

} // namespace octob
```

**File: `libs/octob-ir-core/include/octob-ir-core/IRLoader.hpp`**
```cpp
#pragma once

#include "Types.hpp"
#include <string>
#include <vector>

// Forward declare WDL types (implementation will include WDL headers)
class WDL_ImpulseBuffer;
class WDL_Resampler;

namespace octob {

// Result of IR loading operation
struct IRLoadResult {
    bool success = false;
    std::string errorMessage;
    size_t numSamples = 0;
    int numChannels = 0;
    SampleRate sampleRate = 0.0;
};

class IRLoader {
public:
    IRLoader();
    ~IRLoader();

    // Load IR from WAV file
    IRLoadResult loadFromFile(const std::string& filepath);

    // Resample IR to target sample rate and initialize WDL impulse buffer
    bool resampleAndInitialize(WDL_ImpulseBuffer& impulseBuffer,
                                SampleRate targetSampleRate);

    // Get loaded IR info
    SampleRate getIRSampleRate() const { return irSampleRate_; }
    size_t getNumSamples() const { return numSamples_; }
    int getNumChannels() const { return numChannels_; }

private:
    std::vector<Sample> irBufferL_;
    std::vector<Sample> irBufferR_;
    SampleRate irSampleRate_ = 0.0;
    size_t numSamples_ = 0;
    int numChannels_ = 0;
};

} // namespace octob
```

**File: `libs/octob-ir-core/include/octob-ir-core/IRProcessor.hpp`**
```cpp
#pragma once

#include "Types.hpp"
#include "IRLoader.hpp"
#include "AudioBuffer.hpp"
#include <string>
#include <memory>

// Forward declare WDL types
class WDL_ImpulseBuffer;
class WDL_ConvolutionEngine_Div;

namespace octob {

class IRProcessor {
public:
    IRProcessor();
    ~IRProcessor();

    // Load impulse response from file
    bool loadImpulseResponse(const std::string& filepath, std::string& errorMessage);

    // Set sample rate (must be called before processing)
    void setSampleRate(SampleRate sampleRate);

    // Process stereo audio
    // Input and output buffers must have numFrames samples
    void processStereo(const Sample* inputL, const Sample* inputR,
                       Sample* outputL, Sample* outputR,
                       FrameCount numFrames);

    // Get current IR info
    bool isIRLoaded() const { return irLoaded_; }
    std::string getCurrentIRPath() const { return currentIRPath_; }
    SampleRate getIRSampleRate() const;
    size_t getIRNumSamples() const;

    // Reset processor state
    void reset();

private:
    std::unique_ptr<WDL_ImpulseBuffer> impulseBuffer_;
    std::unique_ptr<WDL_ConvolutionEngine_Div> convolutionEngine_;
    std::unique_ptr<IRLoader> irLoader_;

    SampleRate sampleRate_ = 44100.0;
    std::string currentIRPath_;
    bool irLoaded_ = false;
};

} // namespace octob
```

#### 2.2 Create Core Library Implementation

**File: `libs/octob-ir-core/src/AudioBuffer.cpp`**
```cpp
#include "octob-ir-core/AudioBuffer.hpp"

namespace octob {

AudioBuffer::AudioBuffer(FrameCount numFrames) {
    resize(numFrames);
}

void AudioBuffer::resize(FrameCount numFrames) {
    left_.resize(numFrames);
    right_.resize(numFrames);
}

void AudioBuffer::clear() {
    std::fill(left_.begin(), left_.end(), 0.0f);
    std::fill(right_.begin(), right_.end(), 0.0f);
}

} // namespace octob
```

**File: `libs/octob-ir-core/src/IRLoader.cpp`**

Extract from `opc-vcv-ir.cpp`:
- `LoadIR()` function logic → becomes `IRLoader::loadFromFile()`
- dr_wav usage for WAV file reading
- Channel handling (mono/stereo)
- Error handling for file loading

```cpp
#include "octob-ir-core/IRLoader.hpp"
#include <WDL/ImpulseBuffer.h>
#include <WDL/resample.h>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace octob {

IRLoader::IRLoader() = default;
IRLoader::~IRLoader() = default;

IRLoadResult IRLoader::loadFromFile(const std::string& filepath) {
    IRLoadResult result;

    // TODO: Extract LoadIR() logic from opc-vcv-ir.cpp here
    // - Use dr_wav to open WAV file
    // - Read samples into irBufferL_ and irBufferR_
    // - Handle mono (duplicate to stereo) and stereo files
    // - Store irSampleRate_, numSamples_, numChannels_
    // - Set result.success, result.errorMessage appropriately

    return result;
}

bool IRLoader::resampleAndInitialize(WDL_ImpulseBuffer& impulseBuffer,
                                      SampleRate targetSampleRate) {
    // TODO: Extract ResampleAndInitialize() logic from opc-vcv-ir.cpp here
    // - Create WDL_Resampler instances for L/R channels
    // - Resample irBufferL_/irBufferR_ to targetSampleRate
    // - Initialize impulseBuffer with resampled data
    // - Handle sample rate mismatch vs passthrough

    return true;
}

} // namespace octob
```

**File: `libs/octob-ir-core/src/IRProcessor.cpp`**

Extract from `opc-vcv-ir.cpp`:
- WDL engine initialization
- `process()` function logic → becomes `processStereo()`
- State management

```cpp
#include "octob-ir-core/IRProcessor.hpp"
#include <WDL/ImpulseBuffer.h>
#include <WDL/convoengine.h>

namespace octob {

IRProcessor::IRProcessor()
    : impulseBuffer_(std::make_unique<WDL_ImpulseBuffer>()),
      convolutionEngine_(std::make_unique<WDL_ConvolutionEngine_Div>()),
      irLoader_(std::make_unique<IRLoader>()) {
}

IRProcessor::~IRProcessor() = default;

bool IRProcessor::loadImpulseResponse(const std::string& filepath,
                                       std::string& errorMessage) {
    // Load IR file
    IRLoadResult result = irLoader_->loadFromFile(filepath);

    if (!result.success) {
        errorMessage = result.errorMessage;
        irLoaded_ = false;
        return false;
    }

    // Resample and initialize WDL impulse buffer
    if (!irLoader_->resampleAndInitialize(*impulseBuffer_, sampleRate_)) {
        errorMessage = "Failed to resample IR to target sample rate";
        irLoaded_ = false;
        return false;
    }

    // Reset convolution engine with new IR
    convolutionEngine_->Reset();
    convolutionEngine_->SetImpulse(impulseBuffer_.get(), 0, -1, 0, 0);

    currentIRPath_ = filepath;
    irLoaded_ = true;
    errorMessage.clear();

    return true;
}

void IRProcessor::setSampleRate(SampleRate sampleRate) {
    if (sampleRate_ != sampleRate) {
        sampleRate_ = sampleRate;

        // If IR is loaded, need to re-initialize with new sample rate
        if (irLoaded_) {
            irLoader_->resampleAndInitialize(*impulseBuffer_, sampleRate_);
            convolutionEngine_->Reset();
            convolutionEngine_->SetImpulse(impulseBuffer_.get(), 0, -1, 0, 0);
        }
    }
}

void IRProcessor::processStereo(const Sample* inputL, const Sample* inputR,
                                 Sample* outputL, Sample* outputR,
                                 FrameCount numFrames) {
    if (!irLoaded_) {
        // Passthrough if no IR loaded
        std::copy(inputL, inputL + numFrames, outputL);
        std::copy(inputR, inputR + numFrames, outputR);
        return;
    }

    // TODO: Extract process() logic from opc-vcv-ir.cpp
    // - Call convolutionEngine_->Add()
    // - Call convolutionEngine_->Avail()
    // - Call convolutionEngine_->GetSamples() into output buffers
}

void IRProcessor::reset() {
    if (convolutionEngine_) {
        convolutionEngine_->Reset();
    }
}

SampleRate IRProcessor::getIRSampleRate() const {
    return irLoader_ ? irLoader_->getIRSampleRate() : 0.0;
}

size_t IRProcessor::getIRNumSamples() const {
    return irLoader_ ? irLoader_->getNumSamples() : 0;
}

} // namespace octob
```

#### 2.3 Create Core Library CMake Build

**File: `libs/octob-ir-core/CMakeLists.txt`**
```cmake
cmake_minimum_required(VERSION 3.15)
project(octob-ir-core VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/AudioBuffer.cpp
    src/IRLoader.cpp
    src/IRProcessor.cpp
)

# Create static library
add_library(octob-ir-core STATIC ${SOURCES})

# Include directories
target_include_directories(octob-ir-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/iPlug2/WDL
)

# dr_wav is header-only, include path
target_include_directories(octob-ir-core
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party
)

# WDL sources (needed for ConvolutionEngine, FFT, Resampler)
set(WDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/iPlug2/WDL)

target_sources(octob-ir-core PRIVATE
    ${WDL_DIR}/convoengine.cpp
    ${WDL_DIR}/fft.c
    ${WDL_DIR}/resample.cpp
)

# Platform-specific settings
if(APPLE)
    target_compile_options(octob-ir-core PRIVATE -Wall -Wextra)
elseif(WIN32)
    target_compile_definitions(octob-ir-core PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Export library
set_target_properties(octob-ir-core PROPERTIES
    PUBLIC_HEADER "include/octob-ir-core/IRProcessor.hpp;include/octob-ir-core/IRLoader.hpp;include/octob-ir-core/AudioBuffer.hpp;include/octob-ir-core/Types.hpp"
)
```

**File: `libs/octob-ir-core/README.md`**
```markdown
# octob-ir-core

Platform-agnostic impulse response convolution DSP library.

## Features

- Load WAV impulse responses (mono/stereo)
- Automatic resampling to target sample rate
- FFT-based convolution via WDL ConvolutionEngine
- Zero VCV/JUCE dependencies

## Usage

```cpp
#include <octob-ir-core/IRProcessor.hpp>

octob::IRProcessor processor;
processor.setSampleRate(48000.0);

std::string error;
if (processor.loadImpulseResponse("/path/to/ir.wav", error)) {
    float inL[512], inR[512], outL[512], outR[512];
    processor.processStereo(inL, inR, outL, outR, 512);
}
```

## License

MIT
```

### Phase 3: Refactor VCV Rack Plugin

#### 3.1 Update VCV Plugin to Use Core Library

**File: `plugins/vcv-rack/src/OpcVcvIr.cpp`**

Refactor existing code:
- Remove DSP logic (LoadIR, ResampleAndInitialize, convolution processing)
- Add `#include <octob-ir-core/IRProcessor.hpp>`
- Create `octob::IRProcessor irProcessor;` member in `OpcVcvIr` struct
- Delegate processing to `irProcessor.processStereo()`
- Keep VCV-specific UI code (IrFileDisplay, OpcVcvIrWidget)

```cpp
#include "plugin.hpp"
#include <octob-ir-core/IRProcessor.hpp>
#include <osdialog.h>

// IrFileDisplay widget - KEEP AS-IS from original

struct OpcVcvIr : Module {
    enum ParamId {
        PARAMS_LEN
    };
    enum InputId {
        L_INPUT,
        R_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        L_OUTPUT,
        R_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    octob::IRProcessor irProcessor;  // Use shared library!
    std::string currentIrPath;

    OpcVcvIr() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configInput(L_INPUT, "L");
        configInput(R_INPUT, "R");
        configOutput(L_OUTPUT, "L");
        configOutput(R_OUTPUT, "R");

        // Initialize processor
        irProcessor.setSampleRate(APP->engine->getSampleRate());
    }

    void onSampleRateChange(const SampleRateChangeEvent& e) override {
        irProcessor.setSampleRate(e.sampleRate);
    }

    void process(const ProcessArgs& args) override {
        float inputL = inputs[L_INPUT].getVoltage();
        float inputR = inputs[R_INPUT].getVoltage();
        float outputL = 0.0f;
        float outputR = 0.0f;

        // Delegate to core library
        irProcessor.processStereo(&inputL, &inputR, &outputL, &outputR, 1);

        outputs[L_OUTPUT].setVoltage(outputL);
        outputs[R_OUTPUT].setVoltage(outputR);
    }

    void loadIrFile() {
        char* path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);
        if (path) {
            std::string error;
            if (irProcessor.loadImpulseResponse(path, error)) {
                currentIrPath = path;
                INFO("Loaded IR: %s", path);
            } else {
                WARN("Failed to load IR: %s", error.c_str());
            }
            std::free(path);
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        if (!currentIrPath.empty()) {
            json_object_set_new(rootJ, "irPath", json_string(currentIrPath.c_str()));
        }
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* irPathJ = json_object_get(rootJ, "irPath");
        if (irPathJ) {
            std::string error;
            std::string path = json_string_value(irPathJ);
            if (irProcessor.loadImpulseResponse(path, error)) {
                currentIrPath = path;
            }
        }
    }
};

// OpcVcvIrWidget - KEEP AS-IS from original with IrFileDisplay

Model* modelOpcVcvIr = createModel<OpcVcvIr, OpcVcvIrWidget>("OpcVcvIr");
```

#### 3.2 Update VCV Makefile

**File: `plugins/vcv-rack/Makefile`**
```make
RACK_DIR ?= ../../../Rack-SDK

# Add core library
SOURCES += $(wildcard src/*.cpp)
SOURCES += ../../libs/octob-ir-core/src/AudioBuffer.cpp
SOURCES += ../../libs/octob-ir-core/src/IRLoader.cpp
SOURCES += ../../libs/octob-ir-core/src/IRProcessor.cpp

# WDL sources
WDL_DIR = ../../third_party/iPlug2/WDL
SOURCES += $(WDL_DIR)/convoengine.cpp
SOURCES += $(WDL_DIR)/fft.c
SOURCES += $(WDL_DIR)/resample.cpp

# Include paths
FLAGS += -I../../libs/octob-ir-core/include
FLAGS += -I../../third_party/iPlug2/WDL
FLAGS += -I../../third_party

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

include $(RACK_DIR)/plugin.mk
```

### Phase 4: Create JUCE Plugin

#### 4.1 Create JUCE Plugin Processor

**File: `plugins/juce-multiformat/Source/PluginProcessor.h`**
```cpp
#pragma once

#include <JuceHeader.h>
#include <octob-ir-core/IRProcessor.hpp>

class OctobIRProcessor : public juce::AudioProcessor {
public:
    OctobIRProcessor();
    ~OctobIRProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // IR loading
    void loadImpulseResponse(const juce::String& filepath);
    juce::String getCurrentIRPath() const { return currentIRPath_; }

private:
    octob::IRProcessor irProcessor_;
    juce::String currentIRPath_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OctobIRProcessor)
};
```

**File: `plugins/juce-multiformat/Source/PluginProcessor.cpp`**
```cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

OctobIRProcessor::OctobIRProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
}

OctobIRProcessor::~OctobIRProcessor() {
}

const juce::String OctobIRProcessor::getName() const {
    return JucePlugin_Name;
}

bool OctobIRProcessor::acceptsMidi() const { return false; }
bool OctobIRProcessor::producesMidi() const { return false; }
bool OctobIRProcessor::isMidiEffect() const { return false; }
double OctobIRProcessor::getTailLengthSeconds() const { return 0.0; }

int OctobIRProcessor::getNumPrograms() { return 1; }
int OctobIRProcessor::getCurrentProgram() { return 0; }
void OctobIRProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String OctobIRProcessor::getProgramName(int index) {
    juce::ignoreUnused(index);
    return {};
}
void OctobIRProcessor::changeProgramName(int index, const juce::String& newName) {
    juce::ignoreUnused(index, newName);
}

void OctobIRProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(samplesPerBlock);
    irProcessor_.setSampleRate(sampleRate);
}

void OctobIRProcessor::releaseResources() {
    irProcessor_.reset();
}

bool OctobIRProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void OctobIRProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (totalNumInputChannels >= 2 && totalNumOutputChannels >= 2) {
        float* channelDataL = buffer.getWritePointer(0);
        float* channelDataR = buffer.getWritePointer(1);

        // Process using octob-ir-core
        irProcessor_.processStereo(channelDataL, channelDataR,
                                    channelDataL, channelDataR,
                                    buffer.getNumSamples());
    }
}

bool OctobIRProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* OctobIRProcessor::createEditor() {
    return new OctobIREditor(*this);
}

void OctobIRProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::ValueTree state("OctobIRState");
    state.setProperty("irPath", currentIRPath_, nullptr);

    juce::MemoryOutputStream stream(destData, false);
    state.writeToStream(stream);
}

void OctobIRProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::ValueTree state = juce::ValueTree::readFromData(data, sizeInBytes);

    if (state.isValid()) {
        juce::String path = state.getProperty("irPath").toString();
        if (path.isNotEmpty()) {
            loadImpulseResponse(path);
        }
    }
}

void OctobIRProcessor::loadImpulseResponse(const juce::String& filepath) {
    std::string error;
    if (irProcessor_.loadImpulseResponse(filepath.toStdString(), error)) {
        currentIRPath_ = filepath;
    } else {
        DBG("Failed to load IR: " + juce::String(error));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OctobIRProcessor();
}
```

#### 4.2 Create JUCE Plugin Editor (Stub)

**File: `plugins/juce-multiformat/Source/PluginEditor.h`**
```cpp
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class OctobIREditor : public juce::AudioProcessorEditor {
public:
    OctobIREditor(OctobIRProcessor&);
    ~OctobIREditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OctobIRProcessor& audioProcessor;

    juce::TextButton loadButton_;
    juce::Label irPathLabel_;

    void loadButtonClicked();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OctobIREditor)
};
```

**File: `plugins/juce-multiformat/Source/PluginEditor.cpp`**
```cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

OctobIREditor::OctobIREditor(OctobIRProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

    // Load button
    addAndMakeVisible(loadButton_);
    loadButton_.setButtonText("Load IR");
    loadButton_.onClick = [this] { loadButtonClicked(); };

    // IR path display
    addAndMakeVisible(irPathLabel_);
    irPathLabel_.setText(audioProcessor.getCurrentIRPath(), juce::dontSendNotification);
    irPathLabel_.setJustificationType(juce::Justification::centred);

    setSize(400, 300);
}

OctobIREditor::~OctobIREditor() {
}

void OctobIREditor::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("OctobIR - Impulse Response Loader",
                     getLocalBounds(),
                     juce::Justification::centredTop, 1);
}

void OctobIREditor::resized() {
    auto bounds = getLocalBounds().reduced(20);

    bounds.removeFromTop(40); // Title space

    loadButton_.setBounds(bounds.removeFromTop(40).reduced(100, 0));
    bounds.removeFromTop(20); // Spacing
    irPathLabel_.setBounds(bounds.removeFromTop(30));
}

void OctobIREditor::loadButtonClicked() {
    juce::FileChooser chooser("Select impulse response WAV file",
                              juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                              "*.wav");

    if (chooser.browseForFileToOpen()) {
        juce::File file = chooser.getResult();
        audioProcessor.loadImpulseResponse(file.getFullPathName());
        irPathLabel_.setText(file.getFileName(), juce::dontSendNotification);
    }
}
```

#### 4.3 Create JUCE Plugin CMake Configuration

**File: `plugins/juce-multiformat/CMakeLists.txt`**
```cmake
cmake_minimum_required(VERSION 3.15)
project(OctobIR VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
add_subdirectory(../../third_party/JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# Add core library
add_subdirectory(../../libs/octob-ir-core ${CMAKE_CURRENT_BINARY_DIR}/octob-ir-core)

# Plugin formats to build
set(FORMATS VST3 AU Standalone)

# Add AAX if SDK available (optional)
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/AAX_SDK")
#     list(APPEND FORMATS AAX)
# endif()

# Create plugin
juce_add_plugin(OctobIR
    COMPANY_NAME "YourCompany"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Octb
    PLUGIN_CODE Obir
    FORMATS ${FORMATS}
    PRODUCT_NAME "OctobIR")

# Source files
target_sources(OctobIR PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

# Compile definitions
target_compile_definitions(OctobIR
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

# Link libraries
target_link_libraries(OctobIR
    PRIVATE
        juce::juce_audio_utils
        octob-ir-core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
```

### Phase 5: Root CMake Configuration

**File: `CMakeLists.txt`**
```cmake
cmake_minimum_required(VERSION 3.15)
project(OctobIR-Monorepo VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build different components
option(BUILD_CORE_LIB "Build octob-ir-core library" ON)
option(BUILD_JUCE_PLUGIN "Build JUCE plugin (VST3/AU/AAX)" ON)
option(BUILD_VCV_PLUGIN "Build VCV Rack plugin" OFF)  # VCV uses Makefile

# Core library
if(BUILD_CORE_LIB)
    add_subdirectory(libs/octob-ir-core)
endif()

# JUCE plugin
if(BUILD_JUCE_PLUGIN)
    add_subdirectory(plugins/juce-multiformat)
endif()

# VCV Rack plugin built separately with make
if(BUILD_VCV_PLUGIN)
    message(STATUS "VCV Rack plugin: Use 'make' in plugins/vcv-rack/ directory")
endif()
```

### Phase 6: Documentation and Scripts

#### 6.1 Update Root README

**File: `README.md`**
```markdown
# OctobIR - Impulse Response Loader

Multi-platform impulse response convolution plugin suite.

## Supported Platforms

- **VCV Rack** - Eurorack simulator plugin
- **VST3/AU/AAX/Standalone** - DAW and standalone via JUCE

## Repository Structure

- `libs/octob-ir-core/` - Shared DSP library (platform-agnostic)
- `plugins/vcv-rack/` - VCV Rack plugin
- `plugins/juce-multiformat/` - JUCE plugin (VST3/AU/AAX/Standalone)
- `third_party/` - Dependencies (JUCE, iPlug2/WDL, dr_wav)

## Building

### Prerequisites

```bash
# Clone with submodules
git clone --recursive https://github.com/yourusername/OctobIR.git
cd OctobIR

# Or if already cloned
git submodule update --init --recursive
```

### JUCE Plugin (VST3/AU/AAX/Standalone)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Built plugins will be in:
- macOS: `build/plugins/juce-multiformat/OctobIR_artefacts/`
- Windows: `build\plugins\juce-multiformat\OctobIR_artefacts\`

### VCV Rack Plugin

```bash
cd plugins/vcv-rack
make clean
make install  # Installs to Rack plugins directory
```

## Development

See `MONOREPO_MIGRATION_PLAN.md` for architecture details.

## License

- Core library (`libs/octob-ir-core/`): MIT
- JUCE plugin: AGPLv3 (open source)
- VCV Rack plugin: GPL-3.0+
```

#### 6.2 Setup Script

**File: `scripts/setup-dev.sh`**
```bash
#!/bin/bash

set -e

echo "Setting up OctobIR development environment..."

# Initialize submodules
echo "Initializing git submodules..."
git submodule update --init --recursive

# Verify JUCE
if [ ! -d "third_party/JUCE" ]; then
    echo "Error: JUCE submodule not found"
    exit 1
fi

# Verify iPlug2
if [ ! -d "third_party/iPlug2" ]; then
    echo "Error: iPlug2 submodule not found"
    exit 1
fi

echo "✓ Submodules initialized"

# Build core library
echo "Building octob-ir-core library..."
cmake -B build/core -S libs/octob-ir-core -DCMAKE_BUILD_TYPE=Debug
cmake --build build/core

echo "✓ Core library built"

echo ""
echo "Setup complete! Next steps:"
echo ""
echo "  JUCE Plugin:"
echo "    cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "    cmake --build build --config Release"
echo ""
echo "  VCV Rack Plugin:"
echo "    cd plugins/vcv-rack && make"
echo ""
```

**Make executable:**
```bash
chmod +x scripts/setup-dev.sh
```

#### 6.3 Build All Script

**File: `scripts/build-all.sh`**
```bash
#!/bin/bash

set -e

echo "Building all OctobIR plugins..."

# Build JUCE plugin
echo ""
echo "=== Building JUCE Plugin ==="
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Build VCV Rack plugin
echo ""
echo "=== Building VCV Rack Plugin ==="
cd plugins/vcv-rack
make clean
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
cd ../..

echo ""
echo "✓ All plugins built successfully"
echo ""
echo "JUCE plugin: build/plugins/juce-multiformat/OctobIR_artefacts/"
echo "VCV plugin: plugins/vcv-rack/plugin.dylib (or .so/.dll)"
```

**Make executable:**
```bash
chmod +x scripts/build-all.sh
```

### Phase 7: License Files

**File: `LICENSE`** (Root - MIT)
```
MIT License

Copyright (c) 2025 [Your Name]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

[... standard MIT license text ...]
```

**File: `libs/octob-ir-core/LICENSE`** (Core library - MIT)
```
[Same as root MIT license]
```

**File: `plugins/vcv-rack/LICENSE`** (VCV - GPL-3.0+)
```
[GPL-3.0 license text - required by VCV Rack]
```

**File: `plugins/juce-multiformat/LICENSE`** (JUCE plugin - AGPLv3 or MIT)
```
[AGPLv3 license text if open source]
OR
[MIT license text if you prefer permissive]
```

## Code Extraction Reference

### From `opc-vcv-ir.cpp` to Core Library

**Current location** → **New location**

1. **LoadIR() function** (lines ~50-120)
   - Extract to: `libs/octob-ir-core/src/IRLoader.cpp::loadFromFile()`
   - Dependencies: dr_wav
   - Changes: Replace VCV logger (INFO/WARN) with return codes/error strings

2. **ResampleAndInitialize() function** (lines ~122-180)
   - Extract to: `libs/octob-ir-core/src/IRLoader.cpp::resampleAndInitialize()`
   - Dependencies: WDL_Resampler, WDL_ImpulseBuffer
   - Changes: Take WDL_ImpulseBuffer as parameter instead of member variable

3. **process() convolution loop** (lines ~250-280)
   - Extract to: `libs/octob-ir-core/src/IRProcessor.cpp::processStereo()`
   - Dependencies: WDL_ConvolutionEngine_Div
   - Changes: Take input/output pointers instead of VCV Input/Output objects

4. **Member variables** (lines ~30-48)
   ```cpp
   // Move to IRProcessor class
   WDL_ImpulseBuffer impulseBuffer;
   WDL_ConvolutionEngine_Div engine;
   double sampleRate;
   double irSampleRate;
   // Move to IRLoader class
   std::vector<float> irBufferL, irBufferR;
   size_t numSamples;
   ```

5. **Keep in VCV plugin**:
   - IrFileDisplay widget (lines ~180-220)
   - OpcVcvIrWidget (lines ~300-340)
   - osdialog file browser integration
   - JSON serialization (dataToJson/dataFromJson)

## Testing Checklist

After migration, verify:

### Core Library Tests
- [ ] Core library compiles standalone (no VCV/JUCE dependencies)
- [ ] Can load mono WAV impulse response
- [ ] Can load stereo WAV impulse response
- [ ] Sample rate conversion works (44.1k → 48k, etc.)
- [ ] Convolution produces non-zero output
- [ ] Error handling for invalid files works

### VCV Rack Plugin Tests
- [ ] VCV plugin compiles and loads in Rack
- [ ] Can load IR via file browser
- [ ] IR path persists in patch save/load
- [ ] Audio processing works (convolution output matches pre-refactor)
- [ ] Sample rate changes handled correctly
- [ ] No regressions from original plugin

### JUCE Plugin Tests
- [ ] JUCE plugin compiles for all formats (VST3, AU, Standalone)
- [ ] Plugin loads in DAW (test VST3 and AU)
- [ ] Can load IR via file browser
- [ ] IR path persists in DAW project save/load
- [ ] Audio processing works (convolution output correct)
- [ ] Sample rate changes handled correctly
- [ ] No crashes, no audio glitches

### Build System Tests
- [ ] CMake builds complete successfully
- [ ] VCV Makefile builds complete successfully
- [ ] `scripts/setup-dev.sh` works on clean checkout
- [ ] `scripts/build-all.sh` builds all plugins
- [ ] No compiler warnings (fix with -Wall -Wextra)

## Known Issues & TODOs

### Must Implement Before Testing
1. **IRLoader::loadFromFile()** - Extract LoadIR() logic with dr_wav
2. **IRLoader::resampleAndInitialize()** - Extract resampling logic
3. **IRProcessor::processStereo()** - Extract convolution loop
4. **VCV Rack widget extraction** - Split IrFileDisplay into separate file

### Future Enhancements (Post-Migration)
- Add unit tests for core library
- Implement JUCE UI (currently stubbed)
- Add CI/CD with GitHub Actions
- Create installers with CPack/Pamplejuce
- Add pluginval validation
- Code signing and notarization

## Migration Execution Order

1. **Phase 1**: Repository restructuring (move files, create directories)
2. **Phase 2**: Create core library headers and stubs
3. **Phase 3**: Extract DSP code to core library implementations
4. **Phase 4**: Refactor VCV plugin to use core library
5. **Phase 5**: Test VCV plugin (ensure no regression)
6. **Phase 6**: Create JUCE plugin using core library
7. **Phase 7**: Test JUCE plugin in DAW
8. **Phase 8**: Setup build scripts and CI/CD
9. **Phase 9**: Documentation and licensing

**Critical**: Test VCV Rack plugin after Phase 5 before proceeding. This ensures the core library extraction is correct.

## Questions & Decisions Needed

- [ ] AAX development: Do you have Avid developer account/SDK?
- [ ] Code signing: Do you have Apple Developer ID for macOS distribution?
- [ ] License choice: AGPLv3 vs MIT for JUCE plugin code?
- [ ] CI/CD platform: GitHub Actions (recommended) or other?
- [ ] Installer format preferences: DMG/PKG for macOS, MSI/EXE for Windows?

## Support Resources

- **JUCE Forum**: https://forum.juce.com/
- **JUCE CMake API**: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md
- **Pamplejuce Template**: https://github.com/sudara/pamplejuce
- **VCV Rack SDK**: https://vcvrack.com/manual/PluginDevelopmentTutorial
- **WDL Documentation**: Limited, refer to iPlug2/WDL source code comments