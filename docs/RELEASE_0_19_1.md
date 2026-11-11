# 0.19.1 — Pad/string and SoundFont CPU optimization

This build targets sustained chords and overlapping release tails. It retains 654 factory presets in 20 categories, 754 host slots /100 reserved slots, 64 stable audio parameter IDs, 40 skins, original voice limits, oscillator partials, modulation rates, envelopes, effects and 16-frame latency. User presets, SF2 drag/drop/default folders and project state remain compatible.

## Implementation

Native modulation routing, tremolo and syllabic gains share exact calculations between voices with the same modulation reset origin. A per-sub-block/sample generation prevents stale reuse during automation. Every voice still advances its own clocks. Velocity gains update when the parameter or a note's velocity changes.

EQ shelves enter the exact identity path only when their coefficients form an identity filter and both internal state values are zero. Residual state continues through the original IIR calculations. Saturation-only processing uses the same CRT tanh, 2x FIR oversampling, division, latency and wet/dry interpolation while avoiding the zero-weight hard-clip blend.

The pinned SoundFont decoder and its wrapper remain unchanged. No approximate math or quality mode is added.

## Validation and measurements

Baseline: immutable 0.19.0 commit `4d6011fd971e1e0e73a8aeab2e6195811baf4896`. Identical current test drivers, factory arrays, compiler flags, rates, blocks, notes, automation and voice limits are used on both sides. CPU comparisons use before/after/after/before ordering, with initialization and warm-up excluded on both sides.

Linux comparisons were bit-identical across 846 short scenes /14,327,808 floats, 148 sustained scenes /147,532,832 floats, and 16 layered SoundFont scenes /11,332,824 floats. Sustained scenarios now include live velocity, tremolo and syllabic changes. The independent direct-effects probe matched 196,608 stereo frames exactly, including EQ returning to flat with residual state. Local core synthesis, advanced synthesis, effects and engine suites passed. [Windows runtime validation 43087833818](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43087833818) passed all 12 suites, 5,011 application checks, 887/888 actual bundle-host checks, pluginval strictness 10 and the official Steinberg validator (47 passed /0 failed). All six installer lifecycle checks passed, including the actual 0.19.0 upgrade, reinstall, installed diagnostics, standalone launch and uninstall preserving user data. All three Windows render comparisons were bit-identical too, across 173,193,464 floats in total. The direct effects/EQ-history comparison also matched exactly.

The new original SF2 stress bank has four layers per key, two programs and static/modulated variants with bank-native filter, pitch, gain, envelopes and LFOs. Reference renders cover four rates including 48000.5 Hz, irregular blocks, sustain, bend, tuning, glide, preview, stealing, release overlap, effects and program changes. The separate 256-case CPU benchmark uses identical 4/8/16/32 voice budgets, 64/256-frame blocks, held chords and overlapping releases. Generated banks are not installed.

These are controlled fixture measurements, not real user bank or older-PC results. Neither a 50% CPU time reduction nor a 50% throughput gain was achieved in these workloads. No lower voice counts or quality modes are used to manufacture gains. Actual FL Studio and physical audio/MIDI checks remain owner acceptance; licensing/signing remain deferred as requested.

## Build and installation

Source branch: `feature/pad-sf2-performance-0191`. Build with Visual Studio 2022 x64 /CMake 3.24+: `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1`. VST3 output: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone is adjacent under `Standalone`.

The installer puts the complete VST3 bundle in `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. Close FL Studio and standalone before upgrading. User presets stay in `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`; the default SF2 library stays in `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library` or the custom folder you selected.

## Preliminary Linux CPU results

GCC 14.2 /C++20 /`-O3`, shared runner. Measurements are observational and cannot guarantee exact DAW meter changes.

| Voices | Native pad/string CPU reduction | Simple SF2 CPU reduction | Layered static SF2 reduction | Layered modulated SF2 reduction |
|---:|---:|---:|---:|---:|
| 4 | 4.97% | 10.21% | 13.71% | 10.62% |
| 8 | 4.80% | 9.72% | 9.05% | 8.65% |
| 16 | 5.01% | 9.42% | 8.89% | 6.26% |
| 32 | 3.68% | 5.80% | 4.62% | 7.91% |

## Windows CPU results

MSVC /Visual Studio 2022 x64 Release. These are additional changes relative to 0.19.0, not totals against an older release. All voice limits and sound settings are matched.

| Voices | Native pad/string CPU reduction | Simple SF2 CPU reduction | Layered static SF2 reduction | Layered modulated SF2 reduction |
|---:|---:|---:|---:|---:|
| 4 | 6.37% | 9.62% | 3.47% | 4.81% |
| 8 | 5.42% | 8.92% | 4.63% | 4.71% |
| 16 | 4.89% | 9.89% | 3.85% | 1.88% |
| 32 | 4.77% | 7.65% | 2.69% | 2.15% |

CPU time reduction and reciprocal throughput are both in `cpu-comparison.json`. Timings on one shared runner are observations; they do not establish a fixed FL Studio meter improvement, physical-driver deadlines or real user bank performance. More complex SoundFonts spend more time in the unchanged per-voice decoder/filter path and benefited less in these fixtures.

| Native workload | 4 voices | 8 voices | 16 voices | 32 voices |
|---|---:|---:|---:|---:|
| held-chord | 9.48% | 10.03% | 10.70% | 11.30% |
| release-overlap | 3.75% | 1.86% | 0.64% | 0.25% |

Held chords use simultaneous note starts. Release-overlap cases stagger notes by 32 frames and mix releases with held notes; their gains are smaller. Neither table guarantees results for every playing style or PC. Final packaging/source verification is pending; production binaries are frozen at `3ac7cb29e939ed1a7e2039586038160a304e1518`.
