# Validation record

Development snapshot 0.6.0. The earlier 0.1.1/0.2.0 Windows evidence below is retained as the previous baseline. A passing DSP test is not proof that a host can load the VST3.

## Executed locally

Linux x86_64, GCC 14.2.0, C++20, `-O2 -Wall -Wextra -Wpedantic -Werror`.

- Native DSP compilation: PASS.
- Rendering before preparation safely returns silence: PASS.
- DSP suite: **1,387 checks PASS**.
- AddressSanitizer + UndefinedBehaviorSanitizer: same suite PASS with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer is unavailable in the sandbox/ptrace environment; leak freedom is not claimed.
- Overlapping identical notes release one voice at a time: PASS.
- Note-offs for stolen onsets preserve newer notes of the same pitch: PASS.
- 382 factory parameter sets: unique, audible at the category test note, finite and bounded.
- Quietest onset RMS at 48 kHz over 8,192 samples: 0.000253984.
- Eight synthesis engines at 44.1, 48, and 96 kHz: PASS.
- A4 tuning and two-semitone pitch bend: PASS.
- Sustain channel isolation, note release, zero-velocity note-on, preview timeout / stop, CC120: PASS.
- Voice limit and stealing, legato held-key return, glide code path: PASS.
- All 13 effect controls measurably alter output: PASS.
- Zero heap allocations while rendering and handling core MIDI events: PASS.
- Extreme parameter automation and rate reinitialization: finite / bounded PASS.

These are automated numerical checks. They do not establish perceptual quality, real-time scheduling performance, exact anti-aliasing performance, or that all presets are musically distinct to a listener.

## JUCE integration executed locally

The application/test C++ sources compiled with strict warnings against pinned JUCE 8.0.6 headers. All required JUCE modules compiled and linked into `build-juce-check/gba2-integration` with GCC. The **112 integration checks PASS**:

- MIDI event sample offsets and audible audio through `GbaProcessor`.
- All 64 parameters, preset identity and editor size restored through DAW-style binary state.
- User preset Save As / disk reload / rename / delete, factory protection, invalid-name and malformed-state handling.
- Favorites / history persistence, host programs and previous/next/category navigation.
- GUI keyboard-state events reach the synth; panic releases voices.
- Master-volume attachment, pitch-wheel spring return, A preview and B release.
- D-pad category changes, default macro routing and runtime macro reassignment.
- START browser, SELECT editor, preset-display refresh, editor sizes and piano-octave preservation during resize.
- Actual component snapshot encoded as a single PNG: `docs/images/GBA2-editor.png`, 1920×969.

This is a Linux software-rendering/in-process test executable, not a VST3 host. The local build disabled ALSA/JACK, D-Bus and optional X extensions, used installed FreeType/fontconfig/HarfBuzz libraries, and required no display server or physical audio device. A writable font cache was configured inside the build directory. The normal Windows CMake targets retain JUCE's Windows device backends.

The main Linux render was visually compared with the supplied reference. Master-knob proportions, metallic shading, the wordmark, D-pad arrows and silver START/SELECT caps were refined. The Windows screenshot was subsequently captured and visually inspected as well (`docs/images/GBA2-editor-windows.png`). High-DPI scaling and physical mouse/keyboard interaction in a DAW still need review.

Additive output equivalence and the 32-voice benchmark are documented in [PERFORMANCE.md](PERFORMANCE.md).

## Windows build and VST3 host validation executed

The local machine has no Windows SDK or Visual Studio; the actual Windows build ran on GitHub's `windows-2022` runner using Visual Studio 2022 x64.

[Run 39004268093](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39004268093), application source commit `ee0b681eeadbdc63e72d8376986205846a06c4ba`:

