> Current update: **0.16.3**, unified Vocals category: 74 unchanged sounds in one folder, 574 factory presets across 18 categories. DSP and SoundFont processing are unchanged from 0.16.2. [Current changes and validation](RELEASE_0_16_3.md). Earlier results below describe their historical baselines.

> Current update: **0.16.2**, sustained pad/string and SoundFont CPU optimizations. All 574 factory dictionaries and program/parameter identities are retained. Windows/Ubuntu suites, both VST3 validators, matched CPU/audio probes and installer upgrade checks pass. [Current evidence](RELEASE_0_16_2.md). Earlier results below retain their historical baselines.

> Current update: **0.16.1**, 574 active factory presets in 20 categories. Four vocal categories are removed; all surviving sound dictionaries, original host numbers and DSP are unchanged. Windows/Ubuntu suites, both plugin validators, seventy-six old-project migration/echo cases and installer upgrade checks pass. The 694-scene audio probe is byte-identical to 0.16.0 DSP with identical current arrays. [Current evidence](RELEASE_0_16_1.md). Earlier results below retain their historical baselines.

> Current update: **0.16.0**, 650 presets with all 530 previous patches preserved. The Windows/Ubuntu build, full preset audits, actual-bundle validators and installer upgrade checks pass. The 770-scene audio probe is byte-identical to 0.15.0 using the same current arrays; the prior DSP optimizations are retained. This preset expansion does not claim an additional DSP speedup. [Current evidence and limits](RELEASE_0_16.md). Earlier results below retain their historical baselines.

> Current update: **0.15.0 — twenty skins, factory diversity and CPU work**. [Windows/Ubuntu run 39254648988](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39254648988) passed nine Ubuntu DSP suites and twelve Windows CTest suites: 629,294 native DSP/preset checks, 1,225 Windows application checks (122 library / 164 SoundFont), twenty skin/state snapshots, two actual-bundle hosts, pluginval strictness 10, Steinberg 47 passed / 0 failed and six installer lifecycle checks. The synthetic FL-named host is not FL Studio. [Current measurements and limits](RELEASE_0_15.md). Earlier records below remain historical.

> Current update: **0.14.1 — launch colours, SF2 drops, library folder and startup recall**. [Windows/Ubuntu run 39246825673](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39246825673) passed eight Ubuntu DSP suites and eleven Windows CTest suites, including 774 Windows application checks (121 library and 164 SoundFont checks), three separate OS-process colour launches, native JUCE peer drag/drop dispatch, default/custom folder and last-bank restart checks, two actual-bundle test hosts, pluginval strictness 10, Steinberg 47/0 and six installer lifecycle checks. The synthetic FL-named test host is not FL Studio. [Current evidence and limitations](RELEASE_0_14_1.md). Earlier records below remain historical.

> Current version: **0.14.0 — 530 factory presets, ten skins and audio CPU work**. [Windows/Ubuntu run 39196086290](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39196086290) passed eight Ubuntu DSP suites and eleven Windows CTest suites, including 622,854 native DSP/preset checks, 546 Windows application checks (121 library and 55 SoundFont checks), ten skin/state snapshots, two actual-bundle test hosts, pluginval strictness 10, Steinberg 47/0 and six installer lifecycle checks. The synthetic FL-named test host is not FL Studio. See [the current release evidence](RELEASE_0_14.md). Earlier release records below remain historical.

0.13.1 relocates ROM Hacks beside master volume. [Windows/Ubuntu run 39191191366](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39191191366) passed eight Ubuntu DSP suites and eleven Windows CTest suites, including 453 application checks, actual-bundle host/editor checks, pluginval strictness 10, Steinberg validation and six installer lifecycle checks. The installer upgrade test uses 0.13.0 as its baseline. The FL-named test host is not FL Studio. [Current release evidence](RELEASE_0_13.md).

# 0.10.2 FL Studio editor compatibility candidate

Windows/Ubuntu [run 39071110725](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39071110725), application/build `b9adc08fbdd05e21bc40198d6fba476bbec066b5`: ten Windows suites PASS (87.40 s), 8,007 DSP / 289 application / 265 normal-host / 266 FL-style-name host checks. Both actual-bundle native editor lifecycle runs pass with concurrent audio. pluginval 1.0.4 strictness 10 PASS, seed `0x7760ed3`; separate Steinberg validator 47 passed / 0 failed; five installer checks PASS. Installed binary version 0.10.2 and package digests/contents verified. Local changed application/DSP ASan/UBSan passes 282 checks (cached JUCE modules uninstrumented, leak detection disabled); eight software UI snapshots match the previous version.

