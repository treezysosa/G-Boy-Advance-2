#ifndef Artifacts
  #define Artifacts "..\build\GBoyAdvance2_artefacts\Release"
#endif
#ifndef Output
  #define Output "..\dist"
#endif
#ifndef AppVersion
  #error AppVersion must be supplied by scripts/package-windows.ps1 from CMakeLists.txt
#endif
#ifndef TargetArch
  #define TargetArch "x64"
#endif
#if TargetArch == "x86"
  #define AppGuid "{{8D4F54E2-73EA-4672-A4C2-8179069EEA31}"
  #define ProgramFolder "{autopf32}"
  #define CommonFolder "{commoncf32}"
  #define AppSuffix " (32-bit)"
#else
  #define AppGuid "{{2C50A781-6FD5-4B9C-BC20-C0CF1C2D8CA2}"
  #define ProgramFolder "{autopf}"
  #define CommonFolder "{commoncf64}"
  #define AppSuffix ""
#endif
[Setup]
AppId={#AppGuid}
AppName=G Boy Advance 2{#AppSuffix}
AppVersion={#AppVersion}
AppPublisher=Treezy Audio
DefaultDirName={#ProgramFolder}\Treezy Audio\G Boy Advance 2
#if TargetArch == "x64"
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#endif
PrivilegesRequired=admin
OutputDir={#Output}
OutputBaseFilename=G-Boy-Advance-2-Setup-{#TargetArch}
Compression=lzma2
SolidCompression=yes
UninstallDisplayName=G Boy Advance 2{#AppSuffix}
[Files]
Source: "{#Artifacts}\VST3\G Boy Advance 2.vst3\*"; DestDir: "{#CommonFolder}\VST3\G Boy Advance 2.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artifacts}\Standalone\G Boy Advance 2.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\USER_MANUAL.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\FL_STUDIO.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\scripts\collect-windows-diagnostics.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\docs\LICENSING.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Presets\factory.json"; DestDir: "{app}\Factory Presets"; Flags: ignoreversion
Source: "..\docs\licenses\*"; DestDir: "{app}\Third-party notices"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{autoprograms}\G Boy Advance 2{#AppSuffix}"; Filename: "{app}\G Boy Advance 2.exe"
