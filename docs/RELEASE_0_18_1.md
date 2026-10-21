# G BOY ADVANCE 2 — 0.18.1

The factory folder **Hip Hop Milwaukee** is renamed **Milwaukee**. It still contains the same 24 presets. The new label is used by the browser, category search, D-pad navigation, factory JSON and installation. The original recipe categories and tags remain internal authoring metadata.

All 678 preset names, IDs, tags, synthesis parameters and calibrated gains are identical to 0.18.0. All 754 host program slots, 76 reserved slots, 64 parameter IDs, 21 categories, 30 skins and DSP source are unchanged. The existing project restore logic resolves factory categories by stable ID, so older projects display Milwaukee while retaining their edited parameters. User presets and SoundFont files/preferences are retained. This label-only update adds no CPU improvement claims.

## Verification

[Windows/Ubuntu run 39600289352](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39600289352) passed nine Ubuntu suites and twelve Windows suites, including 633,063 common DSP/preset checks, 3,996 Windows application checks (578 library checks), both native bundle hosts (647/648), pluginval strictness 10 and the official Steinberg validator (47 passed / 0 failed). All six installer lifecycle checks passed, including the actual 0.18.0 → 0.18.1 upgrade, same-version reinstall, installed diagnostics, standalone liveness and uninstall with user-preset/SoundFont/appearance preservation.

The local application suite passed 3,984 checks. Raw and calibrated generation match the exact 0.18.0 bank apart from the 24 category fields. All 754 registered program identities and all DSP source files are unchanged. Existing browser checks verify the 24 Milwaukee rows, four piano search results, loading and project restoration. The Windows UI artifact contains the Milwaukee browser screenshot and all 30 main/advanced skin renders.

The existing CPU/audio comparison also passed its unchanged 1e-6 / 110 dB bounds for 798 short and 148 sustained scenes. This release changes a category label; timing fluctuations on the shared runner are not additional optimization claims. The prior 50% performance target remains unmet. The full 678-preset rendered audit again reported zero duplicate or close-pair flags across 229,503 comparisons; this is objective screening, not human listening acceptance.

## Install

[Download Windows installer](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39600289352/artifacts/12347416437) · [Portable/manual archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39600289352/artifacts/12346941648) · [Milwaukee browser and 30-skin previews](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39600289352/artifacts/12347001732). Close FL Studio and standalone, extract the installer ZIP into a fresh folder, then run G-Boy-Advance-2-Setup-x64.exe. The complete VST3 bundle installs to `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. User presets stay under `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`; the default SoundFont library stays under `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library`.

Source branch: `feature/milwaukee-folder-0181`. Build with `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1` using Visual Studio 2022 x64 and CMake 3.24+. Output: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone is under `Standalone`.

Actual FL Studio and hardware-device testing remain owner acceptance. The prepared build is unsigned; the owner deferred the licensing decision. Earlier 0.18.0 performance and uniqueness evidence remains in [RELEASE_0_18_0.md](RELEASE_0_18_0.md).