**Open user-reported failure:** 0.10.1 standalone works, but FL Studio 20 loses the connection while opening the VST3 editor. 0.10.2 selects the Windows software renderer after native peer attachment and guards temporary zero-size bounds. A synthetic host named FL64.exe is not FL Studio; no actual FL Studio test or confirmed fix is claimed. See [FL Studio investigation](FL_STUDIO.md). Historical results follow.

# Validation record

## Latest — Phase 10 finalization, 0.10.1

[Windows/Ubuntu run 39059534979](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39059534979), validated application/build `f6891afa8d2facb4fa83b9dc2c0f7a8fd24ea253`: **nine Windows CTest suites PASS**, 134.97 seconds, **8,007 DSP / 286 application / 209 actual-VST3 host checks**. pluginval 1.0.4 strictness 10 SUCCESS, seed `0x5552b94`; separate official Steinberg validator **47 passed, zero failed**. Clean install/reinstall/standalone-liveness/uninstall checks PASS, installed hashes match and user fixture survives. All 18 packaged notice texts match pinned upstream content (checkout newline conversion allowed). Both VST3/standalone are AMD64 PE.

The CPU fix bounds simultaneous engines to two and optimizes shared coefficients, modal decay and triangle harmonics. All **66 workload means and p99 times fit the sampled callback budgets**; **two of 10,142 calls exceed budget**, maximum 1.06545×. Original rapid cycling now has zero overruns / worst mean 0.74079×, compared with 812 / 2.2744× in 0.10.0. The sustained mean-budget problem is addressed; these VM/native timings do not certify real-device deadlines. Source/test details and numerical comparison are in PHASE_10_FINALIZATION.md and PERFORMANCE.md.

All seven fully instrumented DSP ASan/UBSan suites PASS **8,007 checks**; the application integration executable with instrumented application/DSP sources passes **281 checks**, with cached JUCE modules uninstrumented and leak detection disabled. Triangle reference errors remain below 6e-7 / over 140 dB SNR across the measured grid; modal reference renders match 3,385,800 floats. All factory parameter arrays / IDs / schema / program positions are unchanged. Factory max RMS relative difference is 1.25e-8. Of 57 offline audition WAVs, 55 are identical to 0.10.0; two differ in 23 samples total, by one PCM16 step at most.

Final Windows artifact `12110608147` digest `0d430dce404e69d70a3753400e34488c18059ee48fc743d062ab056a2a4b7104`, factory/vocal `12110448564` digest `4eb7f554a299deb4ec0645fe61b9967c38b1dabdfb244eaf35c7c8f5770dd52d`, retro `12111350595` digest `1d26ce52cda0b8c247b7521aca4114bc4e892e7ca40b603670ced0151247202c`: downloaded digests/ZIP integrity, logs, 410-preset parameter-only payload, notices, 1,640 audit/410 similarity rows and all 57 six-second stereo/48k/PCM16 clips verified. No audio/authoring banks are in the plugin package. Final source ZIP contains 98 source/assets, including complete workflow/scripts/tests/notices; final evidence-only docs preserve validated application/build source.

**Available code work and automated validation are complete; full manual acceptance is not claimed.** Named-DAW/device, compensated bounce/tails, physical DPI/accessibility, listening and non-admin/interactive/old-version installer checks remain pending. Licensing terms need owner selection; code signing is optional and no signed build is claimed. See RELEASE_CHECKLIST.md. Earlier phase records below are historical; the 0.10.0 CPU blocker is superseded by these results.

## Phase 10 baseline, version 0.10.0

[Windows/Ubuntu run 39046609234](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39046609234), application/build commit `e55a05356d3dc0b1b2c462d761e58c0378d4cb86`: all nine Windows CTest suites PASS (129.41 s), 7,983 DSP assertions, 286 Windows application checks and 209 actual-VST3 host checks. Prepared VST3 latency is 16 frames at 44.1/48/96 kHz and 64/257/1024 blocks. Bypass note-offs, state reload, instances, message-thread serialization concurrent with audio, offline rendering, release and tail reporting pass. Linux integration/changed-application ASan/UBSan: 281 checks PASS; cached JUCE/DSP sanitizer scope is detailed in PHASE_10.md.

pluginval 1.0.4 strictness 10 SUCCESS, seed `0x5b67a8a`. Official Steinberg SDK validator pinned to `cc2adc90382dded9e347caf74e4532f1458715db`: **47 passed, zero failed**, exit 0. One duplicate compatibility-metadata warning is recorded. Float processing is supported; double-precision audio processing is not. Clean admin-VM silent install / same-version reinstall / three-second standalone liveness / uninstall: four acceptance statements PASS; installed hashes match and the unique user-data fixture survives. This does not certify physical audio, separate non-admin users or interactive wizard/old-version upgrades.

