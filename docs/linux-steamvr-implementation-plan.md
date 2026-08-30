# XREAL One SteamVR Driver for Linux: Validation and Implementation Plan

## Status

**Verdict: feasible, with one mandatory display-path validation before full implementation.**

The Linux plan is directionally correct: native SteamVR removes the macOS/Wine GPU-sharing blocker, XREAL One IMU transport has working open-source implementations, and an OpenVR HMD driver already exists for the XREAL Air family. However, the proposed implementation must be updated in several important ways:

1. Current Linux SteamVR should be treated as requiring a direct-mode display. The first milestone must identify the XREAL panel by EDID and prove that `vrcompositor` can acquire its DRM connector. Extended-desktop mode is only a diagnostic fallback, not the primary MVP path.
2. XREAL One SBS output is 3840x1080 total, or 1920x1080 per eye. Existing XRLinuxDriver data exposes 60, 72, and 90 Hz SBS modes and explicitly maps a 120 Hz non-SBS mode to 90 Hz SBS. The MVP must therefore start at 3840x1080@60 Hz, then test 90 Hz; it must not assume 120 Hz SBS.
3. The old Air OpenVR driver is useful as a proof of concept, but not as a code base to preserve unchanged. Its display flags describe an extended/virtual display, its settings are placed in the global `steamvr` section, it converts tracking through Euler angles, and its shared pose state is not thread-safe.
4. A separate IMU daemon is not required for the MVP. Embedding `xreal_one_driver` and Fusion in `driver_xrealone.so` is the smallest reliable architecture. A daemon can be introduced later if multiple consumers or crash isolation justify it.

The project should proceed only after Gate 1 below demonstrates that SteamVR can render to the physical XREAL connector in direct mode.

## Validated building blocks

