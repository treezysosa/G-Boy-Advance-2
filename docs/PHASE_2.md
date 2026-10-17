# Phase 2 — approved interface

Development revision 0.2.0. This phase builds on the working VST3 foundation; it does not replace the synthesizer with a visual prototype.

## Implemented

- Wide 1280×646 reference layout, resizing from approximately 960 to 1920 pixels without moving the selected piano octave.
- Deep indigo main body, layered purple casing bevels, inset panels, moulded-plastic detail and rounded speaker slots. All artwork is generated with JUCE vector drawing.
- Original heavy slanted G BOY lettering, smaller ADVANCE lettering, purple 2, compact VST subtitle and white pixel LCD text sized to the reference.
- Silver volume/macro/glide knobs, full dotted purple master LED ring, spring-return ridged pitch wheel and shaded black/white piano keys.
- Native A/B buttons with circular hit areas, silver START/SELECT caps and visible keyboard-focus feedback.
- Four separate accessible D-pad buttons; blank corners and the neutral centre do not select patches. Arrow keys select patches/categories on the main page.
- Covered main controls cannot receive keyboard interaction while the preset browser or advanced panel is open. F1/F2 toggle their panels; Escape/BACK return immediately. Piano/pitch/glide remain usable.
- Stable macro names and destination descriptions; all four default mappings and reassignment are connected to actual synth parameters.
- Less UI repainting: the timer updates the output LED only when its state changes rather than repainting the whole editor every 40 ms.

## Actual component renders

![Main editor](images/phase2-main.png)

![Preset browser at minimum size](images/phase2-browser.png)

![Advanced editor at minimum size](images/phase2-advanced.png)

These are software renders from the compiled Linux JUCE test executable, not generated mockups. The supplied reference image is not bundled or used as a background.

## Verification

- Strict C++20 compilation against JUCE 8.0.6: PASS on Linux.
- JUCE integration suite: 171 checks PASS on Linux.
- Snapshot coverage: 960/1280/1920 logical widths, main at 125%/150%/200% render scales, browser and advanced editor at minimum size.
- Windows x64 VST3/standalone rebuild, native-window mouse interaction and pluginval strictness 10: pending for this revision.

## Remaining visual acceptance

Inspect the installed standalone and each target DAW at Windows 100%, 125%, 150% and 200% scaling, move between monitors, navigate all controls with Tab/Shift+Tab, test screen-reader names, and compare physical mouse/touch behavior with the reference. Automated snapshots cannot establish those interactive behaviors. The reference's photographic texture is approximated with scalable original vector graphics; exact raster reproduction is not claimed.
