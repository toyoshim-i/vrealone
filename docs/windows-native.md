# Native Windows display validation

## Recorded environment

- Windows 11 Home 64-bit, build 26200.
- AMD Radeon 780M Graphics, driver 32.0.31036.15.
- SteamVR 2.16.7 x64.
- XREAL One exposed as `DISPLAY\\MRG4101` at 1920x1080 and 60 Hz.
- XREAL One was the only active physical monitor.
- OpenTrack HMD emulation was disabled before the final tests.

The Windows x64 driver loaded successfully, registered
`xrealone.XREALONE-UNPROBED` as the active HMD, connected to the IMU endpoint,
and reached the tracking `running` state. This confirms driver loading and
tracking independently of display acquisition.

## Extended-desktop behavior

The default Windows setting, `direct_mode=false`, caused SteamVR to create its
desktop `Headset Window`. The window showed a solid red stereo image during
most tests. The SteamVR dashboard occasionally appeared, but this did not
indicate direct-display acquisition. `vrcompositor.txt` consistently reported:

```text
Headset display is on desktop
```

A single-monitor layout is not a useful extended-desktop configuration: the
XREAL panel simultaneously owns the Windows desktop and the compositor window.
A future Windows extended-mode implementation should enumerate the XREAL
monitor and return its actual desktop coordinates from `GetWindowBounds`.

## Direct-mode experiment

The driver and SteamVR user settings were both changed to `direct_mode=true`.
Temporary diagnostics confirmed all of the values expected by the OpenVR
display contract:

```text
direct_mode=true on_desktop=false real_display=true
Prop_IsOnDesktop_Bool=false error=0
IsDisplayOnDesktop -> false
IsDisplayRealDisplay -> true
```

SteamVR also registered the measured EDID values (`MRG`, product `0x4101`) and
reported that its AMD display-visibility request succeeded:

```text
[Display] Enabling direct mode for AMD ...
[Display] Successfully set display visibility.
```

Despite those results, every compositor start still reported `Headset display
is on desktop`, created the desktop Headset Window, and offered to restart in
Direct Mode. Accepting the restart repeated the same sequence. Windows
continued to enumerate the XREAL panel as an active Generic PnP Monitor.

This test does not establish whether the remaining failure is in SteamVR's GPU
direct-display path, the AMD driver, or an additional undocumented HMD
requirement. It does establish that changing only `IsDisplayOnDesktop`,
`IsDisplayRealDisplay`, `Prop_IsOnDesktop_Bool`, and the EDID properties is not
sufficient on this host. Do not describe native Windows direct mode as
supported until the compositor reports `Headset is using driver direct mode`
and the desktop Headset Window is absent.

## Current recommendation

Keep `direct_mode=false` as the Windows default. Use a separate desktop display
when testing the extended-mode fallback. Treat Windows direct mode as an open
display-integration task; implementing `IVRDriverDirectModeComponent` would be
a separate design decision because it requires the driver to own texture
handoff and presentation.
