# G BOY ADVANCE 2 — 0.18.3

Percussion is removed from the factory browser, category menu, search and D-pad category navigation. Its 24 factory presets are retired. All 654 remaining presets retain their names, tags, synthesis parameters, gains and public program numbers. There are 20 factory categories, plus the existing SoundFonts navigation folder, and 30 skins.

The total host-program count remains 754: 654 active sounds and 100 reserved slots. Projects with removed Percussion sounds restore the settings saved in the project under Saved Project. Save As can keep them as user presets. User files, SoundFonts, SF2 folder browsing and all other instrument categories are retained. No audio-DSP or parameter-ID changes and no new CPU improvement are claimed.

## Validation

Pending current Windows verification. The migration compares every retained factory dictionary and generated patch row with the exact 0.18.2 source and preserves all 754 registry identities. New tests reconstruct edited legacy parameter-state fixtures from the captured 0.18.2 Percussion values, then verify audible notes, state reload/resave, host-program echo and user Save As. These are parameter fixtures, not state exports from an old binary. Both VST3 wrapper hosts exercise the same cases through IComponent. The 76 actual old vocal-project fixtures remain tested.

Core synthesis/preset/diversity checks, Windows application/bundle tests, pluginval strictness 10, the official Steinberg validator and a real 0.18.2-to-0.18.3 installer upgrade are required before sharing the new installer. Actual FL Studio, physical audio/MIDI/DPI and Explorer interactions remain owner acceptance.

## Download and installation

The updated installer link will be added after validation. Close FL Studio and standalone, extract the installer ZIP into a fresh folder and run G-Boy-Advance-2-Setup-x64.exe. User presets and SoundFonts stay in their existing application-data folders.

Source branch: feature/percussion-removed-0183. Build using PowerShell -ExecutionPolicy Bypass -File scripts/build-windows.ps1 with Visual Studio 2022 x64 and CMake 3.24+. VST3 output: build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3. Installation: C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3.

The prepared build is unsigned. The owner deferred distribution licensing; this update does not choose new terms. The earlier 50% performance target remains unmet.
