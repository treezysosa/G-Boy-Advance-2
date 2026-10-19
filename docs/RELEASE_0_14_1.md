# 0.14.1 — launch colours and SoundFont drag and drop

Each newly loaded plugin or standalone instance chooses one of the ten skins, excluding the last colour opened. The shared launch history is `appearance.json` beside the user Presets and SoundFonts directories. Editor initialization is the only new file-I/O path: plugin scanning, state serialization/restore and audio processing do not touch it. A process-local guard and a bounded cross-process lock serialize history updates; a broken, unwritable or busy preference never prevents UI creation. Across-process no-repeat history requires writable app data and successful locking.

Close/reopen of the same instance’s editor, presets, window size/visibility and state refresh keep the colour. SELECT → NEW COLOUR ON LOAD is on by default, including older DAW projects. Turn it off, choose a skin and save the project to keep that specific colour. The setting and fixed skin remain per-instance, non-automatable project properties; all 64 audio parameter IDs, program order, 530 presets, SF2 playback and 16-frame latency are retained.

Drop **one .sf2 file anywhere on the plugin window**, including child controls and the ROM Hacks, preset or advanced panels. The hover overlay identifies an accepted drop. Dropping opens ROM Hacks (or keeps the already-open panel), starts the existing bounded asynchronous loader, activates the bank on successful publication and exposes its instruments. LOAD SF2 remains available. Uppercase .SF2 extensions and Unicode/spaces in paths work. Multiple files, relative paths and other extensions are rejected without changing the instrument. Invalid .sf2 contents, missing files and unsupported banks show loader errors and retain the working bank. No new sample decoder, general audio importer or audio-thread file I/O is introduced.

Windows shell drag and drop requires compatible process privilege levels; normally run File Explorer and the DAW without administrator elevation. Host restrictions may prevent external shell drops; LOAD SF2 remains an alternative. Banks retain the existing 128 MiB / PCM-SF2 limitations and project-cache backup requirements in RELEASE_0_13.md. No SoundFont or recorded audio is bundled.

## Validation

Implementation and automated validation are in progress. The prior 0.14.0 binaries/downloads are historical, not this update. Current source: `feature/launch-skins-sf2-drop-0141`.

Tests cover colour mapping without repeats, persistence across three separate OS processes, project mode/fixed skin, same-instance window reopening, invalid/unwritable appearance preferences, rejected drags, valid/uppercase/Unicode drops, audible dropped-bank MIDI, cache management, repeated drops into the open panel, errors retaining the working bank, and drops over the advanced panel. Windows additionally exercises the native JUCE peer target search/drop dispatch over the instrument combo box. OS File Explorer interactions in the owner’s actual FL Studio are a manual acceptance check; synthetic native peer delivery is not an Explorer or FL Studio test.

Build commands and output locations remain those in README.md. Close FL Studio and standalone before running the new installer. Upgrade preservation of user presets, cached SoundFonts and appearance preferences will be checked. Licensing remains deferred by the owner; prepared builds are unsigned.
