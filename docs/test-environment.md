# Initial Linux Test Environment

Recorded on 2026-08-31 in Asia/Tokyo. This file describes the first development
host and is not a statement of minimum requirements.

## Host

- Ubuntu 24.04.4 LTS (Noble), x86_64.
- Linux 7.0.0-30-generic.
- GNOME on Wayland, with Xwayland available at `DISPLAY=:0`.
- AMD Radeon Vega (PCI ID `1002:15d8`) using the `amdgpu` kernel driver.
- Mesa Vulkan 25.2.8. The sandboxed command-line check exposed llvmpipe rather
  than the physical GPU, so physical-GPU Vulkan validation remains pending.
- Vulkan loader 1.3.275 exposes `VK_EXT_acquire_drm_display`,
  `VK_EXT_direct_mode_display`, and `VK_KHR_display`.

## Build tools

- CMake 3.28.3, Ninja 1.11.1, GCC 13.3.0.
- Rust and Cargo 1.93.1 (recorded for the later sensor-library build).

## XREAL One observations

- USB device: `3318:0438 XREAL XREAL One`.
- Two USB network interfaces appeared and reported link up:
  `enxfcd2b6adcc6c` and `enxfcd2b6adcc6d`.
- The display appeared on `card1-DP-2`, owned by the AMD GPU.
- In full-SBS mode, DRM exposed only `640x480`, the glasses became inoperable,
  and the host reported that the DP display was not detected at 90 Hz.
- In half-SBS mode, `card1-DP-2` remained connected and exposed 1920x1080. The
  glasses stayed usable at FHD/60 Hz, so half SBS is the development profile for
  this host.
- EDID identity: manufacturer `MRG`, raw vendor ID `0x3647`, product ID
  `0x4101`.
- USB network addresses: `169.254.1.10/24` and `169.254.2.10/24`.
- A route to `169.254.2.1` was installed and TCP port 52998 accepted a
  connection.
- Firmware version remains to be recorded.

The first sensor capture received 500 samples in 0.501 seconds (approximately
998 Hz) with no timestamp rollback. A second capture received 5,000 samples in
5.002 seconds (approximately 999.6 Hz), also with no rollback. The second run's
maximum gyroscope magnitude was only 0.0185, indicating that the glasses were
effectively stationary. At that point, controlled rotations were still needed
to establish units and axis signs; the subsequent captures below completed
that work.

The first controlled rotation captured a roughly 90-degree turn from straight
ahead to the left. Over 5,000 samples at 999.95 Hz, the parser-facing integrated
gyroscope values were `[+0.0056, +0.0018, +1.7171]`, with peak absolute values
`[0.2655, 0.3393, 1.4850]`. Leftward yaw therefore maps predominantly to the
parser's positive Z axis. The integrated magnitude is reasonably close to
pi/2 radians, supporting (but not yet completing) the radians-per-second unit
validation. Pitch and roll captures are still required before defining the
OpenVR coordinate transform.

A second controlled capture pitched the glasses roughly 90 degrees upward.
Gravity moved from approximately `+Z` (`[+0.15, -0.12, +9.71]`) to `-Y`
(`[-0.37, -9.88, +1.63]`), independently confirming the physical rotation.
The integrated gyroscope values were
`[-1.3834, -0.0091, +0.1279]`, with peak absolute values
`[0.5746, 0.3322, 0.1068]`. Upward pitch therefore maps predominantly to the
parser's negative X axis. The remaining controlled measurement is right-ear-
down roll.

The final controlled capture tilted the glasses toward the right ear. Gravity
moved from approximately `+Z` (`[-0.28, +0.11, +9.64]`) to `+X`
(`[+9.91, -0.73, +0.43]`). The integrated gyroscope values were
`[-0.1955, -1.5389, +0.1703]`, with peak absolute values
`[0.4738, 1.0201, 0.3578]`. Right-ear-down roll therefore maps predominantly
to the parser's negative Y axis.

Together, the controlled measurements establish the parser-to-OpenVR motion
mapping used by the implementation: parser `-X` is OpenVR `+X` (pitch up),
parser `+Z` is OpenVR `+Y` (yaw left), and parser `-Y` is OpenVR `-Z`
(right-ear-down roll). Equivalently, an axial vector maps as
`[-parser.x, parser.z, parser.y]`.

The fused tracking probe was then run for 4,000 samples. Fusion reported
`calibrating` through sample 2,750, changed to `running` at sample 3,000, and
recentered to zero at that transition. The following stationary interval ended
near `[+0.13, +0.59, +0.04]` degrees in OpenVR pitch/yaw/roll rotation-vector
components. The small yaw change is consistent with expected uncorrected yaw
drift from a gyro/accelerometer-only AHRS; it must be characterized over a
longer interval before release.

