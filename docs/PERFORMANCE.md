> Current update: **0.17.1**, folders renamed to Detroit, West Coast, Florida and Chicago. All 598 sounds, DSP and program/parameter identities are unchanged. Windows/Ubuntu suites, both VST3 validators and the 0.17.0 → 0.17.1 installer upgrade passed. [Current evidence](RELEASE_0_17_1.md). Earlier results below retain their historical baselines.

> Current update: **0.17.0**, Brass contains 24 new presets and all 16 unchanged sax leads; 598 factory presets across 18 categories. Windows/Ubuntu suites, both VST3 validators, browser family filters, edited sax-project restoration and the 0.16.3 → 0.17.0 installer upgrade passed. DSP remains unchanged. [Current evidence](RELEASE_0_17_0.md). Earlier results below retain their historical baselines.

> Current update: **0.16.3**, **Vocals** combines all 74 retained vocal-category sounds; 574 factory presets across 18 categories. Windows and Ubuntu suites, both VST3 validators, edited legacy-category restoration and the 0.16.2 → 0.16.3 installer upgrade passed. DSP and SoundFont processing are unchanged from 0.16.2. [Current evidence](RELEASE_0_16_3.md). Earlier results below retain their historical baselines.

> Current update: **0.16.2**, sustained pad/string and SoundFont CPU optimizations. All 574 factory dictionaries and program/parameter identities are retained. Windows/Ubuntu suites, both VST3 validators, matched CPU/audio probes and installer upgrade checks pass. [Current evidence](RELEASE_0_16_2.md). Earlier results below retain their historical baselines.

> Current update: **0.16.1**, 574 active factory presets in 20 categories. Four vocal categories are removed; all surviving sound dictionaries, original host numbers and DSP are unchanged. Windows/Ubuntu suites, both plugin validators, seventy-six old-project migration/echo cases and installer upgrade checks pass. The 694-scene audio probe is byte-identical to 0.16.0 DSP with identical current arrays. [Current evidence](RELEASE_0_16_1.md). Earlier results below retain their historical baselines.

> Current update: **0.16.0**, 650 presets with all 530 previous patches preserved. The Windows/Ubuntu build, full preset audits, actual-bundle validators and installer upgrade checks pass. The 770-scene audio probe is byte-identical to 0.15.0 using the same current arrays; the prior DSP optimizations are retained. This preset expansion does not claim an additional DSP speedup. [Current evidence and limits](RELEASE_0_16.md). Earlier results below retain their historical baselines.

> Current measured update: **0.15.0**. Matched Windows note/chord/retrigger profiles measured 9.09%–13.85% less accumulated DSP time across one-, four- and eight-voice groups versus 0.14.1. The expanded 650-scene audio comparison is byte-identical. Histories, nonlinear quality, voice limits, full-rate modulation and 16-frame latency are retained. [Measurements, workload details and limits](RELEASE_0_15.md). Earlier entries below retain their historical baselines.

> Current measured release: **0.14.0**. Matched Windows notes/chords/retrigger profiles measured processing-time reductions ranging from 12.2% to 20.3% across one-, four- and eight-voice workload groups. These are native measurements on a shared runner, not FL Studio CPU percentages or physical-driver deadline guarantees. The 650-scene old/new comparison was byte-identical. [Windows/Ubuntu run 39196086290](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39196086290) passed eight Ubuntu DSP suites and eleven Windows CTest suites, including 622,854 native DSP/preset checks, 546 Windows application checks (121 library and 55 SoundFont checks), ten skin/state snapshots, two actual-bundle test hosts, pluginval strictness 10, Steinberg 47/0 and six installer lifecycle checks. The synthetic FL-named test host is not FL Studio. [Implementation, timings and limits](RELEASE_0_14.md). Earlier measurements below describe their recorded builds.

> Current version: **0.13.0, synthesis plus optional ROM Hacks SF2 instruments**. [SoundFont behavior, project paths and limitations](RELEASE_0_13.md). Earlier release records below remain historical.


# Development performance measurements

## 0.10.3 — active-note CPU work