- Visual Studio 2022 CMake configuration: PASS.
- Windows x64 VST3 and standalone compilation/linking: PASS.
- Windows CTest: 2/2 suites PASS (synthesis 2.13 s, plugin state/presets/UI 0.68 s).
- Inno Setup 6.7.1 installer and portable archive packaging: PASS.
- Downloaded artifact SHA256 matches GitHub's digest. The VST3 binary and standalone executable are AMD64 PE images.
- Portable ZIP integrity and bundle structure: PASS. PE import inspection confirms no separate MSVC runtime DLL dependency.
- Actual Windows component screenshot: captured and visually inspected at 1920×969; interactive high-DPI testing remains pending.

[Run 39004957883](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39004957883), commit `6c14e9ef66a5f57ea52a0201672006d3ad6ba803`, repeated Windows compilation and both CTest suites successfully, then validated the actual `.vst3` bundle with **pluginval 1.0.4 at strictness 10: SUCCESS**, random seed `0x7329198`.

Pluginval found one instrument, scanned all 382 program names, opened the VST3 and editor, processed audio at 44.1/48/96 kHz with 64/128/256/512/1024-sample blocks, restored state, exercised automation and editor lifecycle, tested concurrent parameter changes, and fuzzed parameters. Its separate Steinberg validator subtest was explicitly skipped because no Steinberg validator executable was supplied; that test remains pending. Pluginval success does not establish compatibility with every DAW.

The run packaged and uploaded Windows artifact `12090861809`, `G-Boy-Advance-2-Windows-x64`, archive SHA256 `1aa3b0ecb7edef835a499e57b9363ed697fd021c3b6c6fa61fafa5aeed28af0b`. Full pluginval output is included in the artifact at `build/validation-logs/pluginval.txt`. Installer compilation is verified; running installation/uninstallation on an end-user Windows machine remains pending.

The original export helper incorrectly omitted files whose names began with `build`, including the workflow and Windows script. The corrected inventory explicitly checks these entry points and the validator script; all published file hashes match the local source.

## Phase 2 UI revision 0.2.0

The updated editor and tests compile with GCC C++20 and strict warnings. The Linux software-rendered suite passes **171 checks**. Added coverage includes all four default macros, glide attachment, the circular preview hit area, the four focusable D-pad directions and neutral areas, overlay keyboard isolation and F1/F2 return, minimum-size browser/advanced layouts, and main-editor snapshots at 960/1280/1920 widths plus 125%, 150% and 200% render scales.

The mouse-event piano test requires a native window because JUCE's containment checks depend on a platform peer. It is compiled into Windows CI and intentionally excluded from the Linux runner, which has no display server. The pre-existing GUI MIDI-state/audio test still runs on Linux. Software snapshots do not establish behavior under Windows per-monitor DPI changes.

[Windows run 39007537286](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39007537286), source commit `5ed4948b2d182693f05a98d896cd92e59fd87f22`:

- Visual Studio 2022 x64 VST3 and standalone compilation/linking: PASS.
- DSP: 1,387 checks PASS (2.15 s).
- JUCE integration: **174 checks PASS** (1.85 s), including the additional native-window mouse-down/note-on, audio-engine delivery and mouse-up/note-off checks for C4.
- Pluginval 1.0.4, strictness 10: **SUCCESS**, seed `0x6076a20`. Steinberg's separate validator remains unconfigured/pending.
- Portable ZIP and Inno Setup installer compilation: PASS.
- Downloaded Windows artifact `12091647203`: SHA256 `ed915f93da94dfd6aeb06f10e2d90c6b8e87240aa7cf9c0adc09da420e1ca282`, verified against GitHub's digest; ZIP integrity and AMD64 PE checks pass.
- Seven UI snapshots are included under `build/ui-snapshots/`, with the 1920-wide main image at `build/GBA2-editor.png`. The documentation screenshots in `docs/images/phase2-*.png` are now the actual Windows renders and have been visually inspected.

