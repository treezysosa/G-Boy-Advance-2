# Run on the owner's Windows signing machine after installing its certificate.
# No private key or password belongs in the source tree or build logs.
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-fA-F]{40}$')][string]$CertificateThumbprint,
    [string]$Iscc='ISCC.exe',
    [string]$TimestampUrl='http://timestamp.digicert.com'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$cert=Get-Item -LiteralPath "Cert:\CurrentUser\My\$CertificateThumbprint"
if (-not $cert.HasPrivateKey -or $cert.NotAfter -le (Get-Date) -or $cert.NotBefore -gt (Get-Date)) { throw 'The certificate requires an accessible private key and current validity.' }
if (-not ($cert.EnhancedKeyUsageList | Where-Object {$_.ObjectId -eq '1.3.6.1.5.5.7.3.3'})) { throw 'A code-signing certificate is required.' }
$tool=Get-Command signtool.exe -ErrorAction SilentlyContinue
if ($tool) { $signer=$tool.Source } else {
    $kits=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin'
    $signer=Get-ChildItem -LiteralPath $kits -Filter signtool.exe -Recurse -File |
        Where-Object {$_.Directory.Name -eq 'x64'} | Sort-Object FullName -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $signer) { throw 'Install the Windows SDK signing tools.' }
$artifacts=Join-Path $root 'build/GBoyAdvance2_artefacts/Release'
$files=@((Join-Path $artifacts 'VST3/G Boy Advance 2.vst3/Contents/x86_64-win/G Boy Advance 2.vst3'),(Join-Path $artifacts 'Standalone/G Boy Advance 2.exe'))
foreach($f in $files) { if (-not (Test-Path -LiteralPath $f)) { throw "Build target missing: $f" } }
function Sign-And-Verify([string]$Path) {
    & $signer sign /sha1 $CertificateThumbprint /fd SHA256 /tr $TimestampUrl /td SHA256 $Path
    if($LASTEXITCODE -ne 0) { throw "Signing failed: $Path" }
    & $signer verify /pa /all $Path
    if($LASTEXITCODE -ne 0) { throw "Signature verification failed: $Path" }
    $signature=Get-AuthenticodeSignature -LiteralPath $Path
    if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Thumbprint -ne $CertificateThumbprint -or -not $signature.TimeStamperCertificate) {
        throw "Trusted, timestamped signature from the selected certificate required: $Path"
    }
}
foreach($f in $files) { Sign-And-Verify $f }
# Repackage the signed binaries before signing the resulting installer.
& (Join-Path $PSScriptRoot 'package-windows.ps1') -Configuration Release -Iscc $Iscc
$installer=Join-Path $root 'dist/G-Boy-Advance-2-Setup-x64.exe'
Sign-And-Verify $installer
$logs=Join-Path $root 'build/validation-logs'
New-Item -ItemType Directory -Force $logs | Out-Null
@($files)+@($installer) | ForEach-Object {
    $signature=Get-AuthenticodeSignature -LiteralPath $_
    [pscustomobject]@{file=$_;status=$signature.Status.ToString();signer=$signature.SignerCertificate.Subject;thumbprint=$signature.SignerCertificate.Thumbprint;sha256=(Get-FileHash -LiteralPath $_).Hash}
} | ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $logs 'signatures.json')
Write-Host 'Signed payloads and installer verified. Run host/installer acceptance again on these signed packages before release.'
