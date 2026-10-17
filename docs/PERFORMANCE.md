# Development performance measurements

These are wall-clock timings of the native DSP loop on the development Linux VM, not audio-deadline or DAW compatibility guarantees.

## Additive optimization

GCC 14.2.0, C++20, `-O2`; 96 kHz; blocks of 64 stereo frames; 32 held notes (MIDI 24–55), 24 harmonics, brightness 0.95, amplitude sustain 1. Each measurement renders one second of audio; initialization is outside the timed interval.

| Implementation | Render time | Relative time |
|---|---:|---:|
| Direct per-partial trigonometry / exponential weights | 0.9776 s | 100% |
| Sine recurrence and shared per-sample weights | 0.4881 s | 49.9% |

The optimized bank uses a double-precision sine recurrence and shares harmonic weights / prefix normalization across voices. Brightness still updates continuously on every sample. Partial counts remain limited below Nyquist.

A separate before/after comparison rendered 196,608 floating-point output samples: 44.1/96 kHz, MIDI notes 24/60/96/120, brightness 0.05/0.5/0.95, 24 requested harmonics. Maximum absolute difference: **4.9872324e-7**; signal-to-error ratio: **129.80 dB**. These are numerical comparisons, not a perceptual listening certification.

## Optional benchmark

```sh
cmake -S . -B build-perf -DGBA_BUILD_PLUGIN=OFF -DGBA_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-perf --parallel 2
./build-perf/gba2-benchmark
```

Or compile `Tests/Benchmark.cpp`, `Source/dsp/SynthEngine.cpp` and `Source/dsp/Effects.cpp` with a C++20 compiler and `-ISource`. Output is CSV: engine index 0–7 and seconds per rendered second. Engine order is subtractive, FM, additive, wavetable, plucked string, formant, chiptune, modal keys.

Native host scheduling, multiple plugin instances, effects extremes, automation, and low-latency audio-device behavior still require profiling on Windows. A low average render time alone does not establish that every callback meets its deadline.

## Phase 3 core smoke benchmark

The same `Tests/Benchmark.cpp` was compiled with GCC 14.2 `-O2` against the prior 0.2.0 source and 0.3.0, then run sequentially after other local compile/test jobs had finished. One second of audio per engine, 32 voices, 96 kHz, 64-sample blocks:

| Engine | 0.2.0 seconds | 0.3.0 seconds |
|---|---:|---:|
| Subtractive | 0.519948 | 0.496808 |
| FM | 0.382416 | 0.349760 |
| Additive | 0.541680 | 0.508282 |
| Wavetable | 0.482911 | 0.457603 |
| Plucked string | 0.374959 | 0.337368 |
| Formant | 0.367864 | 0.336436 |
| Chiptune | 0.302595 | 0.283635 |
| Modal | 0.612911 | 0.593665 |

This single-pass development-VM measurement shows no observed steady-state CPU regression from the precision/crossfade changes. It is not a statistical performance guarantee or a real host underrun/deadline test. Effects, routing, voice stealing, sample-rate changes, GUI load and discrete transitions need host stress measurements. Zero-depth tremolo/syllable/vibrato paths avoid unnecessary trigonometric evaluation; the new selector fades use fixed-size shared state.

## Phase 4 steady-state smoke benchmark

Same GCC 14.2 `-O2`, 32-voice / 96 kHz / 64-frame workload. The 0.3.0 and 0.4.0 binaries ran sequentially with other compilation/test jobs finished. One pass per engine:

| Engine | 0.3.0 seconds | 0.4.0 seconds |
|---|---:|---:|
| Subtractive | 0.469775 | 0.481589 |
| FM | 0.351911 | 0.358559 |
| Additive | 0.49565 | 0.506934 |
| Wavetable | 0.461547 | 0.496793 |
| Plucked string | 0.343685 | 0.343543 |
| Formant | 0.340574 | 0.346228 |
| Chiptune | 0.287108 | 0.28992 |
| Modal | 0.608348 | 0.602969 |

This single-pass VM comparison observes a small steady-state cost increase for most engines and a larger wavetable difference; it is not a statistical estimate. It excludes engine-switch spikes. Interrupted 5 ms engine transitions can evaluate several engines simultaneously, and physical-model activation clears a fixed delay buffer. Measure worst-case callback deadlines on Windows before commercial release.

## Phase 5 effects cost and memory

Same GCC 14.2 `-O2`, one rendered second, 32 held voices / 96 kHz / 64-frame workload. The 0.4.0 and 0.5.0 binaries ran sequentially after compilation/sanitizers finished, one pass per engine:

| Engine | 0.4.0 seconds | 0.5.0 seconds |
|---|---:|---:|
| Subtractive | 0.480312 | 0.518501 |
| FM | 0.357711 | 0.369607 |
| Additive | 0.495314 | 0.53382 |
| Wavetable | 0.457434 | 0.50934 |
| Plucked string | 0.343958 | 0.378082 |
| Formant | 0.342899 | 0.373638 |
| Chiptune | 0.29282 | 0.312826 |
| Modal | 0.611156 | 0.631771 |

True 2× FIR processing adds CPU in this single-pass comparison; the quality improvement is not free. The filters stay warm even when the nonlinear mix is zero, preserving phase/history and constant 16-frame latency. Only the nonlinear stage runs at 2×. Delay/modulation/tape float storage at 48 kHz decreases from 1,843,584 to 588,672 bytes (about 68% less), excluding reverb/FIR/voice storage. This is not a statistical timing estimate or a Windows audio-deadline guarantee. Profile worst-case callbacks with effects automation, GUI activity and interrupted engine blends on real devices.
