# CHROME VAULT by TrapWin - installation

CHROME VAULT is a preset instrument (a synth plugin): open it, click a sound, play.
It comes as **VST3** (Windows + Mac), **AU** (Mac) and a **Standalone** app.

## Downloads (latest test build)

Every push to the repository rebuilds these files and replaces them under the same links:

| System | File | Contents |
| --- | --- | --- |
| Windows 10 / 11 (64-bit) | [CHROME-VAULT-Setup-Windows.exe](https://github.com/txcjournal-byte/KEYS-KILLA/releases/download/test/CHROME-VAULT-Setup-Windows.exe) | installer: VST3 + Standalone (recommended) |
| Windows 10 / 11 (64-bit) | [CHROME-VAULT-VST3-Windows.zip](https://github.com/txcjournal-byte/KEYS-KILLA/releases/download/test/CHROME-VAULT-VST3-Windows.zip) | `CHROME VAULT.vst3` only, for a manual install |
| macOS 10.13 or newer (Apple Silicon + Intel) | [CHROME-VAULT-Setup-Mac.pkg](https://github.com/txcjournal-byte/KEYS-KILLA/releases/download/test/CHROME-VAULT-Setup-Mac.pkg) | installer: VST3 + AU + Standalone (recommended) |
| macOS 10.13 or newer (Apple Silicon + Intel) | [CHROME-VAULT-Mac.zip](https://github.com/txcjournal-byte/KEYS-KILLA/releases/download/test/CHROME-VAULT-Mac.zip) | `CHROME VAULT.vst3` + `CHROME VAULT.component`, for a manual install |

The Mac files are universal (one file runs natively on Apple Silicon and Intel).
The full CI runs also keep the same files as build artifacts (`CHROME-VAULT-Windows-*`, `CHROME-VAULT-macOS*`).

## Windows

### With the installer (recommended)

1. Download `CHROME-VAULT-Setup-Windows.exe` and run it.
2. If Windows SmartScreen shows "Windows protected your PC": click **More info** and then **Run anyway**.
   The test builds are not code-signed, so Windows does not know the publisher yet.
3. Accept the license and click **Install**. The installer puts:
   * the plugin in `C:\Program Files\Common Files\VST3\CHROME VAULT.vst3`, the standard VST3 folder every host scans;
   * the Standalone app in `C:\Program Files\TrapWin\CHROME VAULT\CHROME VAULT.exe`, with a Start menu entry
     under **TrapWin > CHROME VAULT**.
4. To update, run a newer installer over the old one. To remove the plugin, use **Settings > Apps > CHROME VAULT by TrapWin > Uninstall**.

### Manual install (zip)

1. Download `CHROME-VAULT-VST3-Windows.zip`, right-click it, choose **Properties**, tick **Unblock** if you see it, and click **OK**.
2. Extract it. You get one folder, `CHROME VAULT.vst3`.
3. Move that whole folder (do not open it or take files out of it) to `C:\Program Files\Common Files\VST3\`.
   Windows asks for administrator permission. Click **Continue**.

## macOS

### With the installer (recommended)

1. Download `CHROME-VAULT-Setup-Mac.pkg` and double-click it.
2. Click through **Continue / Agree / Install** and enter your Mac password. The installer puts:
   * `CHROME VAULT.vst3` in `/Library/Audio/Plug-Ins/VST3/`;
   * `CHROME VAULT.component` (Audio Unit) in `/Library/Audio/Plug-Ins/Components/`;
   * `CHROME VAULT.app` (Standalone) in `/Applications/`.
3. If macOS says the package "can't be opened because it is from an unidentified developer", see
   [Unsigned builds on macOS](#unsigned-builds-on-macos) below.

### Manual install (zip)

1. Download `CHROME-VAULT-Mac.zip` and double-click it to unzip.
2. In Finder, choose **Go > Go to Folder...** and enter `/Library/Audio/Plug-Ins/`.
3. Move `CHROME VAULT.vst3` into the `VST3` folder and `CHROME VAULT.component` into the `Components` folder.
   Enter your password when Finder asks.
4. Run the quarantine command from the next section. A zip download is always quarantined, and an unsigned plugin
   in quarantine will not load in any host.
5. Log out and back in, or restart, so Logic / GarageBand see the new Audio Unit. Other hosts only need a rescan.

### Unsigned builds on macOS

The CI signs and notarizes the Mac files only when the Apple signing secrets are set in the repository
(`MAC_APP_SIGN_ID`, `MAC_INST_SIGN_ID`, `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID`, `NOTARY_PASSWORD`). Without them, the
`.pkg`, the plugins and the app are **unsigned**, and Gatekeeper blocks them. You may see "unidentified developer",
"is damaged and can't be opened" or "cannot be opened because Apple cannot check it for malicious software", or the
plugin may just be missing from your host. To get past this:

**Installer (.pkg) or Standalone app:** in Finder, **right-click** (or Control-click) the file, choose **Open**, then
click **Open** in the dialog. On macOS 15 (Sequoia) and newer the right-click way is gone. Instead, double-click the file
once, close the warning, open **System Settings > Privacy & Security**, scroll down to the message about
"CHROME VAULT", click **Open Anyway** and confirm with your password.

**Plugins (after installing them, from the pkg or the zip):** open **Terminal** and run:

```bash
sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/CHROME VAULT.vst3"
sudo xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/Components/CHROME VAULT.component"
sudo xattr -dr com.apple.quarantine "/Applications/CHROME VAULT.app"
```

(`xattr -dr com.apple.quarantine` removes the "downloaded from the internet" mark from the bundle and everything in it.
A line for a file you did not install prints "No such file". You can ignore that.)

If a host still says the plugin is damaged, re-apply the local ad-hoc signature and rescan:

```bash
sudo codesign --force --deep --sign - "/Library/Audio/Plug-Ins/VST3/CHROME VAULT.vst3"
sudo codesign --force --deep --sign - "/Library/Audio/Plug-Ins/Components/CHROME VAULT.component"
```

Then restart the host. For the Audio Unit, also run `killall -9 AudioComponentRegistrar` (or log out and back in) so
macOS reads the plugin list again.

## FL Studio: scan and find the plugin

1. Install CHROME VAULT as above, then start (or restart) FL Studio.
2. Open **Options > Manage plugins** (the Plugin Manager).
3. Check that the plugin folders are in the search paths:
   * Windows: `C:\Program Files\Common Files\VST3` is scanned by default.
   * Mac: `/Library/Audio/Plug-Ins/VST3` and `/Library/Audio/Plug-Ins/Components` are scanned by default.
   If you installed somewhere else, add that folder with the **+** (Add path) button.
4. Turn on **Verify plugins** and click **Find installed plugins** (or **Find more plugins**). Wait for the scan to finish.
   **CHROME VAULT** (manufacturer **TrapWin**) then appears in the list as a **Generator**, shown as VST3 (and also AU on Mac).
5. Click the **star** next to CHROME VAULT to add it to your favorites. It then shows up in
   **Add > Channel** (or the **+** in the Channel Rack) and in the Browser under
   **Plugin database > Generators**.
6. To use it: in the Channel Rack click **+**, choose **CHROME VAULT**, click a sound in its browser and play your
   MIDI keyboard, or draw notes in the Piano roll.

If CHROME VAULT does not appear:

* Click **Find installed plugins** again with **Rescan previously verified plugins** turned on.
* Windows: check that `C:\Program Files\Common Files\VST3\CHROME VAULT.vst3\Contents\x86_64-win\CHROME VAULT.vst3` exists.
  This is a 64-bit plugin, and FL Studio must also run as 64-bit (the default).
* Mac: run the `xattr` commands above, then rescan. In the Plugin Manager the scan log shows the reason when a plugin is
  refused ("not signed", "damaged", ...).

### Other hosts

Any host that loads VST3 or AU instruments works. After installing, run the host's plugin rescan and look for
**CHROME VAULT** by **TrapWin** under instruments / synths. On Mac you can check the Audio Unit with Terminal:
`auval -v aumu Cvlt Trpw` should end with "AU VALIDATION SUCCEEDED".

## Standalone app

`CHROME VAULT.exe` (Windows, Start menu > TrapWin) or `CHROME VAULT.app` (Mac, Applications) runs without a host.
Open **Options** in the app window to pick your audio output and MIDI input.

## Uninstall

* Windows: **Settings > Apps > CHROME VAULT by TrapWin > Uninstall** (or delete the folder `C:\Program Files\Common Files\VST3\CHROME VAULT.vst3`
  and `C:\Program Files\TrapWin\CHROME VAULT` by hand).
* Mac: delete `/Library/Audio/Plug-Ins/VST3/CHROME VAULT.vst3`, `/Library/Audio/Plug-Ins/Components/CHROME VAULT.component`
  and `/Applications/CHROME VAULT.app`. Then run `sudo pkgutil --forget com.trapwin.chromevault`.

## For developers: how CI builds these files

`.github/workflows/build.yml`, steps marked **CHROME VAULT**:

* **windows-quick** (pushes only): after EVOLVE is published, builds `ChromeVault_VST3` + `ChromeVault_Standalone`.
  It runs `installer/win/chromevault.iss` (Inno Setup, output `build/installer-vault/`) and uploads `CHROME-VAULT-Setup-Windows.exe` +
  `CHROME-VAULT-VST3-Windows.zip` to the `test` release (`gh release upload --clobber`).
* **windows**: runs `VaultTests`, pluginval (strictness 10) on `CHROME VAULT.vst3`, and the installer, and uploads them as build artifacts.
* **macos** (universal arm64 + x86_64): runs `VaultTests`, `auval -v aumu Cvlt Trpw` and pluginval (strictness 10).
  It then builds `installer/mac/build_vault_pkg.sh`, which signs and notarizes only when the secrets exist; otherwise
  the files are unsigned. It zips `CHROME VAULT.vst3` + `CHROME VAULT.component` into `CHROME-VAULT-Mac.zip`. On pushes,
  when tests, auval and pluginval passed, it uploads the zip + `CHROME-VAULT-Setup-Mac.pkg` to the `test` release
  (and creates the release if it is missing).

The CHROME VAULT version comes from `CV_VERSION_STRING` in `CMakeLists.txt` (separate from the EVOLVE version).
Local installer builds: `ISCC.exe /DAppVersion=0.1.0 installer\win\chromevault.iss` (Windows, after
`cmake --build build --config Release --target ChromeVault_VST3 ChromeVault_Standalone`) and
`installer/mac/build_vault_pkg.sh 0.1.0` (Mac, after building into `build/`).
