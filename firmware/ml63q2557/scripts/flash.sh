#!/usr/bin/env bash
set -euo pipefail

preset="debug"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --preset)
            [[ $# -ge 2 ]] || { echo "Missing value for --preset" >&2; exit 2; }
            preset="$2"
            shift 2
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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ROHM_DFP_ROOT="${ROHM_DFP_ROOT:-$PROJECT_ROOT/.tooling/rohm-pack}"
USER_SCRIPT="$SCRIPT_DIR/pyocd_user_ml63q25x7.py"
ELF="$PROJECT_ROOT/build/$preset/PlantDoctor.elf"
TARGET="ml63q25x7"

if command -v pyocd >/dev/null 2>&1; then
    PYOCD="$(command -v pyocd)"
elif [[ -x "$HOME/.local/bin/pyocd" ]]; then
    PYOCD="$HOME/.local/bin/pyocd"
else
    echo "pyOCD was not found. Install it with: pipx install pyocd" >&2
    exit 1
fi

[[ -d "$ROHM_DFP_ROOT" ]] || {
    echo "ROHM DFP is missing: $ROHM_DFP_ROOT" >&2
    echo "Run setup-toolchain.sh --rohm-pack-root <ROHM DFP root> first." >&2
    exit 1
}

[[ -f "$USER_SCRIPT" ]] || {
    echo "pyOCD user script is missing: $USER_SCRIPT" >&2
    exit 1
}

[[ -f "$ELF" ]] || {
    echo "ELF is missing: $ELF" >&2
    echo "Build the '$preset' preset first." >&2
    exit 1
}

export ROHM_DFP_ROOT

echo "pyOCD target: $TARGET"
echo "Preset: $preset"
echo "ELF: $ELF"

# ML63Q25x7's CMSIS-Pack ResetSystem sequence can drop SWD with WAIT ACK.
# Flashing itself works, so explicitly suppress pyOCD's pre/post load resets.
# The debugger performs an emulated core reset after programming instead.
exec "$PYOCD" load \
    --target "$TARGET" \
    --pack "$ROHM_DFP_ROOT" \
    --script "$USER_SCRIPT" \
    -O connect_mode=halt \
    -O reset_type=emulated \
    -O load.pre_reset=off \
    -O load.post_reset=off \
    "$ELF"
