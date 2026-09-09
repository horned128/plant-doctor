#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"
tooling_root="$project_root/.tooling"
rohm_root="$tooling_root/rohm"

target_cfg="$rohm_root/ml63q25x7.cfg"

if [[ ! -f "$target_cfg" ]]; then
    cat >&2 <<EOF
ROHM OpenOCD target config was not prepared:
  $target_cfg

Run:
  $script_dir/setup-toolchain.sh --rohm-pack-root /path/to/ML63Q25x7_DFP/<version>

The DFP root must contain:
  Cfg/ml63q25x7.cfg
  SVD/ML63Q25x7.svd
EOF
    exit 1
fi

find_openocd() {
    if command -v openocd >/dev/null 2>&1; then
        command -v openocd
        return 0
    fi

    local candidate
    for candidate in \
        "/opt/homebrew/bin/openocd" \
        "/usr/local/bin/openocd"; do
        if [[ -x "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done
    return 1
}

openocd="$(find_openocd)" || {
    echo "OpenOCD was not found. Run setup-toolchain.sh first (Homebrew: brew install openocd)." >&2
    exit 1
}

find_cmsis_dap_cfg() {
    local prefix candidate

    if command -v brew >/dev/null 2>&1; then
        prefix="$(brew --prefix openocd 2>/dev/null || true)"
        if [[ -n "$prefix" ]]; then
            candidate="$prefix/share/openocd/scripts/interface/cmsis-dap.cfg"
            if [[ -f "$candidate" ]]; then
                printf '%s\n' "$candidate"
                return 0
            fi
        fi
    fi

    for candidate in \
        "/opt/homebrew/share/openocd/scripts/interface/cmsis-dap.cfg" \
        "/usr/local/share/openocd/scripts/interface/cmsis-dap.cfg"; do
        if [[ -f "$candidate" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

cmsis_dap_cfg="$(find_cmsis_dap_cfg)" || {
    echo "OpenOCD's interface/cmsis-dap.cfg was not found." >&2
    exit 1
}

exec "$openocd" \
    -f "$cmsis_dap_cfg" \
    -f "$target_cfg" \
    "$@"
