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

Host tail reporting derives from release time plus delay and room feedback decay to -100 dB relative to their initial level, with a small processing margin. It replaces the fixed 30-second estimate, which could truncate long echoes. Integration checks cover a short dry tail, one echo and the maximum high-feedback case (over two minutes). Actual DAW export-tail behavior still needs manual acceptance.

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
- Linux JUCE processor/state/preset/UI integration: 175 checks PASS, including host latency.
- Factory JSON and compiled parameter data regenerate byte-for-byte.
- Windows [run 39011625622](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39011625622), application/build commit `5c9ab5fa3a38a5b4982c12aa36a4bd8bf0c3cddc`: Visual Studio 2022 x64 VST3 and standalone compilation/linking PASS.
- CTest **5/5 PASS**: 1,387 engine/factory/effects checks (3.27 s), 161 core checks (0.77 s), 150 advanced checks (1.35 s), 138 effects checks (1.11 s), **178 processor/state/preset/UI checks** (1.81 s).
- Actual VST3: pluginval 1.0.4 strictness 10 **SUCCESS**, seed `0x796c4ad`, version 0.5.0. Its separate Steinberg validator remains unconfigured/pending.
- Portable ZIP and Inno Setup installer compilation PASS.
- Artifact `12093173038`: SHA256 `9ab9b62a98d4634e127eebf16f8a0dfb904856cdc5544fc12780328b2597e468`, matching GitHub's digest. Downloaded ZIP integrity, AMD64 VST3/standalone binaries and all packaged test logs verified.
- Linux CI CMake build and all four DSP CTest suites PASS.

Final documentation changes evidence only; application/build source matches this validated commit. CPU cost and buffer memory measurements are recorded in [PERFORMANCE.md](PERFORMANCE.md).

CMake is unavailable locally, so local compilation uses GCC directly; CI verifies CMake and Visual Studio.

The latency evidence consists of a measured 16-frame DSP impulse peak and processor integration checks of `getLatencySamples() == 16`. Pluginval's initial plugin-info line logs 0 from its JUCE host cache; that cache is refreshed in [`prepareToPlay`](https://github.com/juce-framework/JUCE/blob/8.0.6/modules/juce_audio_processors/format_types/juce_VST3PluginFormat.cpp#L2845). The plugin's [VST3 wrapper](https://github.com/juce-framework/JUCE/blob/8.0.6/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp#L3570) forwards the processor value directly. A separate prepared-host latency query and per-DAW compensation/bounce comparison were not run; those remain acceptance work.

## Next work

Proceed to Phase 6 factory-preset sound design and listening review. Complete physical Windows/DAW/DPI acceptance, real audio-device deadline/underrun and engine-blend stress, the separate Steinberg validator, interactive installer tests, release signing and licensing. Full modulation-matrix expansion remains a separate feature.
