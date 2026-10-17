$ErrorActionPreference='Stop'
$root=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$output=Join-Path $root 'build-support'
New-Item -ItemType Directory -Force $output | Out-Null
$collector=Join-Path $PSScriptRoot 'Collect-report.ps1'
foreach($file in Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.ps1') {
    $tokens=$null;$errors=$null
    [System.Management.Automation.Language.Parser]::ParseFile($file.FullName,[ref]$tokens,[ref]$errors) | Out-Null
    if ($errors) {throw ($errors | Out-String)}
}
function Fixture([string]$Fields) {
    $entry=[pscustomobject]@{TimeCreated=Get-Date;RawXml=('<Event><EventData>'+$Fields+'</EventData></Event>')}
    $entry | Add-Member -MemberType ScriptMethod -Name ToXml -Value {return $this.RawXml}
    return $entry
}
$script:supportEvents=@(
    (Fixture '<Data Name="AppName">ilbridge.exe</Data><Data Name="ModuleName">G Boy Advance 2.vst3</Data><Data Name="ExceptionCode">c00000fd</Data><Data Name="FaultingOffset">004e8910</Data>'),
    (Fixture '<Data Name="P1">FL64 (scaled).exe</Data><Data Name="P4">KERNELBASE.dll</Data><Data Name="P7">e06d7363</Data><Data Name="P8">00012345</Data>'),
    (Fixture '<Data Name="AppName">unrelated.exe</Data><Data Name="ModuleName">unrelated-module.dll</Data><Data Name="ExceptionCode">SHOULD_NOT_APPEAR</Data>')
)
$supportEvents=$script:supportEvents
$mockEvents={param($FilterHashtable,$MaxEvents,$ErrorAction) return $supportEvents}.GetNewClosure()
Set-Item Function:Get-WinEvent -Value $mockEvents
try {
    $fixturePath=Join-Path $output 'fixture-report.txt'
    & $collector -OutputPath $fixturePath
    $report=Get-Content -LiteralPath $fixturePath -Raw
    foreach($expected in @('AppName: ilbridge.exe','ModuleName: G Boy Advance 2.vst3','ExceptionCode: c00000fd','FaultingOffset: 004e8910','AppName: FL64 (scaled).exe','ModuleName: KERNELBASE.dll','ExceptionCode: e06d7363')) {
        if (-not $report.Contains($expected)) {Write-Host $report;throw ('Report omitted fixture field: '+$expected)}
    }
    if ($report.Contains('SHOULD_NOT_APPEAR')) {throw 'Unrelated application event leaked into the report.'}
} finally {Remove-Item Function:Get-WinEvent}
$smoke=Join-Path $output 'launcher-smoke.txt'
$launcher=Join-Path $PSScriptRoot 'Run-diagnostics.cmd'
& $env:ComSpec /d /c ('""'+$launcher+'" --no-open "'+$smoke+'""')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $smoke)) {throw 'Double-click launcher smoke test failed.'}
$text=Get-Content -LiteralPath $smoke -Raw
if (-not $text.Contains('G Boy Advance 2 - Windows support report') -or -not $text.Contains('64-bit diagnostic process: True')) {throw 'Launcher did not execute the x64 support collector.'}
Write-Host 'PASS named and WER crash fields, scaled FL process, unrelated-event filtering, PowerShell syntax, and actual Windows PowerShell launcher.'
