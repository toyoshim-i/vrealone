# vrealone

`vrealone` is an Apache-2.0-licensed, native Linux SteamVR driver under
development for XREAL One glasses. The current milestone provides:

- a buildable OpenVR HMD driver with a direct-display component;
- measured XREAL One USB and EDID identification;
- 3840x1080 side-by-side viewport and conservative 60 Hz display defaults;
- a DRM/EDID and IMU diagnostic probe;
- quaternion recentering, thread-safe pose snapshots, and a Fusion AHRS wrapper;
- deterministic tests that do not require SteamVR or connected hardware.

The driver currently publishes a static identity pose for display-path testing.
It does not yet feed live IMU orientation into SteamVR because the sensor unit
and OpenVR axis mapping still require controlled physical validation.

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

The SteamVR driver tree is produced below `build/xrealone/`. Do not register it
until the glasses report 3840x1080 SBS and the measured EDID values match the
defaults.

## Hardware probes

List DRM connectors and decode their EDID identity:

```sh
./build/xrealone_probe --displays
```

Read 500 IMU samples from the default XREAL One endpoint:

```sh
./build/xrealone_probe --sensor 169.254.2.1:52998 500
```

The glasses must expose a USB network route to `169.254.2.1`. Sensor output may
contain private timing and motion data, so review it before attaching logs to a
public issue.

## Current validation gates

See [the implementation plan](docs/linux-steamvr-implementation-plan.md) and
[the recorded test host](docs/test-environment.md). Full Gate 1 completion still
requires SteamVR to DRM-lease the XREAL connector and render a stable SBS image.
The repository contains no source copied from the GPL reference-only projects.
