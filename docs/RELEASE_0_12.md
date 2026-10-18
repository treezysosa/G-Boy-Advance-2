> Historical 0.12.0 record. Sample playback was removed in 0.12.1 at the owner's request; see [RELEASE_0_12_1.md](RELEASE_0_12_1.md).

# 0.12.0 sample playback candidate

The owner explicitly authorized adding sample playback to the existing synthesizer, superseding the original synthesis-only restriction. Eight synthesis engines, five skins, the 410 factory identities, all 64 automation IDs, schema-1 synthesis presets and plugin identity are retained.

## Implemented

- Background WAV/AIFF import (mono/stereo), keyboard-wide transposition with a MIDI root note, whole-file crossfade looping, sample-only and equal-blend hybrid modes.
- Existing joint 32-voice ceiling, mono/legato/stealing, ADSR/filter/LFO/velocity/bend/sustain/glide, stereo filters, shared effects and master gain.
- Immutable sample data with hazard-protected publication and off-audio reclamation. No files, blocking locks, allocation or sample destruction in the callback. Cancelled/stale loader jobs cannot replace a newer selection.
- Cubic interpolation with crossfaded low-pass mip levels, stereo voice-stealing tails and a 128-frame bank/config transition tail.
- Embedded source audio in project state and schema-2 user presets/import/export. Pure synthesis presets remain schema 1; no original audio path/network dependency after save. Factory selection clears sample state.
- SELECT → SAMPLES native file chooser, mode, root note, loop, clear and status/error display.

## Limits

One sample zone across the keyboard, up to 8 MiB source bytes / 4M frames, 1–2 channels, 8–192 kHz. Pitched playback changes duration. No streaming, slicing, time stretch, custom loop points or multi-zone/velocity maps. Audio samples are user supplied; none are added to the factory library or installer. WAV/AIFF is supported; Maize `.mse` decoding is not implemented (a public compatible SDK/specification was not found). `.chl` is still unidentified. See the Maize instrument description: https://www.maizesoft.com/download/msp/document.pdf.

Sample presets/projects can reach 16 MiB. Browser library scans/metadata persistence still use synchronous non-audio I/O; many embedded sample presets consume more memory and scan time. Older plugin versions cannot play schema-2 sample instruments. User-preset/audio-file loading is asynchronous; wait for ready before auditioning. Embedded project samples finish preparation inside the non-real-time state-restore operation, before immediate/offline playback. Audio-file failures keep the last working instrument; corrupt embedded audio reports an error and clears that unavailable sample.

## Validation

Local: 307,221 native sample DSP checks at 44.1/48/96 kHz, including sample/combined modes, sustain/release, finite output and zero allocation in render/MIDI/switch. Application tests cover WAV/AIFF, stereo independence, root pitch, mono rate conversion, loops, layers, polyphony/all-sound-off, preset roundtrip, deleting the original source then restoring embedded state, immediate state save/first MIDI after restoration, rejected files and concurrent load/cancel/publication. The 530-scene / 8,970,240-float synthesis-only render is byte-identical to final 0.10.3 local output (SHA256 dda45b59d786cb135e314ec14dae80db13e240ca87c0a5f0ba4cbfdbe5db361e). Factory calibration volumes and arrays are unchanged; only the guarded DSP source fingerprint was refreshed after that comparison.

[Windows/Ubuntu run 39093321395](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39093321395) succeeded for application commit `ecddf6c92e12166b7457776908cf2cc0192a8d21`, tree `846542d11bf73be83b5bb371d2f30a9c70512536`. Later documentation-only commits preserve the tested application source and binaries.

