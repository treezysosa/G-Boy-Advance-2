# 0.13.0 — ROM Hacks SoundFont instruments

The ROM Hacks cartridge on the main console opens a native SoundFont browser. LOAD SF2 imports a user-provided SoundFont 2 bank. Select its bank/program instrument and play MIDI or the piano keyboard; A previews, B releases preview notes. USE SOUNDFONT enables the bank; RETURN TO SYNTH or loading a normal preset returns to synthesis. F3 toggles ROM Hacks. Main arrows/D-pad left/right browse SoundFont instruments when enabled; D-pad up/down browses banks.

Five skins, the 410 factory presets, 64 audio parameter IDs, plugin identity and the optimized synthesis DSP are retained. No general WAV/AIFF import or game-ROM loader is added. No SoundFont bank or third-party audio is bundled. The feature uses the MIT-licensed TinySoundFont decoder, vendored and pinned at `790a219810cb0fca5defa8cdbd88e2487e5efc7a`.

## Playback and limits

SoundFont 2 PCM banks support instrument/key/velocity zones, sample loops, envelopes, panning, bank/program selection and up to 32 sample voices (layered zones consume voices). MIDI channels 1–16, sustain, pitch wheel, volume/expression/pan, bank/program messages, and modulation-wheel vibrato work. A preview uses a separate internal channel so B does not release normal notes.

Shared output volume, filter cutoff/resonance/type, minimum attack, glide, vibrato and effects apply. Synth oscillator/FM/wavetable/formant controls belong to synthesis presets. SoundFont envelopes retain their bank shape; Attack adds a minimum attack. SoundFont mono mode retriggers; the synthesis engine retains its full mono/legato behavior. Arbitrary SF2 modulation tables are not implemented by TinySoundFont, and extreme generator values are bounded. SoundFonts relying on those features can differ from other players. SF3/compressed/ROM-backed samples are unsupported. Linear interpolation does not claim alias-free high transposition.

File size is limited to 128 MiB, 4096 presets, 1024 regions per preset and 65536 expanded regions per bank. Loading, decoding, caching and old-bank destruction run off the audio callback; callback publication is protected by a hazard pointer. Voices/channels and effects memory are prepared before publication. Large banks and multiple instances still require memory/loader CPU; loading is not a real-time operation.

## Project state and personal files

Imported banks are copied to `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFonts\<uuid>.sf2`. The original file is unchanged. DAW state stores the cached path and selected instrument, alongside normal synth parameters and skin; it does not embed SoundFont audio. Preserve/back up the SoundFonts folder with projects and copy/relink banks when moving computers. A missing bank produces silence and a visible ROM Hacks error rather than an unrelated synth patch. LOAD SF2 selects a replacement bank; reselect the instrument if needed.

`.gba2preset` import/export remains for synthesis patches. Save As/Export refuse to mislabel an active SoundFont as a synthesis patch; save SoundFont selections with the DAW project. Returning to synthesis preserves the loaded bank in that running instance; inactive banks are not required by subsequently saved synthesis-only projects. Bank cache files are not deleted automatically because older projects may reference them.

## Validation

[Windows/Ubuntu run 39188459523](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39188459523) passed eleven Windows CTest suites: 620,993 native DSP/preset checks (612,986 SoundFont checks plus 8,007 existing), 453 application checks including 55 SoundFont and 103 library checks, 265/266 actual-bundle synthetic-host checks, pluginval strictness 10, Steinberg 47/0 and six installer lifecycle checks. The FL-named host harness is not FL Studio.

Validated application commit `88a94d24c02947ce77090d3ad8a266674f2c64ab`, tree `458bba987b81d59f2137f91c3eba997a88e95c7e`. Subsequent documentation-only changes preserve that application/test/build source.

