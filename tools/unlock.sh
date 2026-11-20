#!/bin/bash
# Turn the display on and open the lock screen on a test device (no PIN: emulators only).
#   tools/unlock.sh DEVICE
set -euo pipefail
sfdev ssh --root "$1" -- 'dbus-send --system --type=method_call --dest=com.nokia.mce /com/nokia/mce/request com.nokia.mce.request.req_display_state_on; \
    dbus-send --system --type=method_call --dest=com.nokia.mce /com/nokia/mce/request com.nokia.mce.request.req_tklock_mode_change string:unlocked'
