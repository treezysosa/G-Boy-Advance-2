# Phase 10 — Windows release validation

Development version 0.10.0. Automated Windows validation passed; full release acceptance remains pending. Plugin identifiers, 64 parameter IDs, schema 1, factory identities and DSP/factory values remain stable.

## New checks

`Tests/HostTests.cpp` is a separate JUCE VST3 host, linked without application/DSP source. CTest loads the built bundle, queries prepared latency at 44.1/48/96 kHz and 64/257/1024 frames, checks MIDI sample offsets, audio, bypass/note-off/resume, parameter state across newly loaded instances, isolation, SysEx rejection, message-thread serialization concurrent with worker-thread audio, offline mode, release and tail reporting. This establishes behavior in that host; it does not certify any named DAW or compensated bounce.

The processor overrides bypass to keep normal MIDI/envelope/effects processing and mute stereo output. Previously the inherited JUCE bypass discarded note-offs, freezing held notes. The bypass path retains normal CPU cost. A two-assertion in-process regression supplements actual-bundle acceptance.

The separate official Steinberg SDK validator is pinned to `cc2adc90382dded9e347caf74e4532f1458715db` (`v3.7.12_build_20`) including its submodule revisions, built only as a test tool. It and pluginval are not runtime dependencies.

`Tests/Benchmark.cpp` profiles 54 native DSP cases: three sample rates, 64/256 frames, all eight engines and an interrupted engine-blend workload, 32 voices, effects and cutoff/delay automation. It logs mean, p50/p95/p99/max callback time, budget ratios and observed over-budget calls. Timings are observational on a shared VM, not an underrun or hardware deadline certification. Nonfinite audio fails the run. This is not a VST3/GUI-loaded callback profile.

`test-installer-windows.ps1` refuses existing installations. On a clean disposable admin VM it silently installs to an isolated standalone directory and the real default common VST3 location, compares installed binary hashes, reinstalls, smoke-launches standalone for three seconds, uninstalls and verifies a unique user-data fixture survives. It removes only its fixture. The installed standalone liveness check does not test device audio. Non-admin use, interactive wizard/upgrade from old versions and physical machines remain manual gates.

The portable staging directory is rebuilt before each package, preventing stale files from shipping. The installer and project versions are synchronized at 0.10.0.

## Acceptance boundary

See RELEASE_CHECKLIST.md and VALIDATION.md. Signing, owner license selection, complete third-party notices, human listening, high-DPI/accessibility, non-admin and named-DAW acceptance remain pending. No production release/tag is created by this phase. Windows artifacts are unsigned development test builds.

## Verified results

Validated build commit `e55a05356d3dc0b1b2c462d761e58c0378d4cb86`, Windows/Ubuntu [CI run 39046609234](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39046609234):

- VS2022 x64 VST3 and standalone compile PASS. Nine Windows CTest suites PASS, 129.41 seconds total; actual VST3 host suite 0.32 seconds.
- 7,983 DSP assertions; 286 Windows JUCE integration assertions (83 focused library checks included); 209 actual-VST3 host assertions. Prepared host reports 16 samples at 44.1/48/96 kHz, blocks 64/257/1024. This resolves the earlier prepared-host query gap, not the manual compensation/bounce gate.
- pluginval 1.0.4 strictness 10 SUCCESS, seed `0x5b67a8a`, version 0.10.0. Separate official Steinberg validator: **47 passed, 0 failed**, exit 0. Its warning about both `moduleinfo.json` and `IPluginCompatibility` being present comes from the JUCE bundle and does not fail validation. 64-bit floating-point processing is not supported; the instrument uses float audio processing.
- Clean silent install: binaries at default common VST3 / isolated standalone paths match build hashes; manual, licensing document and bank present. Same-version reinstall PASS. Installed standalone survives three seconds, then is terminated by the test. Silent uninstall removes binaries and registry entry and preserves the unique user-data fixture. Four acceptance statements PASS. Separate non-admin user, real device and wizard/older-version upgrade behavior remain untested.
- Windows profiler: 54 cases / 8,298 callbacks, zero nonfinite output; **812 over-budget calls, all in interrupted engine-blend workloads**. Steady-engine cases observe zero overruns in this short VM run. Blend worst mean/budget **2.2744**, p99/budget **2.4228**, max/budget **2.4996**. This workload fails the performance acceptance goal and remains release-blocking; a successful numerical benchmark exit is not a timing pass.
- Linux strict GCC build and 281 integration checks PASS. Five application/test translation units (processor, preset manager, editor, integration and library tests) pass ASan/UBSan, halt-on-error, with 281 checks. Cached JUCE/DSP objects in this application sanitizer executable are uninstrumented; leak detection is disabled. DSP is unchanged from the seven fully instrumented Phase 8 DSP suites and both native CI jobs repeat its numerical checks.
- Final downloaded Windows artifact `12105914337`, SHA256 `7659570018c946e72bdf5b3670a4e4b3bb91dd7b1a931a2c3c21d6ad8431ed24`: digest, ZIP integrity, AMD64 PE headers, all packaged validation logs and portable 410-preset bank verified. No audio or authoring banks are in the portable plugin package.
- Factory/vocal artifact `12105904332`, SHA256 `cffaca14d03fd019086444aa3ba80e28c012d758e6fdd26324485ba89c955954`; retro artifact `12106705232`, SHA256 `edbb04929503a870df816811176ed0e35d67b1c5966cc443205c5eee884efb08`: digests/ZIPs, 1,640 audit and 410 similarity rows, 29 + 28 six-second stereo/48k/PCM16 excerpts and three indices verified. All 57 WAV payloads match Phase 9 byte-for-byte. These are offline test renders, never plugin assets.

The first host harness incorrectly serialized VST3 state on a worker and timed out (the other eight suites passed). The corrected test keeps state on the message thread and processes audio on a worker; the full successful run above supersedes it. Plugin application changes are limited to the bypass fix. Factory/DSP/parameter contracts remain unchanged. Final evidence-only documentation updates do not change the validated application/build source.

Distribution files in `dist`: `G-Boy-Advance-2-Setup-x64.exe`, `G-Boy-Advance-2-Windows-x64.zip`, `G-Boy-Advance-2-Source.zip`, separate factory/retro audition ZIPs, `build-record.json` and `SHA256SUMS.txt`. The final source ZIP has 75 source/assets including every new test/script. Installer SHA256 `5b3d5d961db2f7e1903f33adf953784d0702729bcaab9053c26f945ebc4fc614`; portable ZIP SHA256 `5bb0f7acf35d893bf2eb1c1a4ab00e0cd498d3550535277faa8edff5c1b16154`. Source commit/checksum evidence is in the build record. These remain unsigned development artifacts, with the pending gates explicitly recorded in RELEASE_CHECKLIST.md.
