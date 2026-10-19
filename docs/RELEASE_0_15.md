# 0.15.0 — skins, preset diversity and CPU work

Source branch: `feature/skins-diversity-cpu-0150`, based on immutable 0.14.1 source `f19b9fafd80cf54aa1a6b264a4ccf2a5af2f3a84`. Validated application/build commit: `36b2ed0e9903882f93072ae2e4140e1570bbbf1a`. [Passing Windows/Ubuntu run](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39254648988). Download the [Windows installer ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39254648988/artifacts/12187219055) or [portable archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39254648988/artifacts/12187453979). Owner licensing decision remains deferred; unsigned preparation only.

## Changes

Ten appended persistent skin IDs: arctic, atomic, graphite, mint, cobalt, copper, lavender, coral, forest, lemon. Twenty colours are available in SELECT and automatic launch selection. Existing ten IDs retain their order and appearance; fixed project colours and nonrepeating launch history remain compatible. Native main/advanced screenshots, selection/state roundtrips, per-instance isolation and separate-process launches are tested.

All 530 factory sounds receive an additional dry/wet, three-register/two-velocity numerical audit. Every unique pair is compared regardless of category. Gain-normalized spectra and amplitude/release envelopes prevent volume and effects alone from establishing distinct dry sounds. Negative controls cover renamed and gain/effects-only duplicates; a sine/saw control verifies a meaningful excitation difference. Initial screening found no <=0.025 duplicates but 26 <0.06 related pairs. Eighteen targeted factory revisions are documented in `Presets/diversity-refinements.json`; their master levels are recalibrated while retaining all other levels. The final bank clears both numerical screens; results are recorded below. No new patch IDs or host program positions are introduced. Saved project/user-preset values retain the previous sound independently of the factory recipes.

The 2x nonlinear chain retains both FIR histories, the original coefficients and 16-frame latency. Known-zero odd taps on the even upsampling phase need no multiply/add; the discarded odd decimator result needs only its history write. No filter or shaping stage is bypassed. Exact-input caches avoid repeated wavetable mip logarithms, plucked-string phase-delay trigonometry, shared chord pitch exponentials and per-voice choir normalization square roots. Automation, rate changes, glide and engine transitions invalidate by input comparison; no approximation or lowered voice count is used.

Bypassed crusher quantization is skipped: rate one leaves the counter at zero, so re-enabling captures a fresh value before reading it. Coincident echo heads use one read; delay feedback, writes and two-head time-change crossfades remain active. The numerical probe includes crusher depth/rate, drive and distortion automation as well as the earlier routing/engine/rate scenarios.

ROM Hacks loading, drag/drop, bank cache, folder preferences, startup recall/project precedence and SF2 playback remain supported. No banks or audio assets are bundled.

## Validation

[Windows/Ubuntu run 39254648988](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39254648988) passed nine Ubuntu DSP suites and twelve Windows CTest suites: 629,294 native DSP/preset checks, 1,225 Windows application checks (122 library / 164 SoundFont), twenty skin/state snapshots, two actual-bundle hosts, pluginval strictness 10, Steinberg 47 passed / 0 failed and six installer lifecycle checks. The synthetic FL-named host is not FL Studio.

The actual AMD64 VST3 and standalone, downloaded installer/portable artifacts, ZIP contents, version, nineteen pinned notices and factory payload are verified after download. The six installer checks include the pinned 0.14.1-to-0.15.0 upgrade, exact installed hashes, diagnostic report, reinstall, standalone process liveness and uninstall while preserving user presets, personal audio, SoundFont cache, library/preferences and launch history. Process liveness does not test a physical audio device.

All 530 presets pass central/register/velocity/release/finite checks. The additional all-pair screen renders 6,360 dry/wet fixtures at notes 48/60/72 and velocities 0.4/0.9, compares 140,185 pairs, and reports zero <=0.025 duplicate candidates and zero <0.06 review flags (minimum 0.060564409). The original central-note signature also retains 530 groups with zero <0.025 flags. See [factory report](FACTORY_LIBRARY.md) and the CSV/JSON artifacts. These are numerical screens, not human listening certification.

The 650-scene / 10,444,800-float Windows audio comparison is byte-identical, using immutable 0.14.1 DSP with identical current 530-patch arrays and the same expanded automation driver. Hashes: before `7b0e9b1029e2978b8a38d4e22a603b1ed8da85bf708b4928b33b98f95160a950`, after `7b0e9b1029e2978b8a38d4e22a603b1ed8da85bf708b4928b33b98f95160a950`. Local GCC's expanded comparison is byte-identical, SHA256 `3a7750974bd86d731c0c6a1b91bdb8f9cc2f506ecf3b64ee694c9923712370da`. The probe includes engine/filter/LFO/chorus/flanger/wow/EQ/glide/envelope changes, crusher depth/rate and drive/distortion automation, sustain, pitch bend, stealing, rate changes and all factory chords/releases.

Matched Windows note/chord/retrigger profiles measured 9.09%–13.85% less accumulated DSP time across one-, four- and eight-voice groups versus 0.14.1. These are native DSP measurements on a shared runner, not FL Studio CPU percentages or hardware deadline guarantees.

| Voices | Held notes/chords: less measured DSP time | Retriggers: less measured DSP time |
|---|---:|---:|
| 1 | 13.68% | 13.85% |
| 4 | 10.68% | 10.79% |
| 8 | 9.09% | 9.24% |

The separate stress profiler exercises all eight engines and three switching cases at 44.1/48/96 kHz, 64/256-frame blocks.

| Voices | All stress workloads: less measured DSP time | Over-budget callback observations | Maximum budget ratio |
|---|---:|---:|---:|
| 8 | 7.46% | 0 | 0.247500 |
| 32 | 6.80% | 0 | 0.852300 |

Local three-pass medians for the final effects implementation measured 3.30%–7.98% less playing DSP time. Own-source ASan/UBSan passed 3,674 DSP checks and 1,213 application checks; cached JUCE modules are uninstrumented and leak detection is disabled under the environment tracer. Final normal/sanitized application tests include SF2 library scans, process restart, drop/state behavior and twelve bank publications during concurrent audio.

All twenty skin IDs roundtrip through project state, remain isolated per instance and notify the host. Native main/advanced PNGs and three separate OS-process launch selections pass. Factory IDs/names/order (530), audio parameter IDs (64) and prepared latency (16 frames) remain stable. Only the eighteen documented factory voicings change; project/user-preset parameter values retain older sounds. No SoundFonts or recorded audio assets are bundled.

## Reproduce

Build with Visual Studio 2022 x64 and CMake as documented in README. `ctest --test-dir build -C Release --output-on-failure` includes `factory-diversity`. Its `factory-diversity.json`, `factory-diversity.csv` and `factory-diversity-review.csv` record thresholds and nearest-neighbor evidence. `scripts/compare-cpu-windows.ps1` builds immutable 0.14.1 DSP with the current arrays and identical drivers. Windows packaging and the pinned 0.14.1-to-0.15.0 installer lifecycle run in CI.

## Limits

Numerical comparisons do not prove that every listener perceives every related patch as different. Actual owner FL Studio 20.8.3, Explorer drag/drop, physical audio-driver deadlines, high-DPI interaction and human listening remain manual acceptance work. Shared-runner profiles are native DSP observations, not FL Studio CPU percentages or guarantees on the owner's computer.
