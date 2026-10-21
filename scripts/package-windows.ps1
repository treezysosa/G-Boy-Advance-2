param([string]$Configuration='Release',[string]$Iscc='ISCC.exe')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
# CMake is the single release-version source for plugin and installer.
$project=Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Raw
$version=[regex]::Match($project,'(?m)^project\(GBoyAdvance2\s+VERSION\s+(\d+\.\d+\.\d+)').Groups[1].Value
if (-not $version) { throw 'Cannot read the product version from CMakeLists.txt.' }
$artifacts=Join-Path $root "build\GBoyAdvance2_artefacts\$Configuration"
$vst=Join-Path $artifacts 'VST3\G Boy Advance 2.vst3'
$standalone=Join-Path $artifacts 'Standalone\G Boy Advance 2.exe'
if (-not (Test-Path $vst) -or -not (Test-Path $standalone)) { throw 'Build the Windows targets first.' }
$dist=Join-Path $root 'dist'
New-Item -ItemType Directory -Force $dist | Out-Null
$stage=Join-Path $dist 'G Boy Advance 2'
# A clean stage prevents stale runtime files from leaking into a new archive.
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item -Recurse -Force $vst $stage
Copy-Item -Force $standalone,$(Join-Path $root 'README.md'),$(Join-Path $root 'docs\USER_MANUAL.md') $stage
Copy-Item -Force (Join-Path $root 'docs\FL_STUDIO.md'),(Join-Path $root 'scripts\collect-windows-diagnostics.ps1') $stage
$factory=Join-Path $stage 'Presets'
New-Item -ItemType Directory -Force $factory | Out-Null
Copy-Item -Force (Join-Path $root 'Presets\factory.json') $factory
Copy-Item -Force (Join-Path $root 'docs\LICENSING.md') $stage
Copy-Item -Recurse -Force (Join-Path $root 'docs\licenses') (Join-Path $stage 'Third-party notices')
Compress-Archive -Force -Path "$stage\*" -DestinationPath (Join-Path $dist 'G-Boy-Advance-2-Windows-x64.zip')
& $Iscc "/DAppVersion=$version" "/DArtifacts=$artifacts" "/DOutput=$dist" (Join-Path $root 'packaging\installer.iss')
if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compilation failed.' }
