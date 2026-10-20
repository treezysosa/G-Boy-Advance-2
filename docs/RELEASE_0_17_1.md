# G Boy Advance 2 — 0.17.1

## Shorter preset folder names

The four regional factory categories are renamed:

| Previous folder | Current folder | Presets |
|---|---|---:|
| Hip-Hop - Detroit | Detroit | 60 |
| Hip-Hop - West Coast | West Coast | 40 |
| Hip-Hop - Florida | Florida | 40 |
| Hip-Hop - Chicago Drill | Chicago | 24 |

All 598 factory sounds, preset names, tags, parameter dictionaries, calibrated levels and IDs are unchanged. There are still 18 categories, including Brass with 40 sounds and Vocals with 74. All 674 host program slots, 76 reserved slots and 64 parameter IDs keep their identities and order.

Browser category selection and D-pad navigation use the new labels. Recognized factory project IDs display the current category while keeping explicit saved settings, including edits. Search tags retain the original production-style terms. Existing user presets, SoundFonts, favorites and recent history remain independent of these display names. Internal design recipe categories are retained, with the display names applied after synthesis generation, so the sounds stay identical.

## Validation

All 598 factory dictionaries were compared against the exact 0.17.0 source: only 164 category fields change. The compiled bank changes only the four category strings. Native DSP source, synthesis designs and calibration gains are unchanged. Existing browser/library tests and installed-bank acceptance checks use the new names.

Local JUCE application/browser checks: **2,987 passed**. [Windows/Ubuntu run 39465006237](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39465006237) passed **nine Ubuntu suites and twelve Windows CTest suites**, including **2,999 Windows application checks**, both actual-bundle hosts (**647 / 648** checks), pluginval strictness 10, Steinberg **47 passed / 0 failed**, and **six installer lifecycle checks** covering the **0.17.0 → 0.17.1** upgrade. Installed-bank checks verify all four new labels and their counts, and reject old Hip-Hop category names. The synthetic FL-named host is not FL Studio. The 718-scene numerical audio comparison and 148-scene sustained comparison remain bit-identical against the existing pinned 0.16.1 DSP baseline with the current factory arrays. That retains the established regression harness while production DSP and all sounds are unchanged from 0.17.0.

FL Studio itself and a physical Windows audio device have not been run in this environment. The owner has deferred the distribution licensing decision; this prepared build does not resolve it.

## Installation

Close FL Studio and the standalone app, extract the installer download into a fresh folder and run `G-Boy-Advance-2-Setup-x64.exe`. User presets, SoundFonts and preferences are retained. The VST3 installs to `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. The default SoundFont folder remains `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library`.

## Prepared Windows build

[Windows installer](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39465006237/artifacts/12285962556) · [Portable/manual archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39465006237/artifacts/12285987607) · [Source branch](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/folder-names-0171). The validated runtime commit is `035742a53c5b902fcefd6c5894e9f03419436b91`. Final source differs only in five documentation files to record results and links. Downloaded artifact digests, installer and portable payloads, actual AMD64 VST3/standalone binaries, the installed 598-preset bank, regional labels and bundled manual were verified.
