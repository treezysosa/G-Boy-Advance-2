#!/usr/bin/env bash
set -euo pipefail
gba_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$gba_root"
gba_builddir="${1:-build-macos}"
gba_artifacts="$gba_builddir/GBoyAdvance2_artefacts/Release"
gba_stage="$gba_builddir/package-root"
gba_version="$(python3 -c 'import re,pathlib; print(re.search(r"project\(GBoyAdvance2 VERSION ([0-9.]+)",pathlib.Path("CMakeLists.txt").read_text())[1])')"
mkdir -p dist "$gba_stage/Library/Audio/Plug-Ins/VST3" "$gba_stage/Library/Audio/Plug-Ins/Components" "$gba_stage/Applications"
for gba_item in 'VST3/G Boy Advance 2.vst3' 'AU/G Boy Advance 2.component' 'Standalone/G Boy Advance 2.app'; do
  [[ -d "$gba_artifacts/$gba_item" ]] || { echo "Missing built bundle: $gba_item" >&2; exit 1; }
  codesign --force --deep --sign - --timestamp=none "$gba_artifacts/$gba_item"
  codesign --verify --deep --strict "$gba_artifacts/$gba_item"
done
ditto "$gba_artifacts/VST3/G Boy Advance 2.vst3" "$gba_stage/Library/Audio/Plug-Ins/VST3/G Boy Advance 2.vst3"
ditto "$gba_artifacts/AU/G Boy Advance 2.component" "$gba_stage/Library/Audio/Plug-Ins/Components/G Boy Advance 2.component"
ditto "$gba_artifacts/Standalone/G Boy Advance 2.app" "$gba_stage/Applications/G Boy Advance 2.app"
gba_support="$gba_stage/Library/Application Support/Treezy Audio/G Boy Advance 2"
mkdir -p "$gba_support/Third-party notices"
cp docs/USER_MANUAL.md docs/PLATFORMS_0_20_0.md docs/LICENSING.md "$gba_support/"
ditto docs/licenses "$gba_support/Third-party notices"
pkgbuild --root "$gba_stage" --identifier audio.treezy.gboyadvance2 --version "$gba_version" --install-location / "dist/G-Boy-Advance-2-macOS-Universal.pkg"
# ditto retains bundle symlinks and executable permissions in the ZIP.
ditto -c -k --sequesterRsrc --keepParent "$gba_stage" "dist/G-Boy-Advance-2-macOS-Universal-Portable.zip"
echo "Prepared universal Mac VST3, AU and standalone. Ad-hoc signed; no Developer ID notarization is claimed."
