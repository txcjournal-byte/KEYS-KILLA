#!/bin/bash
# Builds the macOS installer: VST3 + AU into /Library/Audio/Plug-Ins, Standalone into /Applications.
# Signs + notarizes when the signing identities / credentials are present in the environment:
#   MAC_APP_SIGN_ID    "Developer ID Application: ..."   (code signing)
#   MAC_INST_SIGN_ID   "Developer ID Installer: ..."     (pkg signing)
#   NOTARY_APPLE_ID, NOTARY_TEAM_ID, NOTARY_PASSWORD     (notarytool)
# Usage: installer/mac/build_pkg.sh <version>
set -euo pipefail
source "$(dirname "$0")/notarize.sh"
trap 'echo "::error title=$(basename "$0")::line $LINENO failed: $BASH_COMMAND"' ERR

VERSION="${1:-0.2.0}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ART="$ROOT/build/KeysKilla_artefacts/Release"
STAGE="$ROOT/build/pkgroot"
OUT="$ROOT/build/installer"

rm -rf "$STAGE" && mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Library/Audio/Plug-Ins/Components" "$STAGE/Applications" "$OUT"
cp -R "$ART/VST3/EVOLVE.vst3" "$STAGE/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/AU/EVOLVE.component" "$STAGE/Library/Audio/Plug-Ins/Components/"
cp -R "$ROOT/build/EvolveFX_artefacts/Release/VST3/EVOLVE FX PRO.vst3" "$STAGE/Library/Audio/Plug-Ins/VST3/" 2>/dev/null || true
cp -R "$ROOT/build/EvolveFX_artefacts/Release/AU/EVOLVE FX PRO.component" "$STAGE/Library/Audio/Plug-Ins/Components/" 2>/dev/null || true
cp -R "$ART/Standalone/EVOLVE.app" "$STAGE/Applications/"

if [ -n "${MAC_APP_SIGN_ID:-}" ]; then
    # every bundle in the pkg must be signed (with a secure timestamp) or notarization rejects the whole pkg
    find "$STAGE" -maxdepth 6 \( -name "*.vst3" -o -name "*.component" -o -name "*.app" \) -type d -prune -print0 \
        | while IFS= read -r -d '' b; do
            codesign --force --deep --options runtime --timestamp --sign "$MAC_APP_SIGN_ID" "$b"
            codesign --verify --strict --verbose=2 "$b"
        done
fi

pkgbuild --root "$STAGE" --identifier com.808killa.keyskilla --version "$VERSION" --install-location / "$OUT/keyskilla-component.pkg"

cat > "$OUT/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>EVOLVE by TrapVST $VERSION</title>
    <license file="EULA.txt"/>
    <options customize="never" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <choices-outline><line choice="default"/></choices-outline>
    <choice id="default"><pkg-ref id="com.808killa.keyskilla"/></choice>
    <pkg-ref id="com.808killa.keyskilla" version="$VERSION">keyskilla-component.pkg</pkg-ref>
</installer-gui-script>
XML

mkdir -p "$OUT/resources" && cp "$ROOT/installer/EULA.txt" "$OUT/resources/"
PKG="$OUT/KEYS-KILLA-$VERSION-macOS.pkg"
if [ -n "${MAC_INST_SIGN_ID:-}" ]; then
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" --sign "$MAC_INST_SIGN_ID" "$PKG"
else
    productbuild --distribution "$OUT/distribution.xml" --resources "$OUT/resources" --package-path "$OUT" "$PKG"
fi
rm -f "$OUT/keyskilla-component.pkg"

# notarization (App Store Connect API key from setup_signing.sh, or Apple ID + app password) - see notarize.sh
NOTARIZED=0
if [ -n "${MAC_INST_SIGN_ID:-}" ] && { [ -n "${NOTARY_KEY_PATH:-}" ] || [ -n "${NOTARY_APPLE_ID:-}" ]; }; then
    notarize "$PKG"
    NOTARIZED=1
fi
if [ "$NOTARIZED" = 1 ]; then
    xcrun stapler staple "$PKG"
fi
echo "Created $PKG"