- Local native and own-source ASan/UBSan: 612,986 SoundFont DSP checks, including 256 structured bank mutations. Measured root/bend: 1000/2000 Hz; glide: 1040→1950 Hz; minimum-attack energy ratio 308.773; closed/open filter energy ratio 0.00422308. The native suite also checks mono mode, sustain, isolated preview, three sample rates, voice limits, volume, malformed references and absence of decoder heap operations during render/MIDI/reset.
- Local application and own-source ASan/UBSan: 446 checks, including 55 SoundFont checks. Cached JUCE modules were uninstrumented; leak detection was disabled for the environment tracer. Twelve bank publications completed with concurrent finite audio in normal and sanitizer runs. Final local block counts are in the build record.
- Windows: 453 application checks, 55 SoundFont checks, twelve publications across 2571 audio blocks; original-source deletion after caching, immediate/offline project restoration, missing-bank silence/error, instance isolation, failed-load preservation, load cancellation and native cartridge/browser controls pass. Main/ROM Hacks min/max software snapshots and five skins are retained. These do not establish physical-DPI or native file-dialog acceptance.
- pluginval 1.0.4 strictness 10 SUCCESS, seed `0x76913dc`. Official pinned Steinberg validator: 47 passed, zero failed. Windows PowerShell 5.1/7 installed-diagnostic fixture checks passed.
- Six installer checks: pinned 0.12.1 install, 0.13.0 upgrade/default VST3 path and exact hashes, installed diagnostic hash report, same-version reinstall, three-second standalone process liveness and uninstall. Old preset, personal-audio and SoundFont-cache fixture bytes are preserved. Physical audio devices and interactive/separate-account installation remain manual.
- Original synthesis DSP/effects source, all 410 factory arrays/identities and calibration values are unchanged. The 530-scene / 8,970,240-float Windows comparison is byte-identical to its prior synthesis reference.
- Original native synthesis profiling observed 0/10142 over-budget callbacks, worst ratio 0.835050. This does not profile SF2 mode or certify physical-driver deadlines/FL CPU percentages. Actual bank/voice/driver performance remains a manual check.
- Downloaded artifact SHA256/ZIP integrity, AMD64 VST3/standalone, exact installer and portable binaries, unchanged 410-patch library, all 19 pinned notices including MIT TinySoundFont, complete source inventory and absence of bundled SF2/audio assets were verified.

Test banks are generated from original mathematical waves during tests and are not runtime assets. An inactive-voice channel check in mono mode was guarded before this final build; the measured-expression and mono tests are included in its native suite.

Licensing decision remains deferred by the owner. This is unsigned build preparation; hands-on DAW/audio-device/DPI/listening and final distribution acceptance remain outstanding.


## Downloads and installation

- [Windows installer ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39188459523/artifacts/12158923159): close FL Studio/standalone, extract into a new folder, run `G-Boy-Advance-2-Setup-x64.exe`, keep default paths, then open FL64.
- [Portable/manual installation files](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39188459523/artifacts/12158658811): copy the whole `G Boy Advance 2.vst3` directory including Contents.
- [Windows validation binaries/logs/snapshots](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39188459523/artifacts/12158688729).
- [Source branch](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/rom-hacks-sf2-0130).

Use ROM Hacks → LOAD SF2, choose the bank instrument and play MIDI or the piano. This reads SoundFonts; it is not a game-ROM loader. No bank is included.

Build with Visual Studio 2022 x64 C++ tools, Windows SDK and CMake 3.24+:

```powershell
git clone --branch feature/rom-hacks-sf2-0130 https://github.com/treezysosa/G-Boy-Advance-2.git
cd G-Boy-Advance-2
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1
ctest --test-dir build -C Release --output-on-failure
```

Output: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3` and `build/GBoyAdvance2_artefacts/Release/Standalone/G Boy Advance 2.exe`. Installed VST3: `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. Standalone: `C:\Program Files\Treezy Audio\G Boy Advance 2`. Synth presets: `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`. SoundFont cache: `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFonts`.

## Remaining acceptance

Test the owner's actual SF2 banks in FL64 and complete physical audio/MIDI, real driver-buffer/CPU, DAW reload/bounce/automation, separate-account installer, native file-dialog, mixed-DPI and listening checks in RELEASE_CHECKLIST.md. Earlier owner reports concern older synth versions. Advanced SoundFont modulation tables and compressed/ROM-backed formats are unsupported as described above. Back up the external SoundFont cache with projects. Source/JUCE licensing is still deferred; signing requires an owner certificate.

Setup EXE SHA256: `437e2564c1d999bde7a69b3a507940cf9921f3664aa19f8c5268d6fe78cca67f`.
