# G BOY ADVANCE 2 user manual

## Playing

Load the instrument on a MIDI/instrument track with stereo output. Play a MIDI controller or click the piano keys. The standalone has an audio/MIDI settings menu for device selection. A real audio driver and an enabled MIDI input are required for controller playback.

- Preset display: click to open the browser. The arrow buttons select the adjacent patch.
- D-pad left/right: adjacent patch. Up/down: previous/next category.
- START / F1: preset browser. SELECT / F2: advanced synthesis. Press the same F1/F2 shortcut again, Escape, or BACK to return to the main panel. Hidden main controls are excluded from keyboard interaction while a panel is open; the piano and pitch/glide controls remain available.
- A: audition C4 for about 650 ms, then release through the patch envelope.
- B: release preview voices. Long effect / release tails can remain audible. Preview uses the current voice budget, so it can steal an older note if all voices are occupied.
- VOLUME: master output gain. GLIDE: note-to-note portamento time.
- A–D knobs: cutoff, resonance, attack, and reverb by default. Their names are shown in tooltips; SELECT → Effects & Macros reassigns each knob.
- PITCH: MIDI channel 1 pitch bend. Releasing the wheel returns it to center. External MIDI bend works separately on each channel.
- Piano keys: mouse/touch or the JUCE computer-keyboard layout when the keyboard has focus. The scroll controls expose other octaves. Octave labels appear while the piano is hovered or focused.

Drag a corner to resize. The wide proportions stay fixed between about 960 and 1920 pixels. Double-click a knob to return its target parameter to its default value.

## Browser

Search matches names, category labels, and tags. Select a category, optionally filter favorites / recently used patches, then click a row or move with the arrow keys to load it. Enter or double-click auditions the selected patch. Preview and Stop are also available as buttons.

FAVORITE toggles the current patch. Recently used patches are ordered newest first. Enter a name in the bottom field, then SAVE AS to save the current parameter values as a new user patch. RENAME and DELETE work only on user patches. Deleting asks for confirmation. RESTORE DREAM PAD loads the original default factory patch; it does not erase user patches.

User files live in `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`. Use IMPORT in the browser, or copy `.gba2preset` files into that directory and press REFRESH. Favorites and recent IDs are stored in the adjacent `library.json`. Write failures appear in the browser status line. Factory patches are embedded and always available offline.

## Advanced synthesis

**Oscillators:** select one of eight engines and its timbre controls. Saw / square / pulse use PolyBLEP; additive and generated wavetables limit harmonic content. FM and modal engines limit upper partials where feasible. These strategies reduce aliasing but do not certify alias-free output at every setting.

**Filter & envelopes:** low-pass, high-pass, band-pass, notch, or a vowel filter bank; resonance, cutoff, and filter-envelope amount. Both amplitude and filter ADSRs are editable. Releasing a note starts the corresponding release stage. Repeated pedal-up/controller messages do not restart an existing release. Synthesis-engine, waveform, filter-type and LFO-destination changes blend over 5 ms. Selecting a plucked or modal engine on a held note excites that physical model; leaving it fully and returning excites it again. The vocal engine and formant filter use independent resonators. A zero-sustain pluck naturally fades while the key is held.

**Modulation & voices:** two sine LFOs route independently to pitch, filter, amplitude, vowel, or pan. Vibrato and tremolo have dedicated depth controls; MIDI CC1 adds vibrato. Choose 1–32 voices, mono retriggering, or legato with held-note return. CC64 controls sustain per MIDI channel. CC120 silences that channel immediately; CC123 releases it.

**Effects & macros:** reverb, delay, chorus, phaser, flanger, saturation, distortion, bitcrusher depth/rate, wow/flutter, two tone shelves, and stereo width. The effects are native DSP. Saturation is soft; distortion adds a harder clipping characteristic. Delay-time changes crossfade over 20 ms. Wow blends tape-like pitch movement into the signal. The instrument reports a constant 16-sample latency, automatically compensated by supporting DAWs. Tail reporting follows envelope release and feedback settings; very long delays may advertise over two minutes of tail. Your DAW's export-tail settings determine the rendered ending. Extreme settings can change perceived loudness; use the master knob. Output is softly bounded to protect host routing from extreme combinations.

