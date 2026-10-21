# G BOY ADVANCE 2 — 0.18.0 verified Windows build

## Changes

- 24 Percussion presets across kicks, snares/claps, hats/cymbals, toms/hand drums and wood/metal hits.
- 20 Woodwinds across clarinets, oboes, bassoons, recorders and flute/pipe families.
- 24 Hip Hop Milwaukee presets: four each pianos, bells, plucks and basses, plus two each leads, brass stabs, vox and pads.
- 12 short synthetic Vox patches merged into Vocals (86 total). Deleted vocal categories remain removed.
- 10 additional skins: Amethyst, Seafoam, Sunset, Ice Blue, Wine Red, Olive, Pearl, Bronze, Bubblegum and Smoke. All 30 retain independent instance/project state and optional nonrepeat launch colors.
- 678 active presets across 21 categories; 754 stable host slots including 76 reserved. All 598 previous parameter dictionaries and 674 existing registry entries are identical to 0.17.1.
- SSE2 double-precision interleaved additive recurrences, the original wavetable interpolation, stable blocks for all eight engines and a float-accurate output Pade soft limiter; SoundFont channel setup is skipped when unchanged. No reduction in voice limits, harmonic counts, modulation rate, oversampling or tail length. Reported latency remains 16 frames.

## Verified downloads

[Windows installer](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39541516550/artifacts/12319808620) · [Portable/manual archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39541516550/artifacts/12319554225) · [30-skin/browser previews](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39533676959/artifacts/12317298350) · [Source branch](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/expansion-performance-0180)

## Validation

[Runtime build 39533676959](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39533676959) passed nine Ubuntu suites and twelve Windows suites, both plugin validators and the strict CPU/audio comparison. Its final installer check correctly rejected a stale hardcoded installer version. [Packaging run 39541516550](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39541516550) reused the exact hash-checked runtime binaries after the installer version was derived from CMake. The common DSP/preset suites contain **633,063 checks**. Windows application checks: **3,996** (library 578, SF2 164); native bundle hosts: **647/648**; pluginval strictness 10 passed; Steinberg **47 passed / 0 failed**; six corrected-packaging installer lifecycle checks passed, including the 0.17.1 → 0.18.0 upgrade. Local application checks: 3,984; ASan/UBSan engine, advanced, effects and SF2 suites also passed. Leak detection is disabled under the execution tracer.

Every one of the **678 presets** was checked for audio, release, velocity and register response. The additional dry/wet audit rendered **8,136 scenes** and compared **229,503 pairs** at MIDI 48/60/72 and velocities 0.4/0.9. Result: **0 duplicate pairs, 0 close-pair review flags**; minimum dry distance 0.06010415. Renamed/gain/effects-only negative controls passed. Objective screening does not certify human-perceived uniqueness or acoustic realism.

All **30 skins** passed selection, unique color, project restoration, isolated-instance, nonrepeat launch and main/advanced screenshot checks. The four new browser destinations and their family/tag filters passed. All 598 previous parameter dictionaries and 674 previous registered slot identities are retained exactly. The 80 added slots bring the host total to 754, including 76 reserved.

## Numerical audio comparison

The same compiler and identical current factory arrays/drivers were used with immutable 0.17.1 baseline `f079323677e6024be8356a6afdfa333eb9db4558` and build `833aff0d45259db03a6ae1c49edbb55f2f143cf6`. The 798-scene probe contains 12,263,424 finite floats: maximum absolute difference 5.960464478e-08, signal/error 159.39 dB. The additional 148 eight-second pad/string/SF2 scenes contain 147,532,832 finite floats: maximum difference 5.960464478e-08, signal/error 157.23 dB. Both satisfy the pre-existing 1e-6 / 110 dB tolerance; neither is claimed bit-identical. Latency remains 16 samples.

An upstream limiter approximation was rejected because it changed bitcrusher rounding boundaries. Drive/distortion retain the original libm curve; the approximation is used only for final output limiting. Isolated MSVC probes also rejected direct wavetable blends: retaining the original std::lerp rounding restored the 1e-6 / 110 dB bounds. The faster interpolation was removed. A new Milwaukee pluck was redesigned after the single-register screen found it close to a Detroit pluck. The final full bank passes both screens. Existing patch IDs/settings remain unchanged.

## Measured performance

Matched Windows measurements: 672 native/SF2 pad/string held/overlap workloads, 44.1/48/96 kHz, 64/256-frame callbacks, 4/8/16/32 voices; measured twice per version in before/after/after/before order. Setup and warmup are excluded. The separate 612-scene ordinary-note/chord/retrigger and 66-scene stress profiles are included in the artifact.

| Voices | Native pad/string CPU time reduction | SF2 fixture CPU time reduction |
|---:|---:|---:|
| 4 | 5.34% | 1.68% |
| 8 | 6.06% | 1.67% |
| 16 | 6.68% | 1.40% |
| 32 | 7.21% | 0.91% |

The requested 50% CPU-time reduction was **not achieved** in these matched workloads. Both CPU time and reciprocal throughput are recorded in `cpu-comparison.json`; neither is a promised FL Studio CPU percentage. The optimizations retain voice limits, harmonic counts, envelopes, full-rate modulation, oversampling and tail lengths.

Local GCC 14.2 matched measurements showed 6.28–9.69% native pad/string CPU time reduction and 0.18–9.04% SF2 fixture CPU time reduction. These local numbers are separate observations, not substitutions for the Windows table. Arbitrary large external SoundFonts, audio drivers and machines need their own measurements.

## Build and installation

Source branch: `feature/expansion-performance-0180`. Build with `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1` using Visual Studio 2022 x64 and CMake 3.24+. The VST3 output is `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone is alongside under `Standalone`.

Close FL Studio and standalone, extract the verified installer archive into a fresh folder, then run `G-Boy-Advance-2-Setup-x64.exe`. The installer places the full VST3 bundle in `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. Manual installation copies that entire bundle. User presets remain under `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`; the default SF2 folder is `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library`.

Prepared binaries are unsigned. The owner deferred the licensing decision; this work does not finalize public distribution terms. Real FL Studio/other named DAWs, hardware audio/MIDI, physical DPI and human listening remain owner acceptance steps. The synthetic FL-named host is not FL Studio.
