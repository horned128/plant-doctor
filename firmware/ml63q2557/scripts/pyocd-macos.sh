#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LOCAL_PACK_ROOT="$PROJECT_ROOT/.tooling/rohm-pack"
PACK_ROOT="${ROHM_DFP_ROOT:-$LOCAL_PACK_ROOT}"
USER_SCRIPT="$SCRIPT_DIR/pyocd_user_ml63q25x7.py"

find_pyocd() {
    if command -v pyocd >/dev/null 2>&1; then
        command -v pyocd
        return 0
    fi

    local candidate="$HOME/.local/bin/pyocd"
    if [[ -x "$candidate" ]]; then
        printf '%s\n' "$candidate"
        return 0
    fi

    echo "pyOCD was not found." >&2
    echo "Install it with: pipx install pyocd" >&2
    return 1
}

validate_pack() {
    local root="$1"
    local failed=0
    for rel in \
        "ROHM.ML63Q25x7_DFP.pdsc" \
        "Flash/ML63Q25x7.FLM" \
        "SVD/ML63Q25x7.svd"; do
        if [[ ! -f "$root/$rel" ]]; then
            echo "Missing ROHM DFP asset: $root/$rel" >&2
            failed=1
        fi
    done
    if [[ $failed -ne 0 ]]; then
        echo >&2
        echo "Prepare the local DFP with:" >&2
        echo "  $SCRIPT_DIR/setup-toolchain.sh --rohm-pack-root /path/to/ML63Q25x7_DFP/<version>" >&2
        return 1
    fi
}

PYOCD="$(find_pyocd)"
validate_pack "$PACK_ROOT"
[[ -f "$USER_SCRIPT" ]] || { echo "Missing pyOCD user script: $USER_SCRIPT" >&2; exit 1; }

export ROHM_DFP_ROOT="$PACK_ROOT"
exec "$PYOCD" "$@" --pack "$PACK_ROOT" --script "$USER_SCRIPT"
