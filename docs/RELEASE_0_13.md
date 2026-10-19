# 0.13.0 — ROM Hacks SoundFont instruments

The ROM Hacks cartridge on the main console opens a native SoundFont browser. LOAD SF2 imports a user-provided SoundFont 2 bank. Select its bank/program instrument and play MIDI or the piano keyboard; A previews, B releases preview notes. USE SOUNDFONT enables the bank; RETURN TO SYNTH or loading a normal preset returns to synthesis. F3 toggles ROM Hacks. Main arrows/D-pad left/right browse SoundFont instruments when enabled; D-pad up/down browses banks.

Five skins, the 410 factory presets, 64 audio parameter IDs, plugin identity and the optimized synthesis DSP are retained. No general WAV/AIFF import or game-ROM loader is added. No SoundFont bank or third-party audio is bundled. The feature uses the MIT-licensed TinySoundFont decoder, vendored and pinned at `790a219810cb0fca5defa8cdbd88e2487e5efc7a`.

## Playback and limits

SoundFont 2 PCM banks support instrument/key/velocity zones, sample loops, envelopes, panning, bank/program selection and up to 32 sample voices (layered zones consume voices). MIDI channels 1–16, sustain, pitch wheel, volume/expression/pan, bank/program messages, and modulation-wheel vibrato work. A preview uses a separate internal channel so B does not release normal notes.

Shared output volume, filter cutoff/resonance/type, minimum attack, glide, vibrato and effects apply. Synth oscillator/FM/wavetable/formant controls belong to synthesis presets. SoundFont envelopes retain their bank shape; Attack adds a minimum attack. SoundFont mono mode retriggers; the synthesis engine retains its full mono/legato behavior. Arbitrary SF2 modulation tables are not implemented by TinySoundFont, and extreme generator values are bounded. SoundFonts relying on those features can differ from other players. SF3/compressed/ROM-backed samples are unsupported. Linear interpolation does not claim alias-free high transposition.

File size is limited to 128 MiB, 4096 presets, 1024 regions per preset and 65536 expanded regions per bank. Loading, decoding, caching and old-bank destruction run off the audio callback; callback publication is protected by a hazard pointer. Voices/channels and effects memory are prepared before publication. Large banks and multiple instances still require memory/loader CPU; loading is not a real-time operation.

## Project state and personal files

Imported banks are copied to `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFonts\<uuid>.sf2`. The original file is unchanged. DAW state stores the cached path and selected instrument, alongside normal synth parameters and skin; it does not embed SoundFont audio. Preserve/back up the SoundFonts folder with projects and copy/relink banks when moving computers. A missing bank produces silence and a visible ROM Hacks error rather than an unrelated synth patch. LOAD SF2 selects a replacement bank; reselect the instrument if needed.

`.gba2preset` import/export remains for synthesis patches. Save As/Export refuse to mislabel an active SoundFont as a synthesis patch; save SoundFont selections with the DAW project. Returning to synthesis preserves the loaded bank in that running instance; inactive banks are not required by subsequently saved synthesis-only projects. Bank cache files are not deleted automatically because older projects may reference them.

## Validation

Local tests are in progress. Fresh Windows build, plugin validators and 0.12.1-to-0.13.0 installer acceptance are pending. Earlier version results are not evidence for this build. Test banks are generated from original mathematical waves during tests and are not runtime assets.

Licensing decision remains deferred by the owner. This is unsigned build preparation; hands-on DAW/audio-device/DPI/listening and final distribution acceptance remain outstanding.
