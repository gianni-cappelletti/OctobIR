# Installer Implementation Plan

## Overview

This document outlines the plan to implement professional, wizard-based installers for OctobIR audio plugins with component selection (AU, VST3, Standalone).

### Current State
- Simple DMG (macOS) with drag-and-drop
- ZIP archive (Windows)
- No component selection or installation wizard

### Target State
- **macOS**: Professional `.pkg` installer with component checkboxes for AU/VST3/Standalone
- **Windows**: Inno Setup wizard with component selection for VST3/Standalone
- **Linux**: Tarball with install script (adequate for Linux users)

### Distribution Model
- **JUCE plugins**: Self-distributed via GitHub releases (installers)
- **VCV Rack plugin**: Source submission to VCV Library (they build and distribute)

## Tool Selection

Based on industry research and commercial plugin developer feedback:

### Windows: Inno Setup
- **Tool**: [Inno Setup](https://jrsoftware.org/isinfo.php) (free, open-source)
- **Why**: Industry standard for audio plugins, full customization, script-based
- **Rejected**: CPack (limited customization, "cumbersome" per JUCE forum feedback)

### macOS: pkgbuild + productbuild
- **Tool**: Apple's built-in command-line tools
- **Why**: Native, stable, supports component selection, required for notarization
- **Helper**: Can use [Packages.app](http://s.sudre.free.fr/Software/Packages/about.html) for GUI preview/testing

### Linux: Tarball + Install Script
- **Tool**: tar + bash script
- **Why**: Standard for Linux audio plugins, users expect manual installation

## Installation Paths

### macOS
```
AU:         ~/Library/Audio/Plug-Ins/Components/OctobIR.component
VST3:       ~/Library/Audio/Plug-Ins/VST3/OctobIR.vst3
Standalone: /Applications/OctobIR.app
```

### Windows
```
VST3:       C:\Program Files\Common Files\VST3\OctobIR.vst3
Standalone: C:\Program Files\OctobIR\OctobIR.exe
```

### Linux
```
VST3:       ~/.vst3/OctobIR.vst3
Standalone: /usr/local/bin/OctobIR (or ~/bin/OctobIR)
```

## Implementation Plan

### Phase 1: Windows Installer (Inno Setup)

**Estimated Time**: 3-4 hours

#### Step 1.1: Create Inno Setup Script Template
Create `installers/windows/OctobIR.iss`:

```iss
[Setup]
AppName=OctobIR
AppVersion=1.0.0
AppPublisher=Your Company Name
AppPublisherURL=https://github.com/yourusername/OctobIR
DefaultDirName={autopf}\OctobIR
DefaultGroupName=OctobIR
OutputDir=../../dist
OutputBaseFilename=OctobIR-{#AppVersion}-Windows
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
LicenseFile=../../LICENSE
WizardStyle=modern

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Plugin"; Types: full custom
Name: "standalone"; Description: "Standalone Application"; Types: full custom

[Files]
; VST3 Plugin
Source: "..\..\build\release-juce\plugins\juce-multiformat\OctobIR_artefacts\Release\VST3\OctobIR.vst3\*"; \
  DestDir: "{commoncf}\VST3\OctobIR.vst3"; \
  Components: vst3; \
  Flags: recursesubdirs

; Standalone Application
Source: "..\..\build\release-juce\plugins\juce-multiformat\OctobIR_artefacts\Release\Standalone\OctobIR.exe"; \
  DestDir: "{app}"; \
  Components: standalone

; Documentation
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\OctobIR"; Filename: "{app}\OctobIR.exe"; Components: standalone
Name: "{group}\Uninstall OctobIR"; Filename: "{uninstallexe}"

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;
```

#### Step 1.2: Create Build Script
Create `scripts/build-installer-windows.sh`:

```bash
#!/usr/bin/env bash
set -e

echo "Building Windows installer with Inno Setup..."

# Check if running on Windows or need to use Wine
if command -v iscc &> /dev/null; then
  ISCC="iscc"
elif command -v wine &> /dev/null && [ -f "$HOME/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe" ]; then
  ISCC="wine \"$HOME/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe\""
else
  echo "Error: Inno Setup not found"
  echo "Install from: https://jrsoftware.org/isdl.php"
  exit 1
fi

# Build JUCE plugin first
echo "Building JUCE plugin (Release)..."
cmake -B build/release-juce \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_JUCE_PLUGIN=ON \
  -DBUILD_VCV_PLUGIN=OFF \
  -DBUILD_TESTS=OFF
cmake --build build/release-juce --config Release -j

# Compile installer
echo "Compiling installer..."
eval $ISCC installers/windows/OctobIR.iss

echo "✓ Windows installer created: dist/OctobIR-*-Windows.exe"
```

#### Step 1.3: Update GitHub Actions
Add to `.github/workflows/release-juce.yml` (Windows job):

```yaml
- name: Install Inno Setup
  run: choco install innosetup -y

- name: Build installer
  shell: bash
  run: |
    iscc installers/windows/OctobIR.iss

- name: Upload installer
  uses: actions/upload-artifact@v4
  with:
    name: juce-plugin-windows-installer
    path: dist/OctobIR-*-Windows.exe
```

### Phase 2: macOS Installer (pkgbuild/productbuild)

**Estimated Time**: 4-5 hours (includes code signing setup)

#### Step 2.1: Create Package Scripts Directory Structure
```
installers/macos/
├── scripts/
│   ├── postinstall-au
│   ├── postinstall-vst3
│   └── postinstall-standalone
├── distribution.xml
└── build-pkg.sh
```

#### Step 2.2: Create Component Post-Install Scripts
Create `installers/macos/scripts/postinstall-au`:

```bash
#!/bin/bash
# AU component post-install validation
exit 0
```

(Similar scripts for VST3 and Standalone)

#### Step 2.3: Create Distribution XML
Create `installers/macos/distribution.xml`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>OctobIR</title>
    <organization>com.yourcompany</organization>
    <domains enable_localSystem="true"/>
    <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>

    <!-- Welcome/ReadMe/License -->
    <welcome file="welcome.html" mime-type="text/html"/>
    <license file="LICENSE"/>

    <!-- Component Choices -->
    <choices-outline>
        <line choice="au"/>
        <line choice="vst3"/>
        <line choice="standalone"/>
    </choices-outline>

    <choice id="au" title="Audio Unit Plugin"
            description="Install the AU plugin for Logic Pro, GarageBand, etc.">
        <pkg-ref id="com.yourcompany.octobir.au"/>
    </choice>

    <choice id="vst3" title="VST3 Plugin"
            description="Install the VST3 plugin for most DAWs" selected="true">
        <pkg-ref id="com.yourcompany.octobir.vst3"/>
    </choice>

    <choice id="standalone" title="Standalone Application"
            description="Install the standalone application">
        <pkg-ref id="com.yourcompany.octobir.standalone"/>
    </choice>

    <!-- Package References -->
    <pkg-ref id="com.yourcompany.octobir.au"/>
    <pkg-ref id="com.yourcompany.octobir.vst3"/>
    <pkg-ref id="com.yourcompany.octobir.standalone"/>
</installer-gui-script>
```

#### Step 2.4: Create Build Script
Create `installers/macos/build-pkg.sh`:

```bash
#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build/release-juce/plugins/juce-multiformat/OctobIR_artefacts/Release"
DIST_DIR="$PROJECT_DIR/dist"
VERSION="1.0.0"

# Code signing identity (set to "-" for no signing during development)
SIGNING_IDENTITY="${SIGNING_IDENTITY:--}"

echo "Building macOS installer packages..."

mkdir -p "$DIST_DIR/pkgs"

# Build individual component packages
echo "Building AU package..."
pkgbuild --root "$BUILD_DIR/AU" \
  --identifier com.yourcompany.octobir.au \
  --version "$VERSION" \
  --scripts "$SCRIPT_DIR/scripts" \
  --install-location "/Library/Audio/Plug-Ins/Components" \
  --sign "$SIGNING_IDENTITY" \
  "$DIST_DIR/pkgs/OctobIR-AU.pkg"

echo "Building VST3 package..."
pkgbuild --root "$BUILD_DIR/VST3" \
  --identifier com.yourcompany.octobir.vst3 \
  --version "$VERSION" \
  --install-location "/Library/Audio/Plug-Ins/VST3" \
  --sign "$SIGNING_IDENTITY" \
  "$DIST_DIR/pkgs/OctobIR-VST3.pkg"

echo "Building Standalone package..."
pkgbuild --root "$BUILD_DIR/Standalone" \
  --identifier com.yourcompany.octobir.standalone \
  --version "$VERSION" \
  --install-location "/Applications" \
  --sign "$SIGNING_IDENTITY" \
  "$DIST_DIR/pkgs/OctobIR-Standalone.pkg"

# Combine into distribution package
echo "Building distribution package..."
productbuild --distribution "$SCRIPT_DIR/distribution.xml" \
  --package-path "$DIST_DIR/pkgs" \
  --resources "$PROJECT_DIR" \
  --sign "$SIGNING_IDENTITY" \
  "$DIST_DIR/OctobIR-$VERSION-macOS.pkg"

# Create DMG
echo "Creating DMG..."
hdiutil create -volname "OctobIR $VERSION" \
  -srcfolder "$DIST_DIR/OctobIR-$VERSION-macOS.pkg" \
  -ov -format UDZO \
  "$DIST_DIR/OctobIR-$VERSION-macOS.dmg"

echo "✓ macOS installer created: $DIST_DIR/OctobIR-$VERSION-macOS.dmg"
```

#### Step 2.5: Update GitHub Actions
Add to `.github/workflows/release-juce.yml` (macOS job):

```yaml
- name: Build installer
  env:
    SIGNING_IDENTITY: "-"  # Set to actual identity for signed builds
  run: |
    chmod +x installers/macos/build-pkg.sh
    ./installers/macos/build-pkg.sh

- name: Upload installer
  uses: actions/upload-artifact@v4
  with:
    name: juce-plugin-macos-installer
    path: dist/OctobIR-*-macOS.dmg
```

### Phase 3: Linux Installer (Tarball + Script)

**Estimated Time**: 1-2 hours

#### Step 3.1: Create Install Script
Create `installers/linux/install.sh`:

```bash
#!/usr/bin/env bash
set -e

echo "OctobIR Audio Plugin Installer"
echo "=============================="
echo ""

# Detect installation directories
VST3_DIR="${HOME}/.vst3"
BIN_DIR="${HOME}/bin"

# Create directories if needed
mkdir -p "$VST3_DIR"
mkdir -p "$BIN_DIR"

# Install VST3
if [ -d "OctobIR.vst3" ]; then
  echo "Installing VST3 plugin to $VST3_DIR"
  cp -r OctobIR.vst3 "$VST3_DIR/"
  echo "✓ VST3 installed"
else
  echo "⚠ VST3 plugin not found"
fi

# Install Standalone
if [ -f "OctobIR" ]; then
  echo "Installing standalone application to $BIN_DIR"
  cp OctobIR "$BIN_DIR/"
  chmod +x "$BIN_DIR/OctobIR"
  echo "✓ Standalone installed"

  if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
    echo ""
    echo "Note: Add $BIN_DIR to your PATH to run OctobIR from anywhere:"
    echo "  echo 'export PATH=\"\$HOME/bin:\$PATH\"' >> ~/.bashrc"
  fi
else
  echo "⚠ Standalone application not found"
fi

echo ""
echo "Installation complete!"
```

#### Step 3.2: Update Build Script
Modify `scripts/build-release-juce.sh` for Linux:

```bash
# In the Linux tarball creation section:
cd package
cp ../installers/linux/install.sh OctobIR/
chmod +x OctobIR/install.sh
tar -czf ../OctobIR-Linux.tar.gz OctobIR
cd ..
```

### Phase 4: Integration & Testing

**Estimated Time**: 2-3 hours

#### Step 4.1: Update Makefile
Update `make installers` target:

```makefile
installers:
	@echo "Building platform-specific installers..."
	@PLATFORM=$$(uname -s); \
	case "$$PLATFORM" in \
	  Darwin*) \
	    ./installers/macos/build-pkg.sh ;; \
	  Linux*) \
	    ./scripts/build-release-juce.sh ;; \
	  MINGW*|MSYS*|CYGWIN*) \
	    ./installers/windows/build-installer.sh ;; \
	  *) \
	    echo "Unknown platform: $$PLATFORM" ;; \
	esac
