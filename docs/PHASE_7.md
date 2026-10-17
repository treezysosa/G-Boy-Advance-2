# Phase 7 — synthetic vocal engine and designs

Development version 0.7.0 adds a dedicated original vocal excitation module and explicit profiles for all 75 named vocal designs / 150 base and Afterglow presets. No recordings, speech model, audio import or artist voice imitation is used. Broad production-style names describe synthesis textures, not a language, accent, authentic cultural performance or identifiable performer.

## Implemented

- `Source/dsp/VocalEngine.h`: five independent vowel resonances with bandwidth-normalized peaks; continuous A/E/I/O/U interpolation and formant shifting independent of MIDI pitch. Three upper resonances supplement the first two vowel-defining bands.
- PolyBLEP harmonic excitation, a sine component and linear brightness-controlled spectral tilt. A high-passed noise source blends continuously from voiced to unvoiced. Three pitched excitation layers provide main/upper/lower choir tones with energy compensation. The existing detune/coarse controls set the upper layer; the lower layer mirrors its ratio, with safe high-frequency limits.
- Existing vibrato, modulation wheel, pitch bend, glide, velocity, envelopes, smoothed vowel control, LFO vowel routing and three-Hz syllabic vowel/amplitude motion remain connected. These are synthetic vowel gestures, not actual words or phonemes.
- The post-oscillator formant filter retains its independent three-band state; it can process the new vocal excitation without sharing feedback state. Existing engine fades and per-note reset/release behavior remain intact.
- `Presets/vocal-designs.json` defines 75 explicit timbres, articulation settings, design notes and active Afterglow variations. Seven families cover soul/R&B, pop/indie, global production textures, Southern/West Coast, horror, cartoon/game effects and choirs/textures. Whisper, choir, chop, radio, bass and distorted patches use the corresponding real synthesis/effect controls.
- Factory levels are recalibrated after the engine change. Recipe provenance now includes vocal designs and the relevant DSP implementation. All 382 preset IDs/names/categories/program positions, 64 parameter IDs, schema 1 and plugin identifiers are retained.
- `Tests/VocalTests.cpp` checks rendered filter resonances, formant shifts/rates, harmonic/noise periodicity, brightness, upper/lower choir components, pitch-independent unvoiced tone, syllabic articulation, 32-voice automation, allocation-free rendering and release. Every vocal preset is also rendered in low/high registers at 44.1/96 kHz; the full factory audit adds three registers/two velocities at 48 kHz.
- Offline audition export adds 12 contrasting vocal excerpts and an index under `vocal-auditions/`, alongside the existing 17 category excerpts. All are six-second, stereo 48 kHz / 16-bit PCM test outputs. None is installed or read by the instrument; an excerpt may end before its full effects tail.

## Validation status

Strict GCC C++20 builds pass **7,307 DSP checks**: 1,836 existing engine/core/advanced/effects checks, **1,076 vocal checks** and 4,395 factory checks. The full bank retains 382 dry spectral/envelope groups with zero nearest-neighbor flags below 0.025; all 191 base/Afterglow pairs exceed that threshold. Linux JUCE integration passes **177 checks**. Factory data regenerates byte-for-byte and all legacy identities/program positions match. All six DSP suites pass ASan/UBSan with halt-on-error and leak detection disabled (LeakSanitizer is unsupported in this sandbox). Windows build, seven CTest suites, pluginval and packaging pass as recorded below.

## Windows build and artifact evidence

[Run 39039531021](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39039531021), application/build source `bdfdec3fb161429bd0b0c9a99183c0de59e0dd5e`:

- Visual Studio 2022 x64 VST3 and standalone compilation/linking: PASS.
- CTest: **7/7 PASS** in 90.58 seconds: 7,307 DSP checks plus **180 JUCE integration checks**. The vocal suite took 10.05 s and factory audit/auditions 73.08 s.
- Actual VST3: pluginval 1.0.4 **strictness 10 SUCCESS**, seed `0x89765b`, version 0.7.0. The separately configured Steinberg validator remains pending.
- Portable ZIP and Inno Setup installer compilation: PASS. Windows artifact `12103364347`, SHA256 `5d847a9f44e6248b2c529c0aad2282726bac60994650a45490786a62e5bb7dd8`: downloaded digest matches GitHub. ZIP integrity, AMD64 PE VST3/standalone, all seven suites in the packaged test log, pluginval log and final 382-preset payload verified. No audio or authoring banks are in the portable package.
- Audition artifact `12103930878`, SHA256 `a159bcf853a0818408b145efd67c76b11fa4735a8157f8f213ea79f4fa3bb574`: downloaded digest/ZIP integrity, all **29** stereo 48 kHz / 16-bit PCM six-second WAVs, two indices, 1,528 render rows and 382 similarity rows verified. All audio is synthesized test output and is never loaded by the instrument.
- The [factory report](FACTORY_LIBRARY.md) is generated from the Windows CSVs. Overall central-note maximum rolling 100 ms stereo RMS spread remains 10.41 dB; 346/382 patches are within 0.5 dB of category targets (312 in 0.6.0), with 41 gain-ceiling cases (85 previously). The 150 vocal patches measure 0.01425–0.02000 central RMS, peak 0.124520; one vocal gain reaches the ceiling. These are numerical levels, not perceptual equal loudness.
- Ubuntu CI CMake, all six DSP suites and factory-generation reproducibility PASS. Final repository changes after the tested source commit contain evidence/documentation only.

Processor/DSP latency remains 16 samples in numerical tests. Pluginval's initial cached information line prints zero before preparation; separate prepared-host latency and manual compensation acceptance remain pending as explained in [Phase 5](PHASE_5.md).

## Throughput probe

An ad-hoc GCC 14.2 C++20 `-O2` probe on the Linux VM rendered 32 sustained formant voices, choir 0.8, breath 0.15, effects disabled and 64-frame blocks. Median time over three interleaved one-second renders (preparation/table generation excluded):

| Rate | 0.6.0 seconds / rendered second | 0.7.0 seconds / rendered second |
|---|---:|---:|
| 48 kHz | 0.209 | 0.294 |
| 96 kHz | 0.388 | 0.525 |

The extra resonances and choir layer cost CPU. These aggregate VM measurements do not establish worst-case callback deadlines, DAW scheduling or underrun behavior; profile on the target machine and reduce voice count if needed. Vocal-specific optimization remains open. The full numerical suite separately checks allocation-free real-time rendering.

## Compatibility and remaining acceptance

Saved project parameters retain their IDs and values, and user preset files keep schema 1. The new DSP changes the sound of engine 5, including older vocal, sax-style and other formant patches; numerical state compatibility does not guarantee an identical audio render to 0.6.0. Compare an existing project before replacing a production installation.

Human listening, perceived vowel clarity/uniqueness, naturalism, full-register articulation, quieter gain-ceiling cases and manual target-DAW/device/DPI/installer acceptance remain pending. Parameter automation is still block-read and then smoothed; the modulation system has two LFO routes rather than an arbitrary matrix. Separate Steinberg validation, prepared-host latency/compensation/bypass/export-tail tests and release licensing/signing remain open. Next development phase: Phase 8 original retro-game synthesis/preset refinement.
