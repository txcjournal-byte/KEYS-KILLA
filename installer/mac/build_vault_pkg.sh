#!/bin/bash
# Builds the macOS installer for CHROME VAULT by TrapWin:
#   VST3 -> /Library/Audio/Plug-Ins/VST3, AU -> /Library/Audio/Plug-Ins/Components, Standalone -> /Applications
# Mirrors installer/mac/build_pkg.sh (EVOLVE) but uses its own staging / output folders, so both can run in one CI job.
# Signs + notarizes when the signing identities / credentials are present in the environment:
#   MAC_APP_SIGN_ID    "Developer ID Application: ..."   (code signing)
#   MAC_INST_SIGN_ID   "Developer ID Installer: ..."     (pkg signing)
#   NOTARY_APPLE_ID, NOTARY_TEAM_ID, NOTARY_PASSWORD     (notarytool)
# Without them the pkg (and the plugins inside it) are unsigned - see docs/CHROME_VAULT_INSTALL.md.
# Sample library: when CV_LIBRARY_DIR names a folder holding library.json + audio/ (the Library folder of the
# "vault-library" branch), the pkg also installs it to /Library/Application Support/TrapWin/CHROME VAULT/Library
# (replacing an older library completely) and the script fails when the pkg reaches 2 GB (one GitHub release asset).
# Usage: installer/mac/build_vault_pkg.sh <version>
# Output: build/installer-vault/CHROME-VAULT-<version>-macOS.pkg
#         build/pkgroot-vault/... (the staged, signed-if-possible bundles; CI zips them from here)
set -euo pipefail
VERSION="${1:-1.0.0}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ART="$ROOT/build/ChromeVault_artefacts/Release"
STAGE="$ROOT/build/pkgroot-vault"
OUT="$ROOT/build/installer-vault"
PKGID="com.trapwin.chromevault"
NAME="CHROME VAULT"

VST3_DIR="$STAGE/Library/Audio/Plug-Ins/VST3"
AU_DIR="$STAGE/Library/Audio/Plug-Ins/Components"
APP_DIR="$STAGE/Applications"
LIB_DEST="Library/Application Support/TrapWin/CHROME VAULT/Library"
SCRIPTS="$ROOT/build/pkgscripts-vault"
MAX_BYTES=2000000000

rm -rf "$STAGE" "$OUT" "$SCRIPTS" && mkdir -p "$VST3_DIR" "$AU_DIR" "$APP_DIR" "$OUT"
cp -R "$ART/VST3/$NAME.vst3" "$VST3_DIR/"
cp -R "$ART/AU/$NAME.component" "$AU_DIR/"
cp -R "$ART/Standalone/$NAME.app" "$APP_DIR/"

SCRIPT_ARGS=()
if [ -n "${CV_LIBRARY_DIR:-}" ]; then
    [ -f "$CV_LIBRARY_DIR/library.json" ] || { echo "CV_LIBRARY_DIR=$CV_LIBRARY_DIR has no library.json" >&2; exit 1; }
    mkdir -p "$STAGE/$LIB_DEST"
    cp "$CV_LIBRARY_DIR/library.json" "$STAGE/$LIB_DEST/"
    ditto "$CV_LIBRARY_DIR/audio" "$STAGE/$LIB_DEST/audio"   # library.json + audio only (build reports left out)
    chmod -R u+rwX,go+rX,go-w "$STAGE/Library/Application Support"
    echo "with the sample library: $(du -sh "$STAGE/$LIB_DEST" | cut -f1) from $CV_LIBRARY_DIR"
    # an update replaces the whole library, so no stale samples of an older version stay behind
    mkdir -p "$SCRIPTS"
    cat > "$SCRIPTS/preinstall" <<'SH'
#!/bin/sh
rm -rf "/Library/Application Support/TrapWin/CHROME VAULT/Library"
exit 0
SH
    chmod 755 "$SCRIPTS/preinstall"
    SCRIPT_ARGS=(--scripts "$SCRIPTS")
else
    echo "CV_LIBRARY_DIR not set: plugin-only pkg (an installed library is kept)."
fi

if [ -n "${MAC_APP_SIGN_ID:-}" ]; then
    for b in "$VST3_DIR/$NAME.vst3" "$AU_DIR/$NAME.component" "$APP_DIR/$NAME.app"; do
        codesign --force --deep --options runtime --timestamp --sign "$MAC_APP_SIGN_ID" "$b"
    done
else
    echo "MAC_APP_SIGN_ID not set: plugins stay unsigned (ad-hoc), the pkg is unsigned."
fi

pkgbuild --root "$STAGE" --identifier "$PKGID" --version "$VERSION" --install-location / ${SCRIPT_ARGS[@]+"${SCRIPT_ARGS[@]}"} "$OUT/chromevault-component.pkg"

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

# notarization: App Store Connect API key (NOTARY_KEY_PATH/_ID/ISSUER, set by setup_signing.sh) or Apple ID + app password
NOTARIZED=0
if [ -n "${MAC_INST_SIGN_ID:-}" ] && [ -n "${NOTARY_KEY_PATH:-}" ]; then
    xcrun notarytool submit "$PKG" --key "$NOTARY_KEY_PATH" --key-id "$NOTARY_KEY_ID" --issuer "$NOTARY_ISSUER" --wait --timeout 60m
    NOTARIZED=1
elif [ -n "${MAC_INST_SIGN_ID:-}" ] && [ -n "${NOTARY_APPLE_ID:-}" ]; then
    xcrun notarytool submit "$PKG" --apple-id "$NOTARY_APPLE_ID" --team-id "$NOTARY_TEAM_ID" --password "$NOTARY_PASSWORD" --wait --timeout 60m
    NOTARIZED=1
fi
if [ "$NOTARIZED" = 1 ]; then
    xcrun stapler staple "$PKG"
    # the bundles inside were notarized with the pkg: staple their tickets too (used by the plugin-only zip)
    find "$STAGE" -maxdepth 6 \( -name "*.vst3" -o -name "*.component" -o -name "*.app" \) -type d -prune -print0 2>/dev/null \
        | xargs -0 -I{} xcrun stapler staple "{}" || true
fi
SIZE=$(stat -f%z "$PKG" 2>/dev/null || stat -c%s "$PKG")
echo "Created $PKG ($SIZE bytes)"
if [ "$SIZE" -ge "$MAX_BYTES" ]; then
    echo "ERROR: the pkg is $SIZE bytes - a GitHub release asset must stay below 2 GB (limit used: $MAX_BYTES)." >&2
    exit 1
fi
