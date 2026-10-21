# Factory library audit

678 parameter-only presets across 21 categories. The 102 named instrument designs, 37 named vocal designs, 28 named retro designs and 60 regional hip-hop designs each have an active Afterglow voicing. The 120 groove-pack, 24 brass and 80 expansion designs are individually authored complete patches, without automatic Afterglow variants. The expansion contains 24 Percussion, 20 Woodwinds, 24 Milwaukee and 12 Vox Stabs inside Vocals. Surviving IDs and all host program indices are frozen; seventy-six removed-category slots are reserved by `Presets/factory-index.json`.

This report describes native numerical renders at 48 kHz. It does not certify perceptual uniqueness, natural acoustic realism or commercial sound quality. Human listening acceptance remains pending.

Central-note maximum rolling 100 ms stereo RMS: 0.009269–0.036000 (11.78 dB spread). 571/678 patches are within 0.5 dB of their category target. 112 master gains reached 0.95; quieter articulations remain rather than changing the engine gain or adding hidden normalization.

| Category | Presets | Engines | Central RMS min–max |
|---|---:|---|---:|
| Guitars | 28 | physical modeling | 0.01181–0.02600 |
| Pianos and Keys | 28 | additive, fm, modal, wavetable | 0.01453–0.02600 |
| Bells | 20 | fm, modal | 0.02600–0.02600 |
| Pads | 24 | additive, subtractive, wavetable | 0.01479–0.02800 |
| Bass | 28 | chiptune, fm, physical modeling, subtractive, wavetable | 0.02338–0.03600 |
| Flutes | 20 | additive, chiptune, formant, subtractive, wavetable | 0.01533–0.02600 |
| Plucks | 20 | additive, chiptune, fm, physical modeling, subtractive, wavetable | 0.01132–0.02600 |
| Brass | 40 | additive, chiptune, fm, formant, subtractive, wavetable | 0.01206–0.02600 |
| Strings | 20 | additive, physical modeling, subtractive, wavetable | 0.01518–0.02800 |
| Vocals | 86 | formant | 0.01425–0.02000 |
| Retro Games | 56 | additive, chiptune, fm, formant, modal, physical modeling, wavetable | 0.00975–0.02500 |
| Detroit | 60 | additive, fm, formant, modal, physical modeling, subtractive, wavetable | 0.01372–0.03600 |
| West Coast | 40 | additive, fm, formant, physical modeling, subtractive, wavetable | 0.02000–0.03600 |
| Florida | 40 | fm, formant, modal, physical modeling, subtractive, wavetable | 0.01152–0.03600 |
| Chicago | 24 | additive, chiptune, fm, formant, modal, physical modeling, subtractive, wavetable | 0.00941–0.03600 |
| Funk | 24 | additive, fm, modal, physical modeling, subtractive, wavetable | 0.01127–0.03600 |
| Japan Inspired | 24 | additive, chiptune, fm, formant, modal, physical modeling, subtractive, wavetable | 0.01466–0.03600 |
| 80s Pop | 28 | additive, fm, formant, modal, subtractive, wavetable | 0.00927–0.03600 |
| Percussion | 24 | additive, chiptune, fm, modal, physical modeling, subtractive | 0.00968–0.02600 |
| Woodwinds | 20 | additive, chiptune, fm, formant, subtractive, wavetable | 0.01529–0.02600 |
| Milwaukee | 24 | additive, chiptune, fm, formant, modal, subtractive, wavetable | 0.01017–0.03600 |

## Listening renders

`gba2-preset-tests --auditions` creates one six-second stereo PCM excerpt per category plus an index under `preset-auditions/`. Two notes at different velocities and a release section make patches easy to compare. Eight additional contrasting vocal excerpts are written to `vocal-auditions/`, all 28 base retro designs to `retro-auditions/`, and all 24 new brass designs to `brass-auditions/`. These original synthesized WAVs are test output only and are never installed or loaded by the instrument. Six seconds may not contain the entire effects tail.

## Similarity review

The audit compares 32 log-frequency bands from three Hann-windowed 4096-point FFTs plus eight amplitude-envelope measurements, with effects disabled and volume normalized. It uses the same deterministic excitation seed for each patch. A distance below 0.025 flags a listening candidate; this metric can both miss perceptual similarity and flag intentionally related instruments.

No pairs are below the configured dry spectral/envelope threshold on this audit grid. This does not establish that every listener will perceive every patch as distinct.

## Multi-register all-pair audit (0.18.0)

The additional `gba2-preset-diversity-tests` renders all 678 patches with and without effects at the same MIDI notes 48, 60 and 72, velocities 0.4 and 0.9, 48 kHz: 8,136 renders and 229,503 unique pairs. Each signature includes two 64-band spectra, sixteen held-envelope points and four release measurements. Spectra and envelopes are gain-normalized; dry rendering removes effects. Category names do not restrict comparisons. Renamed and gain/effects-only negative controls must be detected; a sine/saw positive control must remain distinct.

Result: 0 duplicate candidates at <= 0.025; 0 close-pair review flags below 0.06. Minimum dry distance: 0.060104. CSVs record the nearest neighbor of every patch and every flagged pair. Numerical thresholds are screening tools, not perceptual guarantees.

During the earlier 0.15.0 release, the initial screen flagged 26 closely related pairs. `Presets/diversity-refinements.json` records eighteen targeted original excitation/envelope/body revisions and their design notes. Factory IDs, names and program order remain stable. Projects and user presets retain their explicit parameters; explicitly loading a revised factory patch selects its new sound.

## Rebuilding the bank

See [preset format](PRESET_FORMAT.md) for the uncalibrated render → calibration → regeneration workflow. Design recipes, level parameters and identity registry are source metadata; the runtime embeds the final parameter arrays. The separate optional ROM Hacks engine reads user-supplied SF2 banks.
