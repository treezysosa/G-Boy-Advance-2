# 0.19.0 — Ten two-tone skins and CPU optimization

This build adds ten distinct two-tone skins, bringing the total to 40. All previous persistent skin IDs are retained. Onyx & Gold, Neon Circuit, Polar Cobalt, Sakura Peach, Desert Sand, Aurora, Ruby Noir, Espresso Cream, Glacier Night and Lilac Mint are available in SELECT → SKIN and random launch colors.

## DSP changes

A 4,096-cell, double-precision, fifth-degree polynomial cache represents the existing additive harmonic curve. It retains all partials and normalization. The original recurrence handles Nyquist-truncated notes and brightness automation. Cache construction after parameter changes uses a fixed per-frame budget; preparation can build it before playback. No heap allocation, locks or I/O are introduced in render. Added per-instance cache storage is 224 KiB.

Drive bypass keeps the original 16-frame dry delay. After 64 exact-zero drive frames it sleeps the inaudible FIR processing; on reactivation, bounded history replay restores both filters before the current frame. Full 2x oversampling remains active for driven sound. Five fixed effects modulation increments are calculated once per sample-rate preparation. Three independent oscillator phase divisions share one SSE2 instruction with the original float rounding; a portable scalar path remains. Bypass history is recorded only while drive is off, avoiding extra history writes on driven presets.

No voice caps, harmonic reductions, shortened envelopes/tails, changed presets, lower modulation rates or reduced oversampling are used to obtain the measurements. The baseline is immutable 0.18.3 source 0712bc83bfcad19cac9a1e35a76732078937e58e; identical current factory arrays, drivers, compiler flags, notes, automation, sample rates and blocks are used in each comparison.

## Compatibility and build

654 factory presets /20 categories, 754 host slots including 100 reserved, and all 64 stable audio parameter IDs are retained. User presets, SoundFonts, project restoration and the removed-category state fixtures remain covered. Factory calibration gains remain unchanged; the recipe fingerprint records the updated implementation only after numerical comparisons and rendered level checks.

