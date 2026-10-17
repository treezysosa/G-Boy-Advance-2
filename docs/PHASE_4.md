# Phase 4 — advanced synthesis

Development revision 0.4.0 builds on all eight working engines. FM, additive, wavetable, physical modeling and formant synthesis generate audio natively; there are no recordings, sample imports or external banks. All 64 parameter IDs, preset format and project schema version 1 are preserved. The advanced panel already exposes the engine-specific controls.

## Implemented changes

- All eight engines use interruptible 5 ms linear blends. Oscillator, envelope and LFO clocks advance once per voice/sample. Only nonzero engine weights are evaluated; additive normalization also updates while fading out. Reset/prepare starts directly on the selected engine.
- Karplus–Strong delay compensates the feedback low-pass phase at the fundamental. Changing brightness no longer causes the previous systematic flat tuning. Initial excitation removes its DC mean. Delay lines remain fixed-size; buffer initialization runs only when a string is excited.
- Plucked strings and modal keys excite when selected on an already-held note. A model that has fully faded out re-arms for a fresh excitation when selected again. Mono/legato retains the running model while it remains active.
- Mathematical wavetable mip levels fade out before their highest partial reaches 0.45 times the sample rate. Both levels in a blend are safe at the selected pitch; the previous blend retained above-Nyquist partials. Four original table shapes and continuous morphing remain.
- Vocal harmonic/noise excitation and the selectable formant filter have separate three-resonator state. The formant filter now works on the vocal engine too, without reusing resonator state twice or substituting a notch response. Vowels, transitions, breath, choir detune, syllabic motion and formant shifting remain editable.
- Fixed-size engine weights, voice state and model buffers; no rendering/MIDI file I/O, locks or heap allocations.

## Focused tests

`Tests/AdvancedSynthesisTests.cpp` adds **150 checks**:

- FM zero-index carrier purity, ratio-2 odd sidebands and ratio-3 shifted sidebands at 44.1/48/96 kHz.
- Additive one-partial sine, requested harmonics and brightness-dependent upper-partial falloff at all three rates.
- Four audible, spectrally distinct mathematical wavetable shapes, intermediate morph spectra and a folded-eighth-partial regression at all three rates.
- Plucked-string pitch within 2 cents at MIDI 48/60/72/84, brightness 0.1/0.9 and 44.1/48/96 kHz, measured after 30 periods to exclude the noisy pick transient. Damping controls decay; modal keys have the expected inharmonic resonance. This is a tested grid, not a claim about every pitch or automation trajectory.
- Five audible synthetic vowels, separated A/I/U first resonances, independent formant shift, detuned choir excitation and vocal-plus-formant-filter stability. These tests establish synthesized spectral behavior, not natural speech or an identifiable singer.
- Every destination engine fades from subtractive synthesis at three rates, decaying models excite on late held-note selection, and rapid interrupted engine/filter changes remain bounded with zero tracked allocations.

All 1,387 existing engine/factory/effects checks and 161 Phase 3 checks also pass: **1,698 DSP checks**. The Linux JUCE integration suite passes **171 checks**. Local compilation uses GCC 14.2 directly because CMake is unavailable here.

## Before/after measurements

Identical probe sequences compiled against 0.3.0 and 0.4.0, 48 kHz:

| Measurement | 0.3.0 | 0.4.0 |
|---|---:|---:|
| Dark string, MIDI 84, brightness 0.1, pitch error after 30 periods | -53.6645 cents | -0.5011 cents |
| Wavetable 3,520 Hz saw shape, folded eighth partial at 19,840 Hz relative to fundamental | -26.09 dBc | -74.00 dBc |
| Sine-to-wavetable held-note first-sample absolute output step | 0.0163945 | 0.000162531 |

These are specific numerical regressions. Table interpolation, FM sidebands and nonlinear effects are not certified alias-free, and complete preset transitions still require listening/stress review.

## Windows validation

Pending this revision's Visual Studio 2022 build, four CTest suites, actual VST3 pluginval run and packaging. Results will be recorded before delivering the binaries.

## Next steps and boundaries

Proceed to Phase 5 effects quality/performance refinement. Remaining release acceptance includes physical Windows UI/DPI testing, DAW/audio-device listening, full factory-patch audition/level matching, a separate Steinberg validator run, real host deadline/underrun measurements and installer/signing/licensing work. Phase 4 exposes the existing two LFO routes and dedicated controls, not an arbitrary modulation matrix. Interrupted engine fades may temporarily evaluate several engines; stress their worst-case CPU.

AddressSanitizer and UndefinedBehaviorSanitizer pass all three DSP suites with halt-on-error. Leak detection is disabled because LeakSanitizer is unsupported by this sandbox/ptrace environment; leak freedom is not claimed.
