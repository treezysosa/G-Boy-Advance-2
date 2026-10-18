# 0.11.0 candidate — skins and custom synthesis patches

## Scope

- Purple, Glacier Blue, Black, White and Spice Orange via SELECT → SKIN.
- Stable string skin IDs in non-audio project state; safe Purple default for older/unknown IDs; independent editor palettes and host notification of unsaved skin edits.
- START → IMPORT / EXPORT SOUND for `.gba2preset` synthesis instruments. Validated, size-bounded JSON, independent imported IDs/names, atomic saves; no factory overwrite or new runtime bank dependency.
- Static native vector housing cached at rendering density, with independent live controls and LEDs. Very large images use vector painting to bound retained cache memory.
- Unchanged optimized DSP, 410 factory programs, 64 parameter IDs, schema 1, manufacturer/product identity and 16-sample latency.

## File compatibility

The owner confirmed synthesis only. Maizesoft documents `.mse` instruments as sample-based, including audio samples. These cannot be played with this engine; no MSE converter or sample playback is claimed. Source: https://www.maizesoft.com/download/msp/document.pdf (Introduction and Export/FAQ). `.chl` has not been identified by its source application; that format is unsupported pending a real specification/example. Existing `.gba2preset` patches are supported. Merely renaming a file is not conversion.

## Validation

Local JUCE integration: 378 checks passed (including 101 library checks). Clean Windows validation is pending. Tests cover skin selection, project persistence, unknown/legacy fallback, independent plugin instances, snapshots, unchanged synthesis on skin changes, preset import/export roundtrips, collisions, persistence and rejected malformed/oversized/unsupported input. Existing DSP, actual-bundle host, pluginval, Steinberg, performance and installer acceptance remain required.

## Release status

The owner requested a prepared build with licensing decided later. This is an unsigned release candidate, without a final public release tag or a claim of completed commercial licensing, physical DPI/audio-device acceptance or all-DAW certification. The owner reports the earlier plugin working through FL64 and improved CPU usage with reduced polyphony/release settings. See RELEASE_CHECKLIST.md for manual tasks. Build/installation commands and paths are in README.md and USER_MANUAL.md.
