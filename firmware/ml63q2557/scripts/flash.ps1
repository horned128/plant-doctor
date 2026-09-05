param(
    [ValidateSet("debug", "release")]
    [string]$Preset = "debug",

    [ValidateSet("PlantDoctor", "SensorDiagnostic")]
    [string]$Target = "PlantDoctor"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$elfPath = Join-Path $projectRoot "build/$Preset/$Target.elf"

if (-not (Test-Path -LiteralPath $elfPath)) {
    throw "Firmware was not found: $elfPath. Build the $Preset preset first."
}

$openOcd = "C:/LAPIS/LEXIDE/gdb/openocd_arm.exe"
$interfaceConfig = "C:/LAPIS/LEXIDE/Cfg/cmsis-dap.cfg"
$packRoot = Join-Path $env:LOCALAPPDATA "Arm/Packs/ROHM/ML63Q25x7_DFP"
$targetConfig = Get-ChildItem -LiteralPath $packRoot -Directory -ErrorAction Stop |
    Sort-Object { [version]$_.Name } -Descending |
    ForEach-Object { Join-Path $_.FullName "Cfg/ml63q25x7.cfg" } |
    Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1

foreach ($path in @($openOcd, $interfaceConfig, $targetConfig)) {
    if (-not $path -or -not (Test-Path -LiteralPath $path)) {
        throw "OpenOCD prerequisite was not found: $path"
    }
}

$openOcdElfPath = (Resolve-Path -LiteralPath $elfPath).Path.Replace("\", "/")
& $openOcd -f $interfaceConfig -f $targetConfig -c "program {$openOcdElfPath} verify reset exit"
if ($LASTEXITCODE -ne 0) {
    throw "OpenOCD flashing failed with exit code $LASTEXITCODE."
}
