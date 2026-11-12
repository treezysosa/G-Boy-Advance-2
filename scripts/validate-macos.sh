#!/usr/bin/env bash
set -euo pipefail
gba_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$gba_root"
gba_work="${1:-build-macos}"
gba_arch="$(uname -m)"
gba_vst='/Library/Audio/Plug-Ins/VST3/G Boy Advance 2.vst3'
gba_au='/Library/Audio/Plug-Ins/Components/G Boy Advance 2.component'
mkdir -p "$gba_work/validation-logs" "$gba_work/validation-tools"
gba_tool="$gba_work/validation-tools/pluginval"
if [[ ! -d "$gba_tool/pluginval.app" ]]; then
  curl --fail --location --retry 3 'https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_macOS.zip' -o "$gba_work/validation-tools/pluginval.zip"
  ditto -x -k "$gba_work/validation-tools/pluginval.zip" "$gba_tool"
fi
gba_validator="$gba_tool/pluginval.app/Contents/MacOS/pluginval"
[[ -f "$gba_validator" ]] || { echo 'Official pluginval archive has an unexpected layout' >&2; exit 1; }
chmod +x "$gba_validator"
arch -"$gba_arch" "$gba_validator" --strictness-level 10 "$gba_vst" 2>&1 | tee "$gba_work/validation-logs/pluginval-$gba_arch.txt"
# Refresh the per-user AU registry on this isolated CI machine after installing.
killall AudioComponentRegistrar 2>/dev/null || true
arch -"$gba_arch" /usr/bin/auval -v aumu GbA2 Trzy 2>&1 | tee "$gba_work/validation-logs/auval-$gba_arch.txt"
grep -q 'AU VALIDATION SUCCEEDED' "$gba_work/validation-logs/auval-$gba_arch.txt"
codesign --verify --deep --strict "$gba_vst"
codesign --verify --deep --strict "$gba_au"
