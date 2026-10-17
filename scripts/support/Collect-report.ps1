param([string]$OutputPath='', [switch]$OpenReport)
$ErrorActionPreference='Stop'
$documents=[Environment]::GetFolderPath('MyDocuments')
if (-not $OutputPath) {$OutputPath=Join-Path $documents 'G-Boy-Advance-2-diagnostics.txt'}
$lines=[System.Collections.Generic.List[string]]::new()
$lines.Add('G Boy Advance 2 - Windows support report')
$lines.Add('Created: '+[DateTime]::UtcNow.ToString('u'))
$lines.Add('Windows: '+[Environment]::OSVersion.VersionString)
$lines.Add('64-bit Windows: '+[Environment]::Is64BitOperatingSystem)
$lines.Add('64-bit diagnostic process: '+[Environment]::Is64BitProcess)
$common=${env:CommonProgramW6432}
if (-not $common) {$common=${env:CommonProgramFiles}}
$vst=Join-Path $common 'VST3/G Boy Advance 2.vst3/Contents/x86_64-win/G Boy Advance 2.vst3'
$lines.Add('Default x64 VST3 binary present: '+(Test-Path -LiteralPath $vst))
if (Test-Path -LiteralPath $vst) {
    $item=Get-Item -LiteralPath $vst
    $hash=(Get-FileHash -LiteralPath $vst -Algorithm SHA256).Hash
    $lines.Add('Plugin file: '+$item.FullName)
    $lines.Add('Plugin file version: '+$item.VersionInfo.FileVersion)
    $lines.Add('Plugin bytes: '+$item.Length)
    $lines.Add('Plugin SHA256: '+$hash)
    $lines.Add('Matches tested 0.10.2 VST3: '+($hash -eq 'E19BE5AFEC664DAA7587BEF95B0ACBCC9EFD214660F6C699FD4721A9F3B1E0E8'))
}
$lines.Add('Running FL Studio / bridge processes:')
$processes=@(Get-Process -Name FL,FL64,ilbridge,ilbridge64,'FL (scaled)','FL64 (scaled)' -ErrorAction SilentlyContinue)
if ($processes.Count -eq 0) {$lines.Add('No matching process is running.')}
foreach ($process in $processes) {
    try {$lines.Add($process.ProcessName+'; version '+$process.MainModule.FileVersionInfo.FileVersion+'; '+$process.Path)}
    catch {$lines.Add($process.ProcessName+'; executable details unavailable.')}
}
foreach ($base in (@(${env:ProgramW6432},$env:ProgramFiles,${env:ProgramFiles(x86)}) | Where-Object {$_} | Select-Object -Unique)) {
    $imageLine=Join-Path $base 'Image-Line'
    if (-not (Test-Path -LiteralPath $imageLine)) {continue}
    Get-ChildItem -LiteralPath $imageLine -Directory -Filter 'FL Studio*' -ErrorAction SilentlyContinue | ForEach-Object {
        foreach($name in @('FL.exe','FL64.exe')) {
            $exe=Join-Path $_.FullName $name
            if (Test-Path -LiteralPath $exe) {$lines.Add('Installed FL executable: '+$_.Name+'/'+$name+'; version '+(Get-Item -LiteralPath $exe).VersionInfo.FileVersion)}
        }
    }
}
$lines.Add('Recent matching Windows crash events (last 3 days):')
try {
    $events=Get-WinEvent -FilterHashtable @{LogName='Application';Id=1000,1001;StartTime=(Get-Date).AddDays(-3)} -MaxEvents 500 -ErrorAction Stop
    $count=0
    foreach($event in $events) {
        [xml]$xml=$event.ToXml();$data=@{}
        foreach($entry in $xml.Event.EventData.Data) {$data[[string]$entry.Name]=[string]$entry.'#text'}
        # Event 1000 uses named fault fields; Windows Error Reporting 1001 uses P1..P8.
        $app=$data['AppName'];$module=$data['ModuleName'];$code=$data['ExceptionCode'];$offset=$data['FaultingOffset']
        if (-not $app -and $data['P1']) {$app=$data['P1'];$module=$data['P4'];$code=$data['P7'];$offset=$data['P8']}
        if ($app -notmatch '^(FL(64)?( \(scaled\))?|ilbridge(64)?|G Boy Advance 2)\.exe$' -and $module -notmatch '^G Boy Advance 2\.vst3$') {continue}
        $lines.Add('--- '+$event.TimeCreated.ToString('u'))
        $lines.Add('AppName: '+$app);$lines.Add('ModuleName: '+$module)
        $lines.Add('ExceptionCode: '+$code);$lines.Add('FaultingOffset: '+$offset)
        ++$count;if ($count -ge 10) {break}
    }
    if ($count -eq 0) {$lines.Add('No matching event was recorded. This does not rule out a plugin failure.')}
} catch {$lines.Add('Matching Windows crash events were unavailable or none were recorded.')}
$lines.Add('Recent FL Studio text crash logs from default Documents locations:')
$files=@()
foreach($relative in @('Image-Line/FL Studio/Support/Logs/Crash','Image-Line/Data/FL Studio/Support/Logs/Crash')) {
    $folder=Join-Path $documents $relative
    if (Test-Path -LiteralPath $folder) {
        $files+=@(Get-ChildItem -LiteralPath $folder -File -ErrorAction SilentlyContinue | Where-Object {$_.Extension -in '.txt','.log' -and $_.Length -le 262144 -and $_.LastWriteTime -ge (Get-Date).AddDays(-3)})
    }
}
$files=@($files | Sort-Object LastWriteTime -Descending | Select-Object -First 3)
if ($files.Count -eq 0) {$lines.Add('No recent text crash log was found in the default locations.')}
foreach($file in $files) {
    $lines.Add('--- '+$file.Name+'; '+$file.LastWriteTime.ToString('u'))
    try {Get-Content -LiteralPath $file.FullName -Tail 80 -ErrorAction Stop | ForEach-Object {$lines.Add([string]$_)}}
    catch {$lines.Add('Could not read this log.')}
}
$directory=Split-Path -Parent $OutputPath
if ($directory) {New-Item -ItemType Directory -Force $directory | Out-Null}
$lines | Set-Content -LiteralPath $OutputPath -Encoding utf8
Write-Host "Report saved to $OutputPath"
if ($OpenReport) {
    $notepad=Join-Path $env:SystemRoot 'System32/notepad.exe'
    Start-Process -FilePath $notepad -ArgumentList ('"'+$OutputPath+'"')
}
