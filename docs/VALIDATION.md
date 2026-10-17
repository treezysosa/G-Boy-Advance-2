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

The main Linux render was visually compared with the supplied reference. Master-knob proportions, metallic shading, the wordmark, D-pad arrows and silver START/SELECT caps were refined. Native Windows/high-DPI rendering and physical mouse/keyboard interaction in a DAW still need review.

Additive output equivalence and the 32-voice benchmark are documented in [PERFORMANCE.md](PERFORMANCE.md).

## Windows build executed; host acceptance pending

The local machine has no Windows SDK or Visual Studio; the actual Windows build ran on GitHub's `windows-2022` runner using Visual Studio 2022 x64.

[Run 39004268093](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39004268093), application source commit `ee0b681eeadbdc63e72d8376986205846a06c4ba`:

- Visual Studio 2022 CMake configuration: PASS.
- Windows x64 VST3 and standalone compilation/linking: PASS.
- Windows CTest: 2/2 suites PASS (synthesis 2.13 s, plugin state/presets/UI 0.68 s).
- Inno Setup 6.7.1 installer and portable archive packaging: PASS.
- Downloaded artifact SHA256 matches GitHub's digest. The VST3 binary and standalone executable are AMD64 PE images.

The original export helper incorrectly omitted files whose names began with `build`, including the workflow and Windows script. The corrected inventory explicitly checks both entry points, and the corrected workflow ran successfully. The workflow now adds pluginval 1.0.4 at strictness 10 to check the actual VST3 before packaging. That host validation has not completed yet; no DAW compatibility is claimed.

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
