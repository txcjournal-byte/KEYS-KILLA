; Inno Setup script for EVOLVE by TrapVST (Windows, VST3 + Standalone)
; Build: ISCC.exe /DAppVersion=0.2.0 installer\win\keyskilla.iss
#ifndef AppVersion
  #define AppVersion "0.2.0"
#endif

[Setup]
AppId={{4C1E9A77-5B2D-4E0A-9F61-6B3E2A7D0C11}
AppName=EVOLVE by TrapVST
AppVersion={#AppVersion}
AppPublisher=TrapVST
DefaultDirName={commonpf64}\EVOLVE
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

[InstallDelete]
; the plugin used to be called KEYS KILLA, then BREED LAB - the old bundles go, so FL Studio sees one plugin
Type: filesandordirs; Name: "{commoncf64}\VST3\KEYS KILLA.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\BREED LAB.vst3"
Type: filesandordirs; Name: "{commonpf64}\BREED LAB"

[Files]
Source: "..\..\build\KeysKilla_artefacts\Release\VST3\EVOLVE.vst3\*"; DestDir: "{commoncf64}\VST3\EVOLVE.vst3"; Excludes: "*.pdb,*.ilk"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\build\KeysKilla_artefacts\Release\Standalone\EVOLVE.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\EULA.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\EVOLVE"; Filename: "{app}\EVOLVE.exe"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\EVOLVE.vst3"
