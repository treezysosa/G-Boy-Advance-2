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

The full native factory audit passed 6,218 checks: 530 distinct dry spectral/envelope groups, 120/120 regional groups, no near-neighbor flags, and playable levels/registers/releases. Native application and own-source ASan/UBSan passed 539 checks including ten skin choices/restores, new region/category/tag browsing and factory project-state restore. Cached JUCE modules were uninstrumented and leak detection was disabled for the environment tracer. The seven additional native suites passed MIDI, all synthesis engines, modulation, effects, allocation-free render, vocals/retro and 612,986 SF2 checks. Windows validators/installer tests are being completed. No current Windows download is claimed until those finish. See FACTORY_LIBRARY.md for numerical level and similarity evidence; human listening and hardware DAW/DPI/device acceptance remain manual. Licensing is deferred by the owner; unsigned build preparation continues.

## Build

Use Visual Studio 2022 x64 C++ tools, Windows SDK, CMake 3.24+ and JUCE 8.0.6. Run scripts/build-windows.ps1, then ctest --test-dir build -C Release --output-on-failure. Build with GBA_BUILD_BENCHMARKS=ON for the matched stress and note/chord probes. scripts/compare-cpu-windows.ps1 compares against immutable 0.13.1 DSP commit a9969966e116d5b7aa1ff88991f97405b9a7e956 while supplying both versions the same current 530 factory arrays and drivers.

VST3: build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3. Standalone: build/GBoyAdvance2_artefacts/Release/Standalone/G Boy Advance 2.exe. Default install: C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3. User synthesis presets and referenced SoundFont cache retain their existing application-data paths.