The 54-case Windows native profiler renders 8,298 callbacks with finite audio; **812 exceed their time budget**, all in interrupted 32-voice engine blends (worst mean 2.2744× budget). This is an unresolved release-blocking performance result. Short steady-engine cases observe zero over-budget calls; these are not real-driver deadline guarantees. See PERFORMANCE.md for exact workload and quiet-run Linux results.

Windows artifact `12105914337` digest `7659570018c946e72bdf5b3670a4e4b3bb91dd7b1a931a2c3c21d6ad8431ed24`, factory/vocal `12105904332` digest `cffaca14d03fd019086444aa3ba80e28c012d758e6fdd26324485ba89c955954`, retro `12106705232` digest `edbb04929503a870df816811176ed0e35d67b1c5966cc443205c5eee884efb08`: downloaded digests/ZIP integrity, AMD64 PE headers, nine-suite and validator/installer logs, final bank and exclusion of runtime audio/authoring banks verified. Audit CSVs contain 1,640 render and 410 nearest-neighbor rows; all 57 six-second stereo/48k/PCM16 WAV excerpts match Phase 9. Final source package contains 75 source/assets, with final evidence-only documentation updates after the validated build.

**Phase 10 is not fully complete.** Full named-DAW/device, compensated bounce/export tails, physical UI/DPI/accessibility, human listening, non-admin/interactive installer and performance gates remain pending, as do owner licensing/notices/signing. Use RELEASE_CHECKLIST.md for the complete evidence matrix and PHASE_10.md for results, artifact hashes, sanitizer boundaries and the corrected first harness timeout. No production release or named-DAW compatibility is asserted.

Earlier phase results follow as historical evidence.

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

## Phase 8 retro synthesis 0.8.0

Strict GCC C++20 builds pass **7,983 DSP checks** (1,471 engine, 161 core, 150 advanced, 138 effects, 1,076 vocal, 270 retro, 4,717 factory) and **181 Linux JUCE integration checks**. All 56 retro presets are tested in low/high registers at 44.1/96 kHz as well as the three-register/two-velocity 48 kHz full bank. The 410-preset bank has 410 dry spectral/envelope groups, zero nearest-neighbor flags below 0.025 and all 205 base/Afterglow pairs above that threshold. Legacy 382 identities/program positions and all 64 parameter IDs are preserved. All seven DSP suites pass ASan/UBSan with halt-on-error and leak detection disabled (sandbox limitation).

Windows [run 39041332464](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39041332464), source `879d51bdc5ac13e79dca921372ecc09583b850c5`: Visual Studio 2022 x64 VST3/standalone build PASS; CTest **8/8 PASS** in 135.25 s, including **184 integration checks**. Actual VST3 pluginval 1.0.4 strictness 10 **SUCCESS**, seed `0xb07ab3`, version 0.8.0. Portable archive and Inno Setup installer compilation PASS.

Downloaded Windows artifact `12104183297`, SHA256 `239bd30be01caa42fb56e6227acf05037c0ec7dd4eaefdc7274ffc9ade3fc89d`: digest/ZIP, AMD64 VST3/standalone, packaged eight-suite/pluginval logs and final parameter-only bank verified. Factory/vocal artifact `12103929157`, SHA256 `109cee45dd650456c9ce7bd8f4eff4f8dd81b48dce210573c5a3b7c2f9aeeaa2`: digest/ZIP, 29 six-second stereo/48 kHz/PCM16 clips, two indices, 1,640 render and 410 similarity rows verified. Retro artifact `12104068693`, SHA256 `dd06ad81017c5ac54bdc6bc8fad2a7f801500dd9e889fed56612fc18904ca243`: digest/ZIP, all 28 clips of the same format and index verified. No WAVs or authoring dependencies are in the portable plugin package.

The Windows-generated FACTORY_LIBRARY.md reports 366/410 within 0.5 dB of category RMS targets, 48 gain-ceiling cases (11 retro), 11.35 dB overall central RMS spread and zero dry near-neighbor flags. The 56 retro patches span central max-100ms stereo RMS 0.009749–0.025000, peak 0.136118. Numerical separation is not perceptual certification. Ubuntu CMake/all seven DSP suites and factory-data regeneration PASS. Final documentation changes preserve the validated application/build source.

