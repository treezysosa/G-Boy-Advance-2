# G BOY ADVANCE 2 — 0.18.1

The factory folder **Hip Hop Milwaukee** is renamed **Milwaukee**. It still contains the same 24 presets. The new label is used by the browser, category search, D-pad navigation, factory JSON and installation. The original recipe categories and tags remain internal authoring metadata.

All 678 preset names, IDs, tags, synthesis parameters and calibrated gains are identical to 0.18.0. All 754 host program slots, 76 reserved slots, 64 parameter IDs, 21 categories, 30 skins and DSP source are unchanged. The existing project restore logic resolves factory categories by stable ID, so older projects display Milwaukee while retaining their edited parameters. User presets and SoundFont files/preferences are retained. This label-only update adds no CPU improvement claims.

## Verification

Pending Windows build and installer validation. Locally, raw and calibrated generation are compared against the exact 0.18.0 bank; only 24 category fields may differ. Existing application/browser checks will exercise Milwaukee filtering, search, loading and project restoration.

## Install

The updated download will be added after validation. Close FL Studio and standalone, extract the installer ZIP into a fresh folder, then run G-Boy-Advance-2-Setup-x64.exe. The complete VST3 bundle installs to `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. User presets stay under `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`; the default SoundFont library stays under `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library`.

Source branch: `feature/milwaukee-folder-0181`. Build with `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1` using Visual Studio 2022 x64 and CMake 3.24+. Output: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone is under `Standalone`.

Actual FL Studio and hardware-device testing remain owner acceptance. The prepared build is unsigned; the owner deferred the licensing decision. Earlier 0.18.0 performance and uniqueness evidence remains in [RELEASE_0_18_0.md](RELEASE_0_18_0.md).
