# 0.19.3 — Live synthesis CPU optimization

This update targets built-in pad/string chords and ROM Hacks SoundFont processing. All 654 factory sounds, 20 categories, 50 skins, 754 host slots including 100 reserved slots, 64 parameter IDs and 16-frame latency remain. No lower voice limit, shorter release, fewer harmonics, lower modulation rate or reduced oversampling is used to obtain a measurement.

## Processing changes

Oscillator phase wrapping uses the exact known floor in its common positive intervals; signed zero, negative phases, large FM steps and non-finite values retain the original path. Wavetable interpolation reuses its existing nonnegative integer address for the fractional position. A two-million-input reference sweep and boundary tests compare phase wrapping against the original floor expression.

The saturation up/downsampling filters now process left and right together. SSE2 on Windows x64 pairs the channels while retaining each channel's original tap order and float arithmetic; a scalar fallback remains. The complete 33-tap half-band filters, 2x nonlinear processing, libm tanh, 16-frame delay, bypass history and exact wake-up behavior remain. Stereo history has the same total sample storage. This shared effects change also applies to SoundFonts. No third-party decoder code is changed.

The factory-level recipe fingerprint is refreshed for the changed DSP source. Factory volume values and the generated JSON/C++ parameter arrays remain unchanged. User presets, DAW parameters, SF2 libraries, appearance state and the offline listening-feedback bank hash retain their meaning. This is a performance update, not the still-pending human preset listening pass.

## Validation status

The direct effects reference matches all 196,608 stereo frames exactly, including bypass/re-enable intervals, EQ history and three sample rates. Initial matched Linux profiling observed lower render time; it is not a prediction of FL Studio's CPU meter. The local 846-scene native probe, 148 sustained pad/string/SF2 scenes and sixteen layered-bank scenes also match bit-for-bit: 173,193,464 floating-point samples total, including the complete factory bank, irregular blocks, live controls and 44.1/48/96 kHz plus fractional SF2 rates. Final core-suite completion and Windows CPU/audio comparisons are pending.

The Windows pipeline builds VST3 and standalone with Visual Studio 2022 x64, runs all suites, validates the actual bundle with pluginval strictness 10 and the official Steinberg validator, then compares the current engine with immutable 0.19.2 source `f62e8e02d33bb75013ec1daa00584baf4368ff6c`. Baseline/current drivers and settings match, and repeated CPU measurements alternate order. Numerical tolerances remain maximum error 1e-6 and signal-to-error above 110 dB when outputs differ. Direct effect history is required to remain bit-identical.

The installer must pass the real 0.19.2-to-0.19.3 upgrade, same-version reinstall, installed diagnostics, standalone launch and uninstall tests, preserving user presets, SF2 files/cache and launch/library preferences. Source/portable ZIP contents and native AMD64 payloads must be checked before delivery.

## Building and installing

Source branch: `feature/pad-string-performance-0193`. With Visual Studio 2022 C++ tools, Windows SDK and CMake 3.24+: `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1`. Source includes the existing optional offline listening tools; they are not installed or linked into the plugin.

Outputs: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3` and the adjacent `Standalone/G Boy Advance 2.exe`. Close FL Studio and standalone, extract the installer ZIP and run `G-Boy-Advance-2-Setup-x64.exe`. Default plugin location: `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. Use FL Studio x64 and rescan the VST3 if needed.

User presets stay in `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`; the default SoundFont folder remains `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library` or the selected custom directory. Signing and licensing decisions remain deferred. Actual FL Studio/device behavior and subjective listening remain owner acceptance. No fixed CPU percentage, universal 50% gain or physical audio deadline is promised.
