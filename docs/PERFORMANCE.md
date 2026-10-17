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

Or compile `Tests/Benchmark.cpp` and `Source/dsp/SynthEngine.cpp` with a C++20 compiler and `-ISource`. Output is CSV: engine index 0–7 and seconds per rendered second. Engine order is subtractive, FM, additive, wavetable, plucked string, formant, chiptune, modal keys.

Native host scheduling, multiple plugin instances, effects extremes, automation, and low-latency audio-device behavior still require profiling on Windows. A low average render time alone does not establish that every callback meets its deadline.