Build with Visual Studio 2022 x64 and CMake 3.24+: `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1`. VST3: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone: the adjacent `Standalone` directory. Installer: `dist/G-Boy-Advance-2-Setup-x64.exe`. Close all hosts before upgrading. The installer puts the complete VST3 bundle in `C:\Program Files\Common Files\VST3\`.

User presets: `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`. SoundFont library: `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library` or your selected custom folder. Installation preserves user data.

## Validation and performance

The latest source passed all nine independent Linux DSP/preset/SoundFont suites. The Linux 846-scene comparison was bit-identical across 14,327,808 floats. The 148 sustained comparisons covered 147,532,832 floats, with maximum error 4.470348358154297e-8 and 170.35 dB signal-to-error. The independent effects-history probe also matched exactly across 196,608 stereo frames. Existing audio bounds remain maximum error 1e-6 and at least 110 dB signal-to-error. Windows runtime validation [43068557417](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43068557417) passed all 12 suites, including 5,011 application checks and 887/888 actual bundle-host checks. Pluginval strictness 10 and the official Steinberg validator (47 passed /0 failed) passed. All 40 main and 40 advanced skin renders were generated. The Windows 846-scene render comparison was bit-identical; the 148 sustained comparisons had maximum error 4.470348358154297e-8 and 169.05 dB signal-to-error. [Final release validation 43076162724](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43076162724) passed all six installer lifecycle checks, including the actual 0.18.3 upgrade, reinstall, installed diagnostics, standalone launch and uninstall preserving user data. The upgrade test-only previous-version expectation was corrected to match the pinned 0.18.3 installer. Production source/binaries remained unchanged. The independent history probe was bit-identical under MSVC too.

CPU time reduction and reciprocal throughput increase are reported separately. A 50% throughput increase means one-third less CPU time; it does not mean CPU time was halved. Neither metric predicts an exact FL Studio meter value on every machine. Real FL Studio, physical audio and human listening remain owner checks. Prepared binaries are unsigned; distribution licensing remains deferred by the owner.

## Further performance work

The 50% target was not achieved in the matched Windows measurements. Next steps are to profile the remaining per-voice modulation/filter work, prototype SIMD processing across voices while retaining note/automation timing, and benchmark representative user-owned SoundFonts separately. Keep the current audio error limits and full voice/partial counts in every comparison; do not present reduced-polyphony settings as an equivalent engine speedup.

An independent effects-history probe is available as `python scripts/check-drive-bypass-reference.py PATH_TO_EXPORTED_0_18_3 --compiler g++` (or `--compiler cl` in an x64 VS developer shell). It compares 196,608 stereo frames across three rates and eight bypass durations against the old continuously warmed filters; the reference source must be an export of commit `0712bc83bfcad19cac9a1e35a76732078937e58e`. The Linux run was bit-identical.

### Preliminary Linux measurements

GCC 14.2, C++20, Release `-O3`, matching compiler flags, two ABBA-ordered runs per engine, 672 cases per run. Initialization/warm-up are excluded on both sides. Cases include 12 native pad/string patches and two original mathematical SF2 fixtures, three sample rates, 64/256-frame blocks, held chords and overlapping releases. These shared-runner results are observations, not guaranteed changes to FL Studio's CPU meter.

| Voices | Native CPU time reduction | Native throughput increase | SF2 CPU time reduction | SF2 throughput increase |
|---:|---:|---:|---:|---:|
| 4 | 6.42% | 6.86% | -0.40% | -0.40% |
| 8 | 8.52% | 9.31% | 4.56% | 4.78% |
| 16 | 9.73% | 10.78% | 1.50% | 1.53% |
| 32 | 10.71% | 12.00% | 2.02% | 2.07% |

The 50% target was not achieved in these Linux workloads. Small negative measurements indicate a measured regression in that group; SF2 changes were modest and variable. Final Windows numbers are reported separately.

### Windows measurements

MSVC /Visual Studio 2022 x64 Release, identical driver/compiler flags and the same ABBA ordering/workload definitions as above. The runtime binaries were compiled at `d215baddfbc1f168205c796953e4ee3a64a8815f`. Source and preset checks guard final repackaging against production changes.

| Voices | Native CPU time reduction | Native throughput increase | SF2 CPU time reduction | SF2 throughput increase |
|---:|---:|---:|---:|---:|
| 4 | 6.27% | 6.69% | -0.48% | -0.48% |
| 8 | 8.40% | 9.17% | -0.70% | -0.69% |
| 16 | 9.23% | 10.17% | -0.69% | -0.69% |
| 32 | 9.93% | 11.02% | -0.33% | -0.33% |

The native pad/string groups used 6.27–9.93% less measured CPU time (6.69–11.02% more throughput). The two SF2 fixtures were 0.33–0.70% slower; this build does not establish an SF2 speedup. No callbacks exceeded the measured budget in these paired pad/string/SF2 groups. These observations do not certify real driver deadlines or older hardware. **Neither a 50% CPU time reduction nor a 50% throughput increase was achieved.** Voice counts and sound settings were not reduced to manufacture a gain.

## Verified downloads and provenance

[Windows installer](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43076162724/artifacts/13882352121) · [Portable/manual installation](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43076162724/artifacts/13882986703) · [Complete Windows, source and validation bundle](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43076162724/artifacts/13882882046) · [All native skin renders](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43076162724/artifacts/13882147253).

The source ZIP contains 160 complete source/assets at validation commit `182987dd587e759791ce8321e00253ff3535c36c`. The final repository README/release notes add these verified links and completed status; production Source, Presets and CMakeLists.txt are identical to compiled commit `d215baddfbc1f168205c796953e4ee3a64a8815f`. The complete download, source/portable ZIP CRCs, all five payload SHA-256 values, native x64 headers, 80 distinct-within-view skin renders and unchanged factory dictionaries/host identities were independently checked.

| Payload | SHA-256 |
|---|---|
| `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3/Contents/x86_64-win/G Boy Advance 2.vst3` | `da2a72bc855589b6a7c65e52218faf2285c0ab1db07abde1e82401430cb2fac7` |
| `build/GBoyAdvance2_artefacts/Release/Standalone/G Boy Advance 2.exe` | `c356c50331d833061d2bc0193825177c85075613139363f55a161c64d0013bb3` |
| `dist/G-Boy-Advance-2-Setup-x64.exe` | `db5a2c8eb2792e7cec53ea6f43829cda1fd81a2653f40c5b25b4c5d7c0c34268` |
| `dist/G-Boy-Advance-2-Windows-x64.zip` | `abe49502a29dc4be3df6f48db070ffb127021c9c66da93f6d24f14b04c653a74` |
| `dist/G-Boy-Advance-2-Source.zip` | `96c7cff5e39b90f14c15d5ada235f0d719a8fcfb9a7df94b623531e1e8ae14cd` |
