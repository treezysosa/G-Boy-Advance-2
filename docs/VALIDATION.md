# Validation record

Development snapshot 0.1.0. A passing DSP test is not proof that a host can load the VST3.

## Executed locally

Linux x86_64, GCC 14.2.0, C++20, `-O2 -Wall -Wextra -Wpedantic -Werror`.

- Native DSP compilation: PASS.
- Rendering before preparation safely returns silence: PASS.
- DSP suite: **1,384 checks PASS**.
- AddressSanitizer + UndefinedBehaviorSanitizer: same suite PASS with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer is unavailable in the sandbox/ptrace environment; leak freedom is not claimed.
- Overlapping identical notes release one voice at a time: PASS.
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

## Not executed locally

The provided machine has GCC but no CMake, JUCE checkout, Windows SDK, Visual Studio, or DAW. Shell dependency downloads could not reach the proxy. No successful JUCE compilation, Windows binary, GUI screenshot, installer, VST3 validator result, or DAW compatibility is claimed in this record.

The committed GitHub Actions workflow is configured to build Windows x64 VST3 / standalone and run `Tests/PluginTests.cpp`. No workflow run appeared in the repository Actions API during this session, so there is no CI result to report. Open Actions → Build and validate → Run workflow, then record the result here. Source presence does not mean those tests passed.

The unexecuted JUCE integration suite covers sample-offset MIDI, state roundtrips, user preset CRUD, favorites/history persistence, host programs, categories, volume attachment, pitch-wheel return, preview buttons, browser/advanced opening, and editor snapshots at multiple sizes.

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
