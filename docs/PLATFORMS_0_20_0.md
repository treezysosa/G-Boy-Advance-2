# 0.20.0 — Windows 32-bit and universal macOS

Windows x86 and x64 0.20.0 builds and automated validation passed. Universal Mac source, build, packaging and validation scripts are implemented; Mac compilation is not yet verified. GitHub failed to allocate the first Intel runner before any steps executed ("The job was not acquired by Runner of type hosted even after multiple attempts"). Retries using macOS 15 and macOS 26 Apple Silicon pools have also queued without a runner. No Mac binary or DAW compatibility is claimed without its native build and validation.

| Platform | Formats | Architecture | Minimum deployment |
|---|---|---|---|
| Windows 32-bit | VST3, standalone EXE | x86 / Win32 | Windows supported by JUCE 8 / VS 2022; tested under WOW64 on Windows Server 2022 |
| Windows 64-bit | VST3, standalone EXE | x86_64 | Existing x64 support retained |
| macOS universal | VST3, AU, standalone APP | Intel x86_64 and Apple Silicon arm64 in each bundle | macOS 11; latest automated test targets macOS 26 ARM and Intel, not yet executed |

Modern macOS uses 64-bit applications; the Windows 32-bit build is separate from the universal Mac build. AU is the Mac format for hosts such as Logic. No VST2 or AUv3 build is claimed.

## Sound and state compatibility

DSP source and all factory parameters remain identical to the verified 0.19.3 runtime commit `2293291be4241901915e0518bf89420e1591d596`. The library retains 654 active presets, 20 categories, 754 program slots /100 reserved slots, 64 parameter IDs, 50 skins, voice budgets and 16-sample latency. Architecture-specific scalar/SSE2 paths already exist; a new platform is not a sound-quality mode. This port makes no new CPU-percentage claim or human listening claim.

Windows x86/x64 use the same user preset and SoundFont folders under `%APPDATA%\Treezy Audio\G Boy Advance 2`. On Mac the default root is `~/Library/Application Support/Treezy Audio/G Boy Advance 2`. That root contains Presets, SoundFonts, SoundFont Library and appearance/library preferences. SoundFont banks are external files, so a Windows project's cached absolute bank path must be reimported/relinked on a different OS. The parameter-only `.gba2preset` format remains the same.

## Installing Windows 32-bit

Close the DAW and standalone. Extract the x86 installer ZIP and run `G-Boy-Advance-2-Setup-x86.exe`. Use it in a 32-bit host. On 64-bit Windows the default bundle path is `C:\Program Files (x86)\Common Files\VST3\G Boy Advance 2.vst3`; its binary is under `Contents\x86-win`. A 64-bit host needs the x64 edition or an explicit host bridge.

The x86 edition has its own product/uninstall ID and Program Files directory, so it can coexist with x64. User libraries are preserved during removal. The x86 portable ZIP also contains the complete VST3 bundle and standalone; copy the whole bundle, not only its inner binary. The 32-bit process has less address space, which can limit very large SoundFonts. The diagnostic script detects a colocated standalone's architecture; `-Architecture x86` also selects its paths explicitly.

## Installing Mac

Extract the installer download and run `G-Boy-Advance-2-macOS-Universal.pkg`. The payload installs:

- VST3: `/Library/Audio/Plug-Ins/VST3/G Boy Advance 2.vst3`
- AU: `/Library/Audio/Plug-Ins/Components/G Boy Advance 2.component`
- Standalone: `/Applications/G Boy Advance 2.app`

Each bundle contains Intel and Apple Silicon slices. Rosetta is not required for an Apple Silicon host. The portable ZIP contains the same bundle payload under `package-root`; Finder can copy the bundles into the corresponding system or user plugin directories. Preserve bundle contents, symlinks and executable permissions.

Mac code is ad-hoc signed for local loading, without Developer ID signing or Apple notarization. Gatekeeper may require the owner to review and allow this specific download in System Settings; no global security disabling is recommended. The owner deferred distribution licensing/signing. Do not describe this preparation as a notarized commercial release.