A synthetic mouse event delivered to a real native Windows peer is an automated interaction check; physical mouse/touch, screen-reader and per-monitor DPI acceptance remain manual. The final typography revision uses original vector front-panel labels and Consolas in the text-heavy Windows panels. All recorded checks and current screenshots correspond to this revision. The final documentation commit changes only evidence/screenshots; application code and build settings match the validated commit.

## Phase 3 core synthesis 0.3.0

- Direct GCC 14.2 C++20 compilation with strict warnings: PASS.
- Existing engine/factory/effects suite: 1,387 checks PASS.
- Dedicated core-synthesis suite: 161 checks PASS, including all waveform spectra, filter response, ADSR timing, every voice limit, glide/mono/legato/velocity, both LFO routes, ten-minute clocks and allocation-free choice fades.
- ASan + UBSan: both DSP suites PASS with halt-on-error and leak detection disabled (sandbox limitation).
- JUCE processor/state/preset/UI integration: 171 checks PASS on the Linux software runner.
- Old/new behavioral probes and remaining boundaries: see [PHASE_3.md](PHASE_3.md).
- Windows [run 39009067423](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39009067423), source `a815bdd85f25382483ef93af0be4c41b5955491c`: Visual Studio 2022 x64 VST3/standalone build PASS; CTest 3/3 PASS (1,387 engine checks, 161 core-synthesis checks, 174 integration checks).
- Actual Windows VST3: pluginval 1.0.4 strictness 10 SUCCESS, seed `0x78f250d`. Its separately configured Steinberg validator remains pending.
- Windows portable archive and Inno Setup installer compilation PASS. Artifact `12091714487`, SHA256 `83e215e66b01ff1291f072190dd44c0039762900f6a216d4f41130871b5fe4b0`: downloaded digest, ZIP integrity, AMD64 binaries and packaged test logs verified.
- Linux CI CMake build and both DSP CTest suites PASS. No local CMake execution is claimed; CMake is not installed in this workspace.
- The final documentation commit changes evidence only; application source/build settings match the validated commit.

## Manual host acceptance matrix

| Check | FL Studio | Ableton Live | REAPER | Cubase | Studio One |
|---|---|---|---|---|---|
| Scan / instantiate / stereo routing | Pending | Pending | Pending | Pending | Pending |
| MIDI notes / pitch / CC1 / sustain | Pending | Pending | Pending | Pending | Pending |
| Automation / macros / multiple instances | Pending | Pending | Pending | Pending | Pending |
| Save / close / reopen project | Pending | Pending | Pending | Pending | Pending |
| Offline bounce / 44.1, 48, 96 kHz | Pending | Pending | Pending | Pending | Pending |
| Resize / high DPI / keyboard / every button | Pending | Pending | Pending | Pending | Pending |

Compare the real editor against the approved purple reference at 100%, 125%, 150%, and 200% Windows scaling. Inspect all overlay pages and user-preset failures, rapid preset changes, same-note overlaps, mono sustain, stolen-note releases, and device restart.

## Phase 4 advanced synthesis 0.4.0

- GCC 14.2 C++20 with strict warnings: PASS.
- Engine/factory/effects 1,387 checks, core synthesis 161 checks and advanced synthesis 150 checks PASS: 1,698 total DSP checks.
- JUCE processor/state/preset/UI integration: 171 checks PASS on the Linux software runner.
- New spectral/tuning/transition coverage and quantified before/after probes: [PHASE_4.md](PHASE_4.md).
- Windows [run 39010199255](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39010199255), source `96ec03421e9596811f2af94b7a9e40dd63e2ddfa`: Visual Studio 2022 x64 VST3/standalone build PASS; CTest **4/4 PASS** (1,387 engine, 161 core, 150 advanced and 174 integration checks).
- Actual Windows VST3: pluginval 1.0.4 strictness 10 **SUCCESS**, seed `0x746a3a`, version 0.4.0. Separate Steinberg validator remains pending.
- Portable archive and Inno Setup installer compilation PASS. Artifact `12092806969`, SHA256 `b12b8ab46a0873b3a608ea535e2bbc383f6e0a0e96927d71fad624e3c08556a2`: downloaded digest, ZIP integrity, AMD64 binaries and packaged logs verified.
- Linux CI CMake build and all three DSP CTest suites PASS. Factory JSON and compiled data regenerate byte-for-byte.
- Final documentation commit changes evidence only; application/build source matches the validated commit. Manual host/device, DPI, listening and installer acceptance remain pending.

