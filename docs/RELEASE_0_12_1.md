# 0.12.1 — synthesis-only update

Sample playback was removed at the owner's request. This version restores the tested 0.11.0 synthesis DSP, removes the Samples page, audio-file loading, sample worker, interpolation and playback code, and embedded-audio preset authoring. It retains all five color skins, synthesis preset import/export, 410 factory patches, 64 stable audio parameter IDs, plugin identity and the previous CPU improvements.

Older 0.12.0 DAW states restore their synthesis parameters and skin, discard embedded audio from memory and subsequent saves, and show `SYNTH ONLY:` in the preset display. START shows the removal notice. This cannot reproduce the removed sample sound. Selecting a synthesis preset clears the notice. Keep the original project if its old sample content is needed elsewhere.

Sample-containing `.gba2preset` files are rejected or skipped with a browser error; they are left on disk unchanged. Synthesis-only schema 1 `.gba2preset` import/export remains supported. WAV, AIFF, CHL and MSE instrument import is unavailable. The installer does not delete personal audio, projects, user presets or library metadata.

## Verification

Pending: local regression tests and a fresh Windows x64 build, validators and 0.12.0-to-0.12.1 installer upgrade test. Do not treat earlier-version results as a pass for this build.

Unsigned build preparation only. The owner deferred the licensing decision; signing and hands-on final-release acceptance remain outstanding.
