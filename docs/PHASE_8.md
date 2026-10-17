# Phase 8 — original retro synthesis and presets

Development version 0.8.0 expands the chiptune engine and gives the retro bank explicit active designs. The full library now has 410 parameter-only presets in 17 categories, including 28 named retro designs / 56 base and Afterglow voicings. The original 382 factory IDs, names, categories and host program positions are preserved; 28 new entries append. All 64 parameter IDs, schema 1 and plugin identifiers remain stable.

## Implementation

- `RetroEngine.h` adds two pitched oscillators with the existing waveform selection, blend, coarse tune and detune controls. Each oscillator offers mathematical sine, triangle, saw, square, pulse and clocked noise. No commercial game audio, ROM data, recordings or external bank is used.
- Sine/triangle use 16 DAC levels and saw uses 32. Pulse/saw edges use PolyBLEP correction before any intentional digital quantization. A brightness-controlled one-pole tone filter follows the digital source. This reduces some high-frequency energy; it does not make deliberately stepped/bitcrushed sounds alias-free.
- Independent 15-bit and 7-bit maximal-sequence shift registers produce long and short noise. Clock speed follows the note frequency, with a bounded double-precision phase. Brightness blends between the two register timbres while also setting the tone filter. Oscillator 2 has its own states and tuning. These are broad technical inspirations, not cycle-accurate console emulation.
- `retro-designs.json` defines all 28 named designs with synthesis/articulation parameters, active Afterglow overrides, searchable technical-style tags and design notes. Seven styles cover 8-bit handheld, 16-bit FM, 16-bit wavetable aesthetics, early 3D, late-90s soundtracks, early-2000s handheld and arcade systems. Other hybrid engines provide FM bass/bells/brass, additive strings/organ, mathematical wavetable keys/pads/flute, plucked harp, modal marimba and formant choir.
- All 14 originally requested retro names retain their IDs. Fourteen new designs add duty bass, clocked noise snare/hat, triangle sub, FM keys, organ, flute, polygon pad, modal marimba, harp, choir and pulse motion. New names describe original sounds rather than a commercial game's music.
- Factory master levels are measured again after engine changes. Calibration now checks the complete identity set dynamically. Recipe provenance includes the retro authoring file and DSP module. The plugin embeds only final parameter arrays; authoring JSON and audio renders are not runtime dependencies.
- The focused retro suite tests register periods/balance, clock/sample-rate behavior, DAC levels, pulse duty, measured pulse alias reduction, oscillator blend/coarse tuning, brightness, pitch bend/release, host block-size invariance and allocation-free 32-voice automation. All 56 retro presets are additionally rendered in low/high registers at 44.1/96 kHz; the full factory audit covers three registers/two velocities at 48 kHz.
- Offline export creates 28 six-second stereo 48 kHz / 16-bit PCM base-design excerpts under `retro-auditions/`, in a separate CI artifact. These original test renders are never installed or read by the instrument and may end before their full effects tails. The existing 17 category / 12 vocal excerpts are retained separately.

## Validation status

Strict GCC C++20 builds pass **7,983 DSP checks**: 1,471 engine, 161 core, 150 advanced, 138 effects, 1,076 vocal, **270 retro** and **4,717 factory** checks. Linux JUCE integration passes **181 checks**, including appended host-program selection/state restoration. Final bank analysis reports 410 dry spectral/envelope groups, no nearest-neighbor flags below 0.025 and all 205 base/Afterglow pairs above that threshold. These metrics do not certify perceptual uniqueness. All seven DSP suites pass ASan/UBSan with halt-on-error and leak detection disabled (LeakSanitizer is unsupported in this sandbox). Windows validation and artifact checks also pass, as recorded below.

## Windows build and artifacts

[Run 39041332464](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39041332464), application/build source `879d51bdc5ac13e79dca921372ecc09583b850c5`:

