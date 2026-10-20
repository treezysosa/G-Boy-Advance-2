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

Local application and Windows build/validator/installer results: pending.

FL Studio itself and a physical Windows audio device have not been run in this environment. The owner has deferred the distribution licensing decision; this prepared build does not resolve it.

## Installation

Close FL Studio and the standalone app, extract the installer download into a fresh folder and run `G-Boy-Advance-2-Setup-x64.exe`. User presets, SoundFonts and preferences are retained. The VST3 installs to `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. The default SoundFont folder remains `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library`.
