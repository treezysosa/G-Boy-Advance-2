param([string]$Configuration='Release')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$logs=Join-Path $root 'build/validation-logs'
New-Item -ItemType Directory -Force $logs | Out-Null
$baseline=Join-Path $root 'build/cpu-baseline-source'
$baselineBuild=Join-Path $root 'build/cpu-baseline-build'
$archive=Join-Path $root 'build/cpu-baseline.tar'
$baselineCommit='f079323677e6024be8356a6afdfa333eb9db4558'
git fetch --no-tags --depth=1 origin $baselineCommit
if ($LASTEXITCODE -ne 0) {throw 'Could not fetch the immutable 0.17.1 comparison baseline.'}
git archive --format=tar --output=$archive $baselineCommit
if ($LASTEXITCODE -ne 0) {throw 'Could not export the comparison baseline.'}
New-Item -ItemType Directory -Force $baseline | Out-Null
tar -xf $archive -C $baseline
if ($LASTEXITCODE -ne 0) {throw 'Could not extract the comparison baseline.'}
# Use identical drivers with both DSP versions: only the engine under test differs.
Copy-Item (Join-Path $root 'Tests/PerformanceProbe.cpp'),(Join-Path $root 'Tests/Benchmark.cpp'),(Join-Path $root 'Tests/PlayBenchmark.cpp'),(Join-Path $root 'Tests/PadStringBenchmark.cpp'),(Join-Path $root 'Tests/SustainedProbe.cpp'),(Join-Path $root 'Tests/SoundFontFixture.h') (Join-Path $baseline 'Tests')
Copy-Item (Join-Path $root 'Source/FactoryData.h') (Join-Path $baseline 'Source/FactoryData.h')
@'
cmake_minimum_required(VERSION 3.24)
project(GBA2PreviousDsp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_library(gba_dsp STATIC Source/dsp/SynthEngine.cpp Source/dsp/Effects.cpp)
target_include_directories(gba_dsp PUBLIC Source)
target_compile_options(gba_dsp PRIVATE /W4 /permissive-)
add_library(gba_soundfont STATIC Source/dsp/SoundFont.cpp Source/dsp/Sf2Validation.cpp)
target_link_libraries(gba_soundfont PUBLIC gba_dsp)
add_executable(gba2-pad-string-benchmark Tests/PadStringBenchmark.cpp)
target_link_libraries(gba2-pad-string-benchmark PRIVATE gba_soundfont)
add_executable(gba2-sustained-probe Tests/SustainedProbe.cpp)
target_link_libraries(gba2-sustained-probe PRIVATE gba_soundfont)
add_executable(gba2-benchmark Tests/Benchmark.cpp)
target_link_libraries(gba2-benchmark PRIVATE gba_dsp)
add_executable(gba2-performance-probe Tests/PerformanceProbe.cpp)
target_link_libraries(gba2-performance-probe PRIVATE gba_dsp)
add_executable(gba2-play-benchmark Tests/PlayBenchmark.cpp)
target_link_libraries(gba2-play-benchmark PRIVATE gba_dsp)
'@ | Set-Content -LiteralPath (Join-Path $baseline 'CMakeLists.txt')
# The old full project returns early with plugin builds disabled. A small driver
# project uses the same Release compiler/runtime flags and the untouched old DSP.
cmake -S $baseline -B $baselineBuild -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) {throw 'Baseline configure failed.'}
cmake --build $baselineBuild --config $Configuration --parallel 2 --target gba2-benchmark gba2-performance-probe gba2-play-benchmark gba2-pad-string-benchmark gba2-sustained-probe
if ($LASTEXITCODE -ne 0) {throw 'Baseline build failed.'}
$before=Join-Path $baselineBuild $Configuration
$after=Join-Path $root "build/$Configuration"
try {
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
    & (Join-Path $before 'gba2-play-benchmark.exe') (Join-Path $logs 'play-before.csv')
    if ($LASTEXITCODE -ne 0) {throw 'Baseline notes/chords profile produced invalid audio.'}
    & (Join-Path $after 'gba2-play-benchmark.exe') (Join-Path $logs 'play-after.csv')
    if ($LASTEXITCODE -ne 0) {throw 'New notes/chords profile produced invalid audio.'}
    & (Join-Path $before 'gba2-sustained-probe.exe') (Join-Path $logs 'sustained-before.f32')
    if ($LASTEXITCODE -ne 0) {throw 'Baseline sustained render failed.'}
    & (Join-Path $after 'gba2-sustained-probe.exe') (Join-Path $logs 'sustained-after.f32')
    if ($LASTEXITCODE -ne 0) {throw 'Optimized sustained render failed.'}
    foreach($repeat in @(0,1)) {
        $order=if($repeat -eq 0){@('before','after')}else{@('after','before')}
        foreach($kind in $order) {
            $directory=if($kind -eq 'before'){$before}else{$after}
            & (Join-Path $directory 'gba2-pad-string-benchmark.exe') (Join-Path $logs "pad-string-$kind-$repeat.csv")
            if ($LASTEXITCODE -ne 0) {throw 'Pad/string/SF2 profile produced invalid audio.'}
        }
    }
    python (Join-Path $root 'scripts/compare-cpu.py') $logs
    if ($LASTEXITCODE -ne 0) {throw 'CPU/audio comparison failed.'}
} finally {
    # Keep hashes/metrics, not large temporary numerical render files.
    foreach($name in @('cpu-before.f32','cpu-after.f32','sustained-before.f32','sustained-after.f32')) {
        $file=Join-Path $logs $name
        if(Test-Path -LiteralPath $file){Remove-Item -LiteralPath $file -Force}
    }
}