Macro knobs are parameter proxies: reassigning one binds it to a different existing host parameter. Automate the destination parameter ID in the DAW. This keeps cutoff or attack automation stable when a front-panel assignment changes. Assignments are saved in presets and projects.

## Synthetic vocals and retro sounds

Vocal patches combine harmonic excitation, unvoiced noise, three interpolated vowel resonances, breath, detuned choir excitation, vibrato, and optional syllabic motion. They sing synthesized vowel-like timbres at the MIDI pitch. They do not speak words, reproduce languages, or imitate performers. Style names describe broad production textures.

Retro sounds are original oscillators and mathematical patches. There are no game ROMs, ripped sounds, sample banks, recorded vocals, sample loading, or network services.

## Projects and troubleshooting

DAW project state includes every stable parameter, macro assignment, patch metadata, and editor size. User files do not have to remain installed for the saved project sound to restore. Favorites and history are library preferences rather than per-project data.

If silent: confirm the track is an instrument track, MIDI routing is active, stereo output is routed, master volume is nonzero, and the audio device is running. Try A to audition. A long-attack patch can take time to become loud. Verify plugin scan paths and use the complete `.vst3` bundle, not just the binary inside it.

This development version passes Windows compilation, automated tests and pluginval strictness 10. Interactive DAW/device acceptance, high-DPI checks and listening review remain pending. Consult VALIDATION.md before production use.

## Factory bank revision 0.6

The bank retains all 382 legacy factory names/IDs and program positions, with 28 new retro entries appended (410 total). Named instruments have explicit synthesis profiles and Afterglow variants change active excitation/envelope controls. Factory levels are calibrated using a 100 ms stereo RMS metric; quieter articulations remain quieter when the master knob reaches its gain ceiling. This is not perceptual loudness normalization.

Saved projects restore their actual parameter values, even when their factory ID also exists in the revised bank. Loading the factory preset again selects its new voicing. User presets remain independent. Optional category audition WAVs are original test renders for listening review; they are never loaded by the instrument.

## Synthetic vocal controls (0.7.0)

Select a vocal preset or choose the Formant engine in SELECT. Vowel moves continuously through A/E/I/O/U; formant shift changes the resonance range independently of note pitch. Brightness opens the harmonic source's spectral tilt. Breath blends harmonic excitation into filtered noise; high settings create whisper-like tone. Choir blends two additional pitched layers; detune controls their spacing. LFO destination Vowel and syllabic motion animate the vowel. Pitch bend, modulation wheel, velocity, sustain and glide remain playable. These produce melodic synthetic vowel textures, not spoken phrases or identifiable voices.

Version 0.7.0 revises engine 5, so older projects restore the same parameters but can sound different. Audition before replacing a production installation. Offline test artifacts contain 12 vocal examples plus 17 category excerpts; the instrument never reads these audio files.

## Retro synthesis (0.8.0)

Choose the Retro Games category or select Chiptune in SELECT. Oscillator 1 and 2 waveforms, blend, coarse tuning and detune now work in this engine. Pulse width changes pulse duty. Brightness opens the source tone filter and blends long/short clocked noise when Noise is selected. Triangle and sine have 16 digital levels; saw has 32. The deliberate digital steps can alias. Use the shared filter, ADSRs, LFOs, glide, bitcrusher and effects to shape the result. The retro bank also uses FM, additive, original wavetables, plucked strings, modal keys and formants for other technical eras. No commercial game sound, ROM or audio sample is used.

All 14 original requested retro names remain; 14 additional designs each have an Afterglow variation. Search style tags such as `16-bit FM`, `early 3D` or `arcade`. Existing chiptune projects restore their saved parameters but may sound different under the revised engine. Offline retro audition WAVs are test outputs, never plugin assets.

## Browser workflow (0.9.0)

