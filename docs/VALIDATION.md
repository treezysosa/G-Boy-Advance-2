# Validation record

Development snapshot 0.1.1. A passing DSP test is not proof that a host can load the VST3.

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
