# 0.15.0 — skins, preset diversity and CPU work

Source branch: `feature/skins-diversity-cpu-0150`, based on immutable 0.14.1 source `f19b9fafd80cf54aa1a6b264a4ccf2a5af2f3a84`. Windows build and artifact evidence will be recorded here after validation. Owner licensing decision remains deferred; unsigned preparation only.

## Changes

Ten appended persistent skin IDs: arctic, atomic, graphite, mint, cobalt, copper, lavender, coral, forest, lemon. Twenty colours are available in SELECT and automatic launch selection. Existing ten IDs retain their order and appearance; fixed project colours and nonrepeating launch history remain compatible. Native main/advanced screenshots, selection/state roundtrips, per-instance isolation and separate-process launches are tested.

All 530 factory sounds receive an additional dry/wet, three-register/two-velocity numerical audit. Every unique pair is compared regardless of category. Gain-normalized spectra and amplitude/release envelopes prevent volume and effects alone from establishing distinct dry sounds. Negative controls cover renamed and gain/effects-only duplicates; a sine/saw control verifies a meaningful excitation difference. Initial screening found no <=0.025 duplicates but 26 <0.06 related pairs. Eighteen targeted factory revisions are documented in `Presets/diversity-refinements.json`; their master levels are recalibrated while retaining all other levels. Final results are recorded after the revised bank completes validation. No new patch IDs or host program positions are introduced. Saved project/user-preset values retain the previous sound independently of the factory recipes.

The 2x nonlinear chain retains both FIR histories, the original coefficients and 16-frame latency. Known-zero odd taps on the even upsampling phase need no multiply/add; the discarded odd decimator result needs only its history write. No filter or shaping stage is bypassed. Exact-input caches avoid repeated wavetable mip logarithms, plucked-string phase-delay trigonometry, shared chord pitch exponentials and per-voice choir normalization square roots. Automation, rate changes, glide and engine transitions invalidate by input comparison; no approximation or lowered voice count is used.

ROM Hacks loading, drag/drop, bank cache, folder preferences, startup recall/project precedence and SF2 playback remain supported. No banks or audio assets are bundled.

## Validation

Pending final Windows/Ubuntu CI and post-download artifact verification. Local implementation work has passed an initial 650-scene / 10,444,800-float byte comparison against 0.14.1 and native twenty-skin/state/SF2 tests; final source must be rerun after the factory revisions. Performance percentages will be reported from the final matched profiles, not inferred from removed operations.

## Reproduce

Build with Visual Studio 2022 x64 and CMake as documented in README. `ctest --test-dir build -C Release --output-on-failure` includes `factory-diversity`. Its `factory-diversity.json`, `factory-diversity.csv` and `factory-diversity-review.csv` record thresholds and nearest-neighbor evidence. `scripts/compare-cpu-windows.ps1` builds immutable 0.14.1 DSP with the current arrays and identical drivers. Windows packaging and the pinned 0.14.1-to-0.15.0 installer lifecycle run in CI.

## Limits

Numerical comparisons do not prove that every listener perceives every related patch as different. Actual owner FL Studio 20.8.3, Explorer drag/drop, physical audio-driver deadlines, high-DPI interaction and human listening remain manual acceptance work. Shared-runner profiles are native DSP observations, not FL Studio CPU percentages or guarantees on the owner's computer.