START opens the library. Combine category, All sources/Factory/User, Favorites only and Recently used. Every search word must match a name, category or tag; quote a phrase such as `"early 3D"`. Recent results are ordered most recently used first. Arrows/Home/End/Page keys browse without changing the current sound. Click to load; Enter or Space loads and previews. A PREVIEW also loads the highlighted row first. B STOP releases preview. Ctrl+F focuses search; Up/Down moves from search to results. F5 or REFRESH reloads user files and library metadata. Escape or BACK/START returns to the console.

Save As saves the loaded sound and optional tags, with a unique user filename. If another row is highlighted, load it first or clear the selection to save the current loaded project sound. Duplicate user display names are rejected case-insensitively. Rename and Delete act on the selected user patch; Delete asks for confirmation. Factory patches use Save As. Filters remain selected after these actions, so a new User patch may be hidden by an active Factory/category filter. Empty results disable actions that require a selected patch.

Refresh never replaces the parameters currently playing in a project. After Refresh, externally edited user files take effect when explicitly loaded; Rename retains their latest stored parameter values. Separate plugin instances merge sequential favorites/history updates. Separate DAW processes can still race simultaneous metadata edits. Construction/refresh do not add history; loading a preset does. Invalid preset files are skipped and reported. If metadata is corrupt, back up `library.json` before repairing/removing it; the plugin retains that file and reports persistence errors. Preset files and project state remain independent. Disk work runs outside the audio callback, but slow storage can delay the browser.

## Bypass and development-build acceptance

Version 0.10.0 consumes MIDI and advances synthesis while host-bypassed, muting its stereo output. Note-offs and sustain changes during bypass therefore continue to affect the instrument. Bypass retains normal synthesis CPU use. The fixed processing latency remains 16 samples; host compensation and export-tail behavior require the DAW checks in RELEASE_CHECKLIST.md.

These unsigned development builds still require manual host, audio-device, high-DPI/accessibility and listening acceptance. The callback profiler exposes timing limits under 32-voice/rapid-engine automation loads; consult PERFORMANCE.md before release use.

## Engine changes in 0.10.1

Engine changes crossfade for 5 ms. If automation requests another engine during a fade, the current fade finishes and then the latest request begins; intermediate requests are coalesced. After the next audio block observes it, a held engine selection settles within about 10 ms (two fades rounded to sample frames). This bounds processing to two engines and avoids the accumulating CPU cost of rapidly retargeted blends. All 32 voices remain available. Existing waveform/filter/LFO fade behavior is unchanged.

Third-party notice files are included with portable and installed builds. The owner must choose applicable licensing terms before distribution. Optional code signing requires an owner certificate.


## Skins and custom instruments (0.11.0)

Press SELECT and choose SKIN in the header: Purple, Glacier Blue, Black, White or Spice Orange. Each plugin instance keeps its own colour. Save the DAW project to retain it. Standalone state also retains the choice on normal shutdown. Preset changes keep the chosen skin. Old projects open in Purple. The sound and automation parameters are independent of appearance.

To design a custom instrument, select an engine and adjust oscillators, filters, envelopes, modulation and effects in SELECT. Use START → SAVE AS to add it to the User library. All custom instruments are synthesis patches; there is no sample mapping or audio import.

START → IMPORT opens a `.gba2preset` file chooser. The plugin validates the JSON/schema, parameter types, identity and file size (1 MiB maximum), then copies the patch into the user library with a new ID. Duplicate display names gain a numerical suffix, leaving existing files intact. The imported patch becomes the current sound. Your active browser filters can hide it; select User / All categories and clear search to find it.

EXPORT SOUND saves the currently loaded sound, including unsaved edits and the name/tags fields, to another folder. It does not change the loaded patch or library. Load a highlighted browser row before exporting it. Share the exported `.gba2preset` file; no external audio bank is required. Presets store synthesis parameters only; skins are kept in project state.

Maize Sampler `.mse` instruments bundle samples and need their compatible sample player. They are not playable or converted in this synthesis-only plugin. `.chl` is not an identified synthesis format here and is unsupported. Changing an extension does not convert an instrument.
