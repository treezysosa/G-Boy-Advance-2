# Phase 10 finalization — 0.10.1

The 0.10.0 profiler exposed excessive CPU during rapidly retargeted engine blends. This patch addresses that implementation issue and adds retained dependency notices and a signing script. Final Windows validation passes in run 39059534979; successful 0.10.0 results are historical evidence in PHASE_10.md.

## DSP change

Engine transitions now finish each 5 ms two-engine crossfade before starting the latest requested engine. Requests are sampled every audio frame; intermediate choices during an unfinished fade are coalesced. At most two engines run concurrently. A stable request begins within 5 ms and settles within about 10 ms, without dropping the current blend abruptly. Reset/prepare immediately chooses the selected engine. Waveform/filter/LFO routing retain their existing interruptible fades. This intentionally changes the behavior of very rapid engine automation while retaining ordinary engine fades and all parameter IDs.

Shared glide and oscillator-2 detune calculations run once per sample instead of once per voice, including smoothed automation. Zero-depth LFOs advance their clocks without evaluating an inaudible sine. The modal engine computes one exponential decay and derives the other five partial decays as powers in double precision, preserving the original decay-automation formula. Polyphony remains 1–32 voices; no audio-thread allocation/locks or voice-limit reduction is introduced.

New regression checks verify two-engine bounds, gain normalization, fade continuity, reset and latest-target convergence at 44.1/48/96 kHz. The profiler adds alternating modal/additive and modal/retro workloads: 66 cases total. Timing remains observational on a shared VM and is not a hardware/DAW underrun certificate.

Initial remeasurement after shared-coefficient/modal changes gives identical central metrics to the prior Linux audit. After the triangle optimization, the largest absolute 100 ms RMS change across all 410 patches is 4.5e-10 (relative 1.25e-8), peak change 1.49e-8 and DC change 8.34e-10; all master volumes are unchanged. The calibration fingerprint is refreshed after this verification. Factory JSON and embedded FactoryData arrays remain byte-identical; all IDs/program positions and schema remain stable. Full numerical/sanitizer/Windows and audition regression results are recorded below.

## Packaging

Eighteen upstream JUCE 8.0.6/dependency notices are retained under docs/licenses with original paths and Git blob hashes. The check script permits only checkout newline conversion. Portable and installer packages include these notices. This does not choose a source/JUCE license or imply all listed libraries are active on Windows.

`sign-release-windows.ps1` uses an owner's existing valid code-signing certificate/private key in the Windows certificate store. It signs/verifies SHA256 timestamped payloads, repackages, then signs/verifies the installer and writes public signature metadata. No signing certificate/private key is configured in this environment; syntax can be checked here, signing/trust cannot be certified without that credential. Do not share private keys/passwords in chat or the repository.

## Remaining external acceptance

The licensing route is awaiting the owner's choice. Named DAW/device, real compensated bounce/export tails, physical DPI/accessibility, human preset listening, separate non-admin/interactive wizard and signed-package acceptance remain pending in RELEASE_CHECKLIST.md. The environment provides Linux development and Windows CI, with no access to a user's audio interface, physical monitors or installed commercial DAWs. These entries cannot be changed to PASS from source inspection or numerical tests. No production release/tag or false compatibility claim will be made.

## Numerical optimization check

`Tests/ModalReferenceProbe.cpp` (optional benchmark target `gba2-modal-reference-probe`) renders 54 modal cases: 44.1/48/96 kHz, MIDI 24/60/96, decay .005/.4/4 seconds, velocities .4/1. A GCC `-O2` comparison against the preceding direct six-exponential implementation produces **3,385,800 identical float samples**. This is differential evidence for that grid, not a proof for all automation/notes/compiler configurations.

The first bounded-transition Windows candidate passed all functional suites and validators, but the newly added modal/retro workload still measured 1.0915 times budget on average. The modal/LFO and triangle optimizations address this additional case; the final Windows results follow below.

## Triangle hot path

The modal/retro pair remained slightly over budget after the modal optimization (Windows mean/budget 1.0114 in run 39058257788). The final candidate computes the identical eight odd-harmonic triangle definition with a double-precision trigonometric recurrence (two trigonometric calls instead of eight). The harmonic count / Nyquist bound, coefficients and output gain remain the same; high-frequency single-harmonic output retains the original direct sine. Both subtractive and retro engines use it. New tests compare against the direct sine sum over six registers and three sample rates, requiring max error below 1e-6 and error below -120 dB, alongside existing harmonic/alias/DAC tests. This is a numerical optimization; bit-identical audio is not promised at quantizer boundaries.

