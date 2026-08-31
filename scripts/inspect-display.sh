#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

# Read-only Xorg/RandR inspection. Deliberately do not change non-desktop or
# modes: doing so caused GNOME/Xorg session termination on the reference host.

steamvr_status="STOPPED"
if pgrep -x vrserver >/dev/null 2>&1 ||
   pgrep -x vrcompositor >/dev/null 2>&1 ||
   pgrep -x vrmonitor >/dev/null 2>&1; then
  steamvr_status="RUNNING"
fi

echo "SteamVR: ${steamvr_status}"
echo ""
printf '%-18s %-12s %-12s %-10s %s\n' \
  OUTPUT CONNECTION NON_DESKTOP FHD EDID_IDENTITY

xrandr --props | python3 -c '
import re
import sys

outputs = []
current = None
in_edid = False
for line in sys.stdin:
    if line.startswith("Screen "):
        continue
    if line and not line[0].isspace():
        fields = line.split()
        current = {
            "name": fields[0],
            "connected": len(fields) > 1 and fields[1] == "connected",
            "non_desktop": None,
            "modes": set(),
            "edid": "",
        }
        outputs.append(current)
        in_edid = False
        continue
    if current is None:
        continue
    stripped = line.strip()
    if stripped == "EDID:":
        in_edid = True
        continue
    if in_edid and re.fullmatch(r"[0-9a-fA-F]+", stripped or ""):
        current["edid"] += stripped
        continue
    in_edid = False
    match = re.match(r"non-desktop:\s+(\d+)", stripped)
    if match:
        current["non_desktop"] = int(match.group(1))
        continue
    match = re.match(r"(\d+)x(\d+)\s", stripped)
    if match:
        current["modes"].add((int(match.group(1)), int(match.group(2))))

for output in outputs:
    identity = "unknown"
    edid = output["edid"]
    if len(edid) >= 256:
        raw = bytes.fromhex(edid)
        vendor = (raw[8] << 8) | raw[9]
        product = (raw[11] << 8) | raw[10]
        name = ""
        for offset in (54, 72, 90, 108):
            if raw[offset:offset + 3] == b"\x00\x00\x00" and raw[offset + 3] == 0xfc:
                name = raw[offset + 5:offset + 18].split(b"\n", 1)[0].decode("ascii", "replace").rstrip()
        identity = f"0x{vendor:04x}:0x{product:04x} {name}".rstrip()
    output_name = output["name"]
    connection = "connected" if output["connected"] else "disconnected"
    non_desktop = output["non_desktop"] if output["non_desktop"] is not None else -1
    has_fhd = "yes" if (1920, 1080) in output["modes"] else "no"
    print(f"{output_name:<18} {connection:<12} {non_desktop!s:<12} "
          f"{has_fhd:<10} {identity}")
'
