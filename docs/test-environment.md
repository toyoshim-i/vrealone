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
effectively stationary; a controlled rotation is still required to prove
whether the parser values are radians or degrees per second and to verify axis
signs.

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
Their log files had not yet been created; record the exact runtime version and
first-load results after the initial SteamVR launch.

## Current blockers for Gate 1

1. Verify the physical AMD Vulkan ICD outside the restricted build shell.
2. Launch SteamVR and verify that the registered driver loads without safe mode.
3. Prove that `vrcompositor` leases `card1-DP-2` at 1920x1080@60 half SBS.
4. Repeat the display gate on hardware capable of 3840x1080 full SBS before a
   release claims native per-eye transport resolution.
