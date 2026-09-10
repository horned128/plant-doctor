param(
    [ValidateSet("debug", "release")]
    [string]$Preset = "debug",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot

if ($Clean) {
    $buildDir = Join-Path $projectRoot "build/$Preset"
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}

function Find-Executable {
    param(
        [string]$Name,
        [string[]]$Candidates
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "$Name was not found. Run firmware/ml63q2557/scripts/setup-toolchain.ps1 first."
}

$cmake = Find-Executable "cmake.exe" @(
    (Join-Path $env:USERPROFILE ".platformio/packages/tool-cmake/bin/cmake.exe")
)
$ninja = Find-Executable "ninja.exe" @(
    (Join-Path $env:USERPROFILE ".platformio/packages/tool-ninja/ninja.exe")
)
$gcc = Find-Executable "arm-none-eabi-gcc.exe" @(
    (Join-Path $env:USERPROFILE ".platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-gcc.exe")
)

$env:Path = "$(Split-Path -Parent $gcc);$(Split-Path -Parent $ninja);$env:Path"

Push-Location $projectRoot
try {
    & $cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE."
    }

    & $cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) {
        throw "CMake build failed with exit code $LASTEXITCODE."
    }
}
finally {
    Pop-Location
}
