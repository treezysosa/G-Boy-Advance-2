> Current update: **0.18.0**, verified Windows VST3/standalone and 0.17.1 upgrade. 678 presets in 21 categories, 30 skins, zero all-pair duplicate/close-pair flags. The 50% performance target was not reached. [Current evidence](RELEASE_0_18_0.md). Earlier entries below are historical.

## Remaining acceptance and performance target

1. Recheck this build in the owner's FL Studio 20.8.3 x64 with real audio/MIDI devices, low buffers, multi-instance projects, external SF2 banks and project reload. The synthetic FL-named host is not FL Studio.
2. The requested 50% reduction remains outstanding. Profile the slowest rows in `pad-string-*.csv` and `play-*.csv`; investigate further voice/oscillator SIMD and block effects work. Use identical patches/events and `scripts/compare-cpu-windows.ps1`; preserve the existing 1e-6 / 110 dB audio bounds and quantizer boundaries. Do not claim a target by lowering voices, harmonics or quality silently.
3. Listen to all 678 presets, particularly new percussion articulation, synthetic woodwinds and Milwaukee/Vox patches, in real musical phrases and across registers. Automated uniqueness is supporting evidence, not human acceptance.
4. Review all 30 skins on physical high/mixed-DPI monitors and complete the remaining named-DAW/device/manual installer matrix. Signing and distribution licensing remain deferred by the owner.


> Current update: **0.17.1**, folders renamed to Detroit, West Coast, Florida and Chicago. All 598 sounds, DSP and program/parameter identities are unchanged. Windows/Ubuntu suites, both VST3 validators and the 0.17.0 → 0.17.1 installer upgrade passed. [Current evidence](RELEASE_0_17_1.md). Earlier results below retain their historical baselines.

> Current update: **0.17.0**, Brass contains 24 new presets and all 16 unchanged sax leads; 598 factory presets across 18 categories. Windows/Ubuntu suites, both VST3 validators, browser family filters, edited sax-project restoration and the 0.16.3 → 0.17.0 installer upgrade passed. DSP remains unchanged. [Current evidence](RELEASE_0_17_0.md). Earlier results below retain their historical baselines.

> Current update: **0.16.3**, **Vocals** combines all 74 retained vocal-category sounds; 574 factory presets across 18 categories. Windows and Ubuntu suites, both VST3 validators, edited legacy-category restoration and the 0.16.2 → 0.16.3 installer upgrade passed. DSP and SoundFont processing are unchanged from 0.16.2. [Current evidence](RELEASE_0_16_3.md). Earlier results below retain their historical baselines.

# Exact next steps

Version 0.16.2 targets sustained pad/string and SoundFont CPU use without reducing polyphony or tails. [Current evidence](RELEASE_0_16_2.md). [Windows/Ubuntu run 39446025149](https://github.com/treezysosa/G-Boy-Advance-2/actions/runs/39446025149) passed nine Ubuntu suites and twelve Windows CTest suites: 630,175 common DSP/preset checks, 2,884 Windows application checks (332 library / 164 SoundFont), both actual-bundle hosts (647/648), pluginval strictness 10, Steinberg 47 passed / 0 failed and six installer lifecycle checks. The synthetic FL-named host is not FL Studio.; the remaining manual acceptance items are below.

1. Complete the remaining manual matrix in RELEASE_CHECKLIST.md on real Windows devices and the five named DAWs. Record host/Windows versions, interface/driver, sample rate, buffer, date and evidence. Source inspection and a custom host do not establish named-DAW compatibility.
2. Profile low driver buffers with real GUI, multiple instances, note-on bursts and rapid engine/parameter requests. Investigate any actual underruns; the current shared-runner over-budget observations are recorded in RELEASE_0_13.md without assigning a cause. Revalidate/recalibrate audio if further DSP work changes timbre/levels.
3. Check compensated bounce/alignment (16 reported samples after prepare), held-note/pedal changes during bypass, transport/program changes, long-feedback export tails and project reload/automation in each host.
4. Listen to all 574 sounds in low/mid/high registers and velocities. Numerical distinction/levels, over-140 dB triangle reference SNR and near-identical excerpts support verification but do not certify natural articulation or commercial sound quality.
5. Verify standalone real audio/MIDI devices, separate non-admin account use, interactive wizard/cancel paths and upgrades from older versions. Silent automated install tests use a clean admin VM.
6. Accept the approved-reference UI on physical monitors at min/max sizes and 100/125/150/200% / mixed DPI, pointer/keyboard focus and accessibility. Software snapshots are not physical-DPI acceptance.
7. Review slow-storage synchronous library I/O and simultaneous separate-process metadata edits. These are off the audio thread; external file changes require Refresh.
8. Expand two-source LFO routing into a general matrix only if required; add sample-accurate automation where the host/JUCE interface permits. Current block-read parameters smooth per sample; program changes service at 20 Hz.
9. Owner: choose source/JUCE distribution terms and review retained notices/branding. Nineteen pinned upstream texts are packaged. For optional signed distribution, supply an owner certificate and run scripts/sign-release-windows.ps1, then accept signed packages. Unsigned Windows builds can be used without code signing. No production tag, license choice or signature is inferred.

10. Verify the owner's actual SoundFonts in FL64: instruments/banks, keyboard/MIDI/sustain/bend/glide, shared controls, cache backups/relinking and immediate/offline project reload. Profile loaded banks/voice layers and test native dialogs/cancellation, instances and 20 skins/DPI.

11. On the owner’s actual FL64, drop a real SF2 from Explorer over main controls, ROM Hacks and the other panels; verify hover feedback, instrument selection/audio, invalid-file errors, privilege-level behavior, repeated drops and manual LOAD SF2. Reload the plugin and DAW/standalone for nonrepeating launch colours, then disable NEW COLOUR ON LOAD and verify fixed-colour project restore.

12. On the owner’s actual FL64/standalone, populate SoundFont Library, refresh/select banks and instruments, change the folder, restart for last-bank recall, verify opt-out and saved synth/SF2 project precedence, and back up the library/cache/preferences. Test real large/removable/network folders with the documented scan limits.
