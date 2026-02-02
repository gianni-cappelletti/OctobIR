# OctobIR - Impulse Response Loader

Multi-platform impulse response convolution plugin suite.

## Supported Platforms

- **VCV Rack** - Eurorack simulator plugin
- **VST3/AU** - DAW plugins via JUCE (macOS/Windows/Linux)

## Installation

### Option 1: Pre-built Installers (Recommended)

Download the latest release for your platform from [GitHub Releases](https://github.com/gianni-cappelletti/OctobIR/releases):

**macOS**
1. Download `OctobIR-macOS.dmg`
2. Open the DMG and run the installer
3. Select components to install (VST3, AU)
4. Complete installation

**Windows**
1. Download `OctobIR-Windows.exe`
2. Run the installer
3. Follow the installation wizard
4. VST3 will be installed to `C:\Program Files\Common Files\VST3\`

**Linux**
1. Download `OctobIR-Linux.tar.gz`
2. Extract: `tar -xzf OctobIR-Linux.tar.gz`
3. Run the installer: `cd OctobIR && ./install.sh`
4. VST3 will be installed to `~/.vst3/`

**VCV Rack**
- VCV Rack plugins are distributed through the [VCV Library](https://library.vcvrack.com/)
- Search for "OctobIR" in the VCV Rack plugin manager

### Option 2: Build from Source

For the latest development version or to contribute:

**Quick Install (after cloning):**
```bash
# Clone repository
git clone --recursive https://github.com/gianni-cappelletti/OctobIR.git
cd OctobIR

# Install JUCE plugins (VST3/AU)
make install-juce

# Install VCV Rack plugin (requires VCV Rack SDK)
export RACK_DIR=/path/to/Rack-SDK
make install-vcv
```

See the [Building from Source](#building-from-source) section below for detailed instructions.

## Repository Structure

```
OctobIR/
├── libs/octobir-core/         # Shared DSP library (platform-agnostic)
├── plugins/
│   ├── vcv-rack/              # VCV Rack plugin
│   └── juce-multiformat/      # JUCE plugin (VST3/AU)
├── third_party/               # Dependencies (JUCE, WDL, dr_wav)
├── scripts/                   # Build and setup scripts
└── installers/                # Installer scripts for distribution
```

## Features

- Load mono/stereo WAV impulse responses
- Automatic sample rate conversion
- FFT-based convolution via WDL ConvolutionEngine
- Shared DSP core across all plugin formats

## Building from Source

### Prerequisites

```bash
# Clone with submodules
git clone --recursive https://github.com/gianni-cappelletti/OctobIR.git
cd OctobIR

# Or if already cloned
git submodule update --init --recursive

# Initial setup (first time only - installs dependencies)
./scripts/setup-dev.sh
```

### Install to System (Release Builds)

Build and install release versions to your system:

```bash
make install-juce    # Build and install JUCE plugins (VST3 + AU)
make install-vcv     # Build and install VCV Rack plugin
```

**Note**: If you previously installed via the packaged installer, remove the old plugins first:
```bash
sudo rm -rf ~/Library/Audio/Plug-Ins/VST3/OctobIR.vst3
sudo rm -rf ~/Library/Audio/Plug-Ins/Components/OctobIR.component
```

Installed locations:
- **macOS VST3**: `~/Library/Audio/Plug-Ins/VST3/OctobIR.vst3`
- **macOS AU**: `~/Library/Audio/Plug-Ins/Components/OctobIR.component`
- **Linux VST3**: `~/.vst3/OctobIR.vst3`
- **VCV Rack**: `~/Documents/Rack2/plugins/OctobIR` (or your Rack plugins directory)

### Development Builds

For fast iteration during development (debug builds with automatic clean):

```bash
make juce         # Build JUCE plugin (debug, not installed)
make vcv          # Build and install VCV Rack plugin (debug)
make core         # Build core library only
make test         # Build and run unit tests
```

Debug builds are faster to compile and include debugging symbols. Built plugins are in:
- **JUCE**: `build/dev-juce/plugins/juce-multiformat/OctobIR_artefacts/`
- **VCV**: Installed to Rack plugins directory

### Advanced CMake Usage

Direct CMake commands (if not using Makefile):

```bash
# Configure with presets
cmake --preset dev-vcv && cmake --build --preset dev-vcv     # VCV only
cmake --preset dev-juce && cmake --build --preset dev-juce   # JUCE only
cmake --preset release && cmake --build --preset release     # Release

# Or manual configuration
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Build specific targets
cmake --build build --target octobir-core      # Core library only
cmake --build build --target OctobIR_All       # JUCE plugins (VST3 + AU)
cmake --build build --target OctobIR_VST3      # JUCE VST3 only
cmake --build build --target OctobIR_AU        # JUCE AU only (macOS)
cmake --build build --target vcv-plugin        # VCV plugin only
```

## Development

### Development Workflow

After running `./scripts/setup-dev.sh`, use these commands for fast iteration:

```bash
make juce         # Build JUCE plugin (debug, artifacts in build directory)
make vcv          # Build and install VCV plugin (debug, installed to Rack)
make core         # Build core library only
make test         # Run unit tests
make format       # Auto-format code with clang-format
make tidy         # Check formatting and run static analysis
```

Debug builds compile faster and enable debugging symbols. All development builds automatically perform a clean build to ensure consistency.

### Architecture

The project uses a shared core library (`octobir-core`) that contains all DSP logic:

- **IRProcessor** - Main convolution processor
- **IRLoader** - WAV loading and resampling
- **AudioBuffer** - Platform-agnostic buffer wrapper

Plugin implementations (VCV Rack, JUCE) are thin wrappers around this core.

### Testing

The core library includes unit tests using Google Test:

```bash
make test         # Build and run all tests
```

Tests are located in `libs/octobir-core/tests/` and cover:
- Parameter validation and clamping
- DSP logic correctness
- IR loading and state management

## Continuous Integration

The repository uses GitHub Actions for automated testing and releases:

### CI Workflow
- Runs on every push and pull request
- Tests on Linux and macOS
- Builds VCV and JUCE plugins to verify compilation
- All tests must pass before merging

### Release Workflows

Create a release by pushing a git tag:

```bash
git tag v2.0.0
git push origin v2.0.0
```

This automatically triggers the JUCE plugin release workflow:

**JUCE Plugin Release**
- Builds professional installers with component selection
- macOS: `.pkg` installer in DMG with VST3 and AU options
- Windows: `.exe` installer with VST3
- Linux: `.tar.gz` with install script for VST3
- Uploads to GitHub Releases as a draft for review

**VCV Rack Plugin Distribution**

VCV Rack plugins are distributed through the [VCV Library](https://library.vcvrack.com/), not GitHub releases:
1. Update version in `plugins/vcv-rack/plugin.json`
2. Commit and push changes
3. Submit to VCV Library (first time) or comment version + commit hash in your plugin's issue
4. VCV builds for all platforms automatically
5. Users download via VCV Rack's built-in Plugin Manager

## License

**OctobIR is licensed under GPL-3.0 (GNU General Public License v3.0)**

This applies to all components:
- Core library (`libs/octobir-core/`)
- JUCE plugin (VST3/AU)
- VCV Rack plugin

See `LICENSE` for the full license text and `LICENSING.md` for detailed licensing information.

## Third-Party Components

- **JUCE** - Audio plugin framework (VST3/AU) - GPL-3.0
- **WDL** - DSP algorithms: convolution engine, FFT, resampling - zlib-style license
  - Source: https://github.com/justinfrankel/WDL (official Cockos repository)
- **dr_wav** - WAV file loading (header-only) - Public Domain
- **VCV Rack SDK** - VCV Rack plugin API - GPL-3.0+

See `LICENSING.md` and `THIRD_PARTY_NOTICES.txt` for detailed information.
