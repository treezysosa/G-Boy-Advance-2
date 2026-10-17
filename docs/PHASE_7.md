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

Strict GCC C++20 builds pass **7,307 DSP checks**: 1,836 existing engine/core/advanced/effects checks, **1,076 vocal checks** and 4,395 factory checks. The full bank retains 382 dry spectral/envelope groups with zero nearest-neighbor flags below 0.025; all 191 base/Afterglow pairs exceed that threshold. Linux JUCE integration passes **177 checks**. Factory data regenerates byte-for-byte and all legacy identities/program positions match. Final ASan/UBSan and Windows build/pluginval/packaging are under validation. Final evidence will be added after completion.

## Compatibility and remaining acceptance

Saved project parameters retain their IDs and values, and user preset files keep schema 1. The new DSP changes the sound of engine 5, including older vocal, sax-style and other formant patches; numerical state compatibility does not guarantee an identical audio render to 0.6.0. Compare an existing project before replacing a production installation.

Human listening, perceived vowel clarity/uniqueness, naturalism, full-register articulation, quieter gain-ceiling cases and manual target-DAW/device/DPI/installer acceptance remain pending. Parameter automation is still block-read and then smoothed; the modulation system has two LFO routes rather than an arbitrary matrix. Separate Steinberg validation, prepared-host latency/compensation/bypass/export-tail tests and release licensing/signing remain open. Next development phase: Phase 8 original retro-game synthesis/preset refinement.