## Final verified Windows results

Application/build commit `f6891afa8d2facb4fa83b9dc2c0f7a8fd24ea253`, [Windows/Ubuntu run 39059534979](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39059534979):

- VS2022 x64 VST3 and standalone PASS. All **nine CTest suites PASS**, 134.97 seconds total; application suite 3.08 seconds; actual VST3 host suite 0.32 seconds.
- **8,007 DSP assertions**, **286 Windows application assertions**, **209 actual-VST3 host assertions**. Host latency remains 16 samples at all tested rates/block sizes. IDs/schema/program positions are stable.
- pluginval 1.0.4 strictness 10 SUCCESS, seed `0x5552b94`. Official Steinberg validator: **47 passed, zero failed**, exit 0. The existing duplicate compatibility-metadata warning is retained; audio processing is float on native x64 binaries.
- Clean default-common-VST3 install, installed binary hashes, notice manifest/manual/bank, same-version reinstall, three-second installed standalone liveness and uninstall/user-data preservation PASS. This remains a clean admin CI VM test, not device audio or interactive/non-admin/old-version-upgrade acceptance.
- All **66 native callback workloads / 10,142 callbacks** produce finite audio. Worst mean/budget **0.88767**, worst p99/budget **0.95865**; **two calls exceed budget**, maximum **1.06545**. The original rapid-cycle group has **zero overruns**, mean/budget at most **0.74079**, compared with 812 overruns / 2.2744 worst mean in 0.10.0. Modal/additive has zero overruns, mean 0.84167; modal/retro has two overruns, mean 0.88767. The accumulating-engine cost and sustained mean-budget failures are addressed. Two observed excursions remain; this short shared-VM/native-DSP profile is not a guarantee of real-driver/GUI-loaded deadlines. See PERFORMANCE.md.
- All seven fully instrumented DSP suites PASS **ASan/UBSan, 8,007 checks**, halt-on-error. Application plus DSP sources in the integration executable PASS **281 checks**; cached JUCE module objects in that application executable are uninstrumented. Leak detection is disabled. Linux CI also passes all seven native suites and deterministic factory/notice checks.
- Triangle reference comparisons at 44.1/48/96 kHz measure max absolute error **4.77e-7 / 4.77e-7 / 5.96e-7**, SNR **140.922 / 140.935 / 140.482 dB**, across six registers. The modal reference grid retains 3,385,800 identical float samples on GCC. Those are measured grids, not universal bit-compatibility claims.

Downloaded Windows artifact `12110608147`, SHA256 `0d430dce404e69d70a3753400e34488c18059ee48fc743d062ab056a2a4b7104`: digest/ZIP integrity, AMD64 VST3/standalone PE headers, nine-suite/validator/installer logs, portable 410-parameter-preset bank and all 18 packaged upstream notice texts verified. No runtime audio or authoring banks are present.

Factory/vocal artifact `12110448564`, digest `4eb7f554a299deb4ec0645fe61b9967c38b1dabdfb244eaf35c7c8f5770dd52d`; retro artifact `12111350595`, digest `1d26ce52cda0b8c247b7521aca4114bc4e892e7ca40b603670ced0151247202c`: integrity, 1,640 audit / 410 similarity rows, three indices and all 57 six-second stereo/48k/PCM16 excerpts verified. Compared with 0.10.0, **55 WAVs are byte-identical**; the other two differ in only 12 and 11 samples respectively, each by at most one PCM16 step (SNR 103.86 / 100.94 dB). Factory JSON and embedded parameter arrays remain byte-identical. Numerical level changes are documented above; human sound-quality acceptance remains separate.

The source package contains **98 source/assets**, including the reference probe, signing/check scripts, all notices, workflow and tests. Final evidence-only documentation updates preserve this validated application/build source. Source commit/tree and file hashes are recorded in `dist/build-record.json`; all five distribution files are checked by `dist/SHA256SUMS.txt`. Installer SHA256 `9dbcfd2fdd6bfed336fbf274e1b34b63ed1c7a417e03d5d6130947fc2f7036fd`; portable SHA256 `7127d6a74349807365fb658cd944f274e07b6ffe0484ff0d1973f5a6ab230214`.

Code/performance work and available automated validation are complete. **Full manual release acceptance is not claimed**: the named DAW/device, physical DPI/accessibility, listening and non-admin/interactive installer matrix remains pending. Choose source/JUCE distribution terms before public distribution; an owner certificate is needed only for optional signed packages. These are unsigned development artifacts, without a production release tag.
