# Phase 6 — factory preset refinement

Development revision 0.6.0 refines the existing 382-preset library rather than adding renamed parameter duplicates. All 64 parameter IDs, project schema 1, factory IDs, names and host program order are preserved. Sound is still generated only by native synthesis.

## Changes

- `Presets/instrument-designs.json` defines explicit synthesis/articulation parameters for all 102 requested named guitars, pianos/keys, bells, pads, basses, flutes, plucks, sax-style leads and strings. Each has an engine-aware Afterglow variation: physical excitation/damping, FM ratios/index, additive partials, table position, vowel/choir excitation or modal ratios change when active.
- Measured close pairs were refined, including retro racing/sports timbres and a Southern vocal articulation. The deeper vocal and retro libraries remain Phase 7/8 work.
- `factory-index.json` freezes legacy factory identities and program indices. Generation rejects missing/renamed legacy entries and appends new ones. Existing DAW projects carry their actual parameters; integration checks verify that a factory ID does not replace saved parameter values during state reload.
- Native measurements produce bounded master-volume calibration in `factory-levels.json`; the existing volume parameter is after the limiter, so no new gain parameter or engine changes are needed. Category targets preserve differing articulations. Quieter patches at the gain ceiling are explicitly reported.
- Recipe provenance and complete gain coverage are checked during generation. Factory JSON and embedded C++ data remain deterministic. Authoring metadata contains only parameters and descriptions. Portable/installer packages include the final factory JSON, not audio or authoring dependencies.
- A dedicated native factory suite renders low/mid/high notes plus a softer central note, checks domains, levels, velocity, release and headroom, and analyzes dry spectra/envelopes. It generates CSVs and optional six-second synthesized audition excerpts for all 17 categories.

## Validation

Strict GCC C++20 builds pass all 6,231 DSP checks: 1,836 existing checks plus 4,395 factory-audit checks. Linux JUCE integration passes 177 checks, including factory-state restoration. All five DSP suites pass ASan/UBSan with halt-on-error; leak detection is disabled because LeakSanitizer is unsupported in this sandbox. Windows validation and artifact checks also pass, as recorded below.

| Same native audit metric | 0.5.0 baseline | 0.6.0 |
|---|---:|---:|
| Central-note max-100ms stereo RMS spread | 21.9986 dB | 10.41 dB |
| Nearest-neighbor flags below dry distance 0.025 | 46 | 0 |
| Dry spectral/envelope groups at that threshold | Not recorded | 382 |

The 0.6.0 central RMS range is 0.0108593–0.036; largest measured central-note peak is 0.214519. All 191 base/Afterglow pairs differ above the dry threshold. Factory JSON/embedded data regenerate byte-for-byte, and all 382 legacy identity/name/category/program positions match the prior bank. Baseline 0.5.0 central-note max-100ms stereo RMS spread was 21.9986 dB, with 46 nearest-neighbor flags under the 0.025 dry-analysis threshold. The comparison uses identical deterministic excitation and renderer/analysis settings.

## Windows build and artifact evidence

[Run 39037896323](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39037896323), application/build source `2214c4dc58d5105b95e00cc2d85f9a9e88baaa81`:

- Visual Studio 2022 x64 VST3 and standalone compilation/linking: PASS.
- CTest: **6/6 PASS** in 92.99 seconds, including **4,395 factory checks** (84.11 seconds) and **180 JUCE integration checks**. Total DSP checks: **6,231**.
- Actual VST3: pluginval 1.0.4 **strictness 10 SUCCESS**, seed `0x56a454`, version 0.6.0. Its separate Steinberg validator remains unconfigured.
- Portable ZIP and Inno Setup installer compilation: PASS. Downloaded Windows artifact `12102966872`, SHA256 `ac27fcda39f631b7845bfb4b1878d0109611fa71674686fb3b65e74ae9658f87`, matches GitHub's digest. ZIP integrity, AMD64 PE VST3/standalone, six packaged test logs and 382-preset payload verified; no audio or authoring banks are in the portable package.
- Separate audition artifact `12103036745`, SHA256 `006c50a93715550606aeeafe4b8d81775053546a6eb2385affd207ddf7087488`, also matches GitHub's digest. All 17 WAVs are six-second, stereo 48 kHz / 16-bit PCM synthesized outputs, accompanied by an index, 1,528 render rows and 382 nearest-neighbor rows.
- The [factory report](FACTORY_LIBRARY.md) is generated from these Windows CSVs. Windows and GCC results agree at the reported precision: 382 dry groups, zero flags, 312/382 within 0.5 dB of category targets; 85 gain-ceiling cases remain. These statistics do not replace listening acceptance.
- Ubuntu CI CMake build, all five DSP suites and factory-generation reproducibility: PASS. Final repository changes after the tested source commit contain evidence/documentation only.

Processor/DSP latency remains 16 samples in numerical tests. Pluginval's initial cached information line prints zero before preparation; a separate prepared-host latency query and manual compensation tests remain pending, as explained in [Phase 5](PHASE_5.md).

## Limits and next work

RMS matching is not perceptual loudness matching. Spectral/envelope separation is not a listening certificate or proof of acoustic realism. The audit grid is MIDI 48/60/72 (bass 36/48/60), velocities 0.4/0.9, at 48 kHz; existing engine tests cover standard sample rates. Human listening, remaining gain-ceiling cases, full-register articulation and target-DAW/device/DPI/latency/installer acceptance remain open. WAVs are audit output only, never a sampler bank or runtime dependency. Next proceed to Phase 7 vocal-style synthesis/preset refinement, then Phase 8 retro sounds.
