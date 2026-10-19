# G Boy Advance 2 — 0.16.0 preset expansion

This release appends 120 original synthesis presets to the existing 530, for 650 presets in 24 categories. It retains the prior DSP optimizations, all twenty skins, ROM Hacks SoundFont loading, drag/drop, library folders and project state.

| Browser category | New presets | Total in category | Coverage |
|---|---:|---:|---|
| Hip-Hop - Detroit | 20 | 60 | FM, modal, additive and wavetable piano-style beat keys; 30 piano-tagged presets in total |
| Hip-Hop - Chicago Drill | 24 | 24 | Cold keys, bells, strings, brass, gliding basses, synthetic vox, plucks and leads |
| Funk | 24 | 24 | Slap-style and filter basses, clav-style keys, brass, organ, electric keys, physical strings, leads and pads |
| Japan Inspired | 24 | 24 | Mostly city pop, anime-style textures and original retro-game synthesis, plus a few plucked strings and flute-like sounds |
| 80s Pop | 28 | 28 | Poly brass, FM electric pianos, digital bells, basses, strings, pads, solo leads, plucks and vowel chords |

Open START, select Factory, and choose one of these categories. Search `piano` within Hip-Hop - Detroit to find all thirty piano-style sounds. The new patches are available to host program selection and ordinary favorites, recent, Save As and project recall.

`Presets/groove-designs.json` documents all 120 individually authored complete 64-parameter recipes and design notes. They have no automatically generated paired variations. `factory-index.json` appends their identities after the original 530; all prior names, IDs, ordering, tags and parameter values remain unchanged. New polyphonic patches use eight voices; mono/legato basses and solos use one. The advanced panel still permits up to 32 voices.

All sounds are original synthesis approximations of broad production characteristics. No artist voices, song melodies, recordings, game data or sampled instruments are included. User-supplied SoundFonts remain optional and separate.

## Validation

[Download the 0.16.0 Windows installer ZIP](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39265210389/artifacts/12193942690) · [Portable/manual archive](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39265210389/artifacts/12193558215) · [Source](https://github.com/treezysosa/G-Boy-Advance-2/tree/feature/groove-presets-0160). Close FL Studio and standalone, extract the installer ZIP into a new folder, run `G-Boy-Advance-2-Setup-x64.exe`, then reopen FL64. User presets, SoundFont banks and preferences are retained by the installer.

[Windows/Ubuntu run 39265210389](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39265210389) passed nine Ubuntu DSP suites and twelve Windows CTest suites: 632,474 native DSP/preset checks, 1,445 Windows application checks (332 library / 164 SoundFont), both actual-bundle test hosts (267/268 checks), pluginval strictness 10, Steinberg 47 passed / 0 failed and six installer lifecycle checks. The synthetic FL-named host is not FL Studio.

Validated application/build commit: `d1989ee1084b0ff735b69e56b00c6fd49f2efb76`. The downloaded installer, portable archive and full Windows artifact pass SHA256 and ZIP integrity checks. Both runtime binaries are AMD64 PE files. The shipped factory JSON matches the generated source exactly, includes 650 working patches, and retains the previous 530 dictionaries unchanged. Twenty main/advanced skin snapshots and nineteen pinned upstream notices are verified.

All 7,598 central/register/velocity/release factory checks pass, including 650 dry signature groups with zero near-neighbor flags. The separate fresh multi-register audit reports 0 duplicate candidates, 0 review flags, minimum dry distance 0.060104154; thresholds are unchanged. Local native tests pass 632,474 DSP/preset checks and 1,433 application checks (330 library / 159 SoundFont). Own-source ASan/UBSan passes the same 1,433 application checks; cached JUCE modules are uninstrumented and leak detection is disabled under this environment’s tracer.

The Windows 770-scene audio probe matches every byte across 11,919,360 finite floats, before/after SHA256 `e35a5735d1534cb9d421e2ee94441f2406eef12da7e9fd670c9af41edbf4956c`. The independent local GCC comparison is also byte-identical, SHA256 `735172f49c157f4e6e8aa85ad59d0f4dd913b48662f8ee836f12157053702d60`. The native playing benchmark passes all 612 note/chord/retrigger scenes with the previous and current DSP. This release retains the 0.15.0 engine optimizations; adding presets does not establish an additional DSP speedup.

The installer acceptance run verifies a pinned 0.15.0-to-0.16.0 upgrade, installed hashes, diagnostics, reinstall, standalone process liveness and uninstall while preserving user data. Standalone process liveness does not certify physical audio playback. Original synthesized listening renders for every new patch are separate bank-specific workflow artifacts; none are installed or loaded by the instrument.

The sound-difference test removes gain and effects, renders identical notes 48, 60 and 72 at velocities 0.4 and 0.9, and compares all 210,925 pairs from 7,800 dry/wet fixtures. Existing thresholds remain 0.025 for duplicate candidates and 0.06 for close-pair review. The central-note test also checks headroom, velocity, register and release behavior. These numerical screens do not replace listening.

The matched DSP/audio probe compares immutable 0.15.0 source with the current arrays: 770 scenes and 11,919,360 float samples. No audio-thread code changes are required by this preset expansion. The playing benchmark adds five new-bank examples to the existing twelve, for 612 note/chord/retrigger scenes. Shared-runner timings are observations, not FL Studio CPU percentages.

Windows packaging tests upgrade the pinned 0.15.0 installer to 0.16.0 and preserve user presets, SoundFont banks and preferences. Actual owner FL Studio 20.8.3, physical device/DPI and human listening acceptance remain manual checks. The owner’s previously deferred licensing and signing decisions are unchanged.