| Area | Evidence | Conclusion |
| --- | --- | --- |
| OpenVR driver loading | Valve documents native Linux driver binaries, manifests, `IServerTrackedDeviceProvider`, and registration through `vrpathreg`. | No custom runtime is needed. |
| XREAL One transport | [`xreal_one_driver`](https://github.com/rohitsangwan01/xreal_one_driver) connects to `169.254.2.1:52998`, parses the 84-byte IMU messages, and exposes gyro, acceleration, and timestamps through a C ABI. | Reuse the parser; do not reverse-engineer the packet format again. |
| XREAL One model support | [`XRLinuxDriver`](https://github.com/wheaney/XRLinuxDriver) contains One/One Pro/1S USB IDs, One's TCP transport selection, coordinate conversion, calibration, FOV values, and operating notes. | Use it as the behavioral reference for model detection and pose conventions. |
| SteamVR HMD integration | [`OpenVR-xrealAirGlassesHMD`](https://github.com/wheaney/OpenVR-xrealAirGlassesHMD) demonstrates an XREAL SBS `IVRDisplayComponent` and 3DoF HMD registration. | Reuse concepts and compare behavior, but replace its lifecycle, tracking, settings, and display-mode implementation. |
| Modern Linux direct display | A current Linux OpenVR implementation such as [`psvr2-linux-adapter`](https://github.com/unterschall/psvr2-linux-adapter) reports a real, non-desktop display plus EDID vendor/product IDs so SteamVR can DRM-lease the connector. | Use this structure as the primary display reference. |
| Pose contract | [Valve's OpenVR driver documentation](https://github.com/ValveSoftware/openvr/blob/master/docs/Driver_API_Documentation.md) defines the OpenVR coordinate system, valid pose fields, angular velocity, prediction, driver layout, and display interfaces. | Submit native quaternions and angular velocity; avoid Euler conversion in the runtime path. |

## Target scope

### Included in the first usable release

- Linux x86_64 with native SteamVR.
- XREAL One initially; One Pro and 1S remain data-driven follow-ups.
- 3DoF orientation tracking only.
- A physical 3840x1080 SBS display, initially at 60 Hz.
- Native Linux and Proton OpenVR applications.
- Seated play, gamepad input, manual recenter, clean reconnect, and useful diagnostics.

### Explicitly excluded

- Positional tracking, room scale, motion controllers, hand tracking, and passthrough.
- A replacement OpenXR runtime.
- A custom Vulkan compositor or ALVR-style WSI interception layer.
- macOS/Wine support.
- Automatic switching of the XREAL One into SBS mode for the MVP. The user switches the glasses through their menu.

## Proposed architecture

```text
XREAL One
  |-- DisplayPort Alt Mode, 3840x1080 SBS
  |     `-- DRM connector --leased by--> SteamVR vrcompositor
  |
  `-- USB NCM, 169.254.2.1:52998
        `-- xreal_one_driver parser
              `-- Fusion AHRS
                    `-- quaternion + angular velocity + timestamp
                          `-- driver_xrealone.so
                                |-- IServerTrackedDeviceProvider
                                |-- ITrackedDeviceServerDriver
                                `-- IVRDisplayComponent
                                      `-- SteamVR vrserver
```

The SteamVR driver owns the TCP connection and fusion state. A sensor thread publishes immutable pose snapshots to the OpenVR device through a mutex or sequence-lock abstraction. The driver reports the XREAL panel as a real, non-desktop display and supplies the EDID identity so `vrcompositor` performs DRM acquisition and presentation itself.

## Planned repository layout

```text
CMakeLists.txt
cmake/
  Dependencies.cmake
resources/xrealone/
  driver.vrdrivermanifest
  resources/settings/default.vrsettings
src/
  driver_factory.cpp
  device_provider.{hpp,cpp}
  hmd_device.{hpp,cpp}
  display_component.{hpp,cpp}
  tracking/
    imu_transport.{hpp,cpp}
    fusion_tracker.{hpp,cpp}
    pose_snapshot.{hpp,cpp}
    recenter.{hpp,cpp}
  platform/
    linux_display_probe.{hpp,cpp}
tests/
  imu_parser_tests.cpp
  fusion_tracker_tests.cpp
  coordinate_tests.cpp
  recenter_tests.cpp
  display_config_tests.cpp
tools/
  xrealone_probe.cpp
scripts/
  install-driver.sh
  uninstall-driver.sh
  collect-diagnostics.sh
```

Pin OpenVR, `xreal_one_driver`, and xioTechnologies/Fusion to reviewed revisions. Build the Rust parser as a static library through its existing CMake shim and link it into the Linux driver `.so`. Follow [the licensing and source-provenance strategy](licensing-strategy.md): GPL projects remain reference-only unless the project explicitly adopts a different, reviewed licensing plan.

## Delivery gates

### Gate -1: Enforce the accepted project license and source-provenance rules

Apache-2.0 is selected in [ADR 0001](adr/0001-project-license.md). Before implementation or accepting outside contributions:

1. Keep XRLinuxDriver and `psvr2-linux-adapter` reference-only under the permissive implementation path.
2. Apply Apache-2.0 SPDX headers to new first-party source files.
3. Add vendored license texts under `LICENSES/` and generate `THIRD_PARTY_NOTICES.md` when dependencies are first imported.
4. Pin and audit OpenVR (BSD-3-Clause), `xreal_one_driver` (MIT), Fusion (MIT), `byteorder` (MIT OR Unlicense, selecting MIT), and Rust runtime components included in the static library.
5. Replace the placeholder revisions in `THIRD_PARTY.yml` before importing source or producing binaries.

**Exit criterion:** every planned direct dependency is compatible, pinned, and recorded; required notices are present; and no reference-only source has entered the repository.

### Gate 0: Record the reproducible environment

Before changing code, record:

- Distribution, kernel, desktop session, and X11/Wayland status.
- GPU model, kernel driver, Mesa or NVIDIA driver version, and Vulkan ICD.
- SteamVR branch, version, and build ID.
- XREAL One firmware version and USB product ID.
- Relevant SteamVR paths and the locations of `vrserver.txt` and `vrcompositor.txt`.

Create `docs/test-environment.md` from these observations. Avoid treating switching between SteamVR stable and beta as an unrecorded workaround.

**Exit criterion:** another developer can reproduce the same software environment and locate all relevant logs.

### Gate 1: Prove direct-mode display acquisition

This gate comes before production IMU work because it is the only unresolved platform-level risk.

1. Enable SBS on the glasses and verify a 3840x1080 mode with `modetest`, `xrandr`, or compositor display tools.
2. Read the raw EDID from `/sys/class/drm/card*-*/edid`; store `edid-decode` output in test notes and extract the vendor and product IDs used by OpenVR.
3. Confirm which GPU owns the connector and that its Vulkan ICD exposes the direct-display extensions SteamVR requests.
4. Build a minimal pose-less HMD driver using current `openvr_driver.h` interfaces:
   - `IsDisplayRealDisplay()` returns `true`.
   - `IsDisplayOnDesktop()` returns `false`.
   - `Prop_IsOnDesktop_Bool` is `false`.
   - `Prop_EdidVendorID_Int32` and `Prop_EdidProductID_Int32` match the measured panel.
   - Window geometry is 3840x1080 and each eye viewport is 1920x1080.
   - Frequency is initially 60 Hz.
5. Detach or disable the XREAL connector from the ordinary desktop as required by the compositor and start SteamVR on the GPU that owns it.
6. Verify in `vrcompositor.txt` that the intended connector is found and acquired, then display a stable SteamVR compositor image in both eyes.
7. Repeat one cold start, one SteamVR restart, and one cable reconnect.

Keep `direct_mode=false` as a developer-only diagnostic option. Do not plan the release around extended-desktop fullscreen placement: Valve recommends direct mode for real HMDs, and current Linux failures can terminate with `VR requires direct mode`.

**Exit criterion:** SteamVR renders SBS to the glasses for 30 minutes without moving the compositor to a desktop monitor or losing the DRM lease.

**No-go criterion:** if the connector cannot be acquired on the target GPU after EDID, Vulkan ICD, session, and DRM permission checks, stop and document the failure. Do not begin an `IVRDriverDirectModeComponent` or Vulkan WSI interception implementation without a separate design decision; those are substantially larger projects.

### Gate 2: Validate the sensor path independently

Implement `tools/xrealone_probe` before loading sensor code into `vrserver`.

1. Verify that enabling Ethernet on the glasses creates a USB NCM interface.
2. Ensure the host has a route to `169.254.2.1`; support a configurable endpoint while keeping `169.254.2.1:52998` as the default.
3. Connect through the existing C ABI and record raw sample rate, timestamp monotonicity, packet gaps, reconnect behavior, gyro units, axis signs, and acceleration magnitude.
4. Feed gyro and acceleration into Fusion with explicit sample periods. Reproduce XRLinuxDriver's One coordinate mapping only after unit tests establish the incoming and OpenVR frames.
5. Hold the glasses still during startup bias calibration. Reject invalid samples and avoid declaring tracking valid until calibration has completed.
6. Log sensor-to-host latency using a monotonic host timestamp captured immediately after `xo_next` returns.

**Exit criterion:** the probe runs for 60 minutes, survives a cable reconnect, produces a normalized quaternion without NaNs, and shows correct yaw, pitch, and roll directions.

### Gate 3: Implement the OpenVR device lifecycle

Implement a small, current OpenVR driver rather than retaining the Air driver's global-state structure.

1. Export `HmdDriverFactory` and implement `IServerTrackedDeviceProvider`.
2. Register exactly one `TrackedDeviceClass_HMD` when the XREAL display/sensor is available. If the sensor temporarily disappears, keep the driver loaded and publish an invalid/disconnected pose until reconnect succeeds.
3. Implement `ITrackedDeviceServerDriver` and return the `IVRDisplayComponent` for the exact interface version in the pinned SDK.
4. Start sensor work during device activation and use bounded I/O timeouts so `Deactivate()` and `Cleanup()` always join promptly.
5. Keep settings under `driver_xrealone`, never under the global `steamvr` object. Validate missing and invalid values explicitly.
6. Package the driver as `xrealone/bin/linux64/driver_xrealone.so` with a matching manifest and default settings.
7. Register and remove it with SteamVR's own `vrpathreg` rather than copying files into SteamVR's installation directory.

**Exit criterion:** 20 repeated load/activate/deactivate cycles complete without a `vrserver` crash, deadlock, leaked thread, duplicate registration, or safe-mode block.

### Gate 4: Submit correct 3DoF poses

1. Publish the Fusion quaternion directly in `DriverPose_t::qRotation` after one documented coordinate transform into OpenVR's right-handed X-right, Y-up, Z-back frame.
2. Set both transform quaternions to valid identities and position/linear velocity to zero.
3. Set `poseIsValid`, `deviceIsConnected`, and `TrackingResult_Running_OK` only after calibration and fresh sample receipt.
4. Set `willDriftInYaw=true` and evaluate `shouldApplyHeadModel` both ways; default to `false` unless testing shows that SteamVR's synthetic neck model improves seated use.
5. Convert the measured gyro into radians per second in OpenVR coordinates and fill `vecAngularVelocity` so SteamVR can predict the pose.
6. Derive `poseTimeOffset` from the sensor timestamp only after its clock relationship to the host monotonic clock is measured. Until then, use zero rather than an unvalidated offset.
7. Submit from the sensor cadence or a bounded pose-publisher loop using `TrackedDevicePoseUpdated`; never pass through Euler angles.

Add deterministic tests for identity, +90-degree rotations around all three axes, quaternion normalization, gyro-axis mapping, timestamp wrap/rollback, and stale-pose invalidation.

**Exit criterion:** SteamVR's mirror view moves in the same direction and approximate magnitude as the head, remains valid under normal motion, and invalidates within a defined timeout when samples stop.

### Gate 5: Add recentering and failure handling

Represent recenter as an orientation offset:

```text
q_output = inverse(q_at_recenter) * q_current
```

Apply it in quaternion space. Provide these control paths in order:

1. An OpenVR `DebugRequest` command for automated testing.
2. A small CLI communicating over a Unix-domain control socket owned by the driver.
3. An optional desktop hotkey helper outside `vrserver`.

Do not capture global keyboard input inside the SteamVR driver. The tracker must reconnect with bounded exponential backoff, reset Fusion after a discontinuity, repeat bias calibration, and expose disconnected/calibrating/running states in logs.

**Exit criterion:** recenter works at arbitrary roll and pitch without gimbal lock, and unplug/replug restores tracking without restarting SteamVR.

### Gate 6: Calibrate display and timing

Start with conservative, source-backed defaults:

- Total display: 3840x1080.
- Per-eye viewport and initial render target: 1920x1080.
- Refresh: 60 Hz first, then validate 90 Hz SBS.
- XREAL One nominal FOV: 50 degrees as reported by XRLinuxDriver.
- Distortion: identity for the first visible image.
- IPD: SteamVR user setting, with a documented default only if the runtime provides none.

Then calibrate:

1. Confirm eye ordering and correct any left/right swap.
2. Use a grid and known-angle head rotations to tune projection tangents. Do not use a single scalar FOV if measurements show asymmetric per-eye bounds.
3. Determine whether the glasses already present an optically corrected image. Add a distortion model only if measured geometry requires it.
4. Measure photon/display delay separately from IMU processing delay before setting `Prop_SecondsFromVsyncToPhotons_Float`.
5. Test 60 and 90 Hz for frame pacing, judder, dropped frames, and thermal stability.

**Exit criterion:** text and grid geometry are comfortable across the useful field of view, scale is credible, and a 30-minute seated session has stable frame pacing.

### Gate 7: Application compatibility

Test in this order to isolate failure domains:

1. SteamVR dashboard and compositor test scene.
2. A native Linux OpenVR sample.
3. A simple Proton OpenVR application.
4. One target seated game with a normal gamepad.
5. SteamVR restart, application restart, display reconnect, and IMU reconnect cases.

For every run, capture the application version, Proton version, SteamVR build, GPU driver, frame rate, reprojection ratio, and relevant logs. A Proton game failure does not by itself indicate an HMD driver failure if native OpenVR and the dashboard remain healthy.

**Exit criterion:** at least one native sample and one Proton title complete a 30-minute session with working 3DoF tracking and stereo display.

### Gate 8: Package and document

- Provide reproducible release builds and a tarball containing only the required driver tree and licenses.
- Make install/uninstall scripts locate SteamVR and invoke `vrpathreg adddriver`/`removedriver` idempotently.
- Document Ethernet enablement, stabilizer/anchor disablement, firmware expectations, SBS selection, NetworkManager/link-local setup, direct-mode GPU selection, and log collection.
- Add `scripts/collect-diagnostics.sh` that gathers versions, EDID decode, DRM connector state, network route, and SteamVR logs without private account data.
- Document recovery from SteamVR safe mode without silently editing unrelated settings.

**Exit criterion:** a clean supported Linux installation can build, install, run, diagnose, and uninstall the driver by following the README.

## Configuration contract

The initial `driver_xrealone` settings should be narrow and explicit:

```json
{
  "driver_xrealone": {
    "enable": true,
    "direct_mode": true,
    "imu_address": "169.254.2.1:52998",
    "display_width": 3840,
    "display_height": 1080,
    "display_frequency": 60.0,
    "nominal_fov_degrees": 50.0,
    "stale_pose_timeout_ms": 100,
    "reconnect_initial_ms": 250,
    "reconnect_max_ms": 5000
  }
}
```

EDID IDs should come from the measured XREAL One panel and be compiled as supported-device data. Developer overrides may be added only if multiple real EDID variants are observed.

## Principal risks and mitigations

| Risk | Impact | Mitigation |
| --- | --- | --- |
| SteamVR cannot lease the XREAL connector | No headset image | Make direct-display acquisition Gate 1; verify EDID, owning GPU, Vulkan ICD, desktop connector state, and DRM permissions before other implementation. |
| SteamVR updates change Linux driver behavior | Previously working build regresses | Record build IDs, retain a minimal display smoke test, and test stable plus beta only as explicit matrix entries. |
| XREAL firmware changes TCP packets | Tracking stops or corrupts | Isolate transport behind `ImuTransport`, validate every sample, and pin/track upstream parser revisions. |
| Wrong coordinate transform | Motion is inverted or cross-axis | Use quaternion/gyro unit tests and a scripted physical motion checklist before subjective tuning. |
| Yaw drift | View slowly rotates | Expose drift truthfully, keep bias calibration, provide low-friction quaternion recenter, and measure drift per hour. |
| Fusion or TCP work blocks `vrserver` shutdown | SteamVR hangs | Use a dedicated thread, finite socket timeouts, cancellation, and lifecycle stress tests. |
| Multi-GPU mismatch | Direct display acquisition fails | Ensure SteamVR uses the Vulkan ICD for the GPU physically driving the USB-C/DP connector. |
| Old Air driver assumptions leak into One support | Incorrect refresh, FOV, lifecycle, or display flags | Treat the Air driver as a reference and implement current OpenVR contracts in a small new code base. |

## Definition of done

The first release is complete when all of the following are true:

- The XREAL connector is acquired by SteamVR in direct mode on the recorded reference system.
- SteamVR recognizes one HMD without controllers or positional tracking.
- Both eyes receive a correct 1920x1080 view inside the 3840x1080 SBS mode.
- Orientation and angular velocity are submitted in the correct OpenVR coordinate system.
- Tracking can recenter, invalidate stale data, reconnect after unplug, and shut down cleanly.
- A native OpenVR sample and one Proton seated title each run for at least 30 minutes.
- Build, install, diagnostics, recovery, and uninstall procedures are documented and reproducible.

## Deferred decisions

The following are intentionally deferred until the MVP gates pass:

- Reusing XRLinuxDriver as a daemon versus keeping an embedded tracker. Start embedded; reconsider only when another consumer requires shared tracking.
- Supporting One Pro and 1S. Add each from captured EDID, USB identity, FOV, axis, and refresh evidence rather than assuming One values.
- Automatic SBS switching. XRLinuxDriver currently treats One's non-HID transport separately and does not provide the same MCU mode switching path used by Air devices.
- A graphical calibration UI.
- Packaging for multiple distributions or Steam distribution.
- Non-SteamVR runtimes and an OpenXR-native driver.
