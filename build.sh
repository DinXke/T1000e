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
# Ship without debug logging; MeshCore's build.sh reads this.
export DISABLE_DEBUG="${DISABLE_DEBUG:-1}"

"$HERE/apply.sh"

cd "$MESHCORE_DIR"
# Note the subcommand: MeshCore's build.sh silently does nothing if the first
# argument is not one of its verbs, so the env name alone is not enough.
./build.sh build-firmware "$ENV_NAME"

echo
echo "==> artifacts:"
ls -la "$MESHCORE_DIR/out/"

# build.sh copies artifacts with '|| true', so a failed copy would leave an
# empty out/ behind an exit code of 0. Refuse to call that a build.
shopt -s nullglob
artifacts=("$MESHCORE_DIR"/out/*.uf2 "$MESHCORE_DIR"/out/*.zip)
if [ ${#artifacts[@]} -eq 0 ]; then
  echo "ERROR: build produced no .uf2 or .zip in $MESHCORE_DIR/out/" >&2
  exit 1
fi
