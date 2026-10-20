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

Windows validation and packaging are in progress. Verified installer links and measured results will be recorded after completion.

The numerical comparison uses immutable released source commit `6d7eddf567cdd100acb42e0328983950ee92761c` (0.16.1) with identical drivers and factory arrays. It covers the existing 694 scenes / 10,985,472 floats and an additional 148 eight-second sustained scenes / 147,532,832 floats: all 44 factory pad/string patches at 44.1/48/96 kHz plus sixteen SF2 scenes including fractional rates, transposed zones, irregular blocks, sustain, controllers, bend, glide, preview and long releases. The established acceptance tolerance remains maximum absolute error 1e-6 and signal-to-error above 110 dB if samples differ.

The new benchmark measures 672 held-chord/release-overlap workloads, each repeated twice per engine version in before/after/after/before order. Twelve built-in pad/string designs and two original synthetic SF2 fixtures cover 4/8/16/32 voices, 44.1/48/96 kHz and 64/256-frame callbacks. Setup and warmup are excluded; settings/render calls are included. Timing is observational on a shared runner, not a predicted FL Studio meter percentage or a guarantee for arbitrary user banks and devices.

Regression tests cover irregular host block partitioning at three rates, independent prepared-engine string buffers, allocation-free processing, spectral/alias behavior, the full factory library, old project state, UI and SoundFont workflows. Automated VST3 hosts are not FL Studio. Owner FL64, real banks, physical audio/MIDI devices, DPI and human listening remain manual acceptance work. The prepared build is unsigned; the owner's licensing/signing decision remains deferred.
