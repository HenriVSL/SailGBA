#!/bin/bash
# Inject touches on a test device through sfdev (see tools/sfinput.c).
#   tools/touch.sh DEVICE CMD...      e.g. tools/touch.sh emu1 "tap 540 530"
# The helper is built per architecture into build-<arch>/sfinput on first use.
set -euo pipefail
dev=$1; shift
here=$(cd "$(dirname "$0")/.." && pwd)
case $dev in
    phone) arch=aarch64 ;;
    *) arch=i486 ;;
esac
bin=$here/build-$arch/sfinput
if [ ! -x "$bin" ] || [ "$here/tools/sfinput.c" -nt "$bin" ]; then
    mkdir -p "$here/build-$arch"
    (cd "$here/build-$arch" && ~/SailfishOS/bin/sfdk -c target=SailfishOS-5.1.0.11-$arch.SailGBA \
        build-shell gcc -O2 -Wall -o sfinput ../tools/sfinput.c) >&2
fi
sfdev scp "$dev" "$bin" :/tmp/sfinput >/dev/null
args=""
for a in "$@"; do args="$args '$a'"; done
sfdev ssh --root "$dev" -- "/tmp/sfinput $args"
