#!/usr/bin/env bash
#
# Apply the changes and build the firmware. Output lands in meshcore/out/
# as a .uf2 (drag-and-drop over the T1000-E's bootloader drive) and a .zip
# (nRF DFU package, for OTA or nrfutil).
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MESHCORE_DIR="${MESHCORE_DIR:-$HERE/meshcore}"
ENV_NAME="${ENV_NAME:-t1000e_companion_radio_ble_ps}"
export FIRMWARE_VERSION="${FIRMWARE_VERSION:-t1000e-mt}"

"$HERE/apply.sh"

cd "$MESHCORE_DIR"
./build.sh "$ENV_NAME"

echo
echo "==> artifacts:"
ls -la "$MESHCORE_DIR/out/" || true
