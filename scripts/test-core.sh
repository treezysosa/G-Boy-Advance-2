#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build-core
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -ISource Source/dsp/SynthEngine.cpp Tests/EngineTests.cpp Tests/AllocationTracker.cpp -o build-core/gba2-tests
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -ISource Source/dsp/SynthEngine.cpp Tests/CoreSynthesisTests.cpp Tests/AllocationTracker.cpp -o build-core/gba2-core-tests
cd build-core
./gba2-tests "$@"
./gba2-core-tests
