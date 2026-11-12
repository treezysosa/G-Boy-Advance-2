# 0.19.3 — Live synthesis CPU optimization

This update targets built-in pad/string chords and ROM Hacks SoundFont processing. All 654 factory sounds, 20 categories, 50 skins, 754 host slots including 100 reserved slots, 64 parameter IDs and 16-frame latency remain. No lower voice limit, shorter release, fewer harmonics, lower modulation rate or reduced oversampling is used to obtain a measurement.

## Processing changes

Oscillator phase wrapping uses the exact known floor in its common positive intervals; signed zero, negative phases, large FM steps and non-finite values retain the original path. Wavetable interpolation reuses its existing nonnegative integer address for the fractional position. A two-million-input reference sweep and boundary tests compare phase wrapping against the original floor expression.

The saturation up/downsampling filters now process left and right together. SSE2 on Windows x64 pairs the channels while retaining each channel's original tap order and float arithmetic; a scalar fallback remains. The complete 33-tap half-band filters, 2x nonlinear processing, libm tanh, 16-frame delay, bypass history and exact wake-up behavior remain. Stereo history has the same total sample storage. This shared effects change also applies to SoundFonts. No third-party decoder code is changed.

The factory-level recipe fingerprint is refreshed for the changed DSP source. Factory volume values and the generated JSON/C++ parameter arrays remain unchanged. User presets, DAW parameters, SF2 libraries, appearance state and the offline listening-feedback bank hash retain their meaning. This is a performance update, not the still-pending human preset listening pass.

## Measured Windows results

Visual Studio 2022 x64 Release, shared Windows runner, matched to 0.19.2. These are additional render-time reductions for identical settings; they are not FL Studio meter predictions.

| Voices | Native pads/strings | Simple SF2 | Layered static SF2 | Layered modulated SF2 |
|---:|---:|---:|---:|---:|
| 4 | 0.89% | 4.66% | 4.21% | 3.12% |
| 8 | 0.38% | 4.05% | 3.44% | 2.87% |
| 16 | 0.18% | 4.56% | 3.22% | 2.04% |
| 32 | -0.23% | 4.49% | 2.93% | 1.62% |

Negative reduction means a measured increase. Native pad/string results range from 0.89% less CPU to 0.23% more; they do not establish a useful improvement for that bank. Simple SF2 workloads measured 4.05–4.66% less CPU. Layered fixtures measured 1.62–4.21% less. Across the broader steady-engine profile, reductions were 4.73% at eight voices and 1.96% at 32. The earlier Linux native pad/string result of approximately 9–10% did not translate to these Windows workloads. The 50% target remains unmet. Further native voice-loop optimization and profiling on the owner’s machine are outstanding.

The final build keeps the validated exact phase/wavetable and stereo FIR changes. No unvalidated experimental clock-state changes are included.

## Validation

[Windows/core runtime run 43247498302](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43247498302) passed all nine independent core suites and twelve Windows suites, including 6,161 application checks and both actual-bundle hosts (887/888). Pluginval strictness 10 passed; the separate official Steinberg validator passed all 47 checks with zero failures.

Windows and Linux outputs are bit-identical across the 846-scene native probe, 148 eight-second sustained scenes and sixteen layered-bank scenes: 173,193,464 float samples each. The 196,608-stereo-frame direct effects probe is also bit-identical, including bypass/re-enable and EQ history. The scalar FIR fallback matched locally. ASan/UBSan passed all 165 effects checks; leak detection was disabled because the environment tracer does not support it.

The 654-preset all-pair audit still reports zero duplicates and zero close-pair flags at the existing numerical thresholds. This does not certify human listening. The recipe fingerprint alone changed; gains and all factory parameter arrays are unchanged.

All six Windows installer lifecycle checks passed: actual 0.19.2 installation, upgrade to 0.19.3, diagnostics, reinstall, standalone launch smoke test and uninstall, with user presets, SF2 cache/library and preferences preserved. The final packaging workflow reuses the identical validated binaries and checks the complete source archive, x64 payloads and installer again.

Runtime commit: `2293291be4241901915e0518bf89420e1591d596`. Baseline: `f62e8e02d33bb75013ec1daa00584baf4368ff6c`. Machine-readable evidence is `build/validation-logs/cpu-comparison.json` and `release-validation-0193.json` in the final Windows artifact.

## Building and installing

Source branch: `feature/pad-string-performance-0193`. With Visual Studio 2022 C++ tools, Windows SDK and CMake 3.24+: `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1`. Source includes the existing optional offline listening tools; they are not installed or linked into the plugin.

Outputs: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3` and the adjacent `Standalone/G Boy Advance 2.exe`. Close FL Studio and standalone, extract the installer ZIP and run `G-Boy-Advance-2-Setup-x64.exe`. Default plugin location: `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. Use FL Studio x64 and rescan the VST3 if needed.

User presets stay in `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`; the default SoundFont folder remains `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library` or the selected custom directory. Signing and licensing decisions remain deferred. Actual FL Studio/device behavior and subjective listening remain owner acceptance. No fixed CPU percentage, universal 50% gain or physical audio deadline is promised.
