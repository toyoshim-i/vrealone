# vrealone

`vrealone` is an Apache-2.0-licensed SteamVR driver under development for
XREAL One glasses. Linux and Windows x64 builds are supported. The current
milestone provides:

- a buildable OpenVR HMD driver with a direct-display component;
- measured XREAL One USB and EDID identification;
- a measured 1920x1080@60 half-SBS development profile, with an optional
  3840x1080 full-SBS profile;
- a DRM/EDID and IMU diagnostic probe;
- a bounded, reconnecting IMU worker feeding Fusion AHRS pose snapshots;
- quaternion recentering and stale-pose invalidation;
- head-gaze dwell selection;
- a local control endpoint for click, recenter, and status requests;
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

On Windows, build an x64 DLL even when the host is Windows 11 on Arm because
the current SteamVR server process is x64:

```powershell
cmake -S . -B build-win64 -G "Visual Studio 17 2022" -A x64
cmake --build build-win64 --config Release
ctest --test-dir build-win64 -C Release --output-on-failure
```

The Windows package uses `bin/win64/driver_xrealone.dll`. Its default settings
use extended-desktop mode so it can also be tested with a virtual display.

## Install and register with SteamVR

The following package names apply to Ubuntu 24.04. SteamVR must already be
installed for the current user:

```sh
sudo apt install build-essential cmake ninja-build
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

The default build downloads reviewed dependency revisions recorded in
`THIRD_PARTY.yml`. Use the offline build options above when network access is
not appropriate.

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

On Windows, register the x64 build with SteamVR's `vrpathreg.exe`:

```powershell
$steamVr = "${env:ProgramFiles(x86)}\Steam\steamapps\common\SteamVR"
& "$steamVr\bin\win64\vrpathreg.exe" adddriver `
  "$PWD\build-win64\xrealone"
```

Windows includes current DirectX, but it does not necessarily include the
legacy side-by-side D3DX libraries used by some SteamVR components. If startup
reports a missing `d3dx10_43.dll`, run Microsoft's DirectX June 2010 setup
already cached by Steam. Steam normally installs common redistributables when
SteamVR is launched from the Steam client; launching `vrstartup.exe` directly
during driver development can bypass that first-run step:

```powershell
$redist = "${env:ProgramFiles(x86)}\Steam\steamapps\common\Steamworks Shared\_CommonRedist"
& "$redist\DirectX\Jun2010\DXSETUP.exe" /silent
```

Do not download individual DLL files from third-party DLL sites. The Microsoft
installer adds both x64 and x86 side-by-side components without replacing the
DirectX version built into Windows.

If SteamVR logs driver load error 126 and `CONCRT140.dll` is absent, install
Steam's cached Microsoft Visual C++ 2022 redistributables as well:

```powershell
& "$redist\vcredist\2022\VC_redist.x64.exe" /install /quiet /norestart
& "$redist\vcredist\2022\VC_redist.x86.exe" /install /quiet /norestart
```

SteamVR should also be selected as the active OpenXR runtime from SteamVR's
OpenXR settings. Verify the 64-bit Windows registration with:

```powershell
reg query "HKLM\SOFTWARE\Khronos\OpenXR\1" /v ActiveRuntime
```

The value should point to SteamVR's `steamxr_win64.json`.

Windows on Arm uses the same x64 package while SteamVR's `vrserver.exe` is
x64. A Parallels VM can pass the XREAL USB NCM interface through for IMU
tracking, but its virtual display adapter is a separate compositor dependency;
see `docs/windows-on-arm.md`.

Native Windows display testing is recorded in `docs/windows-native.md`.
Windows currently defaults to extended-desktop mode. Reporting the XREAL panel
as a real, non-desktop display is necessary but was not sufficient to make
SteamVR acquire it through AMD direct mode on the recorded host.

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

## Head-gaze controls

Holding the head orientation within the configured angular threshold triggers
`/input/select/click`. The dwell duration, angular threshold, and cooldown are
configured with `gaze_dwell_time_ms`, `gaze_dwell_max_angle_deg`, and
`gaze_dwell_cooldown_ms`. The local control endpoint also supports explicit
diagnostics and fallback actions:

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

- Validate settings at load time, log the effective values, and retain safe
  bounds so malformed configuration cannot create an input loop.
- Add idempotent install and uninstall scripts that locate SteamVR and invoke
  `vrpathreg` without modifying Steam-managed directories.