The owner reports high CPU while playing notes/chords in the working x64 FL Studio setup. This revision removes redundant work while preserving the existing synthesis/effects algorithms:

- Only moving continuous parameters run the per-sample smoother. A parameter leaves that list at the exact float fixed point of the original recurrence, with no epsilon or snap; automation reactivates it.
- Envelope increments and LFO phase increments shared by voices are calculated once rather than dividing separately for every voice. Continuously changing envelope/rate controls retain their original update cadence.
- Per-note velocity square root and the unmodulated stereo gains are computed at note onset. Pan modulation retains the original per-sample gains.
- The filter normalization factor is updated with its tuning instead of dividing on every sample. Mirrored FIR history preserves tap order while removing per-tap wrap calculations; effect delay writes use a conditional wrap instead of division.
- Filter/formant/vocal configurations and EQ coefficients are reused only while their effective inputs match. Prepare/reset invalidates rate-dependent caches.
- Glide, oscillator detune ratio and additive harmonic weights are reused when their continuously smoothed inputs are unchanged. Nonzero pitch modulation still follows the existing per-sample path.

The local GCC 14.2 `-O2` old/new render comparison is byte-identical across 530 scenes and 8,970,240 finite float samples: all eight engines, all five filter types, three sample rates, automation/selector changes, sustain/release/stealing and chords for all 410 factory patches. The offline driver is `Tests/PerformanceProbe.cpp`; its raw float files are temporary test output, never plugin assets. The unchanged source baseline is commit `901538f3158d7925da8d5bc5dc6983c11e1cd2f5` (0.10.2 instrument code plus diagnostic tool).

Local GCC 14.2 `-O2` quiet-run profiles measured 15.59% less accumulated callback time at 8 voices and 6.79% less at 32 voices across the 66-case workloads. The corresponding steady-engine groups measured 17.43% and 7.85% reductions. There were 13 and 18 over-budget callback observations respectively (shared-runner scheduling can affect individual calls); these results do not certify audio deadlines. All 8,007 local DSP/preset checks passed. Native Engine/Core/Effects ASan/UBSan passed 1,770 checks; leak detection was disabled because LeakSanitizer cannot run under the environment tracer. A separate 410-patch old/new central-note audit matched the reported RMS, peak, DC and volume values exactly. Only the calibration recipe fingerprint was refreshed for the changed source; all factory volumes and generated JSON/C++ arrays remain unchanged.

Windows/Ubuntu [run 39083118131](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39083118131), build commit `88c07f8c928b24981b37019e6e9ca9b4a7cb1d1b`, passed both jobs. VS2022 Release old/new comparisons used identical drivers and compiler/runtime flags against the immutable 0.10.2 DSP baseline:

| Voices | All 66 workloads: less measured DSP time | Steady-engine group | Switching group |
|---|---:|---:|---:|
| 8 | 12.69% | 14.18% | 9.85% |
| 32 | 7.45% | 8.87% | 5.00% |

Windows renders were byte-identical across all 530 scenes / 8,970,240 finite float samples (SHA256 `e55c4d1fe95b5e9f9173df8238734647a403cdd39ff70fd7dc0e0e0dcf88009c` for both). The new 8-voice profile had zero over-budget callbacks (maximum ratio 0.3513); the 32-voice profile had one of 10,142 calls over budget (maximum ratio 1.104), in the modal/retro switching case. All case means and p99 values fit the sampled budgets. These short shared-runner timings exclude initialization and initial note bursts and do not certify real-driver deadlines or predict a user's FL Studio CPU meter.

All ten Windows CTest suites passed, with 8,007 DSP/preset checks, 289 application checks and 265/266 actual-bundle host checks. Both native editor hosts completed six lifecycle cycles with concurrent audio. pluginval strictness 10 and the separate Steinberg validator (47/0) passed; five installer checks passed. Package digests, ZIP contents, AMD64 binaries, the unchanged 410-preset payload and all 18 third-party notices were verified after download. Actual FL Studio/device/DPI/listening and 0.10.2-to-0.10.3 project/installer upgrade behavior remain manual acceptance work. `scripts/compare-cpu-windows.ps1` builds the immutable previous DSP with the same drivers as the new engine, compares the render grid and measures 66 cases each at 8 and 32 voices. It retains CSV and `cpu-comparison.json` evidence. These are observational native DSP timings on a shared runner, not FL Studio CPU percentages, listening certification or real-driver deadline guarantees.

