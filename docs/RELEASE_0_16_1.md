# G Boy Advance 2 — 0.16.1

Removed **Vocals - Pop and Indie** (16 factory presets), **Vocals - Southern and West Coast** (14), **Vocals - Cartoon and Gaming** (28) and **Vocals - Soul and R&B** (18). The active factory library now contains **574 presets in 20 categories**. All remaining preset dictionaries, names, IDs and calibrated levels are unchanged. Other vocal categories and vocal patches in the regional hip-hop banks remain available.

The removed categories and their sounds no longer appear in the factory browser, search, category lists, D-pad navigation or factory favorites/recent results. Existing user preset files are retained.

Saved projects keep their explicit synthesis settings, including edited sounds originating from the removed categories. Restoring these projects displays Saved Project as the current category and permits Save As to keep an independent user patch. Seventy-six original host program slots are reserved so every surviving sound retains its program number. Selecting a Reserved N (Default) slot from another program plays Dream Pad; same-slot messages after restoring a project preserve its saved sound; these slots are not factory presets.

Twenty skins, SoundFont loading/drag-drop/library recall, MIDI behavior, 64 audio parameter IDs, 16-frame latency and the prior DSP optimizations are unchanged. No audio-thread code or synthesis DSP changes are required.

## Validation

[Download Windows installer 0.16.1](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39329504889/artifacts/12224422761) · [Portable/manual archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39329504889/artifacts/12224039729) · [Source](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/remove-vocal-categories-0161). Close FL Studio and standalone, extract the ZIP into a fresh folder, run `G-Boy-Advance-2-Setup-x64.exe`, then reopen FL64. User presets, SoundFonts and preferences are retained.

[Windows/Ubuntu run 39329504889](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39329504889) passed nine Ubuntu DSP suites and twelve Windows CTest suites: 630,156 native DSP/preset checks, 2,884 Windows application checks (332 library / 164 SoundFont), both actual-bundle hosts (647/648 checks), pluginval strictness 10, Steinberg 47 passed / 0 failed and six installer lifecycle checks. The synthetic FL-named host is not FL Studio.

Validated application/build commit: `f1b2e34c0eebedd0f8295eb343db44536532aefb`. Downloaded installer, portable archive and full Windows artifact pass SHA256/ZIP integrity checks. Both runtime binaries are AMD64. The packaged factory JSON matches the generated source and exactly equals the 0.16.0 bank with only the seventy-six requested entries removed. Nineteen pinned notices and all twenty main/advanced skin pairs are verified.

All 6,724 central/register/velocity/release factory checks pass, with 574 dry signature groups and zero near-neighbor flags. The independent all-pair audit reports zero duplicate candidates, zero close-pair flags, minimum dry distance 0.060104154. Local native suites pass 630,156 DSP/preset checks and 2,872 application checks. Own-source ASan/UBSan passes the same 2,872 application checks; cached JUCE objects are uninstrumented and leak detection is disabled under the environment tracer.

All seventy-six actual project fixtures restore their edited parameters, identities and host numbers, render audible notes, resave, survive same-program host echoes and can be kept as user patches. All 574 surviving factory host program numbers are checked. The real VST3 wrapper also restores and echoes all seventy-six projects.

The Windows audio probe matches every byte across 694 scenes / 10,985,472 finite floats, before/after SHA256 `e11dc0441a54813b12522f57c77e667df090ab3572ad7e447fead6effffe07bd`. The independent local probe is byte-identical, SHA256 `49ca80753509927c2b27a4f555f1dca053baf897eb758ee9299b1d70e0be6c7c`. The 612-scene native note/chord/retrigger benchmark passes with both engines. This removal does not claim a new DSP speedup; the previous optimized DSP is unchanged.

The installer acceptance run verifies pinned 0.16.0-to-0.16.1 upgrade, the installed 574-preset / 20-category bank, exact runtime hashes, diagnostics, reinstall, standalone process liveness and uninstall while retaining user data. Process liveness does not certify physical audio playback.

Tests cover actual states exported from the 0.16.0 processor for all seventy-six removed presets, including edits to master volume, cutoff and glide. The new processor must restore all parameters, original identities and host slots; render audible notes; survive resave; and permit user Save As. Browser/category checks require 574 active entries, 20 categories and no deleted category. Actual VST3 host tests also restore all seventy-six old states.

The full factory audit checks central notes, additional registers, velocity, release, headroom and dry sound difference. The independent multi-register audit renders 6,888 dry/wet fixtures and compares 164,451 pairs. The matched old/new DSP probe uses immutable 0.16.0 DSP with identical current arrays: 694 scenes and 10,985,472 floats. The installer acceptance script upgrades pinned 0.16.0 to 0.16.1 and checks the installed 574-preset bank and user-data preservation.

Actual owner FL Studio 20.8.3, physical audio/MIDI devices, DPI and human listening remain manual acceptance work. The owner's deferred licensing/signing decisions are unchanged; this prepares an unsigned Windows build without a production tag or main-branch merge.
