# CHROME VAULT by TrapWin - installation

CHROME VAULT is a preset instrument (a synth plugin): open it, click a sound, play.
It comes as **VST3** (Windows + Mac), **AU** (Mac) and a **Standalone** app, plus the **sample library**: the recorded
instruments (pianos, strings, brass, woodwinds, guitars, mallets, voices...) that most sounds play. The library is one
folder of about 1-2 GB, shared by the VST3, the AU and the Standalone app. The installers put it in place for you.

## Downloads (latest test build)

Every push to the repository rebuilds these files and replaces them under the same links:

| System | File | Contents |
| --- | --- | --- |
| Windows 10 / 11 (64-bit) | [CHROME-VAULT-Setup-Windows.exe](https://github.com/txcjournal-byte/KEYS-KILLA/releases/download/test/CHROME-VAULT-Setup-Windows.exe) | installer: VST3 + Standalone + sample library (recommended) |
| Windows 10 / 11 (64-bit) | [CHROME-VAULT-VST3-Windows.zip](https://github.com/txcjournal-byte/KEYS-KILLA/releases/download/test/CHROME-VAULT-VST3-Windows.zip) | `CHROME VAULT.vst3` only, for a manual install (small) |
| macOS 10.13 or newer (Apple Silicon + Intel) | [CHROME-VAULT-Setup-Mac.pkg](https://github.com/txcjournal-byte/KEYS-KILLA/releases/download/test/CHROME-VAULT-Setup-Mac.pkg) | installer: VST3 + AU + Standalone + sample library (recommended) |
| macOS 10.13 or newer (Apple Silicon + Intel) | [CHROME-VAULT-Mac.zip](https://github.com/txcjournal-byte/KEYS-KILLA/releases/download/test/CHROME-VAULT-Mac.zip) | `CHROME VAULT.vst3` + `CHROME VAULT.component`, for a manual install (small) |
| Windows + macOS | [CHROME-VAULT-Library.zip](https://github.com/txcjournal-byte/KEYS-KILLA/releases/download/test/CHROME-VAULT-Library.zip) | the sample library alone, for a manual install (see [Sample library](#sample-library)) |

The installers are large (about the size of the library, up to 2 GB) because the samples are inside. The two small zips
are the plugins alone: use them to update the plugin when the library is already installed.
If the library is larger than 1.9 GB, the release has `CHROME-VAULT-Library-part1-of-N.zip`, `...-part2-of-N.zip`, ...
instead of one `CHROME-VAULT-Library.zip`. Download all parts. Each part is a normal zip (no special tool needed).
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
     under **TrapWin > CHROME VAULT**;
   * the sample library in `C:\ProgramData\TrapWin\CHROME VAULT\Library` (`ProgramData` is a hidden folder; type the
     path into the Explorer address bar to open it). Make sure about 2-3 GB are free on drive C: before you start.
4. To update, run a newer installer over the old one. It replaces the whole library, so no old samples stay behind.
   To remove everything, use **Settings > Apps > CHROME VAULT by TrapWin > Uninstall**.

### Manual install (zip)

1. Download `CHROME-VAULT-VST3-Windows.zip`, right-click it, choose **Properties**, tick **Unblock** if you see it, and click **OK**.
2. Extract it. You get one folder, `CHROME VAULT.vst3`.
3. Move that whole folder (do not open it or take files out of it) to `C:\Program Files\Common Files\VST3\`.
   Windows asks for administrator permission. Click **Continue**.
4. If the sample library is not installed yet, install it as described in [Sample library](#sample-library).

## macOS

### With the installer (recommended)

1. Download `CHROME-VAULT-Setup-Mac.pkg` and double-click it.
2. Click through **Continue / Agree / Install** and enter your Mac password. The installer puts:
   * `CHROME VAULT.vst3` in `/Library/Audio/Plug-Ins/VST3/`;
   * `CHROME VAULT.component` (Audio Unit) in `/Library/Audio/Plug-Ins/Components/`;
   * `CHROME VAULT.app` (Standalone) in `/Applications/`;
   * the sample library in `/Library/Application Support/TrapWin/CHROME VAULT/Library` (about 1-2 GB; an update
     replaces it completely).
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
6. If the sample library is not installed yet, install it as described in [Sample library](#sample-library).

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

## Sample library

Most CHROME VAULT sounds play recorded instruments from the sample library. The plugin looks for the folder
`Library` (holding `library.json` and the folder `audio`) in this order and uses the first one it finds:

| Order | Windows | macOS |
| --- | --- | --- |
| 1 | the folder in the environment variable `CHROME_VAULT_LIBRARY` (an absolute path) | same |
| 2 | `C:\ProgramData\TrapWin\CHROME VAULT\Library` (installer) | `/Library/Application Support/TrapWin/CHROME VAULT/Library` (installer) |
| 3 | `%APPDATA%\TrapWin\CHROME VAULT\Library` (your user only) | `~/Library/Application Support/TrapWin/CHROME VAULT/Library` (your user only) |

### Manual install (zip)

1. Download `CHROME-VAULT-Library.zip` (or every `CHROME-VAULT-Library-partN-of-M.zip`).
   Windows: right-click each zip, choose **Properties**, tick **Unblock** if you see it, click **OK**.
2. Extract it. You get one folder named `Library` with `library.json` and `audio` inside. When you have several parts,
   extract all of them into the same place and let them merge into one `Library` folder.
3. Move the `Library` folder to the system folder (row 2 above). Create the folders `TrapWin` and `CHROME VAULT` first
   if they do not exist. If a `Library` folder is already there, delete it first, so no old files stay behind.
   * Windows: `C:\ProgramData\TrapWin\CHROME VAULT\` (administrator permission needed: click **Continue**).
   * macOS: in Finder choose **Go > Go to Folder...**, enter `/Library/Application Support/`, and create
     `TrapWin/CHROME VAULT/` there (enter your password when Finder asks).
   No admin rights? Use the folder of your user (row 3) instead. On a Mac, `~/Library` is hidden: in Finder hold
   **Option** and open the **Go** menu to see **Library**.
4. Restart your host (or the Standalone app). The samples are not program code, so no quarantine or signing step is
   needed for them.

To keep the library on another drive, put the `Library` folder anywhere and set `CHROME_VAULT_LIBRARY` to its full path:
Windows **Settings > System > About > Advanced system settings > Environment Variables > New** (system variable),
macOS `launchctl setenv CHROME_VAULT_LIBRARY "/Volumes/Samples/CHROME VAULT/Library"` (lasts until the next restart).
Then restart the host.

### Checking it

* The status bar of CHROME VAULT shows **"Sample library not found - install the CHROME VAULT library"** when no
  library was found, and **"This sound needs an update of the sample library"** when the library is older than the
  plugin. Reinstall (or update from the zip) as above.
* **"Loading samples..."** is normal for a moment after you pick a sound: the samples load in the background and the
  sound plays as soon as they are in memory.
* The library uses up to 1.5 GB of RAM for the sounds you play (sounds you have not used for a while are dropped first).
  To change that limit, set the environment variable `CHROME_VAULT_RAM_MB` (for example `800` or `3000`).
* Sample credits: `Sample credits.txt` next to the Standalone app (Windows) and `docs/CHROME_VAULT_CREDITS.md`.

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

* Windows: **Settings > Apps > CHROME VAULT by TrapWin > Uninstall** removes the plugin, the app and the library (or
  delete the folders `C:\Program Files\Common Files\VST3\CHROME VAULT.vst3`, `C:\Program Files\TrapWin\CHROME VAULT`
  and `C:\ProgramData\TrapWin\CHROME VAULT` by hand).
* Mac: delete `/Library/Audio/Plug-Ins/VST3/CHROME VAULT.vst3`, `/Library/Audio/Plug-Ins/Components/CHROME VAULT.component`,
  `/Applications/CHROME VAULT.app` and the folder `/Library/Application Support/TrapWin/CHROME VAULT`. Then run
  `sudo pkgutil --forget com.trapwin.chromevault`.
* A library you installed by hand in your user folder (or with `CHROME_VAULT_LIBRARY`) is never removed automatically.

## For developers: how CI builds these files

`.github/workflows/build.yml`, steps marked **CHROME VAULT**:

* **The sample library in git.** The library (built by `VaultLibBuild` into `/home/user/cv-library`, see `docs/VAULT.md`)
  never goes on the main branch. It lives on the **orphan branch `vault-library`** of this repository (no shared
  history with the code), laid out as `Library/library.json` + `Library/audio/<instrument>/*.ogg` + `README.md`.
  Make / refresh it from a finished library with
  `python3 tools/vault/package_library.py check /home/user/cv-library` (every referenced file exists, every file
  < 50 MB, packaged size) and then `python3 tools/vault/package_library.py branch /home/user/cv-library` (git plumbing:
  the working tree and the current branch are not touched; one commit per ~400 MB). It prints one
  `git push origin <commit>:refs/heads/vault-library` line per commit: push them in order, so no single push comes near
  GitHub's 2 GB push limit. Only `library.json` and the audio files it references are committed. A refreshed library
  is a new orphan history: push it with `--force` (only that branch).
* Both library jobs look for the branch with `git ls-remote`. If it does not exist yet, every step still runs:
  `VaultTests` skips the real-library checks and the installers are plugin-only (an installed library is kept).
* **windows-quick** (pushes only): after EVOLVE is published, it checks out `vault-library` into `./vault-library`
  (`actions/checkout`, `fetch-depth: 1`), validates it (`package_library.py check`), builds `ChromeVault_VST3` +
  `ChromeVault_Standalone` + `VaultTests` and runs `VaultTests` with `CHROME_VAULT_LIBRARY` pointing at it. It runs
  `installer/win/chromevault.iss` (Inno Setup, output `build/installer-vault/`) with `/DLibraryDir=<library>`
  (`lzma2/ultra64`, solid, no disk spanning: one `.exe`). The step fails when the library or the finished installer
  reaches 2,000,000,000 bytes (a GitHub release asset must stay below 2 GiB; OggVorbis hardly compresses, so the
  installer is about as large as the library). It then uploads `CHROME-VAULT-Setup-Windows.exe`,
  `CHROME-VAULT-VST3-Windows.zip` and `CHROME-VAULT-Library.zip` to the `test` release (`gh release upload --clobber`).
  The library zip comes from `package_library.py zip`: one zip up to 1.9 GB, else self-contained
  `CHROME-VAULT-Library-partN-of-M.zip` parts (library assets of the other layout are deleted from the release).
  The big files are removed from `build/` afterwards, so the build cache stays small.
* **windows**: runs `VaultTests` (without the library), pluginval (strictness 10) on `CHROME VAULT.vst3`, and a
  plugin-only installer, and uploads them as build artifacts.
* **macos** (universal arm64 + x86_64): checks out and validates `vault-library` like windows-quick, runs `VaultTests`
  with `CHROME_VAULT_LIBRARY`, `auval -v aumu Cvlt Trpw` and pluginval (strictness 10). It then builds
  `installer/mac/build_vault_pkg.sh` with `CV_LIBRARY_DIR=<library>`: the pkg installs the library to
  `/Library/Application Support/TrapWin/CHROME VAULT/Library` (a `preinstall` script removes the old library first) and
  the script fails when the pkg reaches 2,000,000,000 bytes. The pkg signs and notarizes only when the secrets exist;
  otherwise the files are unsigned. It zips `CHROME VAULT.vst3` + `CHROME VAULT.component` into `CHROME-VAULT-Mac.zip`
  (plugins only). On pushes, when the library check, tests, auval and pluginval passed, it uploads the zip +
  `CHROME-VAULT-Setup-Mac.pkg` to the `test` release (and creates the release if it is missing). The library zip is
  published by windows-quick only (it is the same for both systems).

The CHROME VAULT version comes from `set(CV_VERSION ...)` in `CMakeLists.txt` (separate from the EVOLVE version).
Local installer builds: `ISCC.exe /DAppVersion=1.0.0 installer\win\chromevault.iss` (Windows, after
`cmake --build build --config Release --target ChromeVault_VST3 ChromeVault_Standalone`; add
`"/DLibraryDir=C:\full\path\to\Library"` to include the library) and `installer/mac/build_vault_pkg.sh 1.0.0` (Mac,
after building into `build/`; set `CV_LIBRARY_DIR=/full/path/to/Library` to include the library). Library zip:
`python3 tools/vault/package_library.py zip /path/to/Library out/`.
