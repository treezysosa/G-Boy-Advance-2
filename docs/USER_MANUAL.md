# G BOY ADVANCE 2 user manual

## Playing

Load the instrument on a MIDI/instrument track with stereo output. Play a MIDI controller or click the piano keys. The standalone has an audio/MIDI settings menu for device selection. A real audio driver and an enabled MIDI input are required for controller playback.

- Preset display: click to open the browser. The arrow buttons select the adjacent patch.
- D-pad left/right: adjacent patch. Up/down: previous/next category.
- START / F1: preset browser. SELECT / F2: advanced synthesis. Escape or BACK returns to the main panel.
- A: audition C4 for about 650 ms, then release through the patch envelope.
- B: release preview voices. Long effect / release tails can remain audible. Preview uses the current voice budget, so it can steal an older note if all voices are occupied.
- VOLUME: master output gain. GLIDE: note-to-note portamento time.
- A–D knobs: cutoff, resonance, attack, and reverb by default. Their names are shown in tooltips; SELECT → Effects & Macros reassigns each knob.
- PITCH: MIDI channel 1 pitch bend. Releasing the wheel returns it to center. External MIDI bend works separately on each channel.
- Piano keys: mouse/touch or the JUCE computer-keyboard layout when the keyboard has focus. The scroll controls expose other octaves.

Drag a corner to resize. The wide proportions stay fixed between about 960 and 1920 pixels. Double-click a knob to return its target parameter to its default value.

## Browser

Search matches names, category labels, and tags. Select a category, optionally filter favorites / recently used patches, then click a row or move with the arrow keys to load it. Enter or double-click auditions the selected patch. Preview and Stop are also available as buttons.

FAVORITE toggles the current patch. Recently used patches are ordered newest first. Enter a name in the bottom field, then SAVE AS to save the current parameter values as a new user patch. RENAME and DELETE work only on user patches. Deleting asks for confirmation. RESTORE DREAM PAD loads the original default factory patch; it does not erase user patches.

User files live in `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`. Copy `.gba2preset` files into that directory, then create a new plugin instance to rescan them. Favorites and recent IDs are stored in the adjacent `library.json`. Write failures appear in the browser status line. Factory patches are embedded and always available offline.

## Advanced synthesis

**Oscillators:** select one of eight engines and its timbre controls. Saw / square / pulse use PolyBLEP; additive and generated wavetables limit harmonic content. FM and modal engines limit upper partials where feasible. These strategies reduce aliasing but do not certify alias-free output at every setting.

**Filter & envelopes:** low-pass, high-pass, band-pass, notch, or a vowel filter bank; resonance, cutoff, and filter-envelope amount. Both amplitude and filter ADSRs are editable. Releasing a note starts the corresponding release stage. A zero-sustain pluck naturally fades while the key is held.

**Modulation & voices:** two sine LFOs route independently to pitch, filter, amplitude, vowel, or pan. Vibrato and tremolo have dedicated depth controls; MIDI CC1 adds vibrato. Choose 1–32 voices, mono retriggering, or legato with held-note return. CC64 controls sustain per MIDI channel. CC120 silences that channel immediately; CC123 releases it.

**Effects & macros:** reverb, delay, chorus, phaser, flanger, saturation, distortion, bitcrusher depth/rate, wow/flutter, two tone shelves, and stereo width. The effects are native DSP. Extreme settings can change perceived loudness; use the master knob. Output is softly bounded to protect host routing from extreme combinations.

Macro knobs are parameter proxies: reassigning one binds it to a different existing host parameter. Automate the destination parameter ID in the DAW. This keeps cutoff or attack automation stable when a front-panel assignment changes. Assignments are saved in presets and projects.

## Synthetic vocals and retro sounds

Vocal patches combine harmonic excitation, unvoiced noise, three interpolated vowel resonances, breath, detuned choir excitation, vibrato, and optional syllabic motion. They sing synthesized vowel-like timbres at the MIDI pitch. They do not speak words, reproduce languages, or imitate performers. Style names describe broad production textures.

Retro sounds are original oscillators and mathematical patches. There are no game ROMs, ripped sounds, sample banks, recorded vocals, sample loading, or network services.

## Projects and troubleshooting

DAW project state includes every stable parameter, macro assignment, patch metadata, and editor size. User files do not have to remain installed for the saved project sound to restore. Favorites and history are library preferences rather than per-project data.

If silent: confirm the track is an instrument track, MIDI routing is active, stereo output is routed, master volume is nonzero, and the audio device is running. Try A to audition. A long-attack patch can take time to become loud. Verify plugin scan paths and use the complete `.vst3` bundle, not just the binary inside it.

This development version still needs Windows/DAW acceptance and listening review. Consult VALIDATION.md before production use.