A subsequent 10,000-sample end-to-end tracking capture exercised the three
motions after automatic recentering. Leftward yaw reached approximately
`yaw_y=+72` degrees, upward pitch reached `pitch_x=+51` degrees, and a
right-ear-down roll ended at `roll_z=-37` degrees. Each dominant fused,
recentered, OpenVR-mapped component had the expected axis and sign. Smaller
cross-axis components were present during the manually performed compound head
motion, but did not obscure the dominant axis. This completes the physical
direction check for the pose path without loading SteamVR.

## SteamVR

- Native Linux SteamVR app ID: `250820`.
- Installed build ID: `23791826`.
- Runtime: `/home/toyoshim/.local/share/Steam/steamapps/common/SteamVR`.
- OpenVR config: `/home/toyoshim/.local/share/Steam/config`.
- SteamVR logs: `/home/toyoshim/.local/share/Steam/logs`.
- `vrpathreg`: `bin/linux64/vrpathreg` below the runtime. Direct command-line
  invocation requires the runtime's `bin/linux64` library search path.
- External driver registration: `xrealone` resolves to
  `/home/toyoshim/Work/vrealone/build-release/xrealone`.

The driver was registered while `vrserver` and `vrcompositor` were stopped.
SteamVR 2.16.7 loaded the driver and completed Vulkan compositor startup under
Xorg. Binary inspection and compositor logs showed that its RandR path scans
every advertised mode, compares only width and height, and selects the first
matching output. It selected RandR output `0x53`, measured as `HDMI-A-0`,
because that desktop display advertised 1920x1080 before the XREAL output.
Changing only the desktop's active resolution did not remove its advertised
FHD modes. Driver window offsets and a `directDisplayOutput` settings entry did
not participate in this selection path.

Setting XREAL itself to `non-desktop=1` removed its usable modes from the RandR
candidate scan on this AMD/Xorg host, leaving the desktop display as the only
FHD match. Attempts to toggle `non-desktop` also caused AMD page-flip failures,
GNOME monitor assertions, and display-session termination. That workaround was
retired. Display tooling is read-only until a selection mechanism that does not
mutate the live GNOME/Xorg layout is validated.

Marking only `HDMI-A-0` as `non-desktop=1` was also insufficient. SteamVR still
enumerated the output's 21 advertised modes while RandR reported the output as
disconnected, found its first 1920x1080 mode, and acquired output `0x53`.
Connection state and the `non-desktop` property therefore cannot be used as a
selection guard for this SteamVR build.

### Successful single-display acquisition

With the HDMI display physically disconnected, the preflight state contained
only XREAL One on RandR output `0x54`. SteamVR then reported:

- output `0x53`: zero modes;
- output `0x54`: two 1920x1080 modes;
- requested refresh: 60 Hz;
- selected mode: 1920x1080@60 on output `0x54`;
- `Acquired xlib display!`, `Headset is using direct mode`, and compositor
  startup completion.

The glasses displayed SteamVR Home. With no controller and only the current
static pose, Home showed its center gaze cursor but could not be operated. This
is an input/tracking limitation, not a display-acquisition failure.

That successful display acquisition predates the live sensor-worker build.
The registered build tree now contains the sensor integration, but the active
SteamVR configuration remains disabled. No live-pose SteamVR test has been run,
pending an explicit safe preflight and supervised tracking test.

Do not invoke `vrmonitor -shutdown` as a recovery mechanism. On this host it can
start a new monitor/compositor when it fails to attach to the existing runtime.
Recovery must terminate already-running processes directly from the retained
SSH session, then set `driver_xrealone.enable=false` and
`steamvr.directDisplay=false` before reconnecting other displays.

The first live-orientation SteamVR test successfully selected XREAL output
`0x54` at 60 Hz, acquired it in direct mode, connected the IMU, and changed
tracking from calibrating to running after approximately three seconds. Visual
head tracking appeared correct in SteamVR Home. However, with XREAL as the only
display and no controller, the user could not exit SteamVR or return to the
desktop and again required SSH recovery. All subsequent tests must use
`scripts/run-timed-steamvr-test.sh` so a foreground watchdog stops the runtime
and restores disabled settings after a fixed duration.

## Current blockers for Gate 1

1. Repeat the successful XREAL-only half-SBS acquisition for the required
   30-minute stability interval and reconnect cases.
2. Keep dual-FHD configurations blocked until a non-mutating selection method
   is available; `non-desktop` does not solve the ambiguity.
3. Repeat the display gate on hardware capable of 3840x1080 full SBS before a
   release claims native per-eye transport resolution.
