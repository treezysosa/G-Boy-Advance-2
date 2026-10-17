# Release acceptance record

This is an unsigned development build, not a certified production release. Record machine/Windows version, DAW/version, audio driver, sample rate, block size, date and evidence for each manual result. A blank or pending entry is not a pass.

## Automated gates

- Generated 410-preset library and stable identities / parameter IDs.
- Windows x64 VST3/standalone compile; seven DSP suites and JUCE application tests.
- Actual-bundle host acceptance: prepared latency, MIDI, bypass, state and instances.
- pluginval strictness 10; separate pinned official Steinberg validator.
- 66-workload native DSP timing CSV (including bounded engine switching and two expensive engine pairs), reviewed for over-budget calls.
- Clean silent install/reinstall/uninstall, installed hashes, user-data preservation, standalone liveness.
- Artifact integrity, AMD64 binaries, no runtime audio assets, complete source inventory.

Actual per-build results are recorded in PHASE_10.md and VALIDATION.md. Automation cannot fill the manual entries below.

## Manual gates (all pending)

| Host | Load/scan | MIDI + sustain/pitch/glide | Automation + program/UI | Save/reopen + user bank | Bypass/latency/bounce/tail | Multi-instance stress |
|---|---|---|---|---|---|---|
| FL Studio | Pending | Pending | Pending | Pending | Pending | Pending |
| Ableton Live | Pending | Pending | Pending | Pending | Pending | Pending |
| REAPER | Pending | Pending | Pending | Pending | Pending | Pending |
| Cubase | Pending | Pending | Pending | Pending | Pending | Pending |
| Studio One | Pending | Pending | Pending | Pending | Pending | Pending |

For latency compare a dry note rendered live and offline with host compensation enabled/disabled; confirm the reported 16-frame delay is compensated. Export a high-feedback 1.5-second delay / long-release patch with the declared tail, checking no truncation. Test note-off and pedal changes while bypassed. Exercise rapid program requests while automating, transport stops, editor resize/reopen, concurrent state save and multiple instances. Test low driver buffers under GUI load and record actual underruns/deadlines.

- Windows standalone: choose real audio/MIDI devices; audible stereo, channel routing, shutdown/relaunch.
- Install as admin, use as a separate non-admin account, scan both hosts and standalone; reinstall/old-version upgrade; uninstall preserves that account's presets and unrelated files.
- Interactive installer cancel/error paths and Windows security reputation; no signing claim without verification.
- UI vs approved purple reference at minimum/maximum sizes, 100/125/150/200% and mixed-monitor DPI; pointer/keyboard focus, every control, screen reader and contrast.
- Listen to all 410 factory sounds over low/mid/high registers and velocities; distinctness, levels, clicks, aliasing, vocal vowels, guitar/sax/flute articulation. Synthesized approximations are not acoustic/voice replicas.
- Review performance/aliasing under effects and interrupted engine transitions. Profile fixes must retain calibrated factory levels or trigger recalibration and full validation.
- Review synchronous library I/O on slow storage and cross-process metadata races; explicit Refresh discovers external file changes.

## Owner release decisions (pending)

Choose a source/distribution license and applicable JUCE terms; satisfy obligations and review retained third-party notices. Eighteen exact upstream texts are now included and hash-checked. Audit branding. For signed distribution, supply an owner code-signing certificate and use scripts/sign-release-windows.ps1 to sign/verify executable, VST3 binary and installer in the correct order, then run acceptance on those signed packages. Signing is optional for using the functional Windows build; no signature is claimed for unsigned packages. Only after evidence is complete, create a release tag and publish binaries. Do not infer licensing compliance from source availability.
