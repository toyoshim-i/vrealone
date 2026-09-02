// SPDX-License-Identifier: Apache-2.0
#include "device_provider.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <string>

#include "platform/edid.hpp"

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
  config.edid_vendor_id = static_cast<std::uint32_t>(
      get_int("edid_vendor_id",
              static_cast<std::int32_t>(config.edid_vendor_id)));
  config.edid_product_id = static_cast<std::uint32_t>(
      get_int("edid_product_id",
              static_cast<std::int32_t>(config.edid_product_id)));
  return config;
}

struct SensorConfig {
  std::string address = "169.254.2.1:52998";
  std::chrono::milliseconds stale_pose_timeout{100};
  std::chrono::milliseconds reconnect_initial{250};
  std::chrono::milliseconds reconnect_max{5000};
};

SensorConfig LoadSensorConfig() {
  SensorConfig config;
  vr::EVRSettingsError error = vr::VRSettingsError_None;
  std::array<char, 256> address{};
  vr::VRSettings()->GetString(kSettingsSection, "imu_address", address.data(),
                              static_cast<std::uint32_t>(address.size()),
                              &error);
  config.address = address.data();
  if (error != vr::VRSettingsError_None || config.address.empty()) {
    config.address = "169.254.2.1:52998";
    error = vr::VRSettingsError_None;
  }
  const auto get_duration = [&error](const char* key,
                                     const std::chrono::milliseconds fallback) {
    const auto value = vr::VRSettings()->GetInt32(kSettingsSection, key, &error);
    if (error != vr::VRSettingsError_None || value <= 0) {
      error = vr::VRSettingsError_None;
      return fallback;
    }
    return std::chrono::milliseconds(value);
  };
  config.stale_pose_timeout =
      get_duration("stale_pose_timeout_ms", config.stale_pose_timeout);
  config.reconnect_initial =
      get_duration("reconnect_initial_ms", config.reconnect_initial);
  config.reconnect_max =
      get_duration("reconnect_max_ms", config.reconnect_max);
  if (config.reconnect_max < config.reconnect_initial) {
    config.reconnect_max = config.reconnect_initial;
  }
  return config;
}

input::GazeDwellConfig LoadGazeDwellConfig() {
  input::GazeDwellConfig config;
  vr::EVRSettingsError error = vr::VRSettingsError_None;
  config.enabled =
      vr::VRSettings()->GetBool(kSettingsSection, "gaze_dwell_enabled", &error);
  if (error != vr::VRSettingsError_None) {
    config.enabled = true;
    error = vr::VRSettingsError_None;
  }
  const auto dwell_time_ms = vr::VRSettings()->GetInt32(
      kSettingsSection, "gaze_dwell_time_ms", &error);
  if (error == vr::VRSettingsError_None && dwell_time_ms > 0) {
    config.dwell_time = std::chrono::milliseconds(dwell_time_ms);
  }
  error = vr::VRSettingsError_None;
  const auto angle_deg = vr::VRSettings()->GetFloat(
      kSettingsSection, "gaze_dwell_max_angle_deg", &error);
  if (error == vr::VRSettingsError_None && angle_deg > 0.0F) {
    config.max_angle_deviation_deg = angle_deg;
  }
  error = vr::VRSettingsError_None;
  const auto cooldown_ms = vr::VRSettings()->GetInt32(
      kSettingsSection, "gaze_dwell_cooldown_ms", &error);
  if (error == vr::VRSettingsError_None && cooldown_ms > 0) {
    config.cooldown_time = std::chrono::milliseconds(cooldown_ms);
  }
  return config;
}

double LoadStandingHeight() {
  vr::EVRSettingsError error = vr::VRSettingsError_None;
  const auto height = vr::VRSettings()->GetFloat(
      kSettingsSection, "standing_height_meters", &error);
  if (error == vr::VRSettingsError_None && height > 0.0F) {
    return static_cast<double>(height);
  }
  return 1.5;
}

bool AllowAmbiguousDisplaySelection() {
  vr::EVRSettingsError error = vr::VRSettingsError_None;
  const bool value = vr::VRSettings()->GetBool(
      kSettingsSection, "allow_ambiguous_display_selection", &error);
  return error == vr::VRSettingsError_None && value;
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
  if (config.direct_mode) {
#if !defined(_WIN32)
    const auto assessment = platform::AssessDirectDisplay(
        platform::ProbeDrmDisplays(),
        static_cast<std::uint16_t>(config.edid_vendor_id),
        static_cast<std::uint16_t>(config.edid_product_id), config.width,
        config.height);
    if (!assessment.target_connected || !assessment.target_has_mode) {
      vr::VRDriverLog()->Log(
          "xrealone: refusing direct mode because the configured XREAL "
          "display and mode are not connected");
      return vr::VRInitError_Driver_Failed;
    }
    if (!assessment.conflicting_connectors.empty() &&
        !AllowAmbiguousDisplaySelection()) {
      vr::VRDriverLog()->Log(
          "xrealone: refusing direct mode because another DRM connector "
          "advertises the requested display size");
      for (const auto& connector : assessment.conflicting_connectors) {
        const auto message = std::string("xrealone: conflicting connector: ") +
                             connector.filename().string();
        vr::VRDriverLog()->Log(message.c_str());
      }
      return vr::VRInitError_Driver_Failed;
    }
#endif
  }

  const auto sensor = LoadSensorConfig();
  const auto gaze_dwell = LoadGazeDwellConfig();
  const auto standing_height = LoadStandingHeight();
  hmd_ = std::make_unique<HmdDevice>(
      config, sensor.address, sensor.stale_pose_timeout,
      sensor.reconnect_initial, sensor.reconnect_max, gaze_dwell,
      standing_height);
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