- Visual Studio 2022 x64 VST3 and standalone compilation/linking: PASS.
- CTest **8/8 PASS** in 135.25 s, including 7,983 DSP checks and **184 integration checks**. The retro suite took 5.44 s; the factory audit/excerpts took 107.38 s.
- Actual VST3 pluginval 1.0.4 **strictness 10 SUCCESS**, seed `0xb07ab3`, version 0.8.0. Its separately configured Steinberg validator remains pending.
- Portable ZIP and Inno Setup installer compilation: PASS. Windows artifact `12104183297`, SHA256 `239bd30be01caa42fb56e6227acf05037c0ec7dd4eaefdc7274ffc9ade3fc89d`: downloaded digest, ZIP integrity, AMD64 binaries, packaged eight-suite/pluginval logs and 410-preset payload verified. No audio or authoring banks are in the portable package.
- Factory/vocal audition artifact `12103929157`, SHA256 `109cee45dd650456c9ce7bd8f4eff4f8dd81b48dce210573c5a3b7c2f9aeeaa2`: digest/ZIP, 29 six-second stereo 48 kHz / 16-bit PCM excerpts, two indices, 1,640 audit rows and 410 nearest-neighbor rows verified.
- Retro audition artifact `12104068693`, SHA256 `dd06ad81017c5ac54bdc6bc8fad2a7f801500dd9e889fed56612fc18904ca243`: digest/ZIP, all **28** six-second stereo 48 kHz / 16-bit PCM excerpts and index verified. None is a runtime audio asset.
- The [factory report](FACTORY_LIBRARY.md) is generated from the Windows CSVs: 410 dry groups, zero flags below 0.025, all 205 pairs above that threshold; overall central-note max-100ms stereo RMS spread 11.35 dB. **366/410** patches are within 0.5 dB of category targets; **48** reach the gain ceiling, including 11 retro patches. Retro central RMS is 0.009749–0.025000, peak 0.136118. This is numerical level calibration, not perceptual equal loudness.
- Ubuntu CI CMake, all seven DSP suites and factory-data regeneration PASS. All 382 legacy identities/program positions match. Final repository changes after the tested source commit contain evidence/documentation only.

Processor/DSP latency remains 16 samples in numerical tests. Pluginval's initial cached information line prints zero before preparation; separate prepared-host latency/compensation acceptance remains pending as explained in [Phase 5](PHASE_5.md).

## Throughput probe

An ad-hoc GCC 14.2 C++20 `-O2` Linux VM probe rendered 32 sustained chiptune voices in 64-frame blocks: pulse oscillator 1, triangle oscillator 2, duty 0.25, mix 0.3, coarse +7, brightness 0.7, effects/LFOs off. Median over three interleaved one-second renders, preparation excluded:

| Rate | 0.7.0 seconds / rendered second | 0.8.0 seconds / rendered second |
|---|---:|---:|
| 48 kHz | 0.158 | 0.295 |
| 96 kHz | 0.313 | 0.582 |

The previous engine ignored oscillator 2 and brightness; the expanded DSP performs more work and costs more CPU. These aggregate VM measurements do not establish worst-case callback deadlines, DAW scheduling or underrun behavior. Profile the target machine, including two triangle oscillators, effects, automation and interrupted engine blends. Vocal/retro CPU optimization remains open.

## Compatibility and remaining acceptance

Saved parameters retain their IDs and values. Revised engine 6 now uses oscillator 2 and brightness and has different digital/noise processing, so existing chiptune patches can sound different from 0.7.0. The Phase 7 formant change also remains in effect. State compatibility does not guarantee identical audio renders across DSP revisions.

Human listening, perceived retro character/uniqueness, full-register articulation, quieter gain-ceiling cases and manual target-DAW/device/DPI/installer acceptance remain pending. Register noise and DAC quantization deliberately produce digital artifacts. Separate Steinberg validation, prepared-host latency/compensation/bypass/export-tail behavior, callback deadline/underrun profiling and release licensing/signing remain open. Next development phase: Phase 9 preset management and browser review.
