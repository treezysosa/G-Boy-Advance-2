param([ValidateSet('Debug','Release')][string]$Configuration='Release', [switch]$SkipTests, [ValidateSet('x64','x86')][string]$Architecture='x64')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
Set-Location $root
function Invoke-Checked([string]$Program,[string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
$directory=if($Architecture -eq 'x86'){'build-x86'}else{'build'}
$platform=if($Architecture -eq 'x86'){'Win32'}else{'x64'}
Invoke-Checked 'cmake' @('-S','.','-B',$directory,'-G','Visual Studio 17 2022','-A',$platform,'-DBUILD_TESTING=ON')
Invoke-Checked 'cmake' @('--build',$directory,'--config',$Configuration,'--parallel','2')
if (-not $SkipTests) { Invoke-Checked 'ctest' @('--test-dir',$directory,'-C',$Configuration,'--output-on-failure') }
Write-Host "VST3: $root\$directory\GBoyAdvance2_artefacts\$Configuration\VST3\G Boy Advance 2.vst3"
Write-Host "Standalone: $root\$directory\GBoyAdvance2_artefacts\$Configuration\Standalone\G Boy Advance 2.exe"
