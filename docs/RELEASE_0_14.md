# 0.14.0 — regional hip-hop bank, ten skins and audio CPU work

This build adds 120 original synthesized factory presets, 40 each in Hip-Hop - Detroit, Hip-Hop - West Coast and Hip-Hop - Florida. Total: 530 presets / 20 categories. Previous 410 IDs, metadata, host-program positions and parameter arrays are preserved. The new 60 authored designs and their alternate voicings are documented in Presets/hiphop-designs.json; factory-index.json freezes their appended identities.

New skins: Crimson Red, Emerald Green, Midnight Blue, Gold and Rose Pink. The original five IDs/indices are unchanged; all ten are per-instance project state, separate from audio parameters. SELECT chooses a skin. START browses/searches the built-in regional sounds. ROM Hacks and SF2 paths/limits continue as documented in RELEASE_0_13.md; no audio bank is bundled.

## CPU implementation

- Exact last-input caches share LFO sine values and filter/formant coefficient calculations between matching chord voices. Each voice keeps its own full-rate clocks and private signal histories; staggered notes remain independent.
- Fixed stack storage lists sounding voices once per render segment, retaining the previous summation order and handling releases within the segment.
- Zero-mix wow/chorus/flanger read heads skip unused reads/trigonometry. Their clocks and delay-buffer writes continue so automation can enable a warm effect.
- The default eight-voice limit and restrained effects apply only to new polyphonic patches. Previous patches and project parameters are unchanged; up to 32 voices, 64 stable parameter IDs and 16-sample latency remain available.

## Evidence before Windows packaging

The native 650-scene comparison rendered 10,444,800 float samples byte-identically against the 0.13.1 DSP with identical patch arrays, including staggered notes, filter and effect automation, engine changes, sustain/release and sample-rate reinitialization. Final arrays are rechecked before shipping. Three interleaved native stress profiles showed median processing-time reductions of 9.11% at eight voices and 17.01% at 32 voices. Three matched passes of 432 ordinary note/chord/retrigger scenes showed median reductions between 15.56% and 21.52% depending on workload. These are native measurements on the build machine, not FL Studio CPU percentages or physical audio-driver deadline guarantees.

The full native factory audit passed 6,218 checks: 530 distinct dry spectral/envelope groups, all 120 new regional voicings separated at the configured threshold, zero near-neighbor flags, playable levels/registers/releases and velocity response. This numerical measure is not a perceptual uniqueness certificate. Native application and own-source ASan/UBSan passed 539 checks; cached JUCE modules were uninstrumented and leak detection was disabled for the environment tracer. Twelve SoundFont publications completed during concurrent audio in normal and sanitizer runs. Previous 410 factory dictionaries are unchanged.

## Verified Windows build

[Windows/Ubuntu run 39196086290](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39196086290) passed eight Ubuntu DSP suites and eleven Windows CTest suites, including 622,854 native DSP/preset checks, 546 Windows application checks (121 library and 55 SoundFont checks), ten skin/state snapshots, two actual-bundle test hosts, pluginval strictness 10, Steinberg 47/0 and six installer lifecycle checks. The synthetic FL-named test host is not FL Studio.

Validated application/test/build commit: `865c5ecb50efe3ec582fb8eb3cba2c2b9746fedd`; final source changes after validation are documentation only. The Windows comparison rendered 650 scenes / 10,444,800 float samples byte-identical against immutable 0.13.1 DSP, with identical current factory arrays and event drivers. Matched Windows notes/chords/retrigger profiles measured processing-time reductions ranging from 12.2% to 20.3% across one-, four- and eight-voice workload groups. These are native measurements on a shared runner, not FL Studio CPU percentages or physical-driver deadline guarantees.

| Voices | Workload | Measured processing-time reduction |
|---:|---|---:|
| 1 | held-chord | 18.86% |
| 1 | retrigger | 20.32% |
| 4 | held-chord | 14.84% |
| 4 | retrigger | 14.94% |
| 8 | held-chord | 12.55% |
| 8 | retrigger | 12.15% |

The 8-/32-voice stress profiles, full per-callback CSVs, timers and audio hashes are retained in the [Windows validation artifact](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39196086290/artifacts/12162725473). They are observations on one machine and do not certify every callback deadline. Current 32-voice stress observations: 2/10142 over budget, worst ratio 1.037700. No automatic quality/polyphony reduction was added.

Six installer checks cover the pinned 0.13.1 baseline, 0.13.1-to-0.14.0 upgrade/default VST3 path and exact binary hashes, installed diagnostics, same-version reinstall, standalone process liveness and uninstall preserving preset/personal-audio/SoundFont-cache bytes. The launch smoke test does not test an audio device. Ten native skin snapshots and state restore pass; physical DPI and the owner's actual DAW/audio-device/listening checks remain manual. Owner licensing remains deferred; these are unsigned prepared builds.

[Windows installer ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39196086290/artifacts/12162173204) · [portable files](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39196086290/artifacts/12162158409) · [source branch](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/regional-bank-skins-cpu-0140). Close FL Studio/standalone, extract into a new folder and run Setup, then reopen FL64. New built-in presets: START → Hip-Hop category; skins: SELECT → SKIN. ROM Hacks remains available for user SoundFonts. No bank/audio is bundled. Setup EXE SHA256: `c7746a92f9913fa12ee0a0ff1814d609ef64d52d8b540e0a954da123fdcc8699`.


## Build

Use Visual Studio 2022 x64 C++ tools, Windows SDK, CMake 3.24+ and JUCE 8.0.6. Run scripts/build-windows.ps1, then ctest --test-dir build -C Release --output-on-failure. Build with GBA_BUILD_BENCHMARKS=ON for the matched stress and note/chord probes. scripts/compare-cpu-windows.ps1 compares against immutable 0.13.1 DSP commit a9969966e116d5b7aa1ff88991f97405b9a7e956 while supplying both versions the same current 530 factory arrays and drivers.

VST3: build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3. Standalone: build/GBoyAdvance2_artefacts/Release/Standalone/G Boy Advance 2.exe. Default install: C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3. User synthesis presets and referenced SoundFont cache retain their existing application-data paths.
