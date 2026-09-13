<#
.SYNOPSIS
    ATOMS3 Lite (ESP32-S3) PlatformIO Upload (Flash) Script
#>
[CmdletBinding()]
param(
    [string]$Port
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir

# Locate pio executable
$PioCmd = Get-Command "pio" -ErrorAction SilentlyContinue
if (-not $PioCmd) {
    $DefaultPio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
    if (Test-Path $DefaultPio) {
        $PioPath = $DefaultPio
    } else {
        
        Write-Error "PlatformIO 'pio' command not found. Please install PlatformIO."
        exit 1
    }
} else {
    $PioPath = $PioCmd.Source
}

$UploadArgs = @("run", "-d", $ProjectDir, "--target", "upload")
if ($Port) {
    $UploadArgs += @("--upload-port", $Port)
}

Write-Host "==> Flashing ATOMS3 Lite firmware via PlatformIO..." -ForegroundColor Cyan
& $PioPath @UploadArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
Write-Host "==> ATOMS3 Lite flash completed successfully." -ForegroundColor Green
