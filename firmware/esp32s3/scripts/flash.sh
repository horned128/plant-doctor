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

UPLOAD_ARGS=("run" "-d" "${PROJECT_DIR}" "--target" "upload")
if [ $# -ge 1 ]; then
    UPLOAD_ARGS+=("--upload-port" "$1")
fi

echo "==> Flashing ATOMS3 Lite firmware via PlatformIO..."
"${PIO_CMD}" "${UPLOAD_ARGS[@]}"
echo "==> ATOMS3 Lite flash completed successfully."