- Eleven Windows CTest suites: 315,228 native DSP/preset checks (307,221 sample DSP + 8,007 existing), 428 JUCE integration checks (103 library + 36 sample checks included), 265/266 actual-bundle synthetic-host checks. The FL-named test harness is not FL Studio.
- The corrected publication test completed 12 sample replacements with 1,206 concurrent Windows audio blocks and finite output. Windows host lifecycle tests completed six cycles each with 1,000/947 audio blocks.
- pluginval 1.0.4 strictness 10 SUCCESS, seed `0x23636a5`. Official pinned Steinberg validator: 47 passed, zero failed.
- Six installer checks passed: pinned 0.10.3 installation, upgrade to 0.12.0/default VST3 path/exact binary hashes/user-file preservation, installed diagnostic hash report, same-version reinstall, three-second standalone process liveness, and uninstall preserving the user file. A real audio device, fresh 0.12.0 clean install, native dialogs and separate-account use remain manual.
- Both Windows PowerShell 5.1 and PowerShell 7 diagnostic fixture tests passed. The new diagnostic script reports the current installed hash.
- The 530-scene / 8,970,240-float Windows synthesis-only comparison is byte-identical to 0.10.2 (both streams SHA256 `e55c4d1fe95b5e9f9173df8238734647a403cdd39ff70fd7dc0e0e0dcf88009c`). Local output matches final 0.10.3; all factory calibration values and compiled factory arrays are unchanged. Only the guarded source fingerprint was refreshed after comparison.
- The existing matched native synthesis profile versus 0.10.2 measured 12.25%/6.67% less accumulated callback time at 8/32 voices. Both had zero observed over-budget calls; 32 voices: 0/10,142, worst ratio 0.815325. This profiles synthesis-only cases on one shared runner, excluding DAW/GUI/audio-driver load, initialization and note bursts. It does not measure sample-mode CPU or establish physical-driver deadlines/FL CPU percentages.
- Own application/DSP sources passed local ASan/UBSan integration: 421 checks including sample lifetime stress (12 publications, 276 audio blocks). Cached JUCE modules were uninstrumented; leak detection was disabled because of the environment tracer limitation. A separate native right-only recording probe confirmed a right-channel steal tail while the newly triggered region was silent (tail energy 0.388098 over the observed window).
- After download: GitHub artifact digests/ZIP integrity, exact root Setup EXE, AMD64 VST3/standalone, portable/installer binary equivalence, unchanged 410-patch factory bank, all 18 third-party notices and absence of bundled runtime audio assets passed. Windows sample-page and skin snapshots are retained.

The first candidate's Windows test could finish before its audio thread started; that test scheduling race was corrected to require completed publications/processing. The project-state path was then tightened to finish embedded sample preparation before first/offline MIDI; pitch mip crossfades and right-channel steal tails were verified before this delivery build. The final run above is the shipping evidence.

## Downloads and installation

- [Installer ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39093321395/artifacts/12124365360): close FL Studio/standalone, extract to a new folder, run `G-Boy-Advance-2-Setup-x64.exe`, keep default paths, then open FL64 and rescan if necessary.
- [Portable/manual installation ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39093321395/artifacts/12124101044).
- [Validation binaries/logs/snapshots](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39093321395/artifacts/12123582998).
- [Source branch](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/sample-playback-0120).

```powershell
git clone --branch feature/sample-playback-0120 https://github.com/treezysosa/G-Boy-Advance-2.git
cd G-Boy-Advance-2
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1
ctest --test-dir build -C Release --output-on-failure
```

Requires Visual Studio 2022 x64 C++ tools, Windows SDK and CMake 3.24+; JUCE 8.0.6 is pinned. Output: `build/GBoyAdvance2_artefacts/Release/{VST3,Standalone}`. Installed VST3: `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. Standalone: `C:\Program Files\Treezy Audio\G Boy Advance 2`. User presets: `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets\*.gba2preset`.

Setup EXE SHA256: `e438d1b14f7c6b7f2fd364da52f1898d7ad5737b8141b252e2648cfd884e6c74`. VST3 binary SHA256: `6a9f95f14ade9a9813c7e5b3ed186047c972a415a019a1f35c9724d4e7cf0d4e`.

## Remaining acceptance

Try this candidate in the owner's FL64 environment: load a WAV/AIFF, set its root/mode/loop, play/release/bend/sustain notes, save a custom instrument and a project, delete/move the original audio, then reopen/render immediately. Test both sample-only and combined modes, five skins and native file-dialog cancellation. Complete real-device/DAW/DPI/listening and installation entries in RELEASE_CHECKLIST.md.

No Maize `.mse` or unidentified `.chl` bank decoding is claimed. Multi-zone maps, streaming, slicing, time stretching and custom loop points are beyond this first playback version. Licensing is still explicitly deferred by the owner; this is unsigned with no final public release tag or actual DAW/device certification. Optional signing needs an owner certificate.
