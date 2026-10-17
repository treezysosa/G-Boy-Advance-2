# Phase 10 finalization — 0.10.1

The 0.10.0 profiler exposed excessive CPU during rapidly retargeted engine blends. This patch addresses that implementation issue and adds retained dependency notices and a signing script. Final-candidate Windows verification is pending; successful 0.10.0 results are historical evidence in PHASE_10.md.

## DSP change

Engine transitions now finish each 5 ms two-engine crossfade before starting the latest requested engine. Requests are sampled every audio frame; intermediate choices during an unfinished fade are coalesced. At most two engines run concurrently. A stable request begins within 5 ms and settles within 10 ms, without dropping the current blend abruptly. Reset/prepare immediately chooses the selected engine. Waveform/filter/LFO routing retain their existing interruptible fades. This intentionally changes the behavior of very rapid engine automation while retaining ordinary engine fades and all parameter IDs.

Shared glide and oscillator-2 detune calculations run once per sample instead of once per voice, including smoothed automation. Zero-depth LFOs advance their clocks without evaluating an inaudible sine. The modal engine computes one exponential decay and derives the other five partial decays as powers in double precision, preserving the original decay-automation formula. Polyphony remains 1–32 voices; no audio-thread allocation/locks or voice-limit reduction is introduced.

New regression checks verify two-engine bounds, gain normalization, fade continuity, reset and latest-target convergence at 44.1/48/96 kHz. The profiler adds alternating modal/additive and modal/retro workloads: 66 cases total. Timing remains observational on a shared VM and is not a hardware/DAW underrun certificate.

Remeasurement of all 410 factory central notes gives identical volume, max-100ms stereo RMS, peak and DC metrics to the prior Linux audit, at its stored numeric precision. The calibration fingerprint is refreshed after this verification. Factory JSON and embedded FactoryData arrays remain byte-identical; all IDs/program positions and schema remain stable. Full numerical/sanitizer/Windows and audition regression results will be recorded here after completion.

## Packaging

Eighteen upstream JUCE 8.0.6/dependency notices are retained under docs/licenses with original paths and Git blob hashes. The check script permits only checkout newline conversion. Portable and installer packages include these notices. This does not choose a source/JUCE license or imply all listed libraries are active on Windows.

`sign-release-windows.ps1` uses an owner's existing valid code-signing certificate/private key in the Windows certificate store. It signs/verifies SHA256 timestamped payloads, repackages, then signs/verifies the installer and writes public signature metadata. No signing certificate/private key is configured in this environment; syntax can be checked here, signing/trust cannot be certified without that credential. Do not share private keys/passwords in chat or the repository.

## Remaining external acceptance

The licensing route is awaiting the owner's choice. Named DAW/device, real compensated bounce/export tails, physical DPI/accessibility, human preset listening, separate non-admin/interactive wizard and signed-package acceptance remain pending in RELEASE_CHECKLIST.md. The environment provides Linux development and Windows CI, with no access to a user's audio interface, physical monitors or installed commercial DAWs. These entries cannot be changed to PASS from source inspection or numerical tests. No production release/tag or false compatibility claim will be made.

## Numerical optimization check

`Tests/ModalReferenceProbe.cpp` (optional benchmark target `gba2-modal-reference-probe`) renders 54 modal cases: 44.1/48/96 kHz, MIDI 24/60/96, decay .005/.4/4 seconds, velocities .4/1. A GCC `-O2` comparison against the preceding direct six-exponential implementation produces **3,385,800 identical float samples**. This is differential evidence for that grid, not a proof for all automation/notes/compiler configurations.

The first bounded-transition Windows candidate passed all functional suites and validators, but the newly added modal/retro workload still measured 1.0915 times budget on average. The modal/LFO optimization addresses this additional case; its Windows performance results are pending.

## Triangle hot path

The modal/retro pair remained slightly over budget after the modal optimization (Windows mean/budget 1.0114 in run 39058257788). The final candidate computes the identical eight odd-harmonic triangle definition with a double-precision trigonometric recurrence (two trigonometric calls instead of eight). The harmonic count / Nyquist bound, coefficients and output gain remain the same; high-frequency single-harmonic output retains the original direct sine. Both subtractive and retro engines use it. New tests compare against the direct sine sum over six registers and three sample rates, requiring max error below 1e-6 and error below -120 dB, alongside existing harmonic/alias/DAC tests. This is a numerical optimization; bit-identical audio is not promised at quantizer boundaries.
