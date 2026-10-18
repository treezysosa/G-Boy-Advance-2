# FL Studio editor compatibility investigation

## Initial reported failure

The owner reports that the Windows standalone works, but FL Studio 20.8.3 loses the connection when opening the VST3 editor. The supplied support report confirms Windows build 10.0.18362, FL version 20.8.3.2304 and the exact tested 0.10.2 VST3 hash (`e19be5afec664daa7587bef95b0acbcc9efd214660f6c699fd4721a9f3b1e0e8`). Initial download/install discovery problems were resolved before this report. The running host was named `FL.exe` with a 64-bit `ilbridge.exe`; executable names alone do not establish host architecture. The owner also tried `FL64.exe` in a blank project and reports the same failure. The first report's event reader failed on XML data, so its absence of fault details is not evidence of an empty Windows event log. No default-location FL text crash log was found. The crash module/exception and cause remain unknown. The later x64 launcher success is recorded below; the cause of the earlier bridged failure is not established.

## Latest owner result and CPU revision

A revision-2 report confirmed that the running `FL.exe` is x86 (32-bit), the installed 0.10.2 VST3 is x64 and matches the tested hash, and `FL64 (scaled).exe` beside the host is x64. The corrected reader completed without a parsing error and found no matching recent Windows crash event or default-location text crash log. The owner subsequently confirmed the plugin working in the 64-bit launcher, including a successful retry after another failed launch report. Use that working x64 launcher/shortcut. This does not establish the underlying cause of the bridged failure or a full MIDI/state/device/DAW acceptance pass.

The current 0.10.3 development revision targets CPU cost while playing notes/chords. It retains the Windows editor compatibility behavior and changes DSP calculations only where unchanged values can be reused; no parameter IDs, factory arrays, state schema, synthesis engines or UI are removed. Validate the replacement in a blank project before production use. See PERFORMANCE.md for measured comparisons and practical voice/buffer settings.

## 0.10.2 compatibility candidate

This branch selects JUCE's Software Renderer for the Windows editor whenever its native peer becomes available or changes, including VST3 attachment and reparenting. It also tolerates temporary zero-sized editor bounds without passing a zero key width to the piano or saving that size. The peer is looked up each time; no native window pointer is retained after detachment.

JUCE remains pinned to 8.0.6. Its platform sources are unmodified. JUCE still creates its default peer before the editor can select the renderer: this change is not a guarantee that no graphics-device initialization occurs. Software rendering is a candidate workaround for the reported window failure; a graphics-driver fault has not been established. For developer A/B comparison, configure `-DGBA_WINDOWS_SOFTWARE_EDITOR=OFF` to retain JUCE's default rendering choice.

Synthesis, the 410-preset bank, 64 parameter IDs, plugin class identity and state schema are unchanged. Version 0.10.2 distinguishes the replacement binary from 0.10.1. Test this in a blank project first.

## Double-click support report

A support-only Windows download contains `Run-diagnostics.cmd`, `Collect-report.ps1` and a short readme. Extract it and double-click the launcher after reproducing the failure. The report opens in Notepad and is saved as `G-Boy-Advance-2-diagnostics.txt` in Documents. It reads the standard installed x64 VST3 hash/version, running and installed FL executable versions, matching Windows Application Error/Windows Error Reporting records, and up to three recent text crash logs in the two standard FL Documents locations. The report can include installation/project paths from those logs; review it before sharing. The tool changes no installed plugin or FL setting and uploads nothing. It is a support tool, with no replacement instrument binary or claimed fix.

The diagnostic workflow tests the Windows PowerShell launcher and fixture parsing of named crash fields, WER fields and scaled FL executable names, and excludes unrelated application records. Synthetic fixture exception codes are test data, not an observed cause of the owner's failure. A separate branch keeps the instrument code unchanged while packaging this tool. Actual Windows crash records may be absent when the host catches the exception; an empty event section does not establish that the plugin is healthy.

