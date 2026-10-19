param([string]$Installer='', [string]$BuildDirectory='', [string]$PreviousInstaller='')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
if (-not $Installer) { $Installer=Join-Path $root 'dist/G-Boy-Advance-2-Setup-x64.exe' }
if (-not $BuildDirectory) { $BuildDirectory=Join-Path $root 'build' }
$Installer=(Resolve-Path -LiteralPath $Installer).Path
$logs=Join-Path $BuildDirectory 'validation-logs'
New-Item -ItemType Directory -Force $logs | Out-Null
$app=Join-Path $BuildDirectory 'installer-acceptance-app'
$vst=Join-Path ${env:CommonProgramFiles} 'VST3/G Boy Advance 2.vst3'
# Refuse to touch an existing installation; run this on a disposable Windows VM.
$key='HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{2C50A781-6FD5-4B9C-BC20-C0CF1C2D8CA2}_is1'
if ((Test-Path -LiteralPath $vst) -or (Test-Path -LiteralPath $app) -or (Test-Path $key)) {
    throw 'Installer acceptance requires a clean VM with no G Boy Advance 2 installation.'
}
$bank=Join-Path $env:APPDATA 'Treezy Audio/G Boy Advance 2/Presets'
New-Item -ItemType Directory -Force $bank | Out-Null
$sentinel=Join-Path $bank ('installer-test-'+[guid]::NewGuid().ToString()+'.gba2preset')
[IO.File]::WriteAllText($sentinel,'{"format":"gba2.preset","schemaVersion":2,"sample":{"source":"preservation-fixture"},"parameters":{"volume":0.371}}')
$sentinelHash=(Get-FileHash -LiteralPath $sentinel -Algorithm SHA256).Hash
$audioSentinel=[IO.Path]::ChangeExtension($sentinel,'.wav')
[IO.File]::WriteAllBytes($audioSentinel,[byte[]](82,73,70,70,0,0,0,0,87,65,86,69))
$audioSentinelHash=(Get-FileHash -LiteralPath $audioSentinel -Algorithm SHA256).Hash
$sfDirectory=Join-Path (Split-Path $bank -Parent) 'SoundFonts'
New-Item -ItemType Directory -Force $sfDirectory | Out-Null
$sfSentinel=Join-Path $sfDirectory ('installer-test-'+[guid]::NewGuid().ToString()+'.sf2')
[IO.File]::WriteAllBytes($sfSentinel,[byte[]](82,73,70,70,4,0,0,0,115,102,98,107))
$sfSentinelHash=(Get-FileHash -LiteralPath $sfSentinel -Algorithm SHA256).Hash
$report=[System.Collections.Generic.List[string]]::new()
function Run-Setup([string]$Exe,[string[]]$SetupArguments) {
    $p=Start-Process -FilePath $Exe -ArgumentList $SetupArguments -Wait -PassThru
    if ($p.ExitCode -ne 0) { throw "Setup process failed: $($p.ExitCode)" }
}
function Check-Installed {
    $built=Join-Path $root 'build/GBoyAdvance2_artefacts/Release'
    $pairs=@(
        @((Join-Path $built 'VST3/G Boy Advance 2.vst3/Contents/x86_64-win/G Boy Advance 2.vst3'),(Join-Path $vst 'Contents/x86_64-win/G Boy Advance 2.vst3')),
        @((Join-Path $built 'Standalone/G Boy Advance 2.exe'),(Join-Path $app 'G Boy Advance 2.exe'))
    )
    foreach($pair in $pairs) {
        if (-not (Test-Path -LiteralPath $pair[1])) { throw "Installed binary missing: $($pair[1])" }
        if ((Get-FileHash -LiteralPath $pair[0]).Hash -ne (Get-FileHash -LiteralPath $pair[1]).Hash) { throw 'Installed binary hash mismatch.' }
    }
    foreach($name in @('USER_MANUAL.md','FL_STUDIO.md','collect-windows-diagnostics.ps1','LICENSING.md','Factory Presets/factory.json','Third-party notices/manifest.json','unins000.exe')) {
        if (-not (Test-Path -LiteralPath (Join-Path $app $name))) { throw "Installed file missing: $name" }
    }
    if ((Get-FileHash -LiteralPath $sentinel).Hash -ne $sentinelHash) { throw 'Installer changed user preset data.' }
    if ((Get-FileHash -LiteralPath $audioSentinel).Hash -ne $audioSentinelHash) { throw 'Installer changed personal audio data.' }
    if ((Get-FileHash -LiteralPath $sfSentinel).Hash -ne $sfSentinelHash) { throw 'Installer changed the SoundFont cache.' }
}
$installed=$false
try {
    if($PreviousInstaller) {
        $previous=(Resolve-Path -LiteralPath $PreviousInstaller).Path
        Run-Setup $previous @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',"/DIR=`"$app`"","/LOG=`"$(Join-Path $logs 'install-previous.txt')`"")
        $installed=$true
        if(-not (Test-Path -LiteralPath (Join-Path $vst 'Contents/x86_64-win/G Boy Advance 2.vst3'))) {throw 'Previous-version VST3 was not installed.'}
        if((Get-ItemProperty $key).DisplayVersion -ne '0.13.1') {throw 'Upgrade test requires the pinned 0.13.1 installer.'}
        if ((Get-FileHash -LiteralPath $sentinel).Hash -ne $sentinelHash) {throw 'Previous installer changed user data.'}
        $report.Add('PASS pinned 0.13.1 installation before upgrade, default VST3 path and user-data preservation')
    }
    Run-Setup $Installer @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',"/DIR=`"$app`"","/LOG=`"$(Join-Path $logs 'install.txt')`"")
    $installed=$true;Check-Installed
    if((Get-ItemProperty $key).DisplayVersion -ne '0.14.0') {throw 'Installed registry version does not match the release candidate.'}
    $kind=if($PreviousInstaller){'0.13.1-to-0.14.0 upgrade'}else{'clean install'}
    $report.Add('PASS silent '+$kind+': default common VST3 path, standalone, manual, factory bank, binary hashes')
    $diagnostics=Join-Path $logs 'diagnostics-smoke.txt'
    & (Join-Path $app 'collect-windows-diagnostics.ps1') -OutputPath $diagnostics
    $diagnosticText=Get-Content -LiteralPath $diagnostics -Raw
    $expectedHash=(Get-FileHash -LiteralPath (Join-Path $vst 'Contents/x86_64-win/G Boy Advance 2.vst3') -Algorithm SHA256).Hash
    if ($diagnosticText -notmatch [regex]::Escape($expectedHash)) {throw 'Installed diagnostic script did not report the installed binary hash.'}
    $report.Add('PASS installed read-only diagnostic script writes the actual VST3 hash to its local report')
    Run-Setup $Installer @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',"/DIR=`"$app`"","/LOG=`"$(Join-Path $logs 'reinstall.txt')`"")
    Check-Installed;$report.Add('PASS same-version reinstall and user-preset preservation')
    # No audio device or human interaction is claimed by this liveness check.
    $standalone=Start-Process -FilePath (Join-Path $app 'G Boy Advance 2.exe') -PassThru
    try {
        Start-Sleep -Seconds 3
        if($standalone.HasExited) { throw 'Installed standalone exited during launch smoke test.' }
        $report.Add('PASS installed standalone process survived 3-second launch smoke test (audio device untested)')
    } finally {
        if(-not $standalone.HasExited) { Stop-Process -Id $standalone.Id -Force }
    }
    Run-Setup (Join-Path $app 'unins000.exe') @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/LOG=`"$(Join-Path $logs 'uninstall.txt')`"")
    $installed=$false
    if ((Test-Path -LiteralPath $vst) -or (Test-Path -LiteralPath (Join-Path $app 'G Boy Advance 2.exe')) -or (Test-Path $key)) { throw 'Uninstall left product binaries or registry entry.' }
    if ((Get-FileHash -LiteralPath $sentinel).Hash -ne $sentinelHash) { throw 'Uninstall changed user preset data.' }
    if ((Get-FileHash -LiteralPath $audioSentinel).Hash -ne $audioSentinelHash) { throw 'Uninstall changed personal audio data.' }
    if ((Get-FileHash -LiteralPath $sfSentinel).Hash -ne $sfSentinelHash) { throw 'Uninstall changed the SoundFont cache.' }
    $report.Add('PASS silent uninstall: product binaries/registry removed, old sample preset, personal audio and SoundFont cache preserved')
} finally {
    if($installed -and (Test-Path -LiteralPath (Join-Path $app 'unins000.exe'))) {
        Run-Setup (Join-Path $app 'unins000.exe') @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART')
    }
    Remove-Item -LiteralPath $sentinel -Force
    Remove-Item -LiteralPath $audioSentinel -Force
    Remove-Item -LiteralPath $sfSentinel -Force
    $report | Set-Content -Encoding utf8 (Join-Path $logs 'installer-acceptance.txt')
}
$report | Write-Host
