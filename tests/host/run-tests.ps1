param(
    [string]$Config = "Debug",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$scriptDir = $PSScriptRoot
$projectRoot = Split-Path -Parent (Split-Path -Parent $scriptDir)
$buildDir = Join-Path $projectRoot "build/host"

function Find-Tool {
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

    throw "$Name was not found."
}

$cmake = Find-Tool "cmake.exe" @(
    (Join-Path $env:USERPROFILE ".platformio/packages/tool-cmake/bin/cmake.exe")
)
$ctest = Find-Tool "ctest.exe" @(
    (Join-Path $env:USERPROFILE ".platformio/packages/tool-cmake/bin/ctest.exe")
)

if ($Clean -and (Test-Path -LiteralPath $buildDir)) {
    Remove-Item -LiteralPath $buildDir -Recurse -Force
}

& $cmake -S $scriptDir -B $buildDir
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

& $cmake --build $buildDir --config $Config
if ($LASTEXITCODE -ne 0) {
    throw "CMake build failed with exit code $LASTEXITCODE."
}

& $ctest --test-dir $buildDir -C $Config --output-on-failure
if ($LASTEXITCODE -ne 0) {
    throw "CTest failed with exit code $LASTEXITCODE."
}

Write-Host "All host tests passed successfully."