### Settings for an existing installation

Start with SELECT → MODULATION & VOICES → Voice count = 8 (or 4 for simple parts), 44.1/48 kHz rather than 96 kHz, and a 256–512-sample FL Studio audio buffer. Fewer voices and a shorter amplitude Release reduce overlapping active tails; they can change the playing response. For additive patches, fewer harmonics reduce partial synthesis work. Save these choices as a user preset if needed; loading a factory patch restores that patch's settings. A larger buffer eases deadline pressure and increases latency; it does not remove the per-sample synthesis cost. Rendering completed instrument parts to audio in the DAW can free their live plugin processing.

Effects paths intentionally retain history and constant latency even at zero wet mix; turning a mix to zero does not necessarily eliminate that effect's CPU cost. Drive/distortion at zero skips nonlinear shaping, but the FIR history remains warm. No automatic silence suspension or reduced-quality rendering mode is introduced here.

## Historical — 0.10.1 finalization

[Windows run 39059534979](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39059534979), VS2022 Release, native DSP on a shared VM: 66 cases, 10,142 callbacks, all finite. Rates 44.1/48/96 kHz; 64/256-frame blocks; 32 voices; effects plus cutoff/delay automation. Each case measures about 0.25 rendered seconds after 16 warm-up blocks. Initialization, initial note bursts, output validation, real driver/host/GUI load and CSV writes are outside the timed region.

| Switching workload | Worst mean / budget | Worst p99 / budget | Calls over budget |
|---|---:|---:|---:|
| Rapid eight-choice requests (engine 8) | 0.74079 | 0.88717 | 0 |
| Modal / additive (engine 9) | 0.84167 | 0.89865 | 0 |
| Modal / retro (engine 10) | 0.88767 | 0.95865 | 2 |

All case means and p99 values fit their sampled budgets. Two of 10,142 calls are over budget, maximum 1.06545×; these excursions are recorded, not attributed to a cause without measurement. The original rapid-cycle group recorded 812 over-budget calls / 2.2744 worst mean in 0.10.0. The systematic accumulation and mean-budget failures are addressed; real-driver deadline/underrun acceptance remains pending.

Engine changes now finish each two-engine 5 ms fade before starting the latest choice. Other selectors retain their interrupted fades. Glide and detune factors are shared per sample; zero-depth LFO clocks skip sine work; modal decay uses one exponential plus powers. Triangle odd harmonics use a double recurrence instead of up to eight sine calls. Direct-sum tests measure over 140 dB SNR; the modal reference grid matches 3,385,800 float samples. All 410 measured factory levels remain stable within 1.25e-8 relative RMS, with parameter arrays unchanged. Two of 57 audition WAVs have only one-PCM16-step differences in 23 samples total.

Two expensive-pair cases were added to the original 54-case profiler. They exposed remaining modal/retro costs that rapid cycling alone missed, motivating the modal and triangle work. Full results, sanitizer scope and artifact evidence are in PHASE_10_FINALIZATION.md. Older measurements below are historical, including the superseded 0.10.0 release-blocking CPU result.

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

## Phase 10 callback profiler

The benchmark now runs 54 cases: 44.1/48/96 kHz, 64/256-frame blocks, eight engines plus an interrupted engine blend, 32 held notes, effects and cutoff/delay-time automation. Pass a filename to write CSV; without one CSV goes to stdout. Engine 8 denotes the blend workload. Each case warms 16 callbacks and measures about 0.25 seconds of rendered audio. The loop includes settings updates and render, excludes initialization, initial note-on bursts, output validation and CSV writes and reserves timing storage in advance.

