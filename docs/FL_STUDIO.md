# FL Studio editor compatibility investigation

## Reported failure

The owner reports that the Windows standalone works, but FL Studio 20.8.3, 64-bit, loses the connection when opening the VST3 editor. The failure is reported as continuing after the 0.10.2 candidate was supplied; the installed binary version/hash has not yet been confirmed by a diagnostic report. Initial download/install discovery problems were resolved before this report. The Windows version, full FL build number and crash module/exception are not yet known. The error text alone does not identify the cause. This is an open compatibility failure, not a passed FL Studio acceptance test.

## 0.10.2 compatibility candidate

This branch selects JUCE's Software Renderer for the Windows editor whenever its native peer becomes available or changes, including VST3 attachment and reparenting. It also tolerates temporary zero-sized editor bounds without passing a zero key width to the piano or saving that size. The peer is looked up each time; no native window pointer is retained after detachment.

JUCE remains pinned to 8.0.6. Its platform sources are unmodified. JUCE still creates its default peer before the editor can select the renderer: this change is not a guarantee that no graphics-device initialization occurs. Software rendering is a candidate workaround for the reported window failure; a graphics-driver fault has not been established. For developer A/B comparison, configure `-DGBA_WINDOWS_SOFTWARE_EDITOR=OFF` to retain JUCE's default rendering choice.

Synthesis, the 410-preset bank, 64 parameter IDs, plugin class identity and state schema are unchanged. Version 0.10.2 distinguishes the replacement binary from 0.10.1. Test this in a blank project first.

## Double-click support report

A support-only Windows download contains `Run-diagnostics.cmd`, `Collect-report.ps1` and a short readme. Extract it and double-click the launcher after reproducing the failure. The report opens in Notepad and is saved as `G-Boy-Advance-2-diagnostics.txt` in Documents. It reads the standard installed x64 VST3 hash/version, running and installed FL executable versions, matching Windows Application Error/Windows Error Reporting records, and up to three recent text crash logs in the two standard FL Documents locations. The report can include installation/project paths from those logs; review it before sharing. The tool changes no installed plugin or FL setting and uploads nothing. It is a support tool, with no replacement instrument binary or claimed fix.

The diagnostic workflow tests the Windows PowerShell launcher and fixture parsing of named crash fields, WER fields and scaled FL executable names, and excludes unrelated application records. Synthetic fixture exception codes are test data, not an observed cause of the owner's failure. A separate branch keeps the instrument code unchanged while packaging this tool. Actual Windows crash records may be absent when the host catches the exception; an empty event section does not establish that the plugin is healthy.

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

The 0.10.1 results in PHASE_10_FINALIZATION.md remain historical evidence. **Actual FL Studio acceptance stays open until the owner can load and use the replacement VST3.** The underlying reported crash is not reproduced or diagnosed from a crash dump here. This is a compatibility candidate, with no production release tag or certification.
