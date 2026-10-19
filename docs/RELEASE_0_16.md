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

Local calibration, full-bank sound-difference checks, application tests and the Windows build are in progress. Completion evidence and download links will be recorded after checks finish; this source document does not yet claim a passing Windows build.

The sound-difference test removes gain and effects, renders identical notes 48, 60 and 72 at velocities 0.4 and 0.9, and compares all 210,925 pairs from 7,800 dry/wet fixtures. Existing thresholds remain 0.025 for duplicate candidates and 0.06 for close-pair review. The central-note test also checks headroom, velocity, register and release behavior. These numerical screens do not replace listening.

The matched DSP/audio probe compares immutable 0.15.0 source with the current arrays: 770 scenes and 11,919,360 float samples. No audio-thread code changes are required by this preset expansion. The playing benchmark adds five new-bank examples to the existing twelve, for 612 note/chord/retrigger scenes. Shared-runner timings are observations, not FL Studio CPU percentages.

Windows packaging tests upgrade the pinned 0.15.0 installer to 0.16.0 and preserve user presets, SoundFont banks and preferences. Actual owner FL Studio 20.8.3, physical device/DPI and human listening acceptance remain manual checks. The owner’s previously deferred licensing and signing decisions are unchanged.
