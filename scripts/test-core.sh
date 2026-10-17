#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build-core
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -ISource Source/dsp/SynthEngine.cpp Source/dsp/Effects.cpp Tests/EngineTests.cpp Tests/AllocationTracker.cpp -o build-core/gba2-tests
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -ISource Source/dsp/SynthEngine.cpp Source/dsp/Effects.cpp Tests/CoreSynthesisTests.cpp Tests/AllocationTracker.cpp -o build-core/gba2-core-tests
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -ISource Source/dsp/SynthEngine.cpp Source/dsp/Effects.cpp Tests/AdvancedSynthesisTests.cpp Tests/AllocationTracker.cpp -o build-core/gba2-advanced-tests
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -ISource Source/dsp/SynthEngine.cpp Source/dsp/Effects.cpp Tests/EffectsTests.cpp Tests/AllocationTracker.cpp -o build-core/gba2-effects-tests
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -ISource Source/dsp/SynthEngine.cpp Source/dsp/Effects.cpp Tests/PresetTests.cpp -o build-core/gba2-preset-tests
cd build-core
./gba2-tests "$@"
./gba2-core-tests
./gba2-advanced-tests
./gba2-effects-tests
./gba2-preset-tests --auditions
