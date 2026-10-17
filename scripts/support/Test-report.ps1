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
    $entry=[pscustomobject]@{TimeCreated=Get-Date;RawXml=('<Event xmlns="http://schemas.microsoft.com/win/2004/08/events/event"><EventData>'+$Fields+'</EventData></Event>')}
    $entry | Add-Member -MemberType ScriptMethod -Name ToXml -Value {return $this.RawXml}
    return $entry
}
$broken=Fixture ''
$broken.RawXml='<Event><EventData>'
$script:supportEvents=@(
    # Real logs include unnamed Data elements. PowerShell's XML adapter converts
    # these to strings; the old collector failed here before any useful events.
    (Fixture '<Data>UNRELATED_UNNAMED_EVENT</Data>'),
    (Fixture '<Data />'),
    (Fixture '<Data>UNRELATED_MIXED_EVENT</Data><Data Name="AppName">unrelated.exe</Data>'),
    $broken,
    (Fixture '<Data Name="AppName">ilbridge.exe</Data><Data Name="ModuleName">G Boy Advance 2.vst3</Data><Data Name="ExceptionCode">c00000fd</Data><Data Name="FaultingOffset">004e8910</Data>'),
    (Fixture '<Data Name="P1">FL64 (scaled).exe</Data><Data Name="P4">KERNELBASE.dll</Data><Data Name="P7">e06d7363</Data><Data Name="P8">00012345</Data>'),
    (Fixture '<Data Name="AppName">unrelated.exe</Data><Data Name="ModuleName">unrelated-module.dll</Data><Data Name="ExceptionCode">SHOULD_NOT_APPEAR</Data>')
)
function Write-PeFixture([string]$Path,[uint16]$Machine) {
    $bytes=[byte[]]::new(70)
    $bytes[0]=0x4d;$bytes[1]=0x5a;$bytes[0x3c]=64
    $bytes[64]=0x50;$bytes[65]=0x45
    $bytes[68]=[byte]($Machine -band 255);$bytes[69]=[byte]($Machine -shr 8)
    [IO.File]::WriteAllBytes($Path,$bytes)
}
$peFolder=Join-Path $output 'architecture-fixture'
New-Item -ItemType Directory -Force $peFolder | Out-Null
$fl32=Join-Path $peFolder 'FL.exe';$fl64=Join-Path $peFolder 'FL64.exe';$bridge64=Join-Path $peFolder 'ilbridge.exe'
Write-PeFixture $fl32 0x014c
Write-PeFixture $fl64 0x8664
Write-PeFixture $bridge64 0x8664
$mockProcesses=@([pscustomobject]@{ProcessName='FL';Path=$fl32},[pscustomobject]@{ProcessName='ilbridge';Path=$bridge64})
$supportEvents=$script:supportEvents
$mockEvents={param($FilterHashtable,$MaxEvents,$ErrorAction) return $supportEvents}.GetNewClosure()
$getProcesses={param($Name,$ErrorAction) return $mockProcesses}.GetNewClosure()
Set-Item Function:Get-WinEvent -Value $mockEvents
Set-Item Function:Get-Process -Value $getProcesses
try {
    $fixturePath=Join-Path $output 'fixture-report.txt'
    . $collector -OutputPath $fixturePath
    $report=Get-Content -LiteralPath $fixturePath -Raw
    foreach($expected in @('Collector revision: 2','AppName: ilbridge.exe','ModuleName: G Boy Advance 2.vst3','ExceptionCode: c00000fd','FaultingOffset: 004e8910','AppName: FL64 (scaled).exe','ModuleName: KERNELBASE.dll','ExceptionCode: e06d7363','Unreadable event records skipped: 1','architecture x86 (32-bit)','architecture x64 (64-bit)',('Host-folder executable: '+$fl64))) {
        if (-not $report.Contains($expected)) {Write-Host $report;throw ('Report omitted fixture field: '+$expected)}
    }
    foreach ($unexpected in @('SHOULD_NOT_APPEAR','UNRELATED_UNNAMED_EVENT','UNRELATED_MIXED_EVENT','Event query status:')) {
        if ($report.Contains($unexpected)) {throw ('Unexpected fixture output: '+$unexpected)}
    }
    $arm=Join-Path $peFolder 'arm64.exe';Write-PeFixture $arm 0xaa64
    $invalid=Join-Path $peFolder 'invalid.exe';[IO.File]::WriteAllBytes($invalid,[byte[]]::new(10))
    $offset=Join-Path $peFolder 'offset.exe';Write-PeFixture $offset 0x8664
    $bytes=[IO.File]::ReadAllBytes($offset);$bytes[0x3c]=255;[IO.File]::WriteAllBytes($offset,$bytes)
    $signature=Join-Path $peFolder 'signature.exe';Write-PeFixture $signature 0x8664
    $bytes=[IO.File]::ReadAllBytes($signature);$bytes[64]=0;[IO.File]::WriteAllBytes($signature,$bytes)
    $cases=@(@($fl32,'x86 (32-bit)'),@($fl64,'x64 (64-bit)'),@($arm,'ARM64 (64-bit)'),@($invalid,'Unknown (invalid PE)'),@($offset,'Unknown (invalid PE)'),@($signature,'Unknown (invalid PE)'),@((Join-Path $peFolder 'missing.exe'),'Unavailable'))
    foreach ($case in $cases) {
        if ((Get-ExecutableArchitecture $case[0]) -ne $case[1]) {throw ('Wrong architecture for fixture '+$case[0])}
    }
    # A failed or empty event query must still produce the rest of the report.
    Set-Item Function:Get-WinEvent -Value {param($FilterHashtable,$MaxEvents,$ErrorAction) throw 'TEST_EVENT_LOG_UNAVAILABLE'}
    $unavailable=Join-Path $output 'unavailable-events.txt'
    & $collector -OutputPath $unavailable
    $report=Get-Content -LiteralPath $unavailable -Raw
    if (-not $report.Contains('Event query status: TEST_EVENT_LOG_UNAVAILABLE') -or -not $report.Contains('Recent FL Studio text crash logs')) {throw 'Unavailable event query lost the report.'}
    Set-Item Function:Get-WinEvent -Value {param($FilterHashtable,$MaxEvents,$ErrorAction) return @()}
    $empty=Join-Path $output 'empty-events.txt'
    & $collector -OutputPath $empty
    if (-not (Get-Content -LiteralPath $empty -Raw).Contains('No matching event was recorded.')) {throw 'Empty event query was misreported.'}
} finally {Remove-Item Function:Get-WinEvent;Remove-Item Function:Get-Process}
$smoke=Join-Path $output 'launcher-smoke.txt'
$launcher=Join-Path $PSScriptRoot 'Run-diagnostics.cmd'
& $env:ComSpec /d /c ('""'+$launcher+'" --no-open "'+$smoke+'""')
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $smoke)) {throw 'Double-click launcher smoke test failed.'}
$text=Get-Content -LiteralPath $smoke -Raw
if (-not $text.Contains('Collector revision: 2') -or -not $text.Contains('64-bit diagnostic process: True')) {throw 'Launcher did not execute the x64 revision-2 support collector.'}
Write-Host 'PASS named/WER and unnamed/mixed/empty/malformed XML events, namespace handling, query failures, x86/x64/ARM64 PE architecture, invalid PE inputs, host-sibling discovery, syntax, and actual Windows PowerShell launcher.'
