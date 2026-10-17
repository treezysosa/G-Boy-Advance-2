param([string]$SdkPath='', [string]$Vst3Path='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if (-not $SdkPath) { $SdkPath=Join-Path $root 'Dependencies/vst3sdk' }
if (-not $Vst3Path) { $Vst3Path=Join-Path $root 'build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3' }
$SdkPath=(Resolve-Path -LiteralPath $SdkPath).Path
$Vst3Path=(Resolve-Path -LiteralPath $Vst3Path).Path
$build=Join-Path $root 'build/steinberg-validator'
$logs=Join-Path $root 'build/validation-logs'
New-Item -ItemType Directory -Force $logs | Out-Null
# Official SDK v3.7.12_build_20, build-time validation only; never bundled.
$expected='cc2adc90382dded9e347caf74e4532f1458715db'
$actual=& git -C $SdkPath rev-parse HEAD
if($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $expected) { throw "Expected official SDK commit $expected" }
& cmake -S $SdkPath -B $build -G 'Visual Studio 17 2022' -A x64 -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=ON
if($LASTEXITCODE -ne 0) { throw 'Steinberg validator configure failed.' }
& cmake --build $build --config Release --target validator --parallel 2
if($LASTEXITCODE -ne 0) { throw 'Steinberg validator build failed.' }
$validator=Get-ChildItem -LiteralPath $build -Filter validator.exe -File -Recurse | Where-Object { $_.DirectoryName -match 'Release' } | Select-Object -First 1
if(-not $validator) { throw 'Steinberg validator executable missing.' }
$ErrorActionPreference='Continue'
& $validator.FullName $Vst3Path 2>&1 | Tee-Object -FilePath (Join-Path $logs 'steinberg-validator.txt')
$code=$LASTEXITCODE
$ErrorActionPreference='Stop'
if($code -ne 0) { throw "Steinberg validator failed: exit $code" }
"Official SDK $expected; validator exit 0" | Add-Content (Join-Path $logs 'steinberg-validator.txt')
