#!/bin/bash
# Builds the macOS installer: VST3 + AU into /Library/Audio/Plug-Ins, Standalone into /Applications.
# Signs + notarizes when the signing identities / credentials are present in the environment:
#   MAC_APP_SIGN_ID    "Developer ID Application: ..."   (code signing)
#   MAC_INST_SIGN_ID   "Developer ID Installer: ..."     (pkg signing)
#   NOTARY_APPLE_ID, NOTARY_TEAM_ID, NOTARY_PASSWORD     (notarytool)
# Usage: installer/mac/build_pkg.sh <version>
set -euo pipefail
VERSION="${1:-0.2.0}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ART="$ROOT/build/KeysKilla_artefacts/Release"
STAGE="$ROOT/build/pkgroot"
OUT="$ROOT/build/installer"

rm -rf "$STAGE" && mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Library/Audio/Plug-Ins/Components" "$STAGE/Applications" "$OUT"
cp -R "$ART/VST3/EVOLVE.vst3" "$STAGE/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/AU/EVOLVE.component" "$STAGE/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/Standalone/EVOLVE.app" "$STAGE/Applications/"

if [ -n "${MAC_APP_SIGN_ID:-}" ]; then
    for b in "$STAGE/Library/Audio/Plug-Ins/VST3/EVOLVE.vst3" "$STAGE/Library/Audio/Plug-Ins/Components/EVOLVE.component" "$STAGE/Applications/EVOLVE.app"; do
        codesign --force --deep --options runtime --timestamp --sign "$MAC_APP_SIGN_ID" "$b"
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

if [ -n "${NOTARY_APPLE_ID:-}" ] && [ -n "${MAC_INST_SIGN_ID:-}" ]; then
    xcrun notarytool submit "$PKG" --apple-id "$NOTARY_APPLE_ID" --team-id "$NOTARY_TEAM_ID" --password "$NOTARY_PASSWORD" --wait
    xcrun stapler staple "$PKG"
fi
echo "Created $PKG"
