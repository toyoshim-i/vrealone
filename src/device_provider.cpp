// SPDX-License-Identifier: Apache-2.0
#include "device_provider.hpp"

#include <cstdint>

namespace vrealone {
namespace {

constexpr const char* kSettingsSection = "driver_xrealone";

DisplayConfig LoadDisplayConfig() {
  DisplayConfig config;
  vr::EVRSettingsError error = vr::VRSettingsError_None;

  const auto get_int = [&error](const char* key, const std::int32_t fallback) {
    const auto value =
        vr::VRSettings()->GetInt32(kSettingsSection, key, &error);
    if (error != vr::VRSettingsError_None) {
      error = vr::VRSettingsError_None;
      return fallback;
    }
    return value;
  };
  const auto get_float = [&error](const char* key, const float fallback) {
    const auto value = vr::VRSettings()->GetFloat(kSettingsSection, key, &error);
    if (error != vr::VRSettingsError_None) {
      error = vr::VRSettingsError_None;
      return fallback;
    }
    return value;
  };
  const auto get_bool = [&error](const char* key, const bool fallback) {
    const auto value = vr::VRSettings()->GetBool(kSettingsSection, key, &error);
    if (error != vr::VRSettingsError_None) {
      error = vr::VRSettingsError_None;
      return fallback;
    }
    return value;
  };

  config.width = static_cast<std::uint32_t>(
      get_int("display_width", static_cast<std::int32_t>(config.width)));
  config.height = static_cast<std::uint32_t>(
      get_int("display_height", static_cast<std::int32_t>(config.height)));
  config.render_width = static_cast<std::uint32_t>(
      get_int("render_width", static_cast<std::int32_t>(config.render_width)));
  config.render_height = static_cast<std::uint32_t>(get_int(
      "render_height", static_cast<std::int32_t>(config.render_height)));
  config.frequency_hz = get_float("display_frequency", config.frequency_hz);
  config.horizontal_fov_degrees =
      get_float("nominal_fov_degrees", config.horizontal_fov_degrees);
  config.direct_mode = get_bool("direct_mode", config.direct_mode);
  config.edid_vendor_id = static_cast<std::uint32_t>(get_int("edid_vendor_id", 0));
  config.edid_product_id =
      static_cast<std::uint32_t>(get_int("edid_product_id", 0));
  return config;
}

}  // namespace

vr::EVRInitError DeviceProvider::Init(vr::IVRDriverContext* driver_context) {
  VR_INIT_SERVER_DRIVER_CONTEXT(driver_context);
  const auto config = LoadDisplayConfig();
  if (!config.IsValid()) {
    vr::VRDriverLog()->Log("xrealone: invalid display configuration");
    return vr::VRInitError_Driver_Failed;
  }
  if (config.direct_mode &&
      (config.edid_vendor_id == 0 || config.edid_product_id == 0)) {
    vr::VRDriverLog()->Log(
        "xrealone: direct mode enabled without measured EDID identifiers");
  }

  hmd_ = std::make_unique<HmdDevice>(config);
  if (!vr::VRServerDriverHost()->TrackedDeviceAdded(
          hmd_->SerialNumber().c_str(), vr::TrackedDeviceClass_HMD, hmd_.get())) {
    hmd_.reset();
    return vr::VRInitError_Driver_Failed;
  }
  vr::VRDriverLog()->Log("xrealone: HMD registered");
  return vr::VRInitError_None;
}

void DeviceProvider::Cleanup() {
  hmd_.reset();
  VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

const char* const* DeviceProvider::GetInterfaceVersions() {
  return vr::k_InterfaceVersions;
}

void DeviceProvider::RunFrame() {
  if (hmd_) {
    hmd_->RunFrame();
  }
}

bool DeviceProvider::ShouldBlockStandbyMode() { return false; }
void DeviceProvider::EnterStandby() {}
void DeviceProvider::LeaveStandby() {}

}  // namespace vrealone
