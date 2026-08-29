#!/usr/bin/env bash
#
# Fetch MeshCore at the pinned commit and apply this repository's changes to it.
# Leaves a ready-to-build tree in ./meshcore (override with $MESHCORE_DIR).
#
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MESHCORE_DIR="${MESHCORE_DIR:-$HERE/meshcore}"
MESHCORE_REPO="${MESHCORE_REPO:-https://github.com/ripplebiz/MeshCore}"
COMMIT="$(tr -d '[:space:]' < "$HERE/meshcore.lock")"

if [ ! -d "$MESHCORE_DIR/.git" ]; then
  echo "==> cloning MeshCore into $MESHCORE_DIR"
  git clone "$MESHCORE_REPO" "$MESHCORE_DIR"
fi

cd "$MESHCORE_DIR"
echo "==> checking out pinned commit $COMMIT"
git fetch --quiet origin "$COMMIT" 2>/dev/null || git fetch --quiet origin
git checkout --quiet --detach "$COMMIT"
git clean -qfd
git checkout --quiet -- .

echo "==> copying new files"
# rsync is not guaranteed to be present; cp -R is enough here
(cd "$HERE/overlay" && find . -type f -print0) | while IFS= read -r -d '' f; do
  mkdir -p "$MESHCORE_DIR/$(dirname "$f")"
  cp "$HERE/overlay/$f" "$MESHCORE_DIR/$f"
  echo "    $f"
done

echo "==> applying patches"
for p in "$HERE"/patches/*.patch; do
  echo "    $(basename "$p")"
  git apply --whitespace=nowarn "$p"
done

echo
echo "Ready. Build with:"
echo "  cd $MESHCORE_DIR && FIRMWARE_VERSION=<version> ./build.sh t1000e_companion_radio_ble_ps"
