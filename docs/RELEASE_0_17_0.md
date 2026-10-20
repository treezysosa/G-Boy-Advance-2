# G Boy Advance 2 — 0.17.0

## Brass factory folder

**Brass** replaces the top-level Sax-Style Leads category. All **16 existing sax sounds** are directly available alongside **24 newly authored synthesis brass presets**, for **40 Brass sounds** and **598 factory presets across 18 categories**.

Open START, select Brass and browse all 40 sounds. An optional family selector narrows the results to Sax-Style Leads, Trumpets, Trombones, Horns, Brass Sections or Synth Brass. Search, source filters, favorites and recent history work together with the selected family. Changing category resets the family filter. The main console interface retains its existing layout.

The new instruments use subtractive, FM, additive, mathematical wavetable and chiptune synthesis. They are original synthesized approximations, with no recorded brass or external bank requirement. All new patches use at most eight voices and releases no longer than 1.6 seconds. The established pad/string and SoundFont CPU improvements remain in place; the DSP engine itself is unchanged.

## Compatibility

Every one of the 574 previous presets retains its sound settings, name, tags and ID. The 16 sax presets change category metadata only. All 650 previous host slots keep their positions, including 76 reserved slots. New sounds append slots 650–673, giving 674 host program slots and 64 unchanged parameter IDs. Older edited sax projects display Brass while keeping their explicit saved settings. User presets and SoundFont libraries remain independent.

## Validation

All 574 prior parameter dictionaries have been compared with the exact 0.16.3 source and are unchanged. New presets have complete, range-checked synthesis dictionaries and native level calibration; existing calibrated master gains are preserved.

The local factory sound audit passed **7,000 checks** across 598 patches. The all-pair diversity screen passed **7,232 checks**, covering **7,176 renders and 178,503 pairs**, with zero duplicate or review flags. Local JUCE UI/project integration passed **2,987 checks**, including Brass browsing, family/search intersections and edited project restoration for all 16 sax sounds. [Windows/Ubuntu run 39460496611](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39460496611) passed **nine Ubuntu suites and twelve Windows CTest suites**, including **2,999 Windows application checks**, both actual-bundle hosts (**647 / 648** checks), pluginval strictness 10, Steinberg **47 passed / 0 failed**, and **six installer lifecycle checks** covering the **0.16.3 → 0.17.0** upgrade. The synthetic FL-named host is not FL Studio. The 718-scene numerical audio comparison and 148-scene sustained comparison are bit-identical against the existing pinned 0.16.1 DSP baseline using the current factory arrays. That retains the established regression harness while the production DSP is unchanged from 0.16.3.

FL Studio itself, a physical audio device and human listening acceptance have not been run in this environment. The owner has deferred the distribution licensing decision; this prepared build does not resolve it.

## Installation

Close FL Studio and the standalone app, extract the installer download into a fresh folder and run `G-Boy-Advance-2-Setup-x64.exe`. User presets, SoundFonts and preferences are retained. The VST3 installs to `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. The default SoundFont folder remains `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library`.

## New brass presets

- Velvet Jazz Trumpet
- Bright Soul Trumpet
- Muted Midnight Trumpet
- Funk Trumpet Staccato
- Warm Slide Trombone
- Low Growl Trombone
- Round Tenor Trombone
- Short Bounce Trombone
- Warm French Horn
- Cinematic Horn Bloom
- Muted Chamber Horn
- Sunrise Horn Call
- Soul Horn Section
- Funk Horn Punch
- City Pop Brass Section
- Cinematic Low Brass
- Marching Brass Stab
- Vintage Studio Brass
- Analog Poly Brass
- 80s Arena Synth Brass
- FM Chrome Brass
- Neon Pulse Brass
- Retro Arcade Brass
- Dream Brass Pad

## Prepared Windows build

[Windows installer](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39460496611/artifacts/12284335363) · [Portable/manual archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39460496611/artifacts/12284355262) · [Source branch](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/brass-library-0170). The validated runtime commit is `7500767354b80d1ff44cfbe0edc2e48b6fbc6e24`. Final source differs only in five documentation files to record these results and links. Downloaded artifact digests, installer and portable payloads, actual AMD64 VST3/standalone binaries, the installed 598-preset bank, Brass/sax metadata and bundled manual were verified.
