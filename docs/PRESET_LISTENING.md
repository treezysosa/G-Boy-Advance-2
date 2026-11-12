# Listening review of the 654-preset bank

The offline preparation is complete: 654 original stereo synthesis renders, 20 seconds each at 48 kHz, with 24-bit FLAC listening files. The installed plugin, all factory parameter dictionaries and its optimized DSP are unchanged. This work does not claim a completed human listening pass or replace sounds based on a numerical resemblance alone.

Each sound receives identical events: C3 soft, C4 soft, C4 strong, C5, a C3/E3/G3/C4 chord, then 8.4 seconds of release/effects tail. Preset mono/legato/poly behavior remains active. The common pitches make comparisons meaningful across folders. A long pad gets a three-second strong note and a four-second chord; unusually long effects tails may outlast the excerpt. The complete bank is 218 minutes, so review in sessions with breaks.

## Current findings

The 654-preset render completed without non-finite, unbounded or silent files. The highest phrase true peak was -6.5 dBTP. FFmpeg EBU R128 measurements cover each full phrase, its three-second strong middle note, and its four-second chord. Known-amplitude stereo 1 kHz reference signals passed the true-peak and 6.02 LU relative-gain controls. Meter summaries report values at 0.1-unit precision.

The current screen flags 163 presets more than 3 LU from their sound-family strong-note median, and 44 presets whose existing nearest dry-spectrum/envelope distance is below 0.08. These overlap: 203 sounds have at least one flag. The retained native all-pair audit found zero duplicate candidates at 0.025 and zero close-pair flags at 0.06 across 213,531 pairs. The wider 0.08 queue prioritizes listening; it does not reverse that result or establish perceptual uniqueness.

Strong-note integrated loudness spans -51.2 to -27.5 LUFS. This range includes very short staccato sounds versus held textures; it is not a justification to raise every quiet result. LUFS is a useful screening metric rather than a guarantee of perceived equality for pitched instruments. Family groupings preserve bass, keys, bell, pad, pluck, vocal and other roles across mixed regional folders. No limiter, compressor, voice reduction or new audio-thread work was added.

The report contains bounded, **unapplied** gain proposals: at most 3 dB, constrained by the existing 0.95 master-volume ceiling and measured 3 dB phrase peak headroom. Presets already at the gain ceiling can receive a zero boost proposal. The release's calibration and factory files remain untouched.

## Reviewing the downloads

Extract the entire listening ZIP. Open `Listen.html` in Chrome or Edge; retain its `audio` directory. It works offline without installing or changing the plugin. Each category part is independently playable. The full pack also supports comparisons across every folder. Some nearest neighbors are outside a category part; the full pack contains them all.

Choose a sound, then use A/B with the nearest numerical candidate or any other selected preset. Loudness matching attenuates the louder clip; it never boosts either. Rate Keep, Too quiet, Too loud, Weak or Similar; add the reason and desired trim or replacement direction. Export feedback JSON to back up your decisions. Browser storage may differ between browsers or file locations, so use Import to continue from the export. No sound is marked reviewed merely because it was rendered or played.

The feedback contains the exact factory-bank SHA-256. Wrong-bank feedback is rejected, as are unknown IDs, invalid verdicts and out-of-range trim values. The tool does not apply feedback to the synth automatically. A human can judge transient quality, harshness, musical usefulness and whether two sounds earn separate places in the bank; those decisions are still pending.

## Verification

The native renderer compiled locally with GCC C++20 and warnings treated as errors. All 654 FLAC hashes and stream headers were checked: stereo, 24-bit, 48 kHz, exactly 960,000 samples each. The complete ZIP and 33 independently playable folder parts cover every active preset exactly once. All 37 source/recipe files under Source and Presets match the verified 0.19.2 release.

[Listening-tool CI run 43240810853](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43240810853) passed the clean CMake build, known-signal meter controls, three real synthesis fixtures, complete source packaging and actual offline-browser tests. Browser tests covered FLAC A/B playback, attenuation, search/review filters, persistence, feedback export/import and wrong-bank rejection. Test verdicts were explicitly removed. The browser smoke test uses three sounds; the separate complete-bank audit verifies all 654 audio files. No human verdict is implied.

## Rebuilding the pack

Source branch: `tools/preset-listening-0192`. Dependencies: C++20, CMake 3.24+, Python 3.12, NumPy and FFmpeg with the EBU R128 filter. JUCE is not needed. The optional renderer is excluded from normal plugin targets and is never installed.

```sh
cmake -S . -B build-listening -DGBA_BUILD_PLUGIN=OFF -DBUILD_TESTING=OFF -DGBA_BUILD_AUDITION_TOOLS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-listening --target gba2-preset-audition --parallel 2
python scripts/build-listening-pack.py --self-test
python scripts/build-listening-pack.py --renderer build-listening/gba2-preset-audition --output "build-listening/Listening Pack" --similarity path/to/factory-diversity.csv --zip
```

On Windows use `--config Release` and the renderer at `build-listening/Release/gba2-preset-audition.exe`. A direct compiler alternative is:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -ISource Source/dsp/SynthEngine.cpp Source/dsp/Effects.cpp Tools/PresetAudition.cpp -o gba2-preset-audition
```

Use `--resume` to verify cached audio hashes and native-renderer/factory provenance; otherwise a nonempty output is refused. Two jobs are the default; one to four are supported. `--limit 3` produces an explicitly partial smoke pack. `--zip` writes the complete ZIP plus independent category parts capped near 27 MiB of audio. The CSV and manifest record every original preset ID, metrics, priorities and audio SHA-256. Factory and source packages contain no audition audio.

For the browser smoke test, install Playwright and use `python Tools/test-listening-review.py path/to/pack`. It exercises actual local FLAC playback, level matching, review persistence, JSON exchange and wrong-bank rejection. Automated test verdicts are fixtures and are removed; they are not human reviews.

## Completing the sound changes

1. A human reviewer exports verdicts and listening notes from this bank. Start with flagged sounds, then cover every remaining sound. Listen at comfortable levels on headphones and speakers, with musical context for shortlisted changes.
2. Use listening feedback to choose gain corrections and identify weak or redundant timbres. Redesign only the affected recipes; retain preset IDs, names, categories and host program slots.
3. Re-render uncalibrated affected designs, preserve exact gains for untouched sounds, recalibrate, and regenerate JSON/C++ data together using the authoring workflow in PRESET_FORMAT.md.
4. Re-run levels, release/headroom, velocity, complete dry/wet all-pair checks and A/B the revised sounds. Build and validate a new Windows version, including project restoration and installer upgrade, before distributing changed factory audio.

No listening-dependent redesign or replacement is claimed in this preparation.
