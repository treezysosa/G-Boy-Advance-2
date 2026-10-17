# Licensing and dependencies

- **JUCE 8.0.6**, https://github.com/juce-framework/JUCE — dual licensed under AGPLv3 or JUCE commercial terms. Choose the applicable license and fulfill it before distributing. An unsigned development build or a public repository does not automatically satisfy AGPLv3 or qualify for a commercial license. The splash/reporting compile settings in CMake do not grant rights.
- **Steinberg VST3 SDK**, included by JUCE's VST3 integration — review the exact bundled SDK version and license / trademark notices in the pinned JUCE tree. Pinned license and notice texts are collected under `docs/licenses`, with upstream paths and Git blob identities in its manifest. They are included in both Windows packages. No VST2 SDK is used.
- **CMake**, **Visual Studio 2022 / MSVC / Windows SDK**, **Git**, and optional **Inno Setup 6** are build tools. Inno Setup is needed only to build the installer. Python 3 is needed only to regenerate presets.
- **Official Steinberg validator**, SDK 3.7.12 (`cc2adc90382dded9e347caf74e4532f1458715db`) — separate build-time test dependency, not shipped. Review that pinned SDK license before using/distributing the tool. The plugin uses JUCE's bundled SDK integration separately.
- **pluginval 1.0.4**, https://github.com/Tracktion/pluginval — GPL-licensed validation tool, downloaded only for build-time testing and not bundled with the instrument.
- GitHub Actions checkout/upload actions support CI; they are not runtime dependencies.

The DSP and preset recipes in this project are newly written source. The repository owner must choose a source/distribution license before public release; no proprietary third-party audio is bundled. Factory names describe synthetic sound characteristics. No performer voice, melody, lyric, speech, language performance, game soundtrack, or ROM data is reproduced.

The attached purple reference informed component layout and colors. Its raster artwork is not shipped or used as a UI background. No Nintendo logo, Nintendo artwork, or claim of affiliation is included. Review product naming / branding independently before commercial release.

The plugin runs offline. It has no telemetry, authentication, remote synthesis, license server implementation, browser view, or sample-bank download.

The notices were fetched from the exact JUCE 8.0.6 tree and verified byte-for-byte against its Git blob hashes. They include JUCE's dependency index, bundled VST3 notices, FLAC/Vorbis, JPEG/PNG/zlib, HarfBuzz, SheenBidi, LV2 support libraries and Presonus extensions. Some are compiled only on other configurations; including them does not imply those formats/features are exposed by this instrument. Retaining these notices does not choose or satisfy a source/JUCE distribution license by itself.

`scripts/sign-release-windows.ps1` prepares signing with an existing owner certificate in the Windows CurrentUser store. It requires a valid code-signing certificate with private key access, signs and verifies SHA256/timestamped payloads, repackages them, then signs/verifies the installer. It records public signer metadata and hashes. No certificate/private key is supplied by this repository; the signing path cannot be exercised without one. Signature/trust verification and signed-package acceptance remain required before a signed release.
