# 0.19.0 — Ten two-tone skins and CPU optimization

This source candidate adds ten distinct two-tone skins, bringing the total to 40. All previous persistent skin IDs are retained. Onyx & Gold, Neon Circuit, Polar Cobalt, Sakura Peach, Desert Sand, Aurora, Ruby Noir, Espresso Cream, Glacier Night and Lilac Mint are available in SELECT → SKIN and random launch colors.

## DSP changes

A 4,096-cell, double-precision, fifth-degree polynomial cache represents the existing additive harmonic curve. It retains all partials and normalization. The original recurrence handles Nyquist-truncated notes and brightness automation. Cache construction after parameter changes uses a fixed per-frame budget; preparation can build it before playback. No heap allocation, locks or I/O are introduced in render. Added per-instance cache storage is 224 KiB.

Drive bypass keeps the original 16-frame dry delay. After 64 exact-zero drive frames it sleeps the inaudible FIR processing; on reactivation, bounded history replay restores both filters before the current frame. Full 2x oversampling remains active for driven sound. Five fixed effects modulation increments are calculated once per sample-rate preparation. Three independent oscillator phase divisions share one SSE2 instruction with the original float rounding; a portable scalar path remains. Bypass history is recorded only while drive is off, avoiding extra history writes on driven presets.

No voice caps, harmonic reductions, shortened envelopes/tails, changed presets, lower modulation rates or reduced oversampling are used to obtain the measurements. The baseline is immutable 0.18.3 source 0712bc83bfcad19cac9a1e35a76732078937e58e; identical current factory arrays, drivers, compiler flags, notes, automation, sample rates and blocks are used in each comparison.

## Compatibility and build

654 factory presets /20 categories, 754 host slots including 100 reserved, and all 64 stable audio parameter IDs are retained. User presets, SoundFonts, project restoration and the removed-category state fixtures remain covered. Factory calibration gains remain unchanged; the recipe fingerprint records the updated implementation only after numerical comparisons and rendered level checks.

Build with Visual Studio 2022 x64 and CMake 3.24+: `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1`. VST3: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone: the adjacent `Standalone` directory. Installer: `dist/G-Boy-Advance-2-Setup-x64.exe`. Close all hosts before upgrading. The installer puts the complete VST3 bundle in `C:\Program Files\Common Files\VST3\`.

User presets: `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`. SoundFont library: `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library` or your selected custom folder. Installation preserves user data.

## Validation and performance

Windows compilation, 40 main/40 advanced screenshots, project-state roundtrips, DSP tests, expanded 846-scene short render comparison, 148 sustained comparisons, paired pad/string/SF2 CPU benchmarks, actual bundle loading, pluginval, Steinberg and installer lifecycle checks are pending. Existing audio bounds remain maximum error 1e-6 and at least 110 dB signal-to-error.

CPU time reduction and reciprocal throughput increase will be reported separately. A 50% throughput increase means one-third less CPU time; it does not mean CPU time was halved. Neither metric predicts an exact FL Studio meter value on every machine. Real FL Studio, physical audio and human listening remain owner checks. Prepared binaries are unsigned; distribution licensing remains deferred by the owner.
