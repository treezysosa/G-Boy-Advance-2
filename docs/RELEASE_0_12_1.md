# 0.12.1 — synthesis-only update

Sample playback was removed at the owner's request. This version restores the tested 0.11.0 synthesis DSP, removes the Samples page, audio-file loading, sample worker, interpolation and playback code, and embedded-audio preset authoring. It retains all five color skins, synthesis preset import/export, 410 factory patches, 64 stable audio parameter IDs, plugin identity and the previous CPU improvements.

Older 0.12.0 DAW states restore their synthesis parameters and skin, discard embedded audio from memory and subsequent saves, and show `SYNTH ONLY:` in the preset display. START shows the removal notice. This cannot reproduce the removed sample sound. Selecting a synthesis preset clears the notice. Keep the original project if its old sample content is needed elsewhere.

Sample-containing `.gba2preset` files are rejected or skipped with a browser error; they are left on disk unchanged. Synthesis-only schema 1 `.gba2preset` import/export remains supported. WAV, AIFF, CHL and MSE instrument import is unavailable. The installer does not delete personal audio, projects, user presets or library metadata.

## Verification

[Windows/Ubuntu run 39096250046](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39096250046) passed all ten Windows suites: 8,007 DSP/preset checks, 398 application checks (including 103 library checks), 265/266 actual-bundle synthetic-host checks, pluginval strictness 10, Steinberg 47/0 and six installer lifecycle checks. The FL-named host harness is not FL Studio.

Validated application commit: `8b7fe596259629202fc324c3be9df0fbe39084d2`, tree `29bc5ae3700556630059b4436e56d257538328ff`. Later documentation-only commits do not alter that application source.

- Local seven native suites: 8,007 DSP/preset checks. JUCE application: 391 checks including 101 library checks. Own application/DSP ASan/UBSan: 391 passed; cached JUCE modules uninstrumented, leak detection disabled for the environment tracer. An existing editor formatting warning was retained when compiling that local sanitizer target.
- Small and 5 MiB old embedded-audio states restore only synth settings and skin; resaved states contain no `sampleData`, preserve the notice, and clear it on synthesis preset selection. Old sample preset import fails without changing the original file. The advanced editor has four synthesis tabs and no sample controls.
- Windows CTest: 10/10, 398 application checks. Actual-bundle hosts: 265/266 checks, including editor lifecycle and concurrent audio. Synthetic host results do not prove actual named-DAW compatibility.
- pluginval 1.0.4 strictness 10 passed (seed `0x2861589`); official pinned Steinberg validator: 47 passed, zero failed. Windows PowerShell 5.1/7 diagnostic fixture checks passed.
- Installer: pinned 0.12.0 installation, upgrade to 0.12.1/default VST3 path and exact hashes, installed diagnostic hash report, same-version reinstall, three-second standalone process liveness, uninstall preserving old sample preset and personal-audio fixture bytes. Physical audio devices and interactive/separate-account installation remain manual.
- 530 comparison scenes / 8,970,240 float samples: byte-identical on Windows to the older reference and locally to final optimized 0.10.3. Factory identities, compiled arrays and calibration values are unchanged; the guarded source fingerprint is restored with the exact 0.11.0 DSP.
- After download: GitHub SHA256/ZIP integrity, AMD64 VST3 and standalone, installer/portable binary equivalence, 410 parameter-only factory patches, 18 pinned notices and absence of runtime audio assets were checked.

The native 32-voice profile recorded 7/10142 over-budget callbacks, worst budget ratio 1.484138. These are observations on one shared runner, not audio-driver/DAW performance certification.

Unsigned build preparation only. The owner deferred the licensing decision; signing and hands-on final-release acceptance remain outstanding.


## Downloads and installation

- [Windows installer ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39096250046/artifacts/12124961435): close FL Studio/standalone, extract into a new folder and run `G-Boy-Advance-2-Setup-x64.exe`. Keep default paths, then open FL64 and rescan if required.
- [Portable/manual-install ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39096250046/artifacts/12124154373). Copy the entire `G Boy Advance 2.vst3` bundle, including Contents.
- [Validation logs, binaries and snapshots](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39096250046/artifacts/12125121118).
- [Source branch](https://github.com/treezysosa/G-Boy-Advance-2/tree/release/synth-only-0121).

Build with Visual Studio 2022 x64 C++ tools, Windows SDK and CMake 3.24+:

```powershell
git clone --branch release/synth-only-0121 https://github.com/treezysosa/G-Boy-Advance-2.git
cd G-Boy-Advance-2
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1
ctest --test-dir build -C Release --output-on-failure
```

Build output: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3` and `build/GBoyAdvance2_artefacts/Release/Standalone/G Boy Advance 2.exe`. Installed VST3: `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. Standalone: `C:\Program Files\Treezy Audio\G Boy Advance 2`. User presets: `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets\*.gba2preset`.

## Remaining acceptance

Hands-on FL64/other named DAW, physical audio/MIDI devices, listening, mixed-DPI and interactive/separate-account installation checks remain in RELEASE_CHECKLIST.md. No MSE/CHL instrument decoder is provided. Licensing remains expressly deferred; code signing requires an owner certificate.

Setup EXE SHA256: `a83d8756564caa7ba785f1ab49000d320ccd72952d1b7c612f89f909a9de99dc`.