CSV columns report callback count, mean/p50/p95/p99/max microseconds, sample-budget microseconds, maximum/budget ratio, observed over-budget calls and invalid output counts. Nonfinite output fails; timings do not impose a shared-VM speed threshold. Short runs, VM scheduling and the absence of real driver/host/GUI load limit interpretation. They do not measure actual audio underruns.

Linux GCC 14.2 `-O2`, rerun with local compilation/sanitizers finished: 8,298 measured callbacks, zero nonfinite output, 569 over-budget calls. The interrupted-blend workload accounts for 556, with worst case mean/budget 1.865 and p99/budget 2.526. The maximum individual ratio is 4.729; scheduler preemption can affect any wall-clock sample. An earlier run with other development activity recorded 759 over-budget calls; it is not used as the quiet-run timing estimate. These results do not satisfy a worst-case real-time deadline gate. Profile/optimize rapid engine transitions and 32-voice loads on target hardware before release. Lower polyphony / larger buffers are provisional workarounds, not certified configurations. Windows VS2022 Release, CI run 39046609234: 8,298 callbacks, zero nonfinite output, 812 over-budget calls, all in engine 8 (interrupted blends). The eight steady-engine groups observe no over-budget callbacks in this short run; their largest max/budget is 0.9195. Blend worst mean/budget is 2.2744, p99/budget 2.4228 and max/budget 2.4996. This confirms a real workload cost concern beyond isolated VM scheduling spikes. Rapid 32-voice engine switching remains a release-blocking optimization task; these measurements do not certify target hardware or a DAW/audio driver. Logs are in the Windows artifact at `build/validation-logs/callback-profile.csv`; the locally derived summary is alongside it.


## 0.11.0 UI work

The audio engine is unchanged from 0.10.3. Native controls, keyboard and LEDs remain live. Only the static vector housing is cached per editor; it is regenerated after skin, size or rendering-density changes. This avoids rebuilding gradients, lettering paths and moulded-plastic detail when repainting an LED or control. The cache consumes image memory proportional to editor size and DPI; it is released with the editor. If the image would exceed 16 Mi pixels, painting falls back to vectors rather than retaining a larger cache. No image filtering, frame animation or audio-thread work is introduced for skins.

This GUI change does not promise another reduction in chord DSP time. Continue using the measured 0.10.3 improvements and voice/release settings above. The 0.11.0 Windows run passed: byte-identical 530-scene audio, zero observed over-budget calls at 8/32 voices, and 14.76%/8.25% less accumulated measured DSP time versus the immutable 0.10.2 engine. This measures the retained 0.10.3 optimization; skins do not change the DSP. See [0.11.0 evidence and limitations](RELEASE_0_11.md).


## 0.12.1 synthesis-only restoration

The exact tested 0.11.0 synthesis DSP is restored. The sample worker, playback/interpolation path and embedded-audio instrument authoring are removed. The 530-scene comparison is byte-identical on both platforms. Windows native 32-voice timing observed 7/10142 over-budget calls, worst ratio 1.484138; this is one shared-runner observation, not a DAW CPU or physical deadline guarantee. See [current evidence](RELEASE_0_12_1.md).


## ROM Hacks 0.13.0

Bank decode/cache and reclamation run off the callback. A hazard pointer protects a single audio consumer; voices, 17 channels (including preview) and effects are prepared before publication. Native render/MIDI/reset observe zero decoder heap operations. Import uses a lazy worker; it sleeps when no job or retired bank needs work. The bank view/list is UI-thread metadata. Twelve Windows publications processed 2571 finite concurrent audio blocks. Complex/layered banks consume more of the 32 sample-voice budget and loader memory.

The old synthesis DSP/effects and factory calibration are unchanged; the 530-scene comparison is byte-identical. Its 32-voice shared-runner profile observed 0/10142 over-budget calls, worst ratio 0.835050. That profile excludes SoundFont mode and real DAW/device load. Measure actual banks at the owner's buffer size; no SF2 CPU percentage or physical deadline guarantee is inferred. [Evidence and limits](RELEASE_0_13.md).
