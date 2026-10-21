# G BOY ADVANCE 2 — 0.18.2

START → preset folder menu → **SoundFonts** now opens the existing SF2 library inside the preset browser. Select a bank and instrument, preview it or play MIDI. OPEN FOLDER, CHOOSE FOLDER, REFRESH, LOAD SF2 and remember-last controls are available in that same view. The 21 built-in categories, including Milwaukee, remain in the folder menu.

SoundFonts is a navigation destination. It adds no factory program slots or automation parameters. All 678 factory patches, 754 program identities, 64 parameter IDs, 30 skins, audio DSP and synth/SF2 state handling are unchanged from 0.18.1. The view reuses the existing asynchronous SoundFont manager and default/custom folder. Merely navigating folders keeps the current sound; choosing a native patch returns to synthesis. Synthesis filters are preserved while hidden in SoundFonts. No new CPU improvement is claimed.

## Validation

Pending Windows verification. The local application tests exercise bank/instrument selection through the preset folder menu with stale synthesis filters, audible SF2 MIDI/preview, return to native presets, repeated navigation, project recall and closing the browser without stopping playback. Windows tests will check both actual VST3 bundle hosts, pluginval strictness 10, the official Steinberg validator and the 0.18.1 → 0.18.2 installer upgrade.

The focused Windows workflow verifies that DSP, factory data, processor and preset/SF2 managers match the exact verified 0.18.1 source before running the complete application/UI suite and host/installer checks. The unchanged 0.18.1 DSP, sound-diversity and performance results remain historical evidence; those expensive DSP benchmarks are not rerun for this UI change. The prior 50% performance target remains unmet.

## Install and build

The updated installer link will be added after Windows validation. Close FL Studio and standalone, extract the installer ZIP into a fresh folder and run G-Boy-Advance-2-Setup-x64.exe. The VST3 bundle installs to `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. User presets and SoundFonts stay in their existing application-data folders.

Source branch: `feature/soundfonts-folder-0182`. Build using `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1` with Visual Studio 2022 x64 and CMake 3.24+. Output: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone is under `Standalone`. The complete source package includes the project, tests, presets, docs and packaging scripts.

Actual FL Studio, physical audio/MIDI/DPI and Explorer drag/drop remain manual acceptance steps. Prepared binaries are unsigned; distribution licensing remains deferred by the owner.
