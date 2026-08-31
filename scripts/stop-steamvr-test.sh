#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

# Stop only this user's SteamVR runtime processes.  Do not use
# `vrmonitor -shutdown`: on the reference host that command may launch a new
# monitor/compositor when it cannot attach to the existing runtime.
runtime_clients=(vrmonitor vrdashboard steamtours vrwebhelper)
runtime_core=(vrcompositor vrserver vrstartup)

signal_processes() {
  local signal=$1
  shift
  local name
  local -a pids
  for name in "$@"; do
    mapfile -t pids < <(pgrep -u "$(id -u)" -x "$name" || true)
    if ((${#pids[@]} > 0)); then
      kill "-${signal}" "${pids[@]}" 2>/dev/null || true
    fi
  done
}

signal_processes TERM "${runtime_clients[@]}"
signal_processes TERM "${runtime_core[@]}"

for _ in {1..20}; do
  remaining=false
  for name in "${runtime_clients[@]}" "${runtime_core[@]}"; do
    if pgrep -u "$(id -u)" -x "$name" >/dev/null; then
      remaining=true
      break
    fi
  done
  if [[ $remaining == false ]]; then
    break
  fi
  sleep 0.25
done

signal_processes KILL "${runtime_clients[@]}"
signal_processes KILL "${runtime_core[@]}"

settings_path="${HOME}/.local/share/Steam/config/steamvr.vrsettings"
if [[ -f $settings_path ]] && command -v jq >/dev/null; then
  temporary_settings=$(mktemp)
  jq '.driver_xrealone.enable = false | .steamvr.directDisplay = false' \
    "$settings_path" > "$temporary_settings"
  cp "$temporary_settings" "$settings_path"
  rm -f "$temporary_settings"
fi

echo "SteamVR test stopped; xrealone and direct display are disabled."
