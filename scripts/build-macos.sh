#!/usr/bin/env bash
set -euo pipefail
gba_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$gba_root"
gba_builddir="${1:-build-macos}"
gba_arguments=(-S . -B "$gba_builddir" -G Ninja -DCMAKE_BUILD_TYPE=Release
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
  -DBUILD_TESTING=ON -DGBA_BUILD_BENCHMARKS=OFF)
if [[ -d Dependencies/JUCE ]]; then gba_arguments+=(-DGBA_JUCE_PATH="$gba_root/Dependencies/JUCE"); fi
cmake "${gba_arguments[@]}"
cmake --build "$gba_builddir" --parallel 2
ctest --test-dir "$gba_builddir" --output-on-failure
