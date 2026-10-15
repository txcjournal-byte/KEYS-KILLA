#!/bin/bash
# Builds the macOS installer for TrapWin Lucky Notes (adapted from build_vault_pkg.sh):
#   VST3 -> /Library/Audio/Plug-Ins/VST3, AU -> /Library/Audio/Plug-Ins/Components, Standalone -> /Applications
#   font licenses -> /Library/Application Support/TrapWin/Lucky Notes/Font licenses
# Signs + notarizes when the signing identities / credentials are present in the environment (setup_signing.sh):
#   MAC_APP_SIGN_ID    "Developer ID Application: ..."   (code signing, hardened runtime + timestamp)
#   MAC_INST_SIGN_ID   "Developer ID Installer: ..."     (pkg signing)
#   NOTARY_KEY_PATH / NOTARY_KEY_ID / NOTARY_ISSUER (App Store Connect API key) or NOTARY_APPLE_ID / _TEAM_ID / _PASSWORD
# Without them the pkg (and the plugins inside it) are unsigned.
# Usage: installer/mac/build_luckynotes_pkg.sh <version>
# Output: build/installer-luckynotes/LuckyNotes-<version>-macOS.pkg
#         build/pkgroot-luckynotes/... (the staged, signed-if-possible bundles; CI zips them from here)
set -euo pipefail
source "$(dirname "$0")/notarize.sh"
trap 'echo "::error title=$(basename "$0")::line $LINENO failed: $BASH_COMMAND"' ERR

VERSION="${1:-1.0.0}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ART="$ROOT/build/LuckyNotes_artefacts/Release"
STAGE="$ROOT/build/pkgroot-luckynotes"
OUT="$ROOT/build/installer-luckynotes"
PKGID="com.trapwin.luckynotes"
NAME="Lucky Notes"

VST3_DIR="$STAGE/Library/Audio/Plug-Ins/VST3"
AU_DIR="$STAGE/Library/Audio/Plug-Ins/Components"
APP_DIR="$STAGE/Applications"
LIC_DIR="$STAGE/Library/Application Support/TrapWin/Lucky Notes/Font licenses"

rm -rf "$STAGE" "$OUT" && mkdir -p "$VST3_DIR" "$AU_DIR" "$APP_DIR" "$LIC_DIR" "$OUT"
ditto "$ART/VST3/$NAME.vst3" "$VST3_DIR/$NAME.vst3"
ditto "$ART/AU/$NAME.component" "$AU_DIR/$NAME.component"
ditto "$ART/Standalone/$NAME.app" "$APP_DIR/$NAME.app"
cp "$ROOT"/resources/fonts/OFL-*.txt "$LIC_DIR/"
chmod -R u+rwX,go+rX,go-w "$STAGE/Library/Application Support"

if [ -n "${MAC_APP_SIGN_ID:-}" ]; then
    for b in "$VST3_DIR/$NAME.vst3" "$AU_DIR/$NAME.component" "$APP_DIR/$NAME.app"; do
        codesign --force --deep --options runtime --timestamp --sign "$MAC_APP_SIGN_ID" "$b"
        codesign --verify --strict --verbose=2 "$b"
    done
else
    echo "MAC_APP_SIGN_ID not set: plugins stay unsigned (ad-hoc), the pkg is unsigned."
fi

pkgbuild --root "$STAGE" --identifier "$PKGID" --version "$VERSION" --install-location / "$OUT/luckynotes-component.pkg"

cat > "$OUT/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>TrapWin Lucky Notes $VERSION</title>
    <license file="EULA.txt"/>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <choices-outline><line choice="default"/></choices-outline>
    <choice id="default"><pkg-ref id="$PKGID"/></choice>
    <pkg-ref id="$PKGID" version="$VERSION">luckynotes-component.pkg</pkg-ref>
</installer-gui-script>
XML

mkdir -p "$OUT/resources" && cp "$ROOT/installer/EULA_LUCKY_NOTES.txt" "$OUT/resources/EULA.txt"
PKG="$OUT/LuckyNotes-$VERSION-macOS.pkg"
if [ -n "${MAC_INST_SIGN_ID:-}" ]; then
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" --sign "$MAC_INST_SIGN_ID" "$PKG"
    pkgutil --check-signature "$PKG" || true
else
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" "$PKG"
fi
rm -f "$OUT/luckynotes-component.pkg"

# notarization (App Store Connect API key from setup_signing.sh, or Apple ID + app password) - see notarize.sh
NOTARIZED=0
if [ -n "${MAC_INST_SIGN_ID:-}" ] && { [ -n "${NOTARY_KEY_PATH:-}" ] || [ -n "${NOTARY_APPLE_ID:-}" ]; }; then
    notarize "$PKG"
    NOTARIZED=1
fi
if [ "$NOTARIZED" = 1 ]; then
    xcrun stapler staple "$PKG"
    xcrun stapler validate "$PKG" || true
    # the bundles inside were notarized with the pkg: staple their tickets too (used by the plugin-only zip)
    find "$STAGE" -maxdepth 6 \( -name "*.vst3" -o -name "*.component" -o -name "*.app" \) -type d -prune -print0 2>/dev/null \
        | xargs -0 -I{} xcrun stapler staple "{}" || true
    spctl -a -vvv -t install "$PKG" || true
fi
echo "NOTARIZED=$NOTARIZED"
SIZE=$(stat -f%z "$PKG" 2>/dev/null || stat -c%s "$PKG")
echo "Created $PKG ($SIZE bytes)"
