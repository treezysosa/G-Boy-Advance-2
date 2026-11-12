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

Windows compilation and all 50 skin selections, project roundtrips, instance isolation, nonrepeating launch selection, main/advanced rendering and high-DPI renders for the ten new themes are pending. Existing engine, plugin-state, real bundle-host, pluginval, Steinberg and installer upgrade/reinstall/uninstall checks remain required. Installer testing uses the verified 0.19.1 release as its previous version.

## Build and installation

Source: `feature/themed-skins-0192`. Build with Visual Studio 2022 x64 and CMake 3.24+: `powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1`. VST3 output: `build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3`; standalone is under the adjacent `Standalone` directory.

Close FL Studio and standalone before running the installer. It installs the complete VST3 bundle to `C:\Program Files\Common Files\VST3\G Boy Advance 2.vst3`. User presets stay in `%APPDATA%\Treezy Audio\G Boy Advance 2\Presets`; the default SF2 library stays in `%APPDATA%\Treezy Audio\G Boy Advance 2\SoundFont Library` or your selected custom folder.

Prepared binaries are unsigned; the owner deferred licensing. Actual FL Studio and physical audio/MIDI/DPI checks remain owner acceptance.
