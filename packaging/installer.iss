#ifndef Artifacts
  #define Artifacts "..\build\GBoyAdvance2_artefacts\Release"
#endif
#ifndef Output
  #define Output "..\dist"
#endif
[Setup]
AppId={{2C50A781-6FD5-4B9C-BC20-C0CF1C2D8CA2}
AppName=G Boy Advance 2
AppVersion=0.10.3
AppPublisher=Treezy Audio
DefaultDirName={autopf}\Treezy Audio\G Boy Advance 2
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir={#Output}
OutputBaseFilename=G-Boy-Advance-2-Setup-x64
Compression=lzma2
SolidCompression=yes
UninstallDisplayName=G Boy Advance 2
[Files]
Source: "{#Artifacts}\VST3\G Boy Advance 2.vst3\*"; DestDir: "{commoncf64}\VST3\G Boy Advance 2.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artifacts}\Standalone\G Boy Advance 2.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\USER_MANUAL.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\FL_STUDIO.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\scripts\collect-windows-diagnostics.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\LICENSING.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Presets\factory.json"; DestDir: "{app}\Factory Presets"; Flags: ignoreversion
Source: "..\docs\licenses\*"; DestDir: "{app}\Third-party notices"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{autoprograms}\G Boy Advance 2"; Filename: "{app}\G Boy Advance 2.exe"
