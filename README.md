# vrealone

`vrealone` is an Apache-2.0-licensed, native Linux SteamVR driver under
development for XREAL One glasses. The current milestone provides:

- a buildable OpenVR HMD driver with a direct-display component;
- measured XREAL One USB and EDID identification;
- a measured 1920x1080@60 half-SBS development profile, with an optional
  3840x1080 full-SBS profile;
- a DRM/EDID and IMU diagnostic probe;
- a bounded, reconnecting IMU worker feeding Fusion AHRS pose snapshots;
- quaternion recentering and stale-pose invalidation;
- deterministic tests that do not require SteamVR or connected hardware.

Sensor-enabled builds now feed live IMU orientation and angular velocity into
the OpenVR pose. This path has passed independent transport and deterministic
unit tests. Controlled yaw, pitch, and roll measurements established the
parser-to-OpenVR axis mapping, which is now implemented for both orientation
and world-space angular velocity. The live path has deliberately not yet been
enabled in SteamVR; that requires a separate guarded test. Sensor-disabled
builds retain the static identity pose for display-only diagnostics.

## Build

The normal build fetches three dependencies at reviewed revisions recorded in
`THIRD_PARTY.yml`:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

For an offline build, point CMake at exact source trees matching those
revisions:

```sh
cmake -S . -B build -G Ninja \
  -DVREALONE_OPENVR_SOURCE_DIR=/path/to/openvr \
  -DVREALONE_XREAL_DRIVER_SOURCE_DIR=/path/to/xreal_one_driver \
  -DVREALONE_FUSION_SOURCE_DIR=/path/to/Fusion
```

The SteamVR driver tree is produced below `build/xrealone/`. The default output
uses half SBS: two encoded 960x1080 eye viewports, each horizontally expanded
by the glasses to its native 1920x1080 optical image. SteamVR still renders each
eye at 1920x1080 before output scaling. A full-SBS override is provided at
`resources/xrealone/examples/full-sbs.vrsettings`, but remains unvalidated on
the initial host.

## Hardware probes

List DRM connectors and decode their EDID identity:

```sh
./build/xrealone_probe --displays
```

Refuse a direct-display test unless XREAL One is the only connected DRM
connector advertising the configured half-SBS size:

```sh
./build/xrealone_probe --check-direct-display
```

The same check runs inside the driver before it registers an HMD. Because this
SteamVR build scans retained modes even on disconnected RandR outputs, the
guard treats any non-XREAL connector still advertising 1920x1080 as a conflict,
regardless of its connection status. The default
`allow_ambiguous_display_selection=false` setting prevents SteamVR from
acquiring an ordinary 1920x1080 monitor when more than one connector advertises
the half-SBS size. Keep this guard enabled outside an explicitly supervised
experiment.

On Xorg, inspect the RandR outputs and identify XREAL One by its measured EDID:

```sh
./scripts/inspect-display.sh
```

SteamVR 2.16.7 selects the first RandR output advertising the requested HMD
size, even when that output is disconnected or marked `non-desktop`. Half SBS
therefore collides with ordinary 1920x1080 monitors. Do not change
`non-desktop` or output modes while GNOME or SteamVR is running: this caused
display-session termination on the reference host. The validated Gate 1 test
physically disconnects every competing display before SteamVR starts.

Read 500 IMU samples from the default XREAL One endpoint:

```sh
./build/xrealone_probe --sensor 169.254.2.1:52998 500
```

The glasses must expose a USB network route to `169.254.2.1`. Sensor output may
contain private timing and motion data, so review it before attaching logs to a
public issue.

The summary includes `integrated_gyro_units` and `peak_abs_gyro` for each axis.
For a controlled unit/sign test, keep the glasses still briefly, rotate a
comfortable known angle around one physical axis during a 3,000-sample capture,
then hold still again. If the reported gyro is radians per second, a 45-degree
rotation integrates to approximately 0.79 and a 90-degree rotation to 1.57.
Repeat separately for yaw, pitch, and roll and record the direction of each
rotation; do not force an uncomfortable neck angle.

The sensor worker uses finite TCP timeouts, reports
`disconnected`/`calibrating`/`running` states, invalidates samples older than
`stale_pose_timeout_ms`, and reconnects with bounded exponential backoff.

Validate the fused, OpenVR-mapped orientation without starting SteamVR:

```sh
./build/xrealone_probe --tracking 169.254.2.1:52998 10000
```

Keep the glasses still for the initial three-second Fusion startup. After
`tracking=running recentered=yes` appears, make comfortable 20--30 degree
movements. `pitch_x` should increase when looking up, `yaw_y` should increase
when turning left, and `roll_z` should decrease when lowering the right ear.
The command runs for approximately ten seconds with the default sample count.

## Current validation gates

See [the implementation plan](docs/linux-steamvr-implementation-plan.md) and
[the recorded test host](docs/test-environment.md). Full Gate 1 completion still
requires SteamVR to DRM-lease the XREAL connector and render a stable SBS image.
Half SBS is sufficient for driver and tracking development; full SBS remains a
release-quality display gate.
The repository contains no source copied from the GPL reference-only projects.
