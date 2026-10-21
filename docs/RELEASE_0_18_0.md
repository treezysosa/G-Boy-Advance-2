# G BOY ADVANCE 2 — 0.18.0 release candidate

## Changes

- 24 Percussion presets across kicks, snares/claps, hats/cymbals, toms/hand drums and wood/metal hits.
- 20 Woodwinds across clarinets, oboes, bassoons, recorders and flute/pipe families.
- 24 Hip Hop Milwaukee presets: four each pianos, bells, plucks and basses, plus two each leads, brass stabs, vox and pads.
- 12 short synthetic Vox patches merged into Vocals (86 total). Deleted vocal categories remain removed.
- 10 additional skins: Amethyst, Seafoam, Sunset, Ice Blue, Wine Red, Olive, Pearl, Bronze, Bubblegum and Smoke. All 30 retain independent instance/project state and optional nonrepeat launch colors.
- 678 active presets across 21 categories; 754 stable host slots including 76 reserved. The original 598 dictionaries and 674 registry entries are byte-for-byte unchanged after parsing.
- SSE2 double-precision interleaved additive recurrences, simpler bounded wavetable blends, stable blocks for all eight engines and a float-accurate output Pade soft limiter; SoundFont channel setup is skipped when unchanged. No reduction in voice limits, harmonic counts, modulation rate, oversampling or tail length. Reported latency remains 16 frames.

## Validation status

Windows compilation, host validation, final rendered bank audit, matched CPU/audio comparison and 0.17.1 → 0.18.0 installer lifecycle are pending. Do not treat this candidate as a validated Windows release yet. The new installer link will be added after those checks pass.

The 50% performance request is a target. Both render-time reduction and equivalent throughput gain will be reported from matched workloads against immutable baseline `f079323677e6024be8356a6afdfa333eb9db4558` (0.17.1). Shared-runner native measurements do not predict FL Studio CPU percentages or arbitrary external SoundFonts. No 50% claim is made without evidence.

Automatic diversity screening compares all 678 sounds across MIDI notes 48/60/72 and velocities 0.4/0.9, both with and without effects. It removes gain/effects as disguises and checks every cross-category pair. This is objective spectral/envelope evidence; human listening acceptance remains pending.

## Build and installation

Source branch: `feature/expansion-performance-0180`. Build with `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1` using Visual Studio 2022 x64 and CMake 3.24+. The VST3 output is `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone is alongside under `Standalone`.

Close FL Studio and standalone, extract the verified installer archive into a fresh folder, then run `G-Boy-Advance-2-Setup-x64.exe`. The installer places the full VST3 bundle in `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. Manual installation copies that entire bundle. User presets remain under `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`; the default SF2 folder is `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library`.

Prepared binaries are unsigned. The owner deferred the licensing decision; this work does not finalize public distribution terms. Real FL Studio/other named DAWs, hardware audio/MIDI, physical DPI and human listening remain owner acceptance steps. The synthetic FL-named host is not FL Studio.
