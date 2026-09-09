#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TOOLING_ROOT="$PROJECT_ROOT/.tooling"
LOCAL_PACK_ROOT="$TOOLING_ROOT/rohm-pack"

build_only=0
rohm_pack_root=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-only)
            build_only=1
            shift
            ;;
        --rohm-pack-root)
            [[ $# -ge 2 ]] || { echo "Missing value for --rohm-pack-root" >&2; exit 2; }
            rohm_pack_root="$2"
            shift 2
            ;;
        -h|--help)
            cat <<EOF
Usage:
  $0 --build-only
  $0 --rohm-pack-root /path/to/ML63Q25x7_DFP/<version>

The full setup verifies PlatformIO build tools, installs/verifies pyOCD, and
copies the ROHM DFP into firmware/ml63q2557/.tooling/rohm-pack.
EOF
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

PIO_HOME="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}"
CMAKE="$PIO_HOME/packages/tool-cmake/bin/cmake"
NINJA="$PIO_HOME/packages/tool-ninja/ninja"
ARM_GCC="$PIO_HOME/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-gcc"
ARM_GDB="$PIO_HOME/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-gdb"

missing=0
for tool in "$CMAKE" "$NINJA" "$ARM_GCC" "$ARM_GDB"; do
    if [[ ! -x "$tool" ]]; then
        echo "Missing build/debug tool: $tool" >&2
        missing=1
    fi
done
if [[ $missing -ne 0 ]]; then
    echo >&2
    echo "Install the PlatformIO ARM GCC, CMake, and Ninja packages first." >&2
    echo "The VS Code build task expects them below ~/.platformio/packages/." >&2
    exit 1
fi

echo "Build toolchain:"
"$ARM_GCC" --version | head -n 1

if [[ $build_only -eq 1 ]]; then
    echo "Build-only toolchain setup is complete."
    exit 0
fi

find_pyocd() {
    if command -v pyocd >/dev/null 2>&1; then
        command -v pyocd
        return 0
    fi
    if [[ -x "$HOME/.local/bin/pyocd" ]]; then
        printf '%s\n' "$HOME/.local/bin/pyocd"
        return 0
    fi
    return 1
}

if ! PYOCD="$(find_pyocd)"; then
    if ! command -v pipx >/dev/null 2>&1; then
        if command -v brew >/dev/null 2>&1; then
            echo "Installing pipx with Homebrew..."
            brew install pipx
            pipx ensurepath || true
        else
            echo "pyOCD and pipx are not installed." >&2
            echo "Install pipx, then run: pipx install pyocd" >&2
            exit 1
        fi
    fi

    echo "Installing pyOCD with pipx..."
    pipx install pyocd
    hash -r
    PYOCD="$(find_pyocd)" || {
        echo "pyOCD was installed but is not yet on PATH." >&2
        echo "Open a new terminal and rerun this setup script." >&2
        exit 1
    }
fi

echo "pyOCD: $($PYOCD --version)"

validate_source_pack() {
    local root="$1"
    local failed=0
    for rel in \
        "ROHM.ML63Q25x7_DFP.pdsc" \
        "Flash/ML63Q25x7.FLM" \
        "SVD/ML63Q25x7.svd"; do
        if [[ ! -f "$root/$rel" ]]; then
            echo "Expected: $root/$rel" >&2
            failed=1
        fi
    done
    [[ $failed -eq 0 ]]
}

if [[ -n "$rohm_pack_root" ]]; then
    rohm_pack_root="$(cd "$rohm_pack_root" 2>/dev/null && pwd)" || {
        echo "Invalid ROHM DFP root: $rohm_pack_root" >&2
        exit 1
    }
    if ! validate_source_pack "$rohm_pack_root"; then
        echo "Invalid ROHM DFP root: $rohm_pack_root" >&2
        exit 1
    fi

    mkdir -p "$TOOLING_ROOT"
    rm -rf "$LOCAL_PACK_ROOT"
    mkdir -p "$LOCAL_PACK_ROOT"

    # Keep a complete local copy because the PDSC can reference SVD, FLM, and
    # other pack-relative assets. .tooling/ is intentionally not committed.
    if command -v ditto >/dev/null 2>&1; then
        ditto "$rohm_pack_root" "$LOCAL_PACK_ROOT"
    else
        cp -R "$rohm_pack_root"/. "$LOCAL_PACK_ROOT"/
    fi
elif ! validate_source_pack "$LOCAL_PACK_ROOT" >/dev/null 2>&1; then
    echo "ROHM DFP has not been prepared locally." >&2
    echo "Run:" >&2
    echo "  $0 --rohm-pack-root /path/to/ML63Q25x7_DFP/<version>" >&2
    exit 1
fi

if ! validate_source_pack "$LOCAL_PACK_ROOT"; then
    echo "Local ROHM DFP preparation failed: $LOCAL_PACK_ROOT" >&2
    exit 1
fi

echo "ROHM DFP prepared: $LOCAL_PACK_ROOT"

if ! "$PYOCD" list --targets --pack "$LOCAL_PACK_ROOT" | grep -qi 'ml63q25x7'; then
    echo "pyOCD could not discover target ml63q25x7 from: $LOCAL_PACK_ROOT" >&2
    exit 1
fi

echo "pyOCD target ml63q25x7: OK"
echo "Toolchain setup is complete."
