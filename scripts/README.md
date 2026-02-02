# Build Scripts

Scripts for building OctobIR plugins in release mode.

## Quick Start

```bash
# Build and install release versions locally (for testing)
make release

# Build JUCE distributable installers (DMG/ZIP/tarball)
make installers

# Test VCV plugin locally (before VCV Library submission)
./scripts/build-release-vcv.sh
```

## Distribution Model

**JUCE Plugins (Self-Distributed):**
- Build installers with `make installers`
- Distribute via GitHub releases
- Users download DMG/ZIP/tarball and install

**VCV Rack Plugin (VCV Library):**
- Submit **source code** to VCV Library
- VCV builds for all platforms automatically
- Users download via VCV Rack Plugin Manager
- Local script is for testing only

## JUCE Plugin Release (`build-release-juce.sh`)

Builds JUCE plugin installers exactly as the CI does.

**What it builds:**
- **macOS**: DMG installer with VST3 and AU
- **Windows**: Installer with VST3
- **Linux**: Tarball with VST3

**Output:**
- `OctobIR-macOS.dmg` (macOS)
- `OctobIR-Windows.zip` (Windows)
- `OctobIR-Linux.tar.gz` (Linux)
- `package/OctobIR/` - Package contents

**Testing the installer:**
1. Run the script: `./scripts/build-release-juce.sh`
2. Install the package:
   - macOS: Open `OctobIR-macOS.dmg` and drag to Applications
   - Windows: Extract `OctobIR-Windows.zip` and copy plugins to your DAW's plugin folder
   - Linux: Extract tarball and copy to appropriate plugin directories
3. Test in your DAW or standalone

## VCV Rack Plugin Release (`build-release-vcv.sh`)

Builds VCV Rack plugin distribution package exactly as the CI does.

**What it does:**
- Auto-detects your platform (mac-x64, mac-arm64, lin-x64, win-x64)
- Downloads VCV Rack SDK if not found
- Builds `.vcvplugin` distribution package using `make dist`

**Output:**
- `plugins/vcv-rack/dist/OctobIR-<version>-<platform>.vcvplugin`

**Testing the plugin:**
1. Run the script: `./scripts/build-release-vcv.sh`
2. Copy the `.vcvplugin` file to your VCV Rack plugins directory:
   - macOS: `~/Documents/Rack2/plugins-mac-arm64/` (or `plugins-mac-x64/`)
   - Linux: `~/.Rack2/plugins-lin-x64/`
   - Windows: `%USERPROFILE%\Documents\Rack2\plugins-win-x64\`
3. Launch VCV Rack and test the plugin

**Environment variables:**
- `RACK_DIR`: Path to VCV Rack SDK (auto-downloaded if not set)

## Release Workflow

### Local Testing

```bash
# Build and install release versions
make release
```

Builds release versions of both JUCE and VCV plugins and installs them locally for testing. JUCE plugins install to system directories, VCV installs to your Rack plugins folder.

### Distribution

```bash
# Build JUCE installers for distribution
make installers
```

Creates distributable installers (DMG/ZIP/tarball) for JUCE plugins. These are uploaded to GitHub releases.

## Release Workflow

### JUCE Plugin Release (Self-Distributed)

1. **Test locally:**
   ```bash
   make release        # Build and install release version
   # Test in DAW
   ```

2. **Build installers:**
   ```bash
   make installers     # Creates DMG/ZIP/tarball
   ```

3. **Create GitHub release:**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```
   This triggers the CI workflow that builds installers for all platforms and creates a draft GitHub release.

### VCV Rack Plugin Release (VCV Library)

1. **Test locally:**
   ```bash
   ./scripts/build-release-vcv.sh
   # Install .vcvplugin to Rack and test
   ```

2. **Update version:**
   ```bash
   # Edit plugins/vcv-rack/plugin.json
   # Increment "version": "1.0.0" → "1.0.1"
   git add plugins/vcv-rack/plugin.json
   git commit -m "Bump VCV plugin to v1.0.1"
   git push
   ```

3. **Submit to VCV Library:**
   - **First time:** Create issue at [github.com/VCVRack/library](https://github.com/VCVRack/library) with your plugin slug and source URL
   - **Updates:** Comment in your issue thread with version number + commit hash
   - VCV builds and distributes for all platforms

## Development Scripts

Other useful scripts in this directory:

- `setup-dev.sh` - Install development dependencies (clang-format, clang-tidy, etc.)

## Notes

- All release scripts use `Release` build configuration for optimized builds
- Scripts clean previous builds to ensure fresh builds
- JUCE script auto-detects platform and creates appropriate installer format
- VCV script handles SDK download and platform detection automatically
- All scripts exit with error code on failure for CI compatibility
