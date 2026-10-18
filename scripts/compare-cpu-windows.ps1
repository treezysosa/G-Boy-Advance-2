param([string]$Configuration='Release')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$logs=Join-Path $root 'build/validation-logs'
New-Item -ItemType Directory -Force $logs | Out-Null
$baseline=Join-Path $root 'build/cpu-baseline-source'
$baselineBuild=Join-Path $root 'build/cpu-baseline-build'
$archive=Join-Path $root 'build/cpu-baseline.tar'
$baselineCommit='901538f3158d7925da8d5bc5dc6983c11e1cd2f5'
git fetch --no-tags --depth=1 origin $baselineCommit
if ($LASTEXITCODE -ne 0) {throw 'Could not fetch the immutable 0.10.2 comparison baseline.'}
git archive --format=tar --output=$archive $baselineCommit
if ($LASTEXITCODE -ne 0) {throw 'Could not export the comparison baseline.'}
New-Item -ItemType Directory -Force $baseline | Out-Null
tar -xf $archive -C $baseline
if ($LASTEXITCODE -ne 0) {throw 'Could not extract the comparison baseline.'}
# Use identical drivers with both DSP versions: only the engine under test differs.
Copy-Item (Join-Path $root 'Tests/PerformanceProbe.cpp'),(Join-Path $root 'Tests/Benchmark.cpp') (Join-Path $baseline 'Tests')
@'
add_executable(gba2-performance-probe Tests/PerformanceProbe.cpp)
target_link_libraries(gba2-performance-probe PRIVATE gba_dsp)
'@ | Add-Content -LiteralPath (Join-Path $baseline 'CMakeLists.txt')
cmake -S $baseline -B $baselineBuild -G 'Visual Studio 17 2022' -A x64 -DGBA_BUILD_PLUGIN=OFF -DBUILD_TESTING=OFF -DGBA_BUILD_BENCHMARKS=ON
if ($LASTEXITCODE -ne 0) {throw 'Baseline configure failed.'}
cmake --build $baselineBuild --config $Configuration --parallel 2 --target gba2-benchmark gba2-performance-probe
if ($LASTEXITCODE -ne 0) {throw 'Baseline build failed.'}
$before=Join-Path $baselineBuild $Configuration
$after=Join-Path $root "build/$Configuration"
& (Join-Path $before 'gba2-performance-probe.exe') (Join-Path $logs 'cpu-before.f32')
if ($LASTEXITCODE -ne 0) {throw 'Baseline comparison render failed.'}
& (Join-Path $after 'gba2-performance-probe.exe') (Join-Path $logs 'cpu-after.f32')
if ($LASTEXITCODE -ne 0) {throw 'New comparison render failed.'}
foreach($voices in @(8,32)) {
    & (Join-Path $before 'gba2-benchmark.exe') (Join-Path $logs "cpu-before-$voices.csv") $voices
    if ($LASTEXITCODE -ne 0) {throw 'Baseline profile produced invalid audio.'}
    $name=if ($voices -eq 32) {'callback-profile.csv'} else {'cpu-after-8.csv'}
    & (Join-Path $after 'gba2-benchmark.exe') (Join-Path $logs $name) $voices
    if ($LASTEXITCODE -ne 0) {throw 'New profile produced invalid audio.'}
}
try {
    python (Join-Path $root 'scripts/compare-cpu.py') $logs
    if ($LASTEXITCODE -ne 0) {throw 'CPU/audio comparison failed.'}
} finally {
    # Large raw test renders are temporary; retain hashes/metrics and CSV evidence.
    Remove-Item -LiteralPath (Join-Path $logs 'cpu-before.f32'),(Join-Path $logs 'cpu-after.f32') -Force
}
