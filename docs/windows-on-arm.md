# Windows on Arm and Parallels

## Supported build shape

SteamVR 2.16.7 on the recorded Windows 11 on Arm VM ships x64 `vrserver.exe`
and `vrcompositor.exe` binaries (PE machine `0x8664`). Build this driver as x64
so SteamVR can load it under Windows x64 emulation:

```powershell
cmake -S . -B build-win64 -G "Visual Studio 17 2022" -A x64
cmake --build build-win64 --config Release
ctest --test-dir build-win64 -C Release --output-on-failure
```

The package is written to `build-win64/xrealone`, including
`bin/win64/driver_xrealone.dll`. Windows defaults to `direct_mode=false`
because a VM exposes its virtual display rather than the XREAL panel EDID.

## Recorded environment

- Windows 11 on Arm in Parallels on an Apple silicon MacBook Air.
- SteamVR 2.16.7 x64.
- Parallels Display Adapter (WDDM), driver 20.18.1264.23937.
- XREAL One USB VID/PID `3318:0438` passed through to the VM.
- `UsbNcm Host Device` at `169.254.2.10/24`.
- XREAL IMU endpoint `169.254.2.1:52998` reachable over TCP.

The Windows x64 sensor probe received 500 samples in 0.50 seconds with no
timestamp rollback. SteamVR loaded the driver, registered the HMD, connected
the IMU, reached Fusion's running state, and generated head-gaze dwell clicks.

The Parallels compositor test stopped at:

```text
VRInitError_Compositor_CreateSharedFrameInfoConstantBuffer
```

`vrcompositor.txt` identified the Parallels WDDM adapter and reported that it
could not create the shared-frame-info constant buffer. Driver loading and
tracking therefore work in this VM, while rendered SteamVR output is currently
blocked by the virtual GPU/compositor path.

When started from Steam, this failure appears as a small "starting" dialog
that never completes: `vrcompositor.exe` exits, while `vrserver.exe`,
`vrmonitor.exe`, and the web helpers remain alive. This is not an IMU or driver
deadlock. Exit the remaining SteamVR processes before another test. Changing
the OpenXR active-runtime registration does not change this Direct3D failure.

## Registration and diagnostics

```powershell
$steamVr = "${env:ProgramFiles(x86)}\Steam\steamapps\common\SteamVR"
& "$steamVr\bin\win64\vrpathreg.exe" adddriver `
  "$PWD\build-win64\xrealone"
& "$steamVr\bin\win64\vrpathreg.exe" show

.\build-win64\Release\xrealone_probe.exe --sensor 169.254.2.1:52998 500
.\build-win64\Release\xrealone_probe.exe --tracking 169.254.2.1:52998 10000
```

### Legacy DirectX and Visual C++ prerequisites

The recorded clean Windows VM did not initially contain `d3dx10_43.dll` or
`CONCRT140.dll`. These are optional side-by-side runtime components rather than
the DirectX version built into Windows. Steam normally installs common
redistributables on first launch from its client, but direct `vrstartup.exe`
launches used during driver development may bypass that step. Install the
Microsoft redistributables cached by Steam if SteamVR reports the missing DLL
or driver load error 126:

```powershell
$redist = "${env:ProgramFiles(x86)}\Steam\steamapps\common\Steamworks Shared\_CommonRedist"
& "$redist\DirectX\Jun2010\DXSETUP.exe" /silent
& "$redist\vcredist\2022\VC_redist.x64.exe" /install /quiet /norestart
& "$redist\vcredist\2022\VC_redist.x86.exe" /install /quiet /norestart
```

Use the Microsoft installers and never copy a loose DLL from a third-party
download site. After installation, the recorded versions were
`d3dx10_43.dll` 9.29.952.3111 and `CONCRT140.dll` 14.51.36247.0.

### OpenXR runtime

Select SteamVR as the OpenXR runtime in SteamVR's OpenXR settings, then verify
the standard 64-bit Windows registration:

```powershell
reg query "HKLM\SOFTWARE\Khronos\OpenXR\1" /v ActiveRuntime
```

For the recorded installation, `ActiveRuntime` points to:

```text
C:\Program Files (x86)\Steam\steamapps\common\SteamVR\steamxr_win64.json
```

The manifest loads `bin\vrclient_x64.dll`, matching the x64 SteamVR runtime and
the x64 driver used on Windows on Arm.

The control CLI uses a Windows named pipe:

```powershell
.\build-win64\Release\xrealone_ctl.exe status
.\build-win64\Release\xrealone_ctl.exe click
.\build-win64\Release\xrealone_ctl.exe recenter
```
