$ErrorActionPreference = "Stop"

function Find-PlatformIo {
    $command = Get-Command "pio.exe" -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $defaultPath = Join-Path $env:USERPROFILE ".platformio/penv/Scripts/pio.exe"
    if (Test-Path -LiteralPath $defaultPath) {
        return (Resolve-Path -LiteralPath $defaultPath).Path
    }

    throw "PlatformIO Core was not found. Install the PlatformIO IDE extension in VS Code first."
}

$gccPath = Join-Path $env:USERPROFILE ".platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-gcc.exe"
if (-not (Test-Path -LiteralPath $gccPath)) {
    $pio = Find-PlatformIo
    & $pio pkg install --global --tool "platformio/toolchain-gccarmnoneeabi@1.140201.0"
    if ($LASTEXITCODE -ne 0) {
        throw "ARM GNU Toolchain installation failed with exit code $LASTEXITCODE."
    }
}

$requiredPaths = @(
    $gccPath,
    (Join-Path $env:USERPROFILE ".platformio/packages/tool-cmake/bin/cmake.exe"),
    (Join-Path $env:USERPROFILE ".platformio/packages/tool-ninja/ninja.exe")
)

foreach ($path in $requiredPaths) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required tool was not found: $path"
    }
}

& $gccPath --version | Select-Object -First 1
Write-Host "Toolchain setup is complete. Run scripts/build.ps1 next."
