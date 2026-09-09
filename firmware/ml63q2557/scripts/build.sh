#!/usr/bin/env bash
set -euo pipefail

preset="debug"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --preset)
            preset="${2:-}"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--preset debug|release]"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 2
            ;;
    esac
done

case "$preset" in
    debug|release) ;;
    *)
        echo "Preset must be 'debug' or 'release': $preset" >&2
        exit 2
        ;;
esac

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"
pio_home="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}"

find_executable() {
    local name="$1"
    shift

    if command -v "$name" >/dev/null 2>&1; then
        command -v "$name"
        return 0
    fi

    local candidate
    for candidate in "$@"; do
        if [[ -n "$candidate" && -x "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

cmake="$(find_executable cmake \
    "$pio_home/packages/tool-cmake/bin/cmake")" || {
    echo "cmake was not found. Run firmware/ml63q2557/scripts/setup-toolchain.sh first." >&2
    exit 1
}

ninja="$(find_executable ninja \
    "$pio_home/packages/tool-ninja/ninja")" || {
    echo "ninja was not found. Run firmware/ml63q2557/scripts/setup-toolchain.sh first." >&2
    exit 1
}

gcc="$(find_executable arm-none-eabi-gcc \
    "$pio_home/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-gcc")" || {
    echo "arm-none-eabi-gcc was not found. Run firmware/ml63q2557/scripts/setup-toolchain.sh first." >&2
    exit 1
}

export PATH="$(dirname "$gcc"):$(dirname "$ninja"):$(dirname "$cmake"):$PATH"

echo "CMake: $cmake"
echo "Ninja: $ninja"
echo "ARM GCC: $gcc"
echo "Preset: $preset"

cd "$project_root"
"$cmake" --preset "$preset"
"$cmake" --build --preset "$preset"
