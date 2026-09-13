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

echo "==> Building ATOMS3 Lite firmware with PlatformIO..."
"${PIO_CMD}" run -d "${PROJECT_DIR}"
echo "==> ATOMS3 Lite build completed successfully."
