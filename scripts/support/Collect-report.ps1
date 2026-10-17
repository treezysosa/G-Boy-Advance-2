param([string]$OutputPath='', [switch]$OpenReport)
$ErrorActionPreference='Stop'
function Get-ExecutableArchitecture([string]$Path) {
    $stream=$null;$reader=$null
    try {
        $stream=[IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
        $reader=[IO.BinaryReader]::new($stream)
        if ($stream.Length -lt 64 -or $reader.ReadUInt16() -ne 0x5a4d) {return 'Unknown (invalid PE)'}
        $stream.Position=0x3c
        $header=$reader.ReadInt32()
        if ($header -lt 64 -or $header -gt ($stream.Length-6)) {return 'Unknown (invalid PE)'}
        $stream.Position=$header
        if ($reader.ReadUInt32() -ne 0x4550) {return 'Unknown (invalid PE)'}
        switch ($reader.ReadUInt16()) {
            0x014c {return 'x86 (32-bit)'}
            0x8664 {return 'x64 (64-bit)'}
            0xaa64 {return 'ARM64 (64-bit)'}
            default {return 'Unknown PE machine'}
        }
    } catch {return 'Unavailable'}
    finally {
        if ($reader) {$reader.Dispose()}
        elseif ($stream) {$stream.Dispose()}
    }
}
$documents=[Environment]::GetFolderPath('MyDocuments')
if (-not $OutputPath) {$OutputPath=Join-Path $documents 'G-Boy-Advance-2-diagnostics.txt'}
$lines=[System.Collections.Generic.List[string]]::new()
$lines.Add('G Boy Advance 2 - Windows support report')
$lines.Add('Collector revision: 2')
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
    $lines.Add('Plugin architecture: '+(Get-ExecutableArchitecture $item.FullName))
    $lines.Add('Plugin bytes: '+$item.Length)
    $lines.Add('Plugin SHA256: '+$hash)
    $lines.Add('Matches tested 0.10.2 VST3: '+($hash -eq 'E19BE5AFEC664DAA7587BEF95B0ACBCC9EFD214660F6C699FD4721A9F3B1E0E8'))
}
$lines.Add('Running FL Studio / bridge processes:')
$processes=@(Get-Process -Name FL,FL64,ilbridge,ilbridge64,'FL (scaled)','FL64 (scaled)' -ErrorAction SilentlyContinue)
if ($processes.Count -eq 0) {$lines.Add('No matching process is running.')}
$hostFolders=[System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($process in $processes) {
    try {
        $path=$process.Path
        if (-not $path) {throw 'Executable path unavailable.'}
        $item=Get-Item -LiteralPath $path
        $lines.Add($process.ProcessName+'; version '+$item.VersionInfo.FileVersion+'; architecture '+(Get-ExecutableArchitecture $path)+'; '+$path)
        if ($process.ProcessName -match '^FL(64)?( \(scaled\))?$') {[void]$hostFolders.Add((Split-Path -Parent $path))}
    }
    catch {$lines.Add($process.ProcessName+'; executable details unavailable.')}
}
$lines.Add('FL executables beside the running host:')
if ($hostFolders.Count -eq 0) {$lines.Add('No running FL executable folder could be read.')}
foreach ($folder in $hostFolders) {
    foreach ($name in @('FL.exe','FL64.exe','FL (scaled).exe','FL64 (scaled).exe')) {
        $exe=Join-Path $folder $name
        if (Test-Path -LiteralPath $exe) {
            $item=Get-Item -LiteralPath $exe
            $lines.Add('Host-folder executable: '+$exe+'; version '+$item.VersionInfo.FileVersion+'; architecture '+(Get-ExecutableArchitecture $exe))
        }
    }
}
foreach ($base in (@(${env:ProgramW6432},$env:ProgramFiles,${env:ProgramFiles(x86)}) | Where-Object {$_} | Select-Object -Unique)) {
    $imageLine=Join-Path $base 'Image-Line'
    if (-not (Test-Path -LiteralPath $imageLine)) {continue}
    Get-ChildItem -LiteralPath $imageLine -Directory -Filter 'FL Studio*' -ErrorAction SilentlyContinue | ForEach-Object {
        foreach($name in @('FL.exe','FL64.exe')) {
            $exe=Join-Path $_.FullName $name
            if (Test-Path -LiteralPath $exe) {$lines.Add('Installed FL executable: '+$exe+'; version '+(Get-Item -LiteralPath $exe).VersionInfo.FileVersion+'; architecture '+(Get-ExecutableArchitecture $exe))}
        }
    }
}
$lines.Add('Recent matching Windows crash events (last 3 days):')
try {
    $events=Get-WinEvent -FilterHashtable @{LogName='Application';Id=1000,1001;StartTime=(Get-Date).AddDays(-3)} -MaxEvents 500 -ErrorAction Stop
    $count=0;$skipped=0
    foreach($event in $events) {
        try {
            [xml]$xml=$event.ToXml();$data=@{}
            # SelectNodes returns XmlElement objects in both Windows PowerShell 5.1
            # and PowerShell 7. The XML property adapter can return a string for
            # an unnamed <Data> element, aborting the old report on unrelated events.
            $entries=$xml.SelectNodes("/*[local-name()='Event']/*[local-name()='EventData']/*[local-name()='Data']")
            foreach($entry in $entries) {
                $name=$entry.GetAttribute('Name')
                if ($name) {$data[$name]=[string]$entry.InnerText}
            }
        } catch {++$skipped;continue}
        # Event 1000 uses named fault fields; Windows Error Reporting 1001 uses P1..P8.
        $app=$data['AppName'];$module=$data['ModuleName'];$code=$data['ExceptionCode'];$offset=$data['FaultingOffset']
        if (-not $app -and $data['P1']) {$app=$data['P1'];$module=$data['P4'];$code=$data['P7'];$offset=$data['P8']}
        if ($app -notmatch '^(FL(64)?( \(scaled\))?|ilbridge(64)?|G Boy Advance 2)\.exe$' -and $module -notmatch '^G Boy Advance 2\.vst3$') {continue}
        $lines.Add('--- '+$event.TimeCreated.ToString('u'))
        $lines.Add('AppName: '+$app);$lines.Add('ModuleName: '+$module)
        $lines.Add('ExceptionCode: '+$code);$lines.Add('FaultingOffset: '+$offset)
        ++$count;if ($count -ge 10) {break}
    }
    if ($skipped -gt 0) {$lines.Add('Unreadable event records skipped: '+$skipped)}
    if ($count -eq 0) {$lines.Add('No matching event was recorded. This does not rule out a plugin failure.')}
} catch {$lines.Add('Matching Windows crash events were unavailable or none were recorded.');$lines.Add('Event query status: '+$_.Exception.Message)}
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
