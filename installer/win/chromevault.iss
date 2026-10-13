; Inno Setup script for CHROME VAULT by TrapWin (Windows x64, VST3 + Standalone)
; Build (from the repo root, after building ChromeVault_VST3 + ChromeVault_Standalone into build\ with --config Release):
;   ISCC.exe /DAppVersion=0.1.0 installer\win\chromevault.iss
; Output: build\installer-vault\CHROME-VAULT-<version>-Windows-Setup.exe
; (a separate folder from EVOLVE's build\installer, so the two products' CI steps never pick up each other's installer)
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#define Artefacts "..\..\build\ChromeVault_artefacts\Release"

[Setup]
; NEVER change the AppId after release - Windows uses it to find the installed product for updates / uninstall
AppId={{9F6FBD8D-BF05-49CA-B509-1BE044ABD3A9}
AppName=CHROME VAULT
AppVerName=CHROME VAULT {#AppVersion}
AppVersion={#AppVersion}
AppPublisher=TrapWin
VersionInfoCompany=TrapWin
VersionInfoProductName=CHROME VAULT
VersionInfoVersion={#AppVersion}
DefaultDirName={commonpf64}\TrapWin\CHROME VAULT
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
UninstallDisplayName=CHROME VAULT by TrapWin
UninstallDisplayIcon={app}\CHROME VAULT.exe
LicenseFile=..\EULA_CHROME_VAULT.txt
OutputDir=..\..\build\installer-vault
OutputBaseFilename=CHROME-VAULT-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Files]
; VST3 bundle -> C:\Program Files\Common Files\VST3\CHROME VAULT.vst3 (the folder every VST3 host scans)
Source: "{#Artefacts}\VST3\CHROME VAULT.vst3\*"; DestDir: "{commoncf64}\VST3\CHROME VAULT.vst3"; Excludes: "*.pdb,*.ilk,*.exp,*.lib"; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone app -> C:\Program Files\TrapWin\CHROME VAULT
Source: "{#Artefacts}\Standalone\CHROME VAULT.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\EULA_CHROME_VAULT.txt"; DestDir: "{app}"; DestName: "EULA.txt"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\TrapWin\CHROME VAULT"; Filename: "{app}\CHROME VAULT.exe"
Name: "{commonprograms}\TrapWin\Uninstall CHROME VAULT"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\CHROME VAULT.exe"; Description: "Open CHROME VAULT (standalone)"; Flags: nowait postinstall skipifsilent unchecked runasoriginaluser

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\CHROME VAULT.vst3"
