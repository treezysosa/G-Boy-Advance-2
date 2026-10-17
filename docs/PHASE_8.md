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

Initial focused retro checks pass (270 checks). Final full-bank regression, sanitizer, JUCE integration and Windows VST3/standalone/pluginval/packaging checks are running. Final results and artifact evidence will be recorded here.

## Compatibility and remaining acceptance

Saved parameters retain their IDs and values. Revised engine 6 now uses oscillator 2 and brightness and has different digital/noise processing, so existing chiptune patches can sound different from 0.7.0. The Phase 7 formant change also remains in effect. State compatibility does not guarantee identical audio renders across DSP revisions.

Human listening, perceived retro character/uniqueness, full-register articulation, quieter gain-ceiling cases and manual target-DAW/device/DPI/installer acceptance remain pending. Register noise and DAC quantization deliberately produce digital artifacts. Separate Steinberg validation, prepared-host latency/compensation/bypass/export-tail behavior, callback deadline/underrun profiling and release licensing/signing remain open. Next development phase: Phase 9 preset management and browser review.
