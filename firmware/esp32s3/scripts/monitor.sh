#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

if command -v pio >/dev/null 2>&1; then
    PIO_CMD="pio"
elif [ -f "${HOME}/.platformio/penv/bin/pio" ]; then
    PIO_CMD="${HOME}/.platformio/penv/bin/pio"
else
    echo "Error: PlatformIO 'pio' command not found." >&2
    exit 1
fi

MONITOR_ARGS=("device" "monitor" "-d" "${PROJECT_DIR}" "-b" "115200")
if [ $# -ge 1 ]; then
    MONITOR_ARGS+=("-p" "$1")
fi

echo "==> Starting ATOMS3 Lite Serial Monitor (Ctrl+C to exit)..."
"${PIO_CMD}" "${MONITOR_ARGS[@]}"
