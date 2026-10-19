# G Boy Advance 2 — 0.16.1

Removed **Vocals - Pop and Indie** (16 factory presets) and **Vocals - Southern and West Coast** (14). The active factory library now contains **620 presets in 22 categories**. All remaining preset dictionaries, names, IDs and calibrated levels are unchanged. Other vocal categories and vocal patches in the regional hip-hop banks remain available.

The removed categories and their sounds no longer appear in the factory browser, search, category lists, D-pad navigation or factory favorites/recent results. Existing user preset files are retained.

Saved projects keep their explicit synthesis settings, including edited sounds originating from the removed categories. Restoring these projects displays Saved Project as the current category and permits Save As to keep an independent user patch. Thirty original host program slots are reserved so every surviving sound retains its program number. Selecting a Reserved N (Default) slot plays Dream Pad; these slots are not factory presets.

Twenty skins, SoundFont loading/drag-drop/library recall, MIDI behavior, 64 audio parameter IDs, 16-frame latency and the prior DSP optimizations are unchanged. No audio-thread code or synthesis DSP changes are required.

## Validation

Local and Windows validation are in progress. Completion results and verified installer links will be recorded after the checks finish.

Tests cover actual states exported from the 0.16.0 processor for all thirty removed presets, including edits to master volume, cutoff and glide. The new processor must restore all parameters, original identities and host slots; render audible notes; survive resave; and permit user Save As. Browser/category checks require 620 active entries, 22 categories and no deleted category. Actual VST3 host tests also restore all thirty old states.

The full factory audit checks central notes, additional registers, velocity, release, headroom and dry sound difference. The independent multi-register audit renders 7,440 dry/wet fixtures and compares 191,890 pairs. The matched old/new DSP probe uses immutable 0.16.0 DSP with identical current arrays: 740 scenes and 11,550,720 floats. The installer acceptance script upgrades pinned 0.16.0 to 0.16.1 and checks the installed 620-preset bank and user-data preservation.

Actual owner FL Studio 20.8.3, physical audio/MIDI devices, DPI and human listening remain manual acceptance work. The owner's deferred licensing/signing decisions are unchanged; this prepares an unsigned Windows build without a production tag or main-branch merge.