```

#### Step 4.2: Create Testing Checklist

**Windows Testing:**
- [ ] Run installer on clean Windows VM
- [ ] Verify component checkboxes appear
- [ ] Test installing only VST3
- [ ] Test installing only Standalone
- [ ] Test installing both
- [ ] Verify files installed to correct paths
- [ ] Test uninstaller
- [ ] Verify Start Menu shortcuts created

**macOS Testing:**
- [ ] Run installer on clean macOS VM
- [ ] Verify component selection UI
- [ ] Test installing only AU
- [ ] Test installing only VST3
- [ ] Test installing only Standalone
- [ ] Test various combinations
- [ ] Verify files installed to correct paths
- [ ] Test in Logic Pro (AU), Ableton Live (VST3), Standalone
- [ ] Verify no admin password needed for user ~/Library installs

**Linux Testing:**
- [ ] Extract tarball
- [ ] Run install.sh
- [ ] Verify VST3 in ~/.vst3
- [ ] Verify standalone in ~/bin
- [ ] Test loading in Reaper/Bitwig

#### Step 4.3: Update Documentation

Update `README.md` installation section:

```markdown
## Installation

### macOS
1. Download `OctobIR-macOS.dmg`
2. Open the DMG and run the installer
3. Select components (AU, VST3, Standalone)
4. Complete installation

