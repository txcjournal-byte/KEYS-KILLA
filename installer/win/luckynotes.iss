; Inno Setup script for TrapWin Lucky Notes (Windows x64, VST3 + Standalone)
; Build (from the repo root, after building LuckyNotes_VST3 + LuckyNotes_Standalone into build\ with --config Release):
;   ISCC.exe /DAppVersion=1.0.0 installer\win\luckynotes.iss
; Output: build\installer-luckynotes\LuckyNotes-<version>-Windows-Setup.exe
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define Artefacts "..\..\build\LuckyNotes_artefacts\Release"

[Setup]
; NEVER change the AppId after release - Windows uses it to find the installed product for updates / uninstall
AppId={{A21DCCAF-E332-4CEE-8818-53CC0617EC04}
AppName=Lucky Notes
AppVerName=TrapWin Lucky Notes {#AppVersion}
AppVersion={#AppVersion}
AppPublisher=TrapWin
VersionInfoCompany=TrapWin
VersionInfoProductName=Lucky Notes
VersionInfoVersion={#AppVersion}
DefaultDirName={commonpf64}\TrapWin\Lucky Notes
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
UninstallDisplayName=TrapWin Lucky Notes
UninstallDisplayIcon={app}\Lucky Notes.exe
LicenseFile=..\EULA_LUCKY_NOTES.txt
OutputDir=..\..\build\installer-luckynotes
OutputBaseFilename=LuckyNotes-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
WizardStyle=modern
DiskSpanning=no
Compression=lzma2
SolidCompression=yes

[Files]
; VST3 bundle -> C:\Program Files\Common Files\VST3\Lucky Notes.vst3 (the folder every VST3 host scans)
Source: "{#Artefacts}\VST3\Lucky Notes.vst3\*"; DestDir: "{commoncf64}\VST3\Lucky Notes.vst3"; Excludes: "*.pdb,*.ilk,*.exp,*.lib"; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone app -> C:\Program Files\TrapWin\Lucky Notes
Source: "{#Artefacts}\Standalone\Lucky Notes.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\EULA_LUCKY_NOTES.txt"; DestDir: "{app}"; DestName: "EULA.txt"; Flags: ignoreversion
Source: "..\..\resources\fonts\OFL-*.txt"; DestDir: "{app}\Font licenses"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\TrapWin\Lucky Notes"; Filename: "{app}\Lucky Notes.exe"
Name: "{commonprograms}\TrapWin\Uninstall Lucky Notes"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\Lucky Notes.exe"; Description: "Open Lucky Notes (standalone)"; Flags: nowait postinstall skipifsilent unchecked runasoriginaluser

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Lucky Notes.vst3"