Revision 2 uses namespace-independent XML node selection, skips unnamed `Data` elements, and isolates malformed records so an unrelated event cannot abort all subsequent fault details. It also reads executable PE headers to identify x86/x64/ARM64 architecture and checks for FL executables beside the running host, including installations outside Program Files. Its regression fixtures cover unnamed, mixed, empty and malformed XML preceding relevant named/WER events, query failures, PE headers and host-sibling discovery. The workflow runs these fixtures in both Windows PowerShell 5.1 (used by the double-click launcher) and PowerShell 7. This repairs a diagnostic tool error; it does not change or fix the instrument itself.

## Test and installation

The installer artifact contains a single setup executable. Extract it, close FL Studio and run setup with its default paths. It replaces the common x64 VST3 bundle automatically. The portable artifact directly contains the complete `.vst3` folder and standalone executable; copy the entire folder into `C:\Program Files\Common Files\VST3\` for a manual install. Use only one installed copy of the plugin while diagnosing a version mismatch.

In 64-bit FL Studio, rescan with verification and retry the editor, then test playing notes, closing/reopening the editor and saving/reopening a test project. Record the full FL Studio build, Windows version and whether the wrapper is bridged. A successful standalone launch alone does not validate the VST3.

If the failure remains, run `collect-windows-diagnostics.ps1` from the extracted portable package in PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\collect-windows-diagnostics.ps1
```

The script writes `G-Boy-Advance-2-diagnostics.txt` in Documents with the installed plugin hash/version, standard FL executable versions and up to ten matching recent Application Error event summaries. It does not change installations, wrapper settings or running processes, and sends no report automatically. Share the faulting module name and exception code, or review and attach the report. If Windows records no matching event, use Windows+R, `perfmon /rel`, and inspect the latest FL Studio/ILBridge failure's technical details. No recorded event does not prove a clean exit.

## Automated coverage and limits

The actual-bundle Windows host test now opens the VST3 editor under a native parent six times, resizes, hides/reshows, serializes state and destroys it while another thread processes audio. It checks visible child-window attachment and finite audible processing. A second copy of the test host named `FL64.exe` exercises JUCE's executable-name-based FL detection and wrapper workarounds. That executable is a synthetic JUCE test host, not Image-Line FL Studio; passing it is not FL Studio certification. No physical DPI, audio-device or actual FL Studio test is run by this environment.

Windows/Ubuntu [run 39071110725](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39071110725), application/build commit `b9adc08fbdd05e21bc40198d6fba476bbec066b5`, passed both jobs. All ten Windows CTest suites passed in 87.40 seconds: 8,007 DSP checks, 289 application checks, 265 normal-host and 266 FL-style-name host checks. Both native editor runs completed all six attach/resize/hide/reopen/destroy cycles, processing 968 and 948 concurrent audio blocks respectively. The FL-style executable is a test harness, not FL Studio.

pluginval 1.0.4 strictness 10 passed (seed `0x7760ed3`); the separate Steinberg validator passed 47 tests with zero failures. Five installer checks passed, including the installed local diagnostic script reporting the correct VST3 hash. The installed file resource reports version 0.10.2. The 66-case native DSP profile contains 10,142 callbacks and zero over-budget calls on this runner (largest individual budget ratio 0.9633). DSP source is unchanged; timing differences from earlier runs must not be attributed to a synthesis optimization or interpreted as physical-driver certification.

Local Linux application/DSP ASan/UBSan passed 282 checks; its cached JUCE module objects were uninstrumented and leak detection was disabled. All eight local software-rendered UI snapshots match the previous version byte for byte. Downloaded GitHub digests, ZIP integrity, AMD64 PE headers, the 410-preset payload and all 18 notice hashes were verified. The installer-only artifact contains exactly one setup EXE identical to the tested installer. The portable artifact exposes the complete VST3 folder at its root; all files match the tested portable ZIP.

The 0.10.1 results in PHASE_10_FINALIZATION.md remain historical evidence. **The owner has since confirmed loading 0.10.2 through the x64 launcher. Full manual DAW/device/state acceptance and owner testing of the 0.10.3 CPU build remain open.** The underlying reported crash is not reproduced or diagnosed from a crash dump here. This is a compatibility candidate, with no production release tag or certification.