AddressSanitizer and UndefinedBehaviorSanitizer pass all three DSP suites with halt-on-error. Leak detection is disabled because LeakSanitizer is unsupported by this sandbox/ptrace environment; leak freedom is not claimed.

## Phase 5 effects 0.5.0

The new focused effects suite covers exact bypass/latency/reset, nonlinear spectral behavior, fractional-delay bounds/precision, echo timing/feedback/crossfades, reverb decay/stereo, modulated delays, phaser frequency response, measured shelving gains, continuous wow/flutter, crusher levels/hold times, width, combined automation, zero allocations and block-size invariance. See [PHASE_5.md](PHASE_5.md). All 1,836 DSP checks and 175 Linux JUCE integration checks PASS with strict GCC C++20 compilation. All four DSP suites pass ASan/UBSan with halt-on-error and leak detection disabled (sandbox limitation). Factory data regenerates byte-for-byte. Windows [run 39011625622](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39011625622), source `5c9ab5fa3a38a5b4982c12aa36a4bd8bf0c3cddc`: Visual Studio 2022 x64 VST3/standalone build PASS; CTest **5/5 PASS**, including 138 effects and **178 integration checks**.

Actual VST3 pluginval 1.0.4 strictness 10 **SUCCESS**, seed `0x796c4ad`. Portable ZIP and Inno Setup installer compilation PASS. Artifact `12093173038`, SHA256 `9ab9b62a98d4634e127eebf16f8a0dfb904856cdc5544fc12780328b2597e468`: digest, ZIP integrity, AMD64 binaries and test logs verified. Linux CI CMake/four DSP suites PASS. Final documentation changes evidence only.

Processor latency is 16 samples in integration tests and matches the measured DSP impulse group delay. Pluginval's initial information line prints 0 from its host cache; the pinned JUCE host refreshes it during preparation. A separate prepared-host latency query and manual DAW compensation/bounce tests were not run. See PHASE_5.md for source references. The separate Steinberg validator, physical UI/DPI, real device/DAW, listening and installer acceptance remain pending.

Host tail reporting derives from release time plus delay and room feedback decay to -100 dB relative to their initial level, with a small processing margin. It replaces the fixed 30-second estimate, which could truncate long echoes. Integration checks cover a short dry tail, one echo and the maximum high-feedback case (over two minutes). Actual DAW export-tail behavior still needs manual acceptance.

## Phase 6 factory bank 0.6.0

Explicit designs, engine-aware variants, measured levels, legacy identities/program order, multi-register/velocity/release tests and dry spectral/envelope analysis are documented in [PHASE_6.md](PHASE_6.md). Strict GCC C++20 builds pass 6,231 DSP checks (1,836 existing plus 4,395 factory-audit checks), and Linux integration passes 177. All five DSP suites pass ASan/UBSan with halt-on-error and leak detection disabled (sandbox limitation). Windows [run 39037896323](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39037896323), source `2214c4dc58d5105b95e00cc2d85f9a9e88baaa81`: Visual Studio 2022 x64 VST3/standalone build PASS; CTest **6/6 PASS** (92.99 s), including **4,395 factory checks** and **180 integration checks**. Actual VST3 pluginval 1.0.4 strictness 10 **SUCCESS**, seed `0x56a454`, version 0.6.0. Portable archive and Inno Setup installer compilation PASS.

