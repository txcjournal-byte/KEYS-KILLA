; Inno Setup script for CHROME VAULT by TrapWin (Windows x64, VST3 + Standalone [+ sample library])
; Build (from the repo root, after building ChromeVault_VST3 + ChromeVault_Standalone into build\ with --config Release):
;   ISCC.exe /DAppVersion=1.0.0 installer\win\chromevault.iss                                  (plugin only)
;   ISCC.exe /DAppVersion=1.0.0 "/DLibraryDir=D:\path\vault-library\Library" installer\win\chromevault.iss
; LibraryDir = an ABSOLUTE folder holding library.json + audio\ (the Library folder of the "vault-library" branch,
; docs/CHROME_VAULT_INSTALL.md). With it the sample library is installed to C:\ProgramData\TrapWin\CHROME VAULT\Library,
; the system folder the plugin searches (Source/vault/VaultLibrary.cpp). OggVorbis barely compresses, so the installer is
; about as large as the library: CI checks it stays < 2 GB (one GitHub release asset).
; Output: build\installer-vault\CHROME-VAULT-<version>-Windows-Setup.exe
; (a separate folder from EVOLVE's build\installer, so the two products' CI steps never pick up each other's installer)
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define Artefacts "..\..\build\ChromeVault_artefacts\Release"
#ifdef LibraryDir
  #if !FileExists(AddBackslash(LibraryDir) + "library.json")
    #error LibraryDir does not hold library.json
  #endif
  #pragma message "CHROME VAULT installer: with the sample library from " + LibraryDir
#else
  #pragma message "CHROME VAULT installer: plugin only (no LibraryDir)"
#endif

[Setup]
; NEVER change the AppId after release - Windows uses it to find the installed product for updates / uninstall
AppId={{9F6FBD8D-BF05-49CA-B509-1BE044ABD3A9}
AppName=TrapWin Chrome Vault
AppVerName=TrapWin Chrome Vault {#AppVersion}
AppVersion={#AppVersion}
AppPublisher=TrapWin
VersionInfoCompany=TrapWin
VersionInfoProductName=TrapWin Chrome Vault
VersionInfoVersion={#AppVersion}
DefaultDirName={commonpf64}\TrapWin\CHROME VAULT
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
UninstallDisplayName=TrapWin Chrome Vault
UninstallDisplayIcon={app}\TrapWin Chrome Vault.exe
LicenseFile=..\EULA_CHROME_VAULT.txt
OutputDir=..\..\build\installer-vault
OutputBaseFilename=CHROME-VAULT-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
WizardStyle=modern
; one self-contained .exe (no setup-*.bin disk spanning): it is published as a single download
DiskSpanning=no
#ifdef LibraryDir
; the samples: one solid LZMA2 stream at maximum ratio, compressed in a separate process with 4 block threads
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes
LZMANumBlockThreads=4
#else
Compression=lzma2
SolidCompression=yes
#endif

[Files]
; VST3 bundle -> C:\Program Files\Common Files\VST3\TrapWin Chrome Vault.vst3 (the folder every VST3 host scans)
Source: "{#Artefacts}\VST3\TrapWin Chrome Vault.vst3\*"; DestDir: "{commoncf64}\VST3\TrapWin Chrome Vault.vst3"; Excludes: "*.pdb,*.ilk,*.exp,*.lib"; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone app -> C:\Program Files\TrapWin\CHROME VAULT
Source: "{#Artefacts}\Standalone\TrapWin Chrome Vault.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\EULA_CHROME_VAULT.txt"; DestDir: "{app}"; DestName: "EULA.txt"; Flags: ignoreversion
Source: "..\..\docs\CHROME_VAULT_CREDITS.md"; DestDir: "{app}"; DestName: "Sample credits.txt"; Flags: ignoreversion
#ifdef LibraryDir
; sample library -> C:\ProgramData\TrapWin\CHROME VAULT\Library (library.json + audio\**; anything else is left out)
Source: "{#LibraryDir}\library.json"; DestDir: "{commonappdata}\TrapWin\CHROME VAULT\Library"; Flags: ignoreversion
Source: "{#LibraryDir}\audio\*"; DestDir: "{commonappdata}\TrapWin\CHROME VAULT\Library\audio"; Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
; an update replaces the whole library, so no stale samples of an older version stay behind
; (a plugin-only installer has no [InstallDelete]: it keeps a library that is already installed)
Type: filesandordirs; Name: "{commonappdata}\TrapWin\CHROME VAULT\Library\audio"
Type: files; Name: "{commonappdata}\TrapWin\CHROME VAULT\Library\library.json"
#endif

[InstallDelete]
; the plugin was called "CHROME VAULT" before: remove the old bundle / app so the DAW does not list it twice
Type: filesandordirs; Name: "{commoncf64}\VST3\CHROME VAULT.vst3"
Type: files; Name: "{app}\CHROME VAULT.exe"

[Icons]
Name: "{commonprograms}\TrapWin\TrapWin Chrome Vault"; Filename: "{app}\TrapWin Chrome Vault.exe"
Name: "{commonprograms}\TrapWin\Uninstall TrapWin Chrome Vault"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\TrapWin Chrome Vault.exe"; Description: "Open TrapWin Chrome Vault (standalone)"; Flags: nowait postinstall skipifsilent unchecked runasoriginaluser

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\TrapWin Chrome Vault.vst3"
#ifdef LibraryDir
Type: filesandordirs; Name: "{commonappdata}\TrapWin\CHROME VAULT\Library"
Type: dirifempty; Name: "{commonappdata}\TrapWin\CHROME VAULT"
Type: dirifempty; Name: "{commonappdata}\TrapWin"
#endif