See PHASE_8.md for original-synthesis design, engine-6 sound-change compatibility and measured CPU cost. Human/perceptual listening, full-register articulation, DAW/device/DPI/installer acceptance, separate Steinberg validation, prepared-host latency/compensation/bypass/export-tail behavior, callback deadlines and release licensing/signing remain pending.

## Phase 9 preset management 0.9.0

Strict GCC C++20 builds pass **81 focused library / 279 total JUCE integration checks**. Changed application/test sources pass ASan/UBSan with halt-on-error and leak detection disabled (sandbox limitation). Cached JUCE/DSP module objects are uninstrumented in that application run. The unchanged DSP passed its seven instrumented suites in Phase 8. No DSP, factory preset/recipe or parameter-schema source changed in Phase 9.

Windows [run 39044342090](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39044342090), source `19f6dac809009831fdc0c41adbf2104347b279ca`: Visual Studio 2022 x64 VST3/standalone build PASS; CTest **8/8 PASS** in 133.88 s, including **7,983 DSP / 284 integration checks**. The Windows integration run contains **83 focused library checks**, including the native locked-file replacement test: failed replacement retains original file bytes/model name. Three existing native peer/piano checks account for the other platform-specific assertions.

Actual VST3 pluginval 1.0.4 strictness 10 **SUCCESS**, seed `0x33ffb0e`, version 0.9.0. Portable ZIP and Inno Setup installer compilation PASS. Windows artifact `12104454655`, SHA256 `598fa9a29bfe3085ccd038d2bd082294c44a13ec414e3c9bb35213d961291870`: downloaded digest/ZIP, AMD64 binaries, packaged logs and parameter-only payload verified. Factory/vocal artifact `12105361741`, SHA256 `0fb5ab834715d38ed6da7af5c7b4f83ca3e1a1cf7dbcd28ca8d14f6d7205f960`: 29 synthesized clips, two indices and complete CSVs verified. Retro artifact `12105526318`, SHA256 `7249ac480b29108e09a059659a2e6fcccaac6b4cc5ba6f2449b40a5bc993e2ae`: 28 clips/index verified. Both audition digests and ZIP integrity pass; all 57 six-second stereo/48 kHz/PCM16 WAV payloads match Phase 8 byte-for-byte. No WAVs or authoring banks are in the portable plugin package.

Library/UI coverage includes AND/phrase/tag/source/category/favorite/recent filtering, strict UUID/type/size/name validation, duplicate IDs/names, atomic-write failures, metadata migration/corruption, sequential-instance merging, bounded MRU history, external edits/refresh, factory protection, deferred MIDI program service, arrow/Enter browsing/preview and empty-result/filter-preservation behavior. The 960-wide Linux and Windows browser renders were visually inspected. Ubuntu CMake/all seven DSP suites and factory generation PASS. Final documentation changes preserve the validated application/build source.

See PHASE_9.md for the application-only sanitizer scope, explicit-refresh workflow, synchronous non-audio disk operations and separate-process metadata race limit. Physical UI/DPI/accessibility, manual DAW/device/installer/listening acceptance, prepared-host latency/compensation/bypass/export-tail behavior, callback deadlines, separate Steinberg validation and release licensing/signing remain pending.


## 0.10.3 active-note CPU update

Windows and Ubuntu run [39083118131](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39083118131), commit `88c07f8c928b24981b37019e6e9ca9b4a7cb1d1b`, passed. Runtime changes reuse unchanged math, share envelope/LFO increments, cache filter normalization, track only moving smoothers, and use mirrored FIR history/conditional delay wraps. Parameter IDs, factory arrays, plugin identity, state schema, UI and 16-frame latency are unchanged.

Ten Windows suites passed: 8,007 DSP/preset, 289 application and 265/266 actual-bundle host checks. Six native editor cycles per host processed 1,190 and 1,159 concurrent audio blocks. The FL-named host remains a synthetic harness, not actual FL Studio. pluginval 1.0.4 strictness 10 passed with seed `0x242c1c5`; Steinberg passed 47 tests with zero failures. Five install/reinstall/diagnostic/standalone-liveness/uninstall checks passed; real-device audio and interactive installation are not established by those checks.

Matched Windows profiles measured 12.69% lower accumulated callback time at 8 voices and 7.45% lower at 32 voices. The output comparison matched every byte across 530 scenes / 8,970,240 finite float samples. One of 10,142 32-voice callbacks exceeded its budget (maximum ratio 1.104); the 8-voice profile had no overruns. PERFORMANCE.md records groups, workload and timing limits. No promised FL CPU percentage or hard real-time certification follows from this VM measurement.