Windows artifact `12102966872`, SHA256 `ac27fcda39f631b7845bfb4b1878d0109611fa71674686fb3b65e74ae9658f87`: downloaded digest, ZIP integrity, AMD64 binaries, packaged logs and final 382-preset JSON verified. Separate audition artifact `12103036745`, SHA256 `006c50a93715550606aeeafe4b8d81775053546a6eb2385affd207ddf7087488`: digest/ZIP verification, 17 stereo/48 kHz/PCM16/six-second WAVs, index, 1,528 render rows and 382 similarity rows PASS. WAVs are synthesized test outputs and are never part of the runtime bank.

The [factory audit report](FACTORY_LIBRARY.md) is generated from the Windows CSVs: central max-100ms stereo RMS spread 10.41 dB (0.0108593–0.036), peak 0.214519, 382 dry spectral/envelope groups, zero nearest-neighbor flags below 0.025 and all 191 base/Afterglow pairs above that threshold. 312/382 patches are within 0.5 dB of their category target; 85 master gains reached the ceiling. Human/perceptual listening acceptance remains required.

Ubuntu CI CMake and all five DSP suites PASS. Factory data regenerates byte-for-byte and all legacy identities/program indices match. Final evidence/documentation changes preserve the validated application/build source. Manual DAW/device, physical UI/DPI, prepared-host latency/compensation/bypass/export-tail, Steinberg-validator and installer acceptance, plus licensing/signing, remain pending.

## Phase 7 synthetic vocals 0.7.0

See [PHASE_7.md](PHASE_7.md). Strict GCC C++20 builds pass all **7,307 DSP checks** (1,387 engine + 161 core + 150 advanced + 138 effects + 1,076 vocal + 4,395 factory), including 150 vocal presets at 44.1/96 kHz in low/high registers in addition to the 48 kHz full-bank grid. Linux JUCE integration passes **177 checks**. Factory data regenerates byte-for-byte and legacy identities/program order are preserved. All six DSP suites pass ASan/UBSan with halt-on-error and leak detection disabled (sandbox limitation). Windows [run 39039531021](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39039531021), source `bdfdec3fb161429bd0b0c9a99183c0de59e0dd5e`: Visual Studio 2022 x64 VST3/standalone build PASS; CTest **7/7 PASS** in 90.58 s, including **180 integration checks**. Actual VST3 pluginval 1.0.4 strictness 10 **SUCCESS**, seed `0x89765b`, version 0.7.0. Portable ZIP and Inno Setup installer compilation PASS.

Windows artifact `12103364347`, SHA256 `5d847a9f44e6248b2c529c0aad2282726bac60994650a45490786a62e5bb7dd8`: digest, ZIP integrity, AMD64 binaries, packaged test/validator logs and final parameter-only bank verified. Audition artifact `12103930878`, SHA256 `a159bcf853a0818408b145efd67c76b11fa4735a8157f8f213ea79f4fa3bb574`: digest/ZIP, **29** six-second stereo/48 kHz/PCM16 synthesized excerpts, two indices, 1,528 render rows and 382 similarity rows verified. No WAVs or authoring dependencies are in the portable plugin package.

Windows numerical results agree with GCC at reported precision: 382 dry groups, zero nearest-neighbor flags below 0.025, all 191 base/Afterglow pairs above that threshold, 346/382 within 0.5 dB of category targets, 41 gain-ceiling cases (one vocal). Vocal central max-100ms stereo RMS is 0.01425–0.02000, peak 0.124520. See FACTORY_LIBRARY.md for category data. Ubuntu CMake/all six DSP suites and generation reproducibility PASS. Final documentation changes preserve the validated application/build source.

The formant DSP revision changes the sound of engine 5 despite compatible saved parameters. Human/perceptual listening, full-register articulation, target DAW/device/DPI/installer acceptance, separate Steinberg validation, prepared-host latency/compensation/bypass/export-tail behavior and release licensing/signing remain pending. The expanded vocal engine costs more CPU; PHASE_7.md records a local throughput probe, which is not a real-time deadline guarantee.
