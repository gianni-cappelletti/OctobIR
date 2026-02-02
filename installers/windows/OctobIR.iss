[Setup]
AppName=OctobIR
AppVersion=1.0.0
AppPublisher=OctobIR
AppPublisherURL=https://github.com/gianteagle/OctobIR
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

[Files]
; VST3 Plugin
Source: "..\..\build\release\plugins\juce-multiformat\OctobIR_artefacts\Release\VST3\OctobIR.vst3\*"; \
  DestDir: "{commoncf}\VST3\OctobIR.vst3"; \
  Flags: recursesubdirs

; Documentation
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Uninstall OctobIR"; Filename: "{uninstallexe}"

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;