Local GCC 14.2 `-O2` passed 8,007 checks and the same byte-identical comparison grid. Engine/Core/Effects ASan/UBSan passed 1,770 checks; leak detection was disabled because LeakSanitizer cannot run under the environment tracer. The separate 410-patch old/new central-note audit matched RMS, peak, DC and volume metrics exactly. The calibration source fingerprint was refreshed after that evidence; factory volumes, JSON/C++ parameter arrays and IDs remain unchanged.

Downloaded installer/portable/full validation ZIP digests and integrity were verified. Both instrument binaries are AMD64; the installer-only EXE and portable files match the tested artifacts exactly. All 18 notice hashes and the 410-preset parameter-only bank were checked. The VST3 SHA256 is `717dd58aaa1510b9f4511c6134cae003bfbfe8744a8abf005265e2051a7a4985`. No sampler functionality or runtime audio asset was added.

The first CI attempt stopped at the calibration source-fingerprint guard; the second comparison helper appended a target after the old project's early return. The final successful build uses the refreshed metadata and a minimal baseline DSP driver with matching C++20 Release flags. These corrections did not alter the validated runtime DSP. Later source documentation records results; shipped installer/portable binaries retain their exact tested bytes. The owner reported 0.10.2 working in the x64 FL launcher and requested lower chord CPU; actual FL Studio performance and acceptance of 0.10.3 remain to be checked by the owner.


## 0.11.0 skins/import release candidate

Run [39089737722](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39089737722), application commit `cfd38d9ef9a5d431b84353e3a3dfe3a70a7122f3`, completed successfully on Windows and Ubuntu. Ten Windows suites, 8,007 DSP/preset checks, 385 application checks (103 library), 265/266 synthetic-host checks, pluginval strictness 10, Steinberg 47/0, both PowerShell diagnostic fixtures and six installer upgrade/reinstall/uninstall checks passed. Five skin/main and advanced snapshots are retained. Source, parameter IDs, factory identity/state compatibility, imported-preset roundtrip/collision handling and artifact contents were checked; DSP and factory-bank bytes match 0.10.3. Downloaded installer/portable files match the validated AMD64 binaries.

The matched 530-scene audio probe is byte-identical. Native Windows timing versus 0.10.2 measured 14.76%/8.25% lower accumulated callback time at 8/32 voices, with zero observed over-budget calls in both profiles; 32-voice worst ratio 0.91455. This retains the earlier DSP improvements; it is not an additional skin-related audio optimization or a DAW CPU guarantee. Actual DAW/driver, physical DPI/accessibility/listening, fresh 0.11.0 install, native file dialogs and licensing remain as described in [candidate evidence](RELEASE_0_11.md) and [acceptance checklist](RELEASE_CHECKLIST.md). Licensing was explicitly deferred by the owner. No final release tag or signature is claimed.


## 0.12.1 synthesis-only restoration

[Windows/Ubuntu run 39096250046](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39096250046) passed all ten Windows suites: 8,007 DSP/preset checks, 398 application checks (including 103 library checks), 265/266 actual-bundle synthetic-host checks, pluginval strictness 10, Steinberg 47/0 and six installer lifecycle checks. The FL-named host harness is not FL Studio.

Local ASan/UBSan application checks: 391. Old embedded-audio state migration, sample-file rejection without modification, four-tab editor, five skins and synthesis patch import/export passed. Audio is byte-identical across 530 scenes. The 0.12.0-to-0.12.1 installer test preserves old sample preset and personal-audio fixture bytes. See [complete current evidence and limitations](RELEASE_0_12_1.md).


## ROM Hacks 0.13.0

[Windows/Ubuntu run 39188459523](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39188459523) passed eleven Windows CTest suites: 620,993 native DSP/preset checks (612,986 SoundFont checks plus 8,007 existing), 453 application checks including 55 SoundFont and 103 library checks, 265/266 actual-bundle synthetic-host checks, pluginval strictness 10, Steinberg 47/0 and six installer lifecycle checks. The FL-named host harness is not FL Studio.

Twelve SoundFont publications processed 2571 concurrent Windows blocks. State restores cached bank/instrument after the original file is removed, reports/silences missing banks, and keeps instances independent. Measured pitch/glide/attack/filter, 256 structured bank mutations and mono-mode coverage are included. Local own-source ASan/UBSan application: 446 checks; SoundFont DSP: 612,986; JUCE modules uninstrumented, LSan disabled for tracer. Source/ZIP/binary/notices checks passed. Original synthesis output remains byte-identical. [Full evidence and limitations](RELEASE_0_13.md).
