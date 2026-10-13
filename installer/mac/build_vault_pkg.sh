#!/bin/bash
# Builds the macOS installer for CHROME VAULT by TrapWin:
#   VST3 -> /Library/Audio/Plug-Ins/VST3, AU -> /Library/Audio/Plug-Ins/Components, Standalone -> /Applications
# Mirrors installer/mac/build_pkg.sh (EVOLVE) but uses its own staging / output folders, so both can run in one CI job.
# Signs + notarizes when the signing identities / credentials are present in the environment:
#   MAC_APP_SIGN_ID    "Developer ID Application: ..."   (code signing)
#   MAC_INST_SIGN_ID   "Developer ID Installer: ..."     (pkg signing)
#   NOTARY_APPLE_ID, NOTARY_TEAM_ID, NOTARY_PASSWORD     (notarytool)
# Without them the pkg (and the plugins inside it) are unsigned - see docs/CHROME_VAULT_INSTALL.md.
# Usage: installer/mac/build_vault_pkg.sh <version>
# Output: build/installer-vault/CHROME-VAULT-<version>-macOS.pkg
#         build/pkgroot-vault/... (the staged, signed-if-possible bundles; CI zips them from here)
set -euo pipefail
VERSION="${1:-0.1.0}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ART="$ROOT/build/ChromeVault_artefacts/Release"
STAGE="$ROOT/build/pkgroot-vault"
OUT="$ROOT/build/installer-vault"
PKGID="com.trapwin.chromevault"
NAME="CHROME VAULT"

VST3_DIR="$STAGE/Library/Audio/Plug-Ins/VST3"
AU_DIR="$STAGE/Library/Audio/Plug-Ins/Components"
APP_DIR="$STAGE/Applications"

rm -rf "$STAGE" "$OUT" && mkdir -p "$VST3_DIR" "$AU_DIR" "$APP_DIR" "$OUT"
cp -R "$ART/VST3/$NAME.vst3" "$VST3_DIR/"
cp -R "$ART/AU/$NAME.component" "$AU_DIR/"
cp -R "$ART/Standalone/$NAME.app" "$APP_DIR/"

if [ -n "${MAC_APP_SIGN_ID:-}" ]; then
    for b in "$VST3_DIR/$NAME.vst3" "$AU_DIR/$NAME.component" "$APP_DIR/$NAME.app"; do
        codesign --force --deep --options runtime --timestamp --sign "$MAC_APP_SIGN_ID" "$b"
    done
else
    echo "MAC_APP_SIGN_ID not set: plugins stay unsigned (ad-hoc), the pkg is unsigned."
fi

pkgbuild --root "$STAGE" --identifier "$PKGID" --version "$VERSION" --install-location / "$OUT/chromevault-component.pkg"

cat > "$OUT/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>CHROME VAULT by TrapWin $VERSION</title>
    <license file="EULA.txt"/>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <choices-outline><line choice="default"/></choices-outline>
    <choice id="default"><pkg-ref id="$PKGID"/></choice>
    <pkg-ref id="$PKGID" version="$VERSION">chromevault-component.pkg</pkg-ref>
</installer-gui-script>
XML

mkdir -p "$OUT/resources" && cp "$ROOT/installer/EULA_CHROME_VAULT.txt" "$OUT/resources/EULA.txt"
PKG="$OUT/CHROME-VAULT-$VERSION-macOS.pkg"
if [ -n "${MAC_INST_SIGN_ID:-}" ]; then
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" --sign "$MAC_INST_SIGN_ID" "$PKG"
else
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" "$PKG"
fi
rm -f "$OUT/chromevault-component.pkg"

if [ -n "${NOTARY_APPLE_ID:-}" ] && [ -n "${MAC_INST_SIGN_ID:-}" ]; then
    xcrun notarytool submit "$PKG" --apple-id "$NOTARY_APPLE_ID" --team-id "$NOTARY_TEAM_ID" --password "$NOTARY_PASSWORD" --wait
    xcrun stapler staple "$PKG"
fi
echo "Created $PKG"
