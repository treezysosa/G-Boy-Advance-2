param([string]$OutputPath='')
$ErrorActionPreference='Stop'
if (-not $OutputPath) {
    $OutputPath=Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'G-Boy-Advance-2-diagnostics.txt'
}
$lines=[System.Collections.Generic.List[string]]::new()
$lines.Add('G Boy Advance 2 - local Windows diagnostic report')
$lines.Add('Created: '+[DateTime]::UtcNow.ToString('u'))
$lines.Add('Windows: '+[Environment]::OSVersion.VersionString)
$lines.Add('64-bit Windows: '+[Environment]::Is64BitOperatingSystem)
$vst=Join-Path ${env:CommonProgramFiles} 'VST3/G Boy Advance 2.vst3/Contents/x86_64-win/G Boy Advance 2.vst3'
$lines.Add('Default x64 VST3 binary present: '+(Test-Path -LiteralPath $vst))
if (Test-Path -LiteralPath $vst) {
    $item=Get-Item -LiteralPath $vst
    $lines.Add('Plugin file version: '+$item.VersionInfo.FileVersion)
    $lines.Add('Plugin bytes: '+$item.Length)
    $lines.Add('Plugin SHA256: '+(Get-FileHash -LiteralPath $vst -Algorithm SHA256).Hash)
}
# Enumerate executable metadata without starting or changing FL Studio.
foreach ($base in (@($env:ProgramFiles,${env:ProgramFiles(x86)}) | Select-Object -Unique)) {
    if (-not $base) {continue}
    $imageLine=Join-Path $base 'Image-Line'
    if (-not (Test-Path -LiteralPath $imageLine)) {continue}
    Get-ChildItem -LiteralPath $imageLine -Directory -Filter 'FL Studio*' | ForEach-Object {
        foreach($name in @('FL.exe','FL64.exe')) {
            $exe=Join-Path $_.FullName $name
            if (Test-Path -LiteralPath $exe) {
                $info=(Get-Item -LiteralPath $exe).VersionInfo
                $lines.Add('Installed FL executable: '+$_.Name+'/'+$name+'; version '+$info.FileVersion)
            }
        }
    }
}
$lines.Add('Recent matching Application Error events (last 3 days):')
try {
    $events=Get-WinEvent -FilterHashtable @{LogName='Application';Id=1000;StartTime=(Get-Date).AddDays(-3)} -MaxEvents 500 -ErrorAction Stop
    $count=0
    foreach($event in $events) {
        [xml]$xml=$event.ToXml()
        $data=@{}
        foreach($entry in $xml.Event.EventData.Data) {$data[[string]$entry.Name]=[string]$entry.'#text'}
        $app=$data['AppName'];$module=$data['ModuleName']
        if ($app -notmatch '^(FL(64)?|ilbridge(64)?|G Boy Advance 2)\.exe$' -and $module -notmatch '^G Boy Advance 2\.vst3$') {continue}
        $lines.Add('--- '+$event.TimeCreated.ToString('u'))
        foreach($key in @('AppName','AppVersion','ModuleName','ModuleVersion','ExceptionCode','FaultingOffset')) {
            if ($data.ContainsKey($key)) {$lines.Add($key+': '+$data[$key])}
        }
        ++$count
        if ($count -ge 10) {break}
    }
    if ($count -eq 0) {$lines.Add('No matching crash event was recorded. This does not rule out a plugin failure.')}
} catch {
    $lines.Add('Application Error events were unavailable or none were recorded. Use Reliability Monitor (perfmon /rel) if needed.')
}
$directory=Split-Path -Parent $OutputPath
if ($directory) {New-Item -ItemType Directory -Force $directory | Out-Null}
$lines | Set-Content -LiteralPath $OutputPath -Encoding utf8
Write-Host "Diagnostic report saved to $OutputPath"