To remove the Mac edition, remove the three installed bundles. Preserve the user Application Support folder to retain presets and SF2 data. The common `/Library/Application Support/Treezy Audio/G Boy Advance 2` folder contains installed manuals/notices, separate from personal data.

## Building

Source branch: `feature/windows32-macos-0200`.

```powershell
# Visual Studio 2022, Windows SDK and CMake 3.24+
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -Architecture x86
powershell -ExecutionPolicy Bypass -File scripts/package-windows.ps1 -Architecture x86 -BuildDirectory build-x86 -Iscc 'C:/Program Files (x86)/Inno Setup 6/ISCC.exe'
```

```sh
# Mac: Xcode command-line tools, CMake and Ninja
bash scripts/build-macos.sh
bash scripts/package-macos.sh
```

JUCE is pinned to 8.0.6. The build script accepts an existing `Dependencies/JUCE` checkout; otherwise CMake fetches the pinned dependency. Mac formats are added only on Apple builds. Universal architectures are configured as `arm64;x86_64`. The native code and factory library do not need an internet connection at runtime.

## Completed Windows validation

The shipped Windows binaries were compiled from commit `ed0cbdf2d727dc7332a1a9b021ff8883f4925516`. Later Mac workflow/test-thread portability and documentation commits preserve the instrument source, DSP, factory data and parameter identities. Local ZIP CRC, PE machine and VST3/standalone SHA-256 comparisons also passed.

| Check | Windows x86 | Windows x64 |
|---|---|---|
| Native build and test suites | 12 passed in 32-bit processes | 12 passed in 64-bit processes |
| Actual VST3 host checks | 887; 888 under the matching FL-style executable name | Both matching-bitness host suites passed |
| JUCE application integration | 6,161 checks | 6,161 checks |
| Official Steinberg validator | 47 passed, 0 failed; validator compiled Win32 | 47 passed, 0 failed |
| pluginval | Not run: official downloaded Windows tool is x64 | Strictness 10 passed |
| Installer | Clean/reinstall/launch/uninstall, user data preserved; existing x64 binary unchanged | 0.19.3 upgrade/reinstall/launch/uninstall, user data preserved |

Evidence: [Windows x86 build and validation](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43259811064) (the Mac allocation failure makes the overall run red, while every Windows step passed), [Windows x64 and independent core suites](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43259810885), [macOS 15 ARM retry](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43264242887), [latest macOS 26 ARM retry](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43265581053).

## Remaining Mac validation and owner acceptance

If the queued Mac job is cancelled for runner allocation again, rerun the `mac-universal` job when GitHub Mac capacity is available. Alternatively run the documented build/package commands on a Mac, install the resulting package, run `scripts/validate-macos.sh` and `scripts/verify-platform.py --platform macos --build build-macos --installed`, then test the exact package and carried universal test executables on the other CPU architecture. Native compiler/test failures must be resolved before publishing a Mac download. Developer ID signing/notarization remains deferred.

Windows x86 passed all twelve suites in actual 32-bit test processes, both matching-bitness VST3 hosts and the official Steinberg validator built as Win32. The official downloaded pluginval Windows executable is x64, so it is not misused to claim 32-bit validation. The clean/reinstall/launch/uninstall test checked x86 beside an installed verified x64 release and confirmed the x64 plugin hash and user data remained unchanged. PE headers identified both shipped instrument binaries as x86.

The Mac build must pass eleven suites, pluginval strictness 10 and AU validation on Apple Silicon. The same universal package and universal test executables are then carried to an Intel runner and executed with `arch -x86_64`; no Intel rebuild substitutes for the delivered binary. Both machines install the actual package and load its VST3/AU. `lipo`, bundle version, code-signature and SHA-256 checks cover all three universal bundles. The standalone receives a process-liveness check; physical audio/MIDI is not inferred from it.

All 50 skin selections, snapshots, preset browser, MIDI, project state, SoundFont import/library behavior and synth tests remain in the test suites. Physical input/display/device behavior and the owner's actual FL Studio/Logic/Ableton/REAPER/Cubase/Studio One sessions remain manual acceptance. The synthetic FL-named host is not FL Studio.
