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

Strict GCC C++20 builds pass all 6,231 DSP checks: 1,836 existing checks plus 4,395 factory-audit checks. Linux JUCE integration passes 177 checks, including factory-state restoration. The original four DSP suites pass ASan/UBSan; the expanded bank audit is still running under sanitizers. Windows build, six CTest suites, pluginval and packaging remain pending.

| Same native audit metric | 0.5.0 baseline | 0.6.0 |
|---|---:|---:|
| Central-note max-100ms stereo RMS spread | 21.9986 dB | 10.41 dB |
| Nearest-neighbor flags below dry distance 0.025 | 46 | 0 |
| Dry spectral/envelope groups at that threshold | Not recorded | 382 |

The 0.6.0 central RMS range is 0.0108593–0.036; largest measured central-note peak is 0.214519. All 191 base/Afterglow pairs differ above the dry threshold. Factory JSON/embedded data regenerate byte-for-byte, and all 382 legacy identity/name/category/program positions match the prior bank. Baseline 0.5.0 central-note max-100ms stereo RMS spread was 21.9986 dB, with 46 nearest-neighbor flags under the 0.025 dry-analysis threshold. The comparison uses identical deterministic excitation and renderer/analysis settings.

## Limits and next work

RMS matching is not perceptual loudness matching. Spectral/envelope separation is not a listening certificate or proof of acoustic realism. The audit grid is MIDI 48/60/72 (bass 36/48/60), velocities 0.4/0.9, at 48 kHz; existing engine tests cover standard sample rates. Human listening, remaining gain-ceiling cases, full-register articulation and target-DAW/device/DPI/latency/installer acceptance remain open. WAVs are audit output only, never a sampler bank or runtime dependency. Next proceed to Phase 7 vocal-style synthesis/preset refinement, then Phase 8 retro sounds.
