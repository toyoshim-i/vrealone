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
- opt-in microphone-plus-IMU temple-tap controls for gaze selection and
  recentering;
- a user-only local control socket for click, recenter, and status requests;
- deterministic tests that do not require SteamVR or connected hardware.

Sensor-enabled builds now feed live IMU orientation and angular velocity into
the OpenVR pose. This path has passed independent transport and deterministic
unit tests. Controlled yaw, pitch, and roll measurements established the
parser-to-OpenVR axis mapping, which is now implemented for both orientation
and world-space angular velocity. Live display and tracking have passed a
guarded XREAL-only SteamVR test on the recorded development host.
Sensor-disabled builds retain the static identity pose for display-only
diagnostics.

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

## Install and register with SteamVR

The following package names apply to Ubuntu 24.04. SteamVR must already be
installed for the current user:

```sh
sudo apt install build-essential cmake ninja-build libasound2-dev
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

The default build downloads reviewed dependency revisions recorded in
`THIRD_PARTY.yml`. Use the offline build options above when network access is
not appropriate. ALSA is dynamically linked for microphone capture; no ALSA
source is copied into this Apache-2.0 project.

Close SteamVR, then register the build directory as an external driver:

```sh
steamvr_root="$HOME/.local/share/Steam/steamapps/common/SteamVR"
"$steamvr_root/bin/vrpathreg.sh" adddriver "$(pwd)/build-release/xrealone"
"$steamvr_root/bin/vrpathreg.sh" show
```

Rebuilding updates the registered driver in place. Remove the registration
with:

```sh
steamvr_root="$HOME/.local/share/Steam/steamapps/common/SteamVR"
"$steamvr_root/bin/vrpathreg.sh" removedriver "$(pwd)/build-release/xrealone"
```

Do not copy files into SteamVR's installation directory. Registration through
`vrpathreg` keeps project files separate from Steam-managed files.

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

## Temple-tap controls

XREAL One advertises a two-channel USB microphone, but both captured channels
are identical on the tested hardware. Left- and right-temple taps therefore
cannot be distinguished. The driver instead accepts a tap only when a loud
microphone peak and an IMU acceleration impulse occur together. It processes
20 ms peak windows and does not record or write audio samples.

Tap input is opt-in. Merge these values into the `driver_xrealone` object in
`~/.local/share/Steam/config/steamvr.vrsettings` while SteamVR is stopped:

```json
{
  "driver_xrealone": {
    "tap_input_enabled": true,
    "tap_audio_device": "pulse",
    "tap_audio_peak_threshold": 80000000,
    "tap_imu_deviation_m_s2": 3.0
  }
}
```

`pulse` uses PipeWire's PulseAudio-compatible shared capture path and works
inside the tested Steam Runtime, where the native ALSA `pipewire` PCM name is
not available to `vrserver`. Select **XREAL One Analog Stereo** as the default
input source and confirm it with `wpctl status` before starting SteamVR.

The initial gesture mapping is:

- one temple tap: gaze select after the double-tap decision window;
- two distinct taps roughly 200--650 ms apart: recenter after the glasses
  settle.

If a light tap is ignored, tap slightly more firmly instead of immediately
lowering the thresholds. A loud sound without an IMU impulse, or an IMU impulse
without a loud microphone peak, is rejected.

Test detection without starting SteamVR:

```sh
./build-release/xrealone_tap_probe 15
```

After the ready message, wait one second, tap once, wait one second, then tap
twice about 300 ms apart. The expected summary is exactly
`clicks=1 recenters=1`. Optional arguments select the duration, IMU address,
and ALSA capture device:

```sh
./build-release/xrealone_tap_probe 15 169.254.2.1:52998 pulse
```

While the driver is running, the local user-only control socket also supports
explicit diagnostics and fallback actions:

```sh
./build-release/xrealone_ctl status
./build-release/xrealone_ctl click
./build-release/xrealone_ctl recenter
```

## Bounded SteamVR tests

Never start a display-acquisition test without an independent exit path. With
XREAL as the only display and no controller, the dashboard cannot be used to
exit SteamVR. After physically disconnecting every competing display, selecting
1920x1080@60, and retaining SSH access, run a bounded test:

```sh
./scripts/run-timed-steamvr-test.sh 60
```

The script refuses ambiguous display layouts or a non-60-Hz XREAL mode, enables
the driver, launches SteamVR once, and stops it after the requested 10--600
seconds. Cleanup directly terminates the current user's SteamVR processes and
restores the pre-test settings. It never invokes `vrmonitor -shutdown` and never
restarts SteamVR. For manual recovery from SSH, run:

```sh
./scripts/stop-steamvr-test.sh
```

## Current validation gates

See [the implementation plan](docs/linux-steamvr-implementation-plan.md) and
[the recorded test host](docs/test-environment.md). Full Gate 1 completion still
requires SteamVR to DRM-lease the XREAL connector and render a stable SBS image.
Half SBS is sufficient for driver and tracking development; full SBS remains a
release-quality display gate.
The repository contains no source copied from the GPL reference-only projects.

## TODO

- Expose every tap-classification timing value through `driver_xrealone`
  settings: microphone/IMU correlation window, double-tap window, tap
  refractory period, post-double-tap settling delay, IMU quiet/rearm period,
  microphone startup suppression, and OpenVR click-pulse duration.
- Validate settings at load time, log the effective values, and retain safe
  bounds so malformed configuration cannot create an input loop.
- Add a guided calibration tool that measures microphone background and impact
  peaks, IMU background and impact deviation, audio-server latency, and the
  user's deliberate double-tap cadence. It should recommend a complete settings
  block, report confidence and false-positive margin, and update
  `steamvr.vrsettings` only after making a backup and receiving explicit
  approval.
- Extend `xrealone_tap_probe` with a passive false-positive test and
  machine-readable output so calibration results can be compared across
  firmware versions and hosts.
- Select the XREAL microphone by stable PipeWire/udev identity instead of
  relying on the current default capture source.
- Add idempotent install and uninstall scripts that locate SteamVR and invoke
  `vrpathreg` without modifying Steam-managed directories.
