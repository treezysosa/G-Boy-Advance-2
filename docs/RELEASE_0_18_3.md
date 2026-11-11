# G BOY ADVANCE 2 — 0.18.3

The factory Percussion folder and its 24 presets are removed from the browser, search and D-pad category navigation. All **654 remaining presets in 20 factory categories** retain their names, tags, synthesis parameters, gains and public program numbers. SoundFonts navigation and all 30 skins remain available.

The host-program count remains 754, including 100 reserved slots. Projects using removed Percussion sounds restore their stored settings under Saved Project; Save As keeps them as user presets. User files and SoundFonts are retained. No audio-DSP or parameter-ID changes and no additional CPU improvement are claimed.

## Verified downloads

[Windows installer](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43060644657/artifacts/13874091563) · [Portable/manual archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43060644657/artifacts/13873817144) · [Validation evidence and build source ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43060644657/artifacts/13874210729) · [Current source](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/percussion-removed-0183).

Close FL Studio and standalone, extract the installer ZIP into a fresh folder and run G-Boy-Advance-2-Setup-x64.exe. The complete VST3 bundle installs to C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3. User presets and SoundFonts stay in their existing application-data folders.

## Validation

[Core/runtime build 43056128423](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43056128423) passed all nine independent DSP/preset/diversity suites, the Windows factory-preset suite and both actual VST3 bundle hosts. Every retained dictionary and generated patch row was compared with 0.18.2; all 754 registry identities were preserved.

[Completed release validation 43060644657](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43060644657) passed **4,171 Windows application checks**, including **505 library checks** and **199 SoundFont checks**. Actual VST3 hosts passed **887/888 checks**. Pluginval strictness **10** passed, and the official Steinberg validator passed **47 tests / 0 failures**. All **six** installer lifecycle checks passed, including the actual 0.18.2-to-0.18.3 upgrade, reinstall, installed diagnostics, standalone liveness and uninstall with user-data preservation.

Twenty-four new legacy-parameter-state cases verify audible notes, edited settings, reload/resave, host-program echo and user Save As. The fixtures capture 0.18.2 parameter dictionaries and reconstruct project states with the unchanged serializer; they are not exports from an old binary. The 76 actual old vocal-project fixtures remain covered. SoundFonts folder loading, project recall, all 30 main/advanced skin images and absence of Percussion in the native category menu were verified.

The core render audit reports 654 presets in 20 categories, no coarse near-pair flags, 560 presets within 0.5 dB of target and 99 at the gain ceiling. Objective diversity screening is not human listening acceptance. The full current report is in the DSP-validation artifact of the core/runtime run.

Two test defects were corrected during validation: an exact floating-point comparison was replaced with the existing per-parameter host-normalization tolerance, and an overly broad count replacement was restricted to size assertions so an unrelated external-edit expectation stayed 6789. Existing production code, sound parameters and audio-quality thresholds were unchanged.

## Source and binary provenance

Production binaries were compiled at 0856bd8c3f31a3ab041aa39016dd0e2fb908b6fe. Validation source 3fdb2269190e91d03bb5a54997bf3b37bedb6e8e changes test/build tooling only; Source, Presets and CMakeLists.txt were required to be identical before reusing those binaries. Final README/release notes add the verified links. The CI build-source ZIP has 154 source files and was checked for complete contents and ZIP CRC; the current source branch includes the final download notes.

Build using powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1 with Visual Studio 2022 x64 and CMake 3.24+. VST3 output: build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3. Standalone output is alongside under Standalone.

SHA-256 of verified payloads:

| Payload | SHA-256 |
|---|---|
| build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3/Contents/x86_64-win/G Boy Advance 2.vst3 | 800c3d325833fa3f310079134eca8c25837893361298bc06b2d2dbe74bbb4f46 |
| build/GBoyAdvance2_artefacts/Release/Standalone/G Boy Advance 2.exe | 98e17e468d6c9d719bd1f1e6623fc9db2537922928a64af6ae517e0b19dd255c |
| dist/G-Boy-Advance-2-Setup-x64.exe | 57702f97afb455c4f0943784f6bbb288430815ded8016afb2dbd7fb73854cf76 |
| dist/G-Boy-Advance-2-Windows-x64.zip | 241d4ef8f8f072a1e277f945619dc414fc4a1c44903c192169a6461136b353b6 |
| dist/G-Boy-Advance-2-Source.zip | 8f9f44d195d81a5d3d53b0eaaa6b0c775b3ff36b19360b532438c64264826452 |

Actual FL Studio, physical audio/MIDI/DPI and Explorer interactions remain owner acceptance. Prepared binaries are unsigned. The owner deferred distribution licensing. The earlier 50% performance target remains unmet.
