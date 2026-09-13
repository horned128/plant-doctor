<#
.SYNOPSIS
    ATOMS3 Lite (ESP32-S3) PlatformIO Serial Monitor Script
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

$MonitorArgs = @("device", "monitor", "-d", $ProjectDir, "-b", "115200")
if ($Port) {
    $MonitorArgs += @("-p", $Port)
}

Write-Host "==> Starting ATOMS3 Lite Serial Monitor (Ctrl+C to exit)..." -ForegroundColor Cyan
& $PioPath @MonitorArgs
