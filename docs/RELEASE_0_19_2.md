# 0.19.2 — Ten original themed skins

This build adds ten skins, bringing the total to 50. The previous 40 persistent IDs and palette entries remain in their original order.

| Skin | Direction | Original decoration |
|---|---|---|
| Meadow Quest | Nature/creature adventure | Leaf sprigs |
| Ember Trail | Nature/creature adventure | Winding contour trails and diamond markers |
| Neon Grid | Digital fantasy | Right-angle traces and square nodes |
| Prism Circuit | Digital fantasy | Faceted triangles |
| Solar Surge | Anime energy | Diagonal streaks and four-point glints |
| Ink & Ivory | Classic cartoon | Halftone dots and ink ribbons |
| Evergreen Glow | Christmas | Evergreen sprigs and twinkles |
| Frosted Berry | Winter/Christmas | Six-arm geometric snowflakes |
| Tidal Breeze | Beach | Wave contours |
| Sunset Coast | Beach | Sun/horizon and waves |

The themes use original names, palettes and hand-coded vector geometry. They include no franchise characters, logos, slogans, captured game artwork or distinctive franchise symbols. The directions describe broad visual styles; no affiliation is represented.

## Implementation and compatibility

Each new skin has a two-tone housing, coordinated highlights and original static decorations. The artwork is clipped to the housing and drawn behind controls and labels. It is generated only when the existing housing cache is rebuilt for size, display density or skin changes; no animation timer is added. The ten new themes use a darker preset-row selection fill with opaque metadata text so pale accents keep white labels legible; all prior skin selection colors are retained. Extreme display sizes keep the existing vector fallback.

The selector, random/nonrepeating launch choices, fixed project colors and per-instance state use all 50 IDs. Existing skins and project IDs are preserved. Synthesis/effects source and factory data are byte-identical to 0.19.1 commit `60239ebfff5357228bc1c5747ade3bdaa944283d`; the pipeline checks this directly. The release introduces no new audio-performance claim. The previous measured improvements and limitations are documented in RELEASE_0_19_1.md.

654 factory presets /20 categories, 754 host slots /100 reserved slots, 64 stable audio parameter IDs and 16-frame latency are preserved. User presets, SoundFonts, ROM Hacks and the existing UI layout remain available.

## Validation

The Windows x64 VST3 and standalone compiled with Visual Studio 2022. All 9 independent core suites and 12 Windows suites passed, including skin selection, saved-state roundtrips, per-instance isolation and nonrepeating launch colors. Native captures cover all 50 main and advanced appearances, plus the ten new preset browsers and 150% density renders. The 50 existing/new skin IDs are distinct; all prior 40 palette initializers are unchanged. A pixel comparison of the prior 40 native main views also matched the previous build outside the state-dependent preset-name display.

The actual VST3 passed pluginval strictness 10, all 47 official Steinberg checks and both bundle-host integrations. Automated installer acceptance passed all six stages: pinned 0.19.1 installation, upgrade to 0.19.2, installed diagnostics, same-version reinstall, standalone launch smoke test and uninstall. User presets, launch history, SF2 cache, default SoundFont folder and library preferences were preserved. Source/portable ZIP CRCs, x64 PE payloads and exact source-package contents were checked.

Runtime source: `7cd2a5193cbb2c4db17e7705bd3564f853a730b4`; [Windows/core validation run](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/43225072376). The release packaging workflow reuses those identical production binaries, checks their source identity and verifies the final installer and complete source archive. Its machine-readable evidence is `build/validation-logs/release-validation-0192.json` in the Windows artifact.

[Native preview of the ten new skins](images/skins-0192.jpg). These are JUCE component captures from the Windows build. Physical display scaling, listening, MIDI devices and actual FL Studio remain owner acceptance; an automated host named `FL64.exe` is not a copy of FL Studio.

## Build and installation

Source: `feature/themed-skins-0192`. Build with Visual Studio 2022 x64 and CMake 3.24+: `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1`. VST3 output: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone is under the adjacent `Standalone` directory.

Close FL Studio and standalone before running the installer. It installs the complete VST3 bundle to `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. User presets stay in `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`; the default SF2 library stays in `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library` or your selected custom folder.

Prepared binaries are unsigned; the owner deferred licensing. Actual FL Studio and physical audio/MIDI/DPI checks remain owner acceptance.
