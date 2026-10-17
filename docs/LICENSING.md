# Licensing and dependencies

- **JUCE 8.0.6**, https://github.com/juce-framework/JUCE — dual licensed under AGPLv3 or JUCE commercial terms. Choose the applicable license and fulfill it before distributing. An unsigned development build or a public repository does not automatically satisfy AGPLv3 or qualify for a commercial license. The splash/reporting compile settings in CMake do not grant rights.
- **Steinberg VST3 SDK**, included by JUCE's VST3 integration — review the exact bundled SDK version and license / trademark notices in the pinned JUCE tree. Retain required notices. No VST2 SDK is used.
- **CMake**, **Visual Studio 2022 / MSVC / Windows SDK**, **Git**, and optional **Inno Setup 6** are build tools. Inno Setup is needed only to build the installer. Python 3 is needed only to regenerate presets.
- GitHub Actions checkout/upload actions support CI; they are not runtime dependencies.

The DSP and preset recipes in this project are newly written source. The repository owner must choose a source/distribution license before public release; no proprietary third-party audio is bundled. Factory names describe synthetic sound characteristics. No performer voice, melody, lyric, speech, language performance, game soundtrack, or ROM data is reproduced.

The attached purple reference informed component layout and colors. Its raster artwork is not shipped or used as a UI background. No Nintendo logo, Nintendo artwork, or claim of affiliation is included. Review product naming / branding independently before commercial release.

The plugin runs offline. It has no telemetry, authentication, remote synthesis, license server implementation, browser view, or sample-bank download.
