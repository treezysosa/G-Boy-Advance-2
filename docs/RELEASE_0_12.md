# 0.12.0 sample playback candidate

The owner explicitly authorized adding sample playback to the existing synthesizer, superseding the original synthesis-only restriction. Eight synthesis engines, five skins, the 410 factory identities, all 64 automation IDs, schema-1 synthesis presets and plugin identity are retained.

## Implemented

- Background WAV/AIFF import (mono/stereo), keyboard-wide transposition with a MIDI root note, whole-file crossfade looping, sample-only and equal-blend hybrid modes.
- Existing joint 32-voice ceiling, mono/legato/stealing, ADSR/filter/LFO/velocity/bend/sustain/glide, stereo filters, shared effects and master gain.
- Immutable sample data with hazard-protected publication and off-audio reclamation. No files, blocking locks, allocation or sample destruction in the callback. Cancelled/stale loader jobs cannot replace a newer selection.
- Cubic interpolation with prepared low-pass mip levels; 128-frame bank/config transition tail.
- Embedded source audio in project state and schema-2 user presets/import/export. Pure synthesis presets remain schema 1; no original audio path/network dependency after save. Factory selection clears sample state.
- SELECT → SAMPLES native file chooser, mode, root note, loop, clear and status/error display.

## Limits

One sample zone across the keyboard, up to 8 MiB source bytes / 4M frames, 1–2 channels, 8–192 kHz. Pitched playback changes duration. No streaming, slicing, time stretch, custom loop points or multi-zone/velocity maps. Audio samples are user supplied; none are added to the factory library or installer. WAV/AIFF is supported; Maize `.mse` decoding is not implemented (a public compatible SDK/specification was not found). `.chl` is still unidentified. See the Maize instrument description: https://www.maizesoft.com/download/msp/document.pdf.

Sample presets/projects can reach 16 MiB. Browser library scans/metadata persistence still use synchronous non-audio I/O; many embedded sample presets consume more memory and scan time. Older plugin versions cannot play schema-2 sample instruments. Restoring embedded samples is asynchronous; wait for ready before auditioning. Audio-file failures keep the last working instrument; corrupt embedded audio reports an error and clears that unavailable sample.

## Validation

Local: 307,220 native sample DSP checks at 44.1/48/96 kHz, including sample/combined modes, sustain/release, finite output and zero allocation in render/MIDI/switch. Application tests cover WAV/AIFF, stereo independence, root pitch, mono rate conversion, loops, layers, polyphony/all-sound-off, preset roundtrip, deleting the original source then restoring embedded state, immediate save during restore, rejected files and concurrent load/cancel/publication. The 530-scene / 8,970,240-float synthesis-only render is byte-identical to final 0.10.3 local output (SHA256 dda45b59d786cb135e314ec14dae80db13e240ca87c0a5f0ba4cbfdbe5db361e). Factory calibration volumes and arrays are unchanged; only the guarded DSP source fingerprint was refreshed after that comparison.

Clean Windows build/CTest, pluginval, Steinberg, profile and installer acceptance are pending. No actual new-build FL Studio/audio-device/physical DPI or listening certification is claimed. Licensing remains explicitly deferred by the owner and signing has not been performed. The candidate must complete its automated checks before delivery; final public release requires the decisions/manual entries in RELEASE_CHECKLIST.md.
