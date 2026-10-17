# Phase 5 — effects

Development revision 0.5.0 improves the native effects chain while preserving all 64 parameter IDs and project schema 1. No audio assets or runtime network dependencies are added. The existing effects controls and macro assignments remain available in SELECT.

## Implemented changes

- Saturation/distortion use true 2× interpolation/nonlinear processing/decimation with two original 33-tap Blackman-windowed half-band FIRs. Distortion blends a harder clipping curve with soft saturation. Only the nonlinear stage is oversampled. The FIRs stay warm when bypassed; dry and wet align at a constant 16-frame group delay, reported to the host.
- Delay-time changes use stationary old/new taps and a 20 ms linear crossfade. Further changes queue until the fade finishes; the read head does not slide during a fade.
- Independent bounded double-precision modulation phases remove the prior non-integer-rate discontinuities at a shared clock wrap. Wow blends continuously from dry instead of switching immediately to a delayed signal above a threshold.
- Fractional reads wrap integer indices before interpolation. This preserves tiny fractions near ring boundaries and avoids a one-past-end read caused by floating-point wrap rounding. Sanitizers caught the edge case during local development; the fix has a dedicated fractional-echo regression.
- Proper second-order low/high shelving EQs provide the expected dB endpoints and half-gain frequency midpoint. Derived drive/quantization/room/EQ coefficients update every 16 samples. The FDN reverb uses sample-rate-correct 3.2 kHz damping.
- Delay, chorus/flange and tape buffers are sized to their required 1.5 s / 26 ms / 6 ms maxima rather than allocating 1.6 s for each. At 48 kHz, their combined float storage drops from 1,843,584 to 588,672 bytes (about 68% less); reverb/FIR/voice memory is separate.
- Native reverb, delay, chorus, phaser, flanger, saturation, distortion, bitcrusher, wow/flutter, EQ and stereo width remain real DSP. Processing and core MIDI have no heap allocations, locks or file I/O.

## Focused acceptance tests

`Tests/EffectsTests.cpp` adds **138 checks**, primarily at 44.1/48/96 kHz:

- Exact 16-frame bypass impulse, stereo isolation, measured nonlinear group delay, repeatable reset and silence after clearing a populated effects tail.
- A driven-tone folded harmonic at least 14 dB below a direct unfiltered nonlinear reference; carrier retention; distinct saturation/distortion spectra.
- Exact echo times and geometric feedback amplitudes, tiny fractional-tap precision at ring wrap, no first-sample jump on a time change and convergence to the new fixed tap.
- Reverb minimum onset, stereo diffusion and size-dependent tail decay.
- Chorus/flanger onset and stereo offsets, phaser frequency-dependent response, EQ endpoint/midpoint gains and opposite-band preservation.
- Continuous wow/flutter through the former shared-clock wrap and tiny-amount activation.
- Bitcrusher quantization/hold intervals, mid/side width behavior, finite combined extreme automation, zero tracked allocations and identical full-engine audio under different block partitions.

The JUCE integration suite also checks the reported 16-sample host latency. Numerical tests do not certify perceptual quality or every DAW's latency compensation.

## Before/after measurements

Identical probes against 0.4.0 and 0.5.0, 48 kHz:

| Measurement | 0.4.0 | 0.5.0 |
|---|---:|---:|
| Driven 10,080 Hz tone, folded third at 17,760 Hz relative to carrier | -14.89 dBc | -52.06 dBc |
| Low-shelf gain at 180 Hz with +12 dB setting | +9.273 dB | +5.999 dB |
| Output change against bypass when enabling wow at 0.0002 | 0.16173 | 0.000023508 |

The alias comparison is one selected tone/level and does not establish full-chain alias freedom. The final limiter remains at base rate; bitcrushing and retro synthesis intentionally introduce spectral artifacts. Existing factory patches remain parameter-compatible but should be auditioned and level-matched after the effects changes.

## Validation status

- Strict GCC 14.2 C++20 compilation: PASS. All 1,836 DSP checks (1,387 engine/factory/effects + 161 core + 150 advanced + 138 effects) PASS.
- AddressSanitizer and UndefinedBehaviorSanitizer: all four DSP suites PASS with halt-on-error. Leak detection is disabled because LeakSanitizer is unsupported in this sandbox/ptrace environment; leak freedom is not claimed.
- Linux JUCE processor/state/preset/UI integration: 172 checks PASS, including host latency.
- Factory JSON and compiled parameter data regenerate byte-for-byte.
- Windows Visual Studio 2022 build, five CTest suites, pluginval and packaging: pending this revision's CI run.

CMake is unavailable locally, so local compilation uses GCC directly; CI verifies CMake and Visual Studio.

## Next work

Proceed to Phase 6 factory-preset sound design and listening review. Complete physical Windows/DAW/DPI acceptance, real audio-device deadline/underrun and engine-blend stress, the separate Steinberg validator, interactive installer tests, release signing and licensing. Full modulation-matrix expansion remains a separate feature.
