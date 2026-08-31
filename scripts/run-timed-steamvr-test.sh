#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

duration_seconds=${1:-60}
if [[ ! $duration_seconds =~ ^[0-9]+$ ]] ||
   ((duration_seconds < 10 || duration_seconds > 600)); then
  echo "Usage: $0 [duration-seconds: 10-600]" >&2
  exit 2
fi

script_directory=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repository_root=$(cd -- "${script_directory}/.." && pwd)
probe="${repository_root}/build-release/xrealone_probe"
settings_path="${HOME}/.local/share/Steam/config/steamvr.vrsettings"
settings_backup=""
cleanup_started=false

cleanup() {
  if [[ $cleanup_started == true ]]; then
    return
  fi
  cleanup_started=true
  "${script_directory}/stop-steamvr-test.sh" || true
  if [[ -n $settings_backup && -f $settings_backup ]]; then
    cp "$settings_backup" "$settings_path"
    rm -f "$settings_backup"
  fi
}
trap cleanup EXIT INT TERM HUP

if [[ ! -x $probe ]]; then
  echo "Missing probe: ${probe}" >&2
  exit 1
fi
if [[ ! -f $settings_path ]]; then
  echo "Missing SteamVR settings: ${settings_path}" >&2
  exit 1
fi
if ! command -v jq >/dev/null || ! command -v steam >/dev/null; then
  echo "This test requires jq and steam in PATH." >&2
  exit 1
fi

for name in vrserver vrcompositor vrmonitor; do
  if pgrep -u "$(id -u)" -x "$name" >/dev/null; then
    echo "Refusing to start: ${name} is already running." >&2
    exit 1
  fi
done

"$probe" --check-direct-display

xrandr_state=$(xrandr --current)
if ! awk '
  /^DisplayPort-1 connected/ { in_xreal = 1; next }
  /^[^[:space:]]/ { in_xreal = 0 }
  in_xreal && /1920x1080/ && /60\.00\*/ { found = 1 }
  END { exit(found ? 0 : 1) }
' <<< "$xrandr_state"; then
  echo "Refusing to start: XREAL DisplayPort-1 is not active at 60 Hz." >&2
  exit 1
fi

settings_backup=$(mktemp)
cp "$settings_path" "$settings_backup"
temporary_settings=$(mktemp)
jq '.driver_xrealone.enable = true |
    .driver_xrealone.tap_input_enabled = true |
    .steamvr.directDisplay = true |
    .steamvr.preferredRefreshRate = 60' \
  "$settings_path" > "$temporary_settings"
cp "$temporary_settings" "$settings_path"
rm -f "$temporary_settings"

echo "Starting one SteamVR test for at most ${duration_seconds} seconds."
echo "The watchdog will directly stop SteamVR and restore its settings."
steam -applaunch 250820

for ((remaining = duration_seconds; remaining > 0; --remaining)); do
  if ((remaining == duration_seconds || remaining % 10 == 0)); then
    echo "SteamVR test: ${remaining} seconds remaining"
  fi
  sleep 1
done

echo "SteamVR test time limit reached."
