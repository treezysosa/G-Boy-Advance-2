# G Boy Advance 2 — 0.17.0

## Brass factory folder

**Brass** replaces the top-level Sax-Style Leads category. All **16 existing sax sounds** are directly available alongside **24 newly authored synthesis brass presets**, for **40 Brass sounds** and **598 factory presets across 18 categories**.

Open START, select Brass and browse all 40 sounds. An optional family selector narrows the results to Sax-Style Leads, Trumpets, Trombones, Horns, Brass Sections or Synth Brass. Search, source filters, favorites and recent history work together with the selected family. Changing category resets the family filter. The main console interface retains its existing layout.

The new instruments use subtractive, FM, additive, mathematical wavetable and chiptune synthesis. They are original synthesized approximations, with no recorded brass or external bank requirement. All new patches use at most eight voices and releases no longer than 1.6 seconds. The established pad/string and SoundFont CPU improvements remain in place; the DSP engine itself is unchanged.

## Compatibility

Every one of the 574 previous presets retains its sound settings, name, tags and ID. The 16 sax presets change category metadata only. All 650 previous host slots keep their positions, including 76 reserved slots. New sounds append slots 650–673, giving 674 host program slots and 64 unchanged parameter IDs. Older edited sax projects display Brass while keeping their explicit saved settings. User presets and SoundFont libraries remain independent.

## Validation

All 574 prior parameter dictionaries have been compared with the exact 0.16.3 source and are unchanged. New presets have complete, range-checked synthesis dictionaries and native level calibration; existing calibrated master gains are preserved.

The local factory sound audit passed **7,000 checks** across 598 patches. The all-pair diversity screen passed **7,232 checks**, covering **7,176 renders and 178,503 pairs**, with zero duplicate or review flags. Local JUCE UI/project integration passed **2,987 checks**, including Brass browsing, family/search intersections and edited project restoration for all 16 sax sounds. Windows build, VST3 validator and installer upgrade checks are pending.

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
