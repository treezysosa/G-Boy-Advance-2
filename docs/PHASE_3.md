# Phase 3 — core synthesis

Development revision 0.3.0 builds on the existing working engine and Phase 2 UI. All 64 parameter IDs and project schema version 1 are preserved.

## Changes

- Amplitude/filter ADSRs use double-precision accumulation for sample-rate-correct long stages. Release captures its current level/time once; repeated note-release/controller/pedal messages do not restart the release.
- Both LFOs, vibrato, tremolo and syllabic modulation use bounded double-precision phases. They keep advancing on long-held notes instead of relying on an ever-growing float elapsed-time value. Modal decay elapsed time also uses double precision.
- Waveform selectors, filter type and both LFO destinations use interruptible 5 ms linear crossfades. Reset/prepare begins directly at the selected values. Fast retargeting starts from the current blend. The synthesis-engine selector remains discrete.
- A single TPT state update exposes low/high/band/notch outputs for blending. The formant-filter branch is included in the selector fade. Coefficients update before excitation processing, including at note onset. The control counter wraps at 16 samples and cannot overflow on hours-long notes.
- Changes use fixed-size state, with no render-time allocation, file I/O, network access or locks.

## Focused tests

`Tests/CoreSynthesisTests.cpp` adds 161 checks:

- ADSR attack/decay/sustain/release timing at 44.1/48/96 kHz, interrupted attack, repeated release requests and 12-second releases.
- Five modulation clocks advanced through ten minutes at 96 kHz; no stopped phase or material drift.
- Measured low/high/band/notch frequency response, resonant cutoff gain and finite extreme cutoff modulation.
- Six audible subtractive waveforms at all three rates, with fundamental/even/odd harmonic measurements; second-oscillator octave tuning reaches audio.
- Every voice limit from 1 to 32, stealing/release sequences, repeated pedal-up regression, cross-channel last-held-key priority, mono retrigger versus legato, velocity amplitude law and measured glide trajectory.
- Both LFOs independently affect pitch/filter/amplitude/vowel/pan; MIDI CC1 vibrato, tremolo and filter ADSR change real audio.
- Smaller held-note waveform/filter/LFO-route transition steps, final selected timbre, rapid retargeting, bounded output and zero crossfade allocations.

Together with the 1,387 factory/engine/effects checks, this is **1,548 DSP checks**. The Linux JUCE integration suite passes 171 checks. AddressSanitizer and UndefinedBehaviorSanitizer pass both DSP suites with halt-on-error; LeakSanitizer is disabled because the sandbox/ptrace environment does not support it. CMake is unavailable locally; local compilation uses GCC 14.2 directly, while CI verifies CMake/Visual Studio builds.

## Before/after evidence

A probe built against the prior 0.2.0 source and this engine ran the same MIDI and parameter sequence at 48 kHz:

| Measurement | 0.2.0 | 0.3.0 |
|---|---:|---:|
| Active voices after repeated pedal-up messages during a 60 ms release | 1 | 0 |
| Waveform change first-sample absolute step | 0.0465916 | 0.000293870 |
| Filter change first-sample absolute step | 0.0465504 | 0.000702281 |

These transition examples are numerical regressions, not a claim that every automation/preset change is perceptually click-free or alias-free.

## Remaining validation

Windows x64 VST3/standalone compilation, all three CTest suites, pluginval strictness 10 and packaging are pending for this revision. Manual DAW/device listening, real host deadline/underrun tests, the Steinberg validator and the carried-forward Phase 2 DPI/accessibility/visual checks remain open. Advanced-engine quality work, arbitrary modulation-matrix expansion, measured anti-aliasing and synthesis-engine crossfades are later work.
