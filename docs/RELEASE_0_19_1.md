# 0.19.1 — Pad/string and SoundFont CPU optimization

This candidate targets sustained chords and overlapping release tails. It retains 654 factory presets in 20 categories, 754 host slots /100 reserved slots, 64 stable audio parameter IDs, 40 skins, original voice limits, oscillator partials, modulation rates, envelopes, effects and 16-frame latency. User presets, SF2 drag/drop/default folders and project state remain compatible.

## Implementation

Native modulation routing, tremolo and syllabic gains share exact calculations between voices with the same modulation reset origin. A per-sub-block/sample generation prevents stale reuse during automation. Every voice still advances its own clocks. Velocity gains update when the parameter or a note's velocity changes.

EQ shelves enter the exact identity path only when their coefficients form an identity filter and both internal state values are zero. Residual state continues through the original IIR calculations. Saturation-only processing uses the same CRT tanh, 2x FIR oversampling, division, latency and wet/dry interpolation while avoiding the zero-weight hard-clip blend.

The broader modulation-phase cache, approximate saturation/stereo-limiter trial and SoundFont math memoization trials were rejected after measurements showed regressions. They are absent from this build. The pinned SoundFont decoder and its wrapper remain unchanged.

## Validation and measurements

Baseline: immutable 0.19.0 commit `4d6011fd971e1e0e73a8aeab2e6195811baf4896`. Identical current test drivers, factory arrays, compiler flags, rates, blocks, notes, automation and voice limits are used on both sides. CPU comparisons use before/after/after/before ordering, with initialization and warm-up excluded on both sides.

Linux comparisons were bit-identical across 846 short scenes /14,327,808 floats, 148 sustained scenes /147,532,832 floats, and 16 layered SoundFont scenes /11,332,824 floats. Sustained scenarios now include live velocity, tremolo and syllabic changes. The independent direct-effects probe matched 196,608 stereo frames exactly, including EQ returning to flat with residual state. Local core synthesis, advanced synthesis, effects and engine suites passed. Windows build, host, CPU and installer validation is pending.

The new original SF2 stress bank has four layers per key, two programs and static/modulated variants with bank-native filter, pitch, gain, envelopes and LFOs. Reference renders cover four rates including 48000.5 Hz, irregular blocks, sustain, bend, tuning, glide, preview, stealing, release overlap, effects and program changes. The separate 256-case CPU benchmark uses identical 4/8/16/32 voice budgets, 64/256-frame blocks, held chords and overlapping releases. Generated banks are not installed.

These are controlled fixture measurements, not real user bank or older-PC results. The requested 50% performance target is not established by this candidate. No lower voice counts or quality modes are used to manufacture gains. Actual FL Studio and physical audio/MIDI checks remain owner acceptance; licensing/signing remain deferred as requested.

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
