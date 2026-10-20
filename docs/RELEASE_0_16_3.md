# G Boy Advance 2 — 0.16.3

## Unified Vocals folder

The factory browser now groups the 24 Global Pop Textures presets, 14 Horror presets and 36 Choirs and Textures presets under **Vocals**. All 74 sounds are retained, with their existing names, IDs, tags, synthesis settings and calibrated levels. The complete bank contains **574 presets across 18 categories**.

Preset search, favorites, recent history, host program numbers and user presets keep their existing identities. The D-pad and browser use the single Vocals category. Older projects with a recognized factory preset ID display its current category while retaining the explicit saved parameters, including edits. The 76 previously retired factory slots remain reserved and retain their existing project-restoration behavior.

This release retains the 0.16.2 pad/string and SoundFont CPU improvements. Native DSP, effects, SoundFont processing, voice limits, skins and ROM Hacks workflows are unchanged.

## Validation

The generated 574-preset bank has been compared against the exact 0.16.2 source: only 74 category fields change. All preset parameter dictionaries, names, tags and IDs match; all 650 host identities and their order match. The generator preserves the original internal recipe groups and search tags, changing the output category after synthesis recipes have run.

Application regression coverage checks the 74-sound Vocals filter, absence of the three old browser categories, navigation through all 18 categories, and edited project restoration/resave from each previous category. The previous retired-category project cases remain enabled. Installer acceptance checks require the installed 574-preset bank, 18 categories and exactly 74 Vocals entries, and test upgrade from 0.16.2 while preserving user data.

Local JUCE application checks: **2,890 passed**, including the merged folder, edited project migration, browser navigation, existing retired-category migration, preset saving and SoundFont library behavior. Pinned notices and complete 139-file source packaging passed. The local factory audit passed all **6,724** checks. [Windows/Ubuntu run 39451806321](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39451806321) passed **nine Ubuntu suites and twelve Windows CTest suites**, including **2,902 Windows application checks**, both actual-bundle hosts (**647 / 648** checks), pluginval strictness 10, Steinberg **47 passed / 0 failed**, and **six installer lifecycle checks**, including the **0.16.2 → 0.16.3** upgrade. The synthetic FL-named host is not FL Studio. Numerical audio comparisons are bit-identical across the 694-scene and 148-scene sustained probes; the 6,888-render diversity audit has zero duplicate or review flags. Both numerical probes compare against the pinned 0.16.1 DSP with the current factory arrays, retaining the existing CPU-regression baseline.

FL Studio itself and a physical Windows audio device have not been run in this environment. The owner has deferred the distribution licensing decision; this prepared build does not resolve that decision.

## Windows installation

Close FL Studio and the standalone app. Extract the installer download into a fresh folder, then run `G-Boy-Advance-2-Setup-x64.exe`. Existing user presets, SoundFonts and preferences are retained. The VST3 bundle installs to `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`; the standalone installs under `C:\Program Files\Treezy Audio\G Boy Advance 2`.

The default user SoundFont folder remains `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library`. User presets remain under `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`.

## Prepared build

[Windows installer](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39451806321/artifacts/12279079837) · [Portable/manual archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39451806321/artifacts/12279986533) · [Source branch](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/unified-vocals-0163). The validated runtime commit is `a2edbe489fd4cd5fe6689ce41d72b6fa31d340a6`; the final source changes only documentation to record these results and download links. The downloaded installer and portable payloads, actual AMD64 binaries, installed factory bank and bundled manual are checked against the successful run.