### Windows
1. Download `OctobIR-Windows.exe`
2. Run the installer
3. Select components (VST3, Standalone)
4. Complete installation

### Linux
1. Download `OctobIR-Linux.tar.gz`
2. Extract: `tar -xzf OctobIR-Linux.tar.gz`
3. Run: `cd OctobIR && ./install.sh`
```

## Code Signing & Notarization (Optional Phase 5)

**Estimated Time**: 4-6 hours (includes Apple Developer setup)

### macOS Code Signing
Requires Apple Developer Program membership ($99/year).

**Setup:**
1. Create certificates in Apple Developer portal
2. Download and install in Keychain
3. Set `SIGNING_IDENTITY` environment variable
4. Update build script to use actual identity

**Notarization:**
```bash
# After building PKG
xcrun notarytool submit OctobIR.pkg \
  --apple-id "your@email.com" \
  --team-id "TEAMID" \
  --password "app-specific-password" \
  --wait

# Staple ticket
xcrun stapler staple OctobIR.pkg
```

### Windows Code Signing
Requires code signing certificate (e.g., from Certum, Sectigo).

```powershell
signtool sign /f certificate.pfx /p password /t http://timestamp.digicert.com OctobIR-Installer.exe
```

## Timeline Summary

| Phase | Description | Time Estimate |
|-------|-------------|---------------|
| 1 | Windows Installer (Inno Setup) | 3-4 hours |
| 2 | macOS Installer (pkgbuild) | 4-5 hours |
| 3 | Linux Installer (tarball) | 1-2 hours |
| 4 | Integration & Testing | 2-3 hours |
| **Total** | **Core Implementation** | **10-14 hours** |
| 5 (Optional) | Code Signing/Notarization | 4-6 hours |

## Success Criteria

- [ ] Windows installer shows component selection wizard
- [ ] macOS installer shows component selection wizard
- [ ] All installers place files in correct system locations
- [ ] Installers tested on clean VMs
- [ ] GitHub Actions builds installers automatically
- [ ] Users can select individual components (AU, VST3, Standalone)
- [ ] Uninstallers work properly
- [ ] Documentation updated

## References

### Official Documentation
- [JUCE Plugin Packaging Tutorial](https://juce.com/tutorials/tutorial_app_plugin_packaging/)
- [Inno Setup Official Site](https://jrsoftware.org/isinfo.php)
- [Inno Setup Documentation](https://jrsoftware.org/ishelp/)

### Real-World Examples & Forums
- [JUCE Forum: Moving to CMake & Creating Installers](https://forum.juce.com/t/moving-to-cmake-creating-installers/52791)
- [JUCE Forum: Building with CPack](https://forum.juce.com/t/building-plugin-installers-with-cpack-productbuild-generator/55832)
- [KVR: Best Installer Options](https://www.kvraudio.com/forum/viewtopic.php?t=506798)
- [KVR: Inno Setup VST2 Example](https://www.kvraudio.com/forum/viewtopic.php?t=501615)

### Guides & Tutorials
- [How to Make macOS Installers for JUCE](https://moonbase.sh/articles/how-to-make-macos-installers-for-juce-projects-with-pkgbuild-and-productbuild/)
- [hansen-audio/vst3-cpack](https://github.com/hansen-audio/vst3-cpack) (CPack alternative we rejected)

## Notes

- **CPack was considered but rejected**: Commercial developers report it's "cumbersome" with limited customization
- **Manual approach preferred**: More initial work but better long-term results and industry standard
- **Platform-specific runners required**: Cannot cross-compile installers (code signing, notarization are platform-specific)
- **VCV Rack distribution is separate**: Source submission to VCV Library, not self-hosted installers
