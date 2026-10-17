param([string]$Vst3Path='', [ValidateRange(1,10)][int]$Strictness=10)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
$root=Split-Path $PSScriptRoot -Parent
if (-not $Vst3Path) {
    $Vst3Path=Join-Path $root 'build/GBoyAdvance2_artefacts/Release/VST3/G Boy Advance 2.vst3'
}
$Vst3Path=(Resolve-Path -LiteralPath $Vst3Path).Path
$tool=Join-Path $root 'build/validation-tools/pluginval-1.0.4'
$logs=Join-Path $root 'build/validation-logs'
New-Item -ItemType Directory -Force $tool,$logs | Out-Null
$archive=Join-Path $tool 'pluginval.zip'
Invoke-WebRequest 'https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Windows.zip' -OutFile $archive
Expand-Archive -Force -LiteralPath $archive -DestinationPath $tool
$validator=Get-ChildItem $tool -Filter pluginval.exe -File -Recurse | Select-Object -First 1
if (-not $validator) { throw 'pluginval executable missing from the official archive.' }
Write-Host "Validating $Vst3Path with pluginval 1.0.4 at strictness $Strictness"
$ErrorActionPreference='Continue'
& $validator.FullName --strictness-level $Strictness $Vst3Path 2>&1 | Tee-Object -FilePath (Join-Path $logs 'pluginval.txt')
$code=$LASTEXITCODE
$ErrorActionPreference='Stop'
if ($code -ne 0) { throw "pluginval failed with exit code $code. See build/validation-logs/pluginval.txt." }

