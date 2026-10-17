# Phase 10 — Windows release validation

Development version 0.10.0. Automated evidence pending this phase's Windows run; this document will record the actual result before delivery. Plugin identifiers, 64 parameter IDs, schema 1, factory identities and DSP/factory values remain stable.

## New checks

`Tests/HostTests.cpp` is a separate JUCE VST3 host, linked without application/DSP source. CTest loads the built bundle, queries prepared latency at 44.1/48/96 kHz and 64/257/1024 frames, checks MIDI sample offsets, audio, bypass/note-off/resume, parameter state across newly loaded instances, isolation, SysEx rejection, concurrent serialization, offline mode, release and tail reporting. This establishes behavior in that host; it does not certify any named DAW or compensated bounce.

The processor overrides bypass to keep normal MIDI/envelope/effects processing and mute stereo output. Previously the inherited JUCE bypass discarded note-offs, freezing held notes. The bypass path retains normal CPU cost. A two-assertion in-process regression supplements actual-bundle acceptance.

The separate official Steinberg SDK validator is pinned to `cc2adc90382dded9e347caf74e4532f1458715db` (`v3.7.12_build_20`) including its submodule revisions, built only as a test tool. It and pluginval are not runtime dependencies.

`Tests/Benchmark.cpp` profiles 54 native DSP cases: three sample rates, 64/256 frames, all eight engines and an interrupted engine-blend workload, 32 voices, effects and cutoff/delay automation. It logs mean, p50/p95/p99/max callback time, budget ratios and observed over-budget calls. Timings are observational on a shared VM, not an underrun or hardware deadline certification. Nonfinite audio fails the run. This is not a VST3/GUI-loaded callback profile.

`test-installer-windows.ps1` refuses existing installations. On a clean disposable admin VM it silently installs to an isolated standalone directory and the real default common VST3 location, compares installed binary hashes, reinstalls, smoke-launches standalone for three seconds, uninstalls and verifies a unique user-data fixture survives. It removes only its fixture. The installed standalone liveness check does not test device audio. Non-admin use, interactive wizard/upgrade from old versions and physical machines remain manual gates.

The portable staging directory is rebuilt before each package, preventing stale files from shipping. The installer and project versions are synchronized at 0.10.0.

## Acceptance boundary

See RELEASE_CHECKLIST.md and VALIDATION.md. Signing, owner license selection, complete third-party notices, human listening, high-DPI/accessibility, non-admin and named-DAW acceptance remain pending. No production release/tag is created by this phase. Windows artifacts are unsigned development test builds.
