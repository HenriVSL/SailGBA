#!/bin/bash
# Build, install and (re)start SailGBA on a device.
#   sfdev run emu -- tools/deploy.sh      (or: sfdev run phone -- tools/deploy.sh)
# Optional: ROM=/path/on/device.gba to start a game directly.
set -euo pipefail
cd "$(dirname "$0")/.."
log=build-$SFDEV_ARCH/build.log
mkdir -p build-$SFDEV_ARCH
if ! rpms=$(sfdev build "$SFDEV_DEVICE" 2>"$log"); then
    grep -E 'error' "$log" | head -20 >&2
    exit 1
fi
sfdev install "$SFDEV_DEVICE" $rpms >/dev/null
sfdev ssh "$SFDEV_DEVICE" -- "pkill -f '^/usr/bin/harbour-sailgba'; sleep 2; \
    (nohup invoker --type=silica-qt5 -s /usr/bin/harbour-sailgba ${ROM:-} >/tmp/sailgba.log 2>&1 </dev/null &)"
echo "started on $SFDEV_DEVICE"
