# 0.11.0 candidate — skins and custom synthesis patches

## Scope

- Purple, Glacier Blue, Black, White and Spice Orange via SELECT → SKIN.
- Stable string skin IDs in non-audio project state; safe Purple default for older/unknown IDs; independent editor palettes and host notification of unsaved skin edits.
- START → IMPORT / EXPORT SOUND for `.gba2preset` synthesis instruments. Validated, size-bounded JSON, independent imported IDs/names, atomic saves; no factory overwrite or new runtime bank dependency.
- Static native vector housing cached at rendering density, with independent live controls and LEDs. Very large images use vector painting to bound retained cache memory.
- Unchanged optimized DSP, 410 factory programs, 64 parameter IDs, schema 1, manufacturer/product identity and 16-sample latency.

## File compatibility

The owner confirmed synthesis only. Maizesoft documents `.mse` instruments as sample-based, including audio samples. These cannot be played with this engine; no MSE converter or sample playback is claimed. Source: https://www.maizesoft.com/download/msp/document.pdf (Introduction and Export/FAQ). `.chl` has not been identified by its source application; that format is unsupported pending a real specification/example. Existing `.gba2preset` patches are supported. Merely renaming a file is not conversion.

## Validation

[Windows/Ubuntu run 39089737722](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39089737722) succeeded for application commit `cfd38d9ef9a5d431b84353e3a3dfe3a70a7122f3` (tree `ee8591ef4ccd706d3fd912a1f4113cc06df7ef9f`). Later documentation-only commits do not change the tested binaries.

- Ten Windows CTest suites: 8,007 DSP/preset checks; 385 JUCE application checks including 103 library checks. Local Linux application checks: 378 including 101 library checks; platform-specific tests explain the difference.
- Tests cover the five skin selections, project persistence, legacy/unknown IDs, unchanged sound on appearance changes, independent editor palettes and host state notifications. Minimum/maximum editor sizes and 125/150/200% snapshot scaling are exercised. Every skin and its advanced panel has a Windows snapshot.
- Preset tests cover complete parameter roundtrip, copied IDs, duplicate-name suffixes, reload from disk, original-file retention, failed-write preservation, and malformed/oversized/unsupported-file rejection. Native interactive file chooser use still requires manual acceptance.
- Actual-bundle synthetic hosts: 265 normal-host checks and 266 FL-named-host checks; six editor lifecycles each, with 1,007/961 concurrent audio blocks. The FL-named harness is not FL Studio.
- pluginval 1.0.4 strictness 10 SUCCESS, seed `0x44ffcc9`; official pinned Steinberg validator: 47 passed / 0 failed.
- Installed diagnostic collector revision 3 passed mocked-event/architecture checks in Windows PowerShell 5.1 and PowerShell 7. The installed script reported the real installed VST3 hash.
- Six installer checks passed: pinned 0.10.3 install on a clean VM, upgrade to 0.11.0 with exact binary hashes/default VST3 path/user-file preservation, installed diagnostic report, same-version reinstall, three-second standalone process liveness, and uninstall with user-file preservation. This does not test an audio device. Fresh 0.11.0 installation without the old version, interactive dialogs and separate-account behavior remain manual checks.
- The 530-scene / 8,970,240-float audio comparison is byte-identical to 0.10.2, with SHA256 `e55c4d1fe95b5e9f9173df8238734647a403cdd39ff70fd7dc0e0e0dcf88009c` on both streams. DSP sources and factory-bank bytes also match 0.10.3.
- Matched Windows profile versus 0.10.2: 14.76% less accumulated measured DSP time at 8 voices and 8.25% less at 32 voices, across 66 cases per voice count. No observed over-budget callbacks in either profile; 32-voice profile: 0/10,142, worst ratio 0.91455. This reruns the existing 0.10.3 optimizations; the skin update introduces no additional DSP change. Shared-runner timings exclude physical drivers/DAW/GUI, initialization and note bursts, and are not FL CPU percentages or real-time guarantees.
- Local software rendering observations confirm much cheaper warm housing repaints at 960/1280/1920 logical widths. The probe excludes child controls, audio, cold cache rebuilds and actual DAW/DPI/device behavior; project objects were O2 and shared JUCE objects O0. No whole-plugin CPU reduction is inferred from that probe.
- Downloaded artifact SHA256/ZIP integrity, exact installer root EXE, AMD64 VST3/standalone, unchanged 410-patch parameter bank, absence of runtime audio assets and all 18 third-party notices were checked after download. The portable payload and installer contain the same binaries as the validation job.

## Downloads and installation

- [Installer ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39089737722/artifacts/12122736368): extract into a new folder, close FL Studio and the standalone, run `G-Boy-Advance-2-Setup-x64.exe`, retain the default destinations, then open the 64-bit FL Studio launcher and rescan if needed.
- [Portable/manual install ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39089737722/artifacts/12122691618).
- [Full validation output and UI snapshots](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39089737722/artifacts/12122811049).
- [Source branch and build commands](https://github.com/treezysosa/G-Boy-Advance-2/tree/release/skins-presets-0110).

Installed VST3: `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. Standalone: `C:\Program Files\Treezy Audio\G Boy Advance 2`. User presets: `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets\*.gba2preset`. CMake output: `build/GBoyAdvance2_artefacts/Release/VST3` and `Standalone`.

Tested Setup EXE SHA256: `b6101dd1e0370e139898b0e230693092e26153741974a1fc6e14d971091a6c88`. VST3 binary SHA256: `727ff2afd867471a5a8ec72e607144e753f11a41d55b42290974d1a5d0f1179f`.

## Release status

The owner requested a prepared build with licensing decided later. This is an unsigned release candidate, without a final public release tag or a claim of completed commercial licensing, physical DPI/audio-device acceptance or all-DAW certification. The owner reports the earlier plugin working through FL64 and improved CPU usage with reduced polyphony/release settings. See RELEASE_CHECKLIST.md for manual tasks. Build/installation commands and paths are in README.md and USER_MANUAL.md.


## Next exact checks

1. In the owner's FL64 environment, load this 0.11.0 candidate, play chords with the preferred voice/release settings, select each skin, and save/reopen a project with two differently coloured instances.
2. Create a sound in SELECT, Save As, Export Sound to Documents, then Import that file and check the copied User patch. Exercise the native file dialog and cancellation.
3. Supply the originating application/specification and one `.chl` example to assess real synthesis-parameter import. Maize `.mse` sample playback remains excluded by the owner's synthesis-only choice.
4. Complete the applicable manual host/audio-device/DPI/listening and fresh-install entries in RELEASE_CHECKLIST.md; choose source/JUCE distribution terms before a final public release. Optional signing requires an owner certificate.
