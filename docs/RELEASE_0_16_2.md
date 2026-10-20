# G Boy Advance 2 — 0.16.2

This update targets CPU use while playing built-in pads/strings and user-loaded ROM Hacks SoundFonts, especially chords and overlapping release tails.

## Processing changes

- Stable subtractive, additive and wavetable voices process in short runs with compact hot state. Exact per-frame caches retain shared chord LFO, pitch and filter calculations. Parameter smoothing, selector transitions and preview deadlines continue through the existing sample-by-sample path.
- Plucked-string delay storage is separate from the hot voice records. Prepared-engine copies retain independent delay memory through stable indices.
- Low/middle-register additive banks check their highest harmonic once. The original individual Nyquist tests remain for high notes. Full-band triangle sums expose their constant denominators without changing the recurrence or summation order.
- SoundFont playback caches immutable per-onset pitch/rate terms, identical filter coefficients, smoothing coefficients and glide coefficients. Active MIDI channels share identical vibrato/bend calculations. Bank pitch scaling, tuning, controllers, glide, preview timing and fractional-rate behavior are retained.
- Fully bypassed nonlinear processing retains all FIR histories while skipping an unused decimator output. The original limiter, saturation curves, oversampling, latency and effect histories are retained.

All 574 factory parameter dictionaries, calibrated gains, 650 host slots (76 reserved), 64 parameter IDs, twenty skins, user presets, saved-project migration and SF2 library workflows are retained. Voice limits stay at 1–32; this update does not reduce preset polyphony or shorten tails.

## Validation

[Download Windows installer 0.16.2](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39446025149/artifacts/12276169340) · [Portable/manual archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39446025149/artifacts/12276089363) · [Source](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/pad-string-cpu-0162). Close FL Studio and standalone, extract into a fresh folder, run `G-Boy-Advance-2-Setup-x64.exe`, then reopen FL64. User presets, banks and preferences are retained.

[Windows/Ubuntu run 39446025149](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39446025149) passed nine Ubuntu suites and twelve Windows CTest suites: 630,175 common DSP/preset checks, 2,884 Windows application checks (332 library / 164 SoundFont), both actual-bundle hosts (647/648), pluginval strictness 10, Steinberg 47 passed / 0 failed and six installer lifecycle checks. The synthetic FL-named host is not FL Studio.

The existing 694-scene / 10,985,472-float probe is byte-identical: **True**, maximum error 0.0, SHA256 `e11dc0441a54813b12522f57c77e667df090ab3572ad7e447fead6effffe07bd`. The additional 148 eight-second pad/string/SF2 scenes / 147,532,832 floats are byte-identical: **True**, maximum error 0.0, SHA256 `b82eb8711e49eb3cc033b810df7b3a83d2d386b75f73f8a55d94bbbc3e9dfa7a`. These compare the same compiler and drivers against immutable released 0.16.1 source.

Matched Windows measurements across all three rates, both callback sizes and both workloads, repeated in before/after/after/before order:

| Requested voices | Built-in pad/string DSP time reduction | SF2 fixture DSP time reduction |
|---:|---:|---:|
| 4 | 6.17% | 1.39% |
| 8 | 8.70% | 2.48% |
| 16 | 9.65% | 3.47% |
| 32 | 8.88% | 5.11% |

Positive percentages mean less measured render time; negative values mean more on that shared runner. Setup/warmup are excluded. These results are not predicted FL Studio CPU percentages or guarantees for arbitrary SoundFonts, computers and audio drivers.

Validated build commit: `39660883ee043b3f52927a27c8d25cabe4915359`. Downloaded installer/portable/full artifacts pass digest and ZIP checks; both runtimes are AMD64. Packaged factory JSON exactly matches all 574 previous dictionaries. All twenty skin pairs and nineteen pinned notices are checked. Pinned 0.16.1-to-0.16.2 upgrade, reinstall, standalone process liveness, diagnostics and uninstall retain user data. Process liveness is not a physical audio-device test.

Local native suites pass 630,175 checks and 2,872 application checks. ASan/UBSan passes 2,872 own-source application checks plus 180 instrumented native core checks. Cached JUCE objects are uninstrumented; leak detection is disabled under the environment tracer. The separate local 842-scene / 158,518,304-float comparison is byte-identical. Full factory/diversity audits pass with zero duplicate/review flags on the numerical grid; this is not human listening certification.

The numerical comparison uses immutable released source commit `6d7eddf567cdd100acb42e0328983950ee92761c` (0.16.1) with identical drivers and factory arrays. It covers the existing 694 scenes / 10,985,472 floats and an additional 148 eight-second sustained scenes / 147,532,832 floats: all 44 factory pad/string patches at 44.1/48/96 kHz plus sixteen SF2 scenes including fractional rates, transposed zones, irregular blocks, sustain, controllers, bend, glide, preview and long releases. The established acceptance tolerance remains maximum absolute error 1e-6 and signal-to-error above 110 dB if samples differ.

The new benchmark measures 672 held-chord/release-overlap workloads, each repeated twice per engine version in before/after/after/before order. Twelve built-in pad/string designs and two original synthetic SF2 fixtures cover 4/8/16/32 voices, 44.1/48/96 kHz and 64/256-frame callbacks. Setup and warmup are excluded; settings/render calls are included. Timing is observational on a shared runner, not a predicted FL Studio meter percentage or a guarantee for arbitrary user banks and devices.

Regression tests cover irregular host block partitioning at three rates, independent prepared-engine string buffers, allocation-free processing, spectral/alias behavior, the full factory library, old project state, UI and SoundFont workflows. Automated VST3 hosts are not FL Studio. Owner FL64, real banks, physical audio/MIDI devices, DPI and human listening remain manual acceptance work. The prepared build is unsigned; the owner's licensing/signing decision remains deferred.
