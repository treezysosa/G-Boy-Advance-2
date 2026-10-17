param([ValidateSet('Debug','Release')][string]$Configuration='Release', [switch]$SkipTests)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Set-Location $root
function Invoke-Checked([string]$Program,[string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
Invoke-Checked 'cmake' @('-S','.','-B','build','-G','Visual Studio 17 2022','-A','x64','-DBUILD_TESTING=ON')
Invoke-Checked 'cmake' @('--build','build','--config',$Configuration,'--parallel','2')
if (-not $SkipTests) { Invoke-Checked 'ctest' @('--test-dir','build','-C',$Configuration,'--output-on-failure') }
Write-Host "VST3: $root\build\GBoyAdvance2_artefacts\$Configuration\VST3\G Boy Advance 2.vst3"
Write-Host "Standalone: $root\build\GBoyAdvance2_artefacts\$Configuration\Standalone\G Boy Advance 2.exe"
