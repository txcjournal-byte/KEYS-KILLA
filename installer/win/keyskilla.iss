; Inno Setup script for KEYS KILLA (Windows, VST3 + Standalone)
; Build: ISCC.exe /DAppVersion=0.2.0 installer\win\keyskilla.iss
#ifndef AppVersion
  #define AppVersion "0.2.0"
#endif

[Setup]
AppId={{4C1E9A77-5B2D-4E0A-9F61-6B3E2A7D0C11}
AppName=KEYS KILLA
AppVersion={#AppVersion}
AppPublisher=808 KILLA
DefaultDirName={commonpf64}\KEYS KILLA
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
LicenseFile=..\EULA.txt
OutputDir=..\..\build\installer
OutputBaseFilename=KEYS-KILLA-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes

[Files]
Source: "..\..\build\KeysKilla_artefacts\Release\VST3\KEYS KILLA.vst3\*"; DestDir: "{commoncf64}\VST3\KEYS KILLA.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\build\KeysKilla_artefacts\Release\Standalone\KEYS KILLA.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\EULA.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\KEYS KILLA"; Filename: "{app}\KEYS KILLA.exe"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\KEYS KILLA.vst3"
