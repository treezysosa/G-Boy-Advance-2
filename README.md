# G BOY ADVANCE 2

A C++20 / JUCE 8 hybrid synthesis instrument targeting Windows x64 VST3 and standalone. The purple, wide console interface uses native scalable controls. All sound is generated mathematically: there is no sampler, sample import, commercial game audio, or network dependency at runtime.

**Development build 0.7.0 — Phase 7 synthetic vocals.** A dedicated five-resonance vocal module adds harmonic spectral tilt, voiced/unvoiced blending and three pitched choir layers. All 75 named vocal designs / 150 voicings have explicit profiles, active Afterglow variations and recalibrated levels. The bank retains 382 presets across 17 categories, 64 parameter IDs and project schema 1. Final Phase 7 validation is running. See [Phase 7](docs/PHASE_7.md), [factory audit](docs/FACTORY_LIBRARY.md), [preset authoring](docs/PRESET_FORMAT.md), [validation](docs/VALIDATION.md) and [Phase 2 visual acceptance](docs/PHASE_2.md).

![G Boy Advance 2 Phase 2 native JUCE editor](docs/images/phase2-main.png)

Actual Phase 2 Windows JUCE component render at 1280×646 from [passing CI run 39007537286](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39007537286). All controls are native components with vector painting; no reference bitmap is used. Interactive Windows high-DPI and DAW testing remain pending.

## Build on Windows

Install Visual Studio 2022 with **Desktop development with C++**, a Windows SDK, CMake 3.24+, and Git. In PowerShell:

```powershell
git clone https://github.com/treezysosa/G-Boy-Advance-2.git
cd G-Boy-Advance-2
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1
```

You can also open [Build and validate in GitHub Actions](https://github.com/treezysosa/G-Boy-Advance-2/actions/workflows/build.yml), choose **Run workflow**, and download the Windows artifact after the job succeeds. The workflow and Windows script are included in the corrected source upload. The previous 0.6.0 passed all six CTest suites, pluginval 1.0.4 strictness 10 and portable/installer packaging in [run 39037896323](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39037896323). Phase 7 adds a seventh CTest suite and 12 vocal audition excerpts to the separate `Factory-preset-auditions-Windows` artifact. Final Windows validation is pending. No audition audio is installed or loaded by the instrument. Full results are recorded in VALIDATION.md.

The first local build downloads the pinned JUCE 8.0.6 source. For an offline build, supply an existing checkout:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DGBA_JUCE_PATH=C:/SDKs/JUCE-8.0.6
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
# Optional host validation (downloads the build-time pluginval tool):
powershell -ExecutionPolicy Bypass -File scripts/validate-windows.ps1
```

Output:

- `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3/` (copy the entire bundle)
- `build/GBoyAdvance2_artefacts/Release/Standalone/G Boy Advance 2.exe`

Close your DAW, copy the entire `.vst3` directory into `C:\Program Files\Common Files\VST3\`, reopen the DAW, and rescan plugins. The standalone requires selecting a working audio device and MIDI input in its settings. Manual tests in FL Studio, Ableton Live, REAPER, Cubase, and Studio One are pending.

## Implemented in source

- Eight engines: subtractive, FM, additive, mipmapped original wavetables, Karplus–Strong plucked strings, formant vocals, chiptune, and modal keys.
- Sine, harmonic-limited triangle, PolyBLEP saw / square / pulse, and noise; resonant low-pass, high-pass, band-pass, notch, and formant filters.
- Two ADSRs, two routable LFOs, vibrato, tremolo, velocity, 16-channel MIDI pitch bend / modulation / sustain, 1–32 voices, stealing, mono / legato, glide.
- Reverb, delay, chorus, phaser, flanger, saturation, distortion, bitcrusher, tape wow / flutter, low / high shelves, and stereo width.
- 382 original factory patches across 17 categories, explicit named-instrument and vocal profiles, calibrated levels and stable program identities. Vocal patches are synthesized vowel-like timbres and contain no recorded voices or speech.
- Reference-based vector UI, playable keyboard, spring-return pitch wheel, functional D-pad, preview A / stop B, START browser, SELECT advanced editor, assignable A–D controls.
- Host-visible parameters, project state, user preset Save As / rename / delete, favorites / recent patches, category search, tags, keyboard navigation.
- Windows CI, portable packaging and Inno Setup installer script, DSP and JUCE integration tests.

## Factory and user presets

Canonical library: [`Presets/factory.json`](Presets/factory.json). The compiled plugin embeds the same parameter values in `Source/FactoryData.h`; users do not need to install an external bank. Regenerate both with `python scripts/generate_presets.py`. Recipe changes require the offline calibration workflow in the preset-format document; no authoring files or audio renders are needed at runtime.

User patches: `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets\*.gba2preset`. Favorites and recents: `library.json` in the parent directory. Every patch contains parameters and metadata only. See [preset schema](docs/PRESET_FORMAT.md).

## Tests without JUCE

```sh
cmake -S . -B build-core -DGBA_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core --parallel 2
ctest --test-dir build-core --output-on-failure
```

Or, run all six DSP suites with a C++20 compiler alone:

```sh
bash scripts/test-core.sh
```

To compile the original factory/effects suite separately:

```sh
mkdir -p build-core
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -ISource Source/dsp/SynthEngine.cpp Source/dsp/Effects.cpp Tests/EngineTests.cpp Tests/AllocationTracker.cpp -o build-core/gba2-tests
cd build-core
./gba2-tests --render
```

`--render` optionally creates six synthesized demonstration WAVs as test output. These are never used by the instrument. Every test run writes `preset-validation.csv` with factory patch RMS / peak levels.

See [performance measurements](docs/PERFORMANCE.md) for the additive-engine optimization and the optional benchmark target.

## Packaging and licensing

`python scripts/package-source.py` creates a complete source ZIP and checks that the Windows build script and GitHub workflow are included.

With Inno Setup 6 installed, run `scripts/package-windows.ps1` after a successful release build. This creates an unsigned Windows archive and installer under `dist/`. Release signing, listening review, the separate Steinberg VST3 validator, interactive installer checks, and DAW testing remain release gates. Pluginval strictness 10 has passed on Windows.

Engine 5 has a revised sound in 0.7.0, including existing formant patches. Saved parameters remain compatible; audio renders may differ.

Read [third-party and licensing notes](docs/LICENSING.md) before distribution. JUCE 8 is dual licensed under AGPLv3 and commercial terms; source availability by itself does not fulfill all license obligations. No Nintendo branding or artwork is bundled.

[User manual](docs/USER_MANUAL.md) · [DSP design](docs/ARCHITECTURE.md) · [Validation](docs/VALIDATION.md) · [Remaining work](docs/NEXT_STEPS.md)
