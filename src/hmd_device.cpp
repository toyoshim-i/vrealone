// SPDX-License-Identifier: Apache-2.0
#include "hmd_device.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>

#if VREALONE_HAS_SENSOR
#include <xreal_one_driver.h>
#endif

#include "tracking/coordinate_transform.hpp"

namespace vrealone {

HmdDevice::HmdDevice(DisplayConfig display_config, std::string imu_address,
                     const std::chrono::milliseconds stale_pose_timeout,
                     const std::chrono::milliseconds reconnect_initial,
                     const std::chrono::milliseconds reconnect_max,
                     input::TapControlConfig tap_control_config)
    : display_config_(display_config),
      display_component_(display_config),
      imu_address_(std::move(imu_address)),
      stale_pose_timeout_(stale_pose_timeout),
      reconnect_initial_(reconnect_initial),
      reconnect_max_(reconnect_max),
      tap_control_config_(std::move(tap_control_config)),
      audio_tap_input_(tap_control_config_.audio) {}

HmdDevice::~HmdDevice() {
  control_server_.Stop();
  StopTapInput();
  StopSensor();
}

vr::EVRInitError HmdDevice::Activate(const std::uint32_t object_id) {
  device_index_.store(object_id);
  const auto container =
      vr::VRProperties()->TrackedDeviceToPropertyContainer(object_id);
  vr::VRProperties()->SetStringProperty(container, vr::Prop_ManufacturerName_String,
                                        "XREAL");
  vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String,
                                        "XREAL One");
  vr::VRProperties()->SetStringProperty(container,
                                        vr::Prop_SerialNumber_String,
                                        serial_number_.c_str());
  vr::VRProperties()->SetFloatProperty(container, vr::Prop_DisplayFrequency_Float,
                                       display_config_.frequency_hz);
  vr::VRProperties()->SetFloatProperty(
      container, vr::Prop_SecondsFromVsyncToPhotons_Float, 0.0F);
  vr::VRProperties()->SetBoolProperty(container, vr::Prop_IsOnDesktop_Bool,
                                      !display_config_.direct_mode);
  vr::VRProperties()->SetBoolProperty(container, vr::Prop_WillDriftInYaw_Bool,
                                      true);
  vr::VRProperties()->SetStringProperty(
      container, vr::Prop_InputProfilePath_String,
      "{xrealone}/input/xrealone_hmd_profile.json");
  vr::VRProperties()->SetStringProperty(container,
                                        vr::Prop_ControllerType_String,
                                        "xrealone_hmd");
  if (display_config_.edid_vendor_id != 0) {
    vr::VRProperties()->SetInt32Property(
        container, vr::Prop_EdidVendorID_Int32,
        static_cast<std::int32_t>(display_config_.edid_vendor_id));
  }
  if (display_config_.edid_product_id != 0) {
    vr::VRProperties()->SetInt32Property(
        container, vr::Prop_EdidProductID_Int32,
        static_cast<std::int32_t>(display_config_.edid_product_id));
  }
  if (vr::VRDriverInput()->CreateBooleanComponent(
          container, "/input/select/click", &select_click_handle_) !=
      vr::VRInputError_None) {
    vr::VRDriverLog()->Log("xrealone: failed to create gaze select input");
    select_click_handle_ = vr::k_ulInvalidInputComponentHandle;
  }
  if (!control_server_.Start(
          control::DefaultSocketPath(),
          [this](const control::Command command) {
            return HandleControlCommand(command);
          })) {
    vr::VRDriverLog()->Log("xrealone: failed to start control socket");
  } else {
    vr::VRDriverLog()->Log("xrealone: control socket ready");
  }
  StartSensor();
  StartTapInput();
  return vr::VRInitError_None;
}

void HmdDevice::Deactivate() {
  control_server_.Stop();
  StopTapInput();
  StopSensor();
  select_click_handle_ = vr::k_ulInvalidInputComponentHandle;
  device_index_.store(vr::k_unTrackedDeviceIndexInvalid);
}

void HmdDevice::EnterStandby() {}

void* HmdDevice::GetComponent(const char* name_and_version) {
  if (name_and_version != nullptr &&
      std::strcmp(name_and_version, vr::IVRDisplayComponent_Version) == 0) {
    return &display_component_;
  }
  return nullptr;
}

void HmdDevice::DebugRequest(const char* request, char* response,
                             const std::uint32_t response_size) {
  if (request != nullptr && std::strcmp(request, "recenter") == 0) {
    recenter_requested_.store(true);
    WriteResponse(response, response_size, "queued");
    return;
  }
  if (request != nullptr && std::strcmp(request, "status") == 0) {
    const auto state = pose_store_.Read().state;
    const char* status = "disconnected";
    if (state == tracking::TrackingState::calibrating) {
      status = "calibrating";
    } else if (state == tracking::TrackingState::running) {
      status = "running";
    }
    WriteResponse(response, response_size, status);
    return;
  }
  WriteResponse(response, response_size, "unknown request");
}

vr::DriverPose_t HmdDevice::GetPose() {
  const auto snapshot = pose_store_.Read();
  const auto openvr_orientation =
      tracking::ParserOrientationToOpenVr(snapshot.orientation);
  const auto orientation = recenter_.Apply(openvr_orientation);
  const auto body_angular_velocity = tracking::ParserVectorToOpenVr(
      {snapshot.angular_velocity_rad_s[0],
       snapshot.angular_velocity_rad_s[1],
       snapshot.angular_velocity_rad_s[2]});
  const auto world_angular_velocity = recenter_.ApplyVector(
      openvr_orientation.Rotate(body_angular_velocity));
  vr::DriverPose_t pose{};
  pose.qWorldFromDriverRotation.w = 1.0;
  pose.qDriverFromHeadRotation.w = 1.0;
  pose.qRotation = {orientation.w, orientation.x, orientation.y,
                    orientation.z};
  pose.vecAngularVelocity[0] = world_angular_velocity[0];
  pose.vecAngularVelocity[1] = world_angular_velocity[1];
  pose.vecAngularVelocity[2] = world_angular_velocity[2];

#if VREALONE_HAS_SENSOR
  const bool fresh = tracking::IsPoseFresh(
      snapshot, std::chrono::steady_clock::now(), stale_pose_timeout_);
  pose.deviceIsConnected = snapshot.state != tracking::TrackingState::disconnected;
  pose.poseIsValid = snapshot.state == tracking::TrackingState::running && fresh;
  if (snapshot.state == tracking::TrackingState::calibrating) {
    pose.result = vr::TrackingResult_Calibrating_InProgress;
  } else if (snapshot.state == tracking::TrackingState::running && fresh) {
    pose.result = vr::TrackingResult_Running_OK;
  } else if (snapshot.state == tracking::TrackingState::running) {
    pose.result = vr::TrackingResult_Running_OutOfRange;
  } else {
    pose.result = vr::TrackingResult_Uninitialized;
  }
#else
  pose.poseIsValid = true;
  pose.deviceIsConnected = true;
  pose.result = vr::TrackingResult_Running_OK;
#endif
  pose.willDriftInYaw = true;
  pose.shouldApplyHeadModel = false;
  return pose;
}

void HmdDevice::RunFrame() {
  const auto index = device_index_.load();
  if (index != vr::k_unTrackedDeviceIndexInvalid) {
    if (recenter_requested_.exchange(false)) {
      recenter_.SetOrigin(tracking::ParserOrientationToOpenVr(
          pose_store_.Read().orientation));
      vr::VRDriverLog()->Log("xrealone: orientation recentered");
    }
    if (select_click_handle_ != vr::k_ulInvalidInputComponentHandle) {
      const auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count();
      const bool pressed = now_ns < select_click_until_ns_.load();
      vr::VRDriverInput()->UpdateBooleanComponent(select_click_handle_, pressed,
                                                   0.0);
    }
    vr::VRServerDriverHost()->TrackedDevicePoseUpdated(index, GetPose(),
                                                       sizeof(vr::DriverPose_t));
  }
}

const std::string& HmdDevice::SerialNumber() const { return serial_number_; }

std::string HmdDevice::HandleControlCommand(const control::Command command) {
  if (command == control::Command::recenter) {
    recenter_requested_.store(true);
    return "queued recenter";
  }
  if (command == control::Command::click) {
    QueueSelectClick();
    return "queued click";
  }
  if (command == control::Command::status) {
    const auto state = pose_store_.Read().state;
    if (state == tracking::TrackingState::running) {
      return "running";
    }
    if (state == tracking::TrackingState::calibrating) {
      return "calibrating";
    }
    return "disconnected";
  }
  return "unknown command";
}

void HmdDevice::StartTapInput() {
  if (!tap_control_config_.enabled) {
    return;
  }
  const bool started = audio_tap_input_.Start(
      [this](const input::TapAction action) {
        if (action == input::TapAction::click) {
          QueueSelectClick();
        } else if (action == input::TapAction::recenter) {
          recenter_requested_.store(true);
        }
      },
      [this] {
        return std::chrono::steady_clock::time_point(
            std::chrono::nanoseconds(last_imu_impact_ns_.load()));
      });
  vr::VRDriverLog()->Log(started ? "xrealone: tap input ready"
                                 : "xrealone: tap input unavailable");
}

void HmdDevice::StopTapInput() { audio_tap_input_.Stop(); }

void HmdDevice::QueueSelectClick() {
  constexpr auto kClickDuration = std::chrono::milliseconds(150);
  const auto until = std::chrono::steady_clock::now() + kClickDuration;
  select_click_until_ns_.store(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          until.time_since_epoch())
          .count());
}

#if VREALONE_HAS_SENSOR
void HmdDevice::StartSensor() {
  StopSensor();
  pose_store_.Publish({.state = tracking::TrackingState::disconnected});
  sensor_thread_ =
      std::jthread([this](const std::stop_token token) { SensorLoop(token); });
}

void HmdDevice::StopSensor() {
  if (!sensor_thread_.joinable()) {
    return;
  }
  sensor_thread_.request_stop();
  reconnect_condition_.notify_all();
  sensor_thread_.join();
  pose_store_.Publish({.state = tracking::TrackingState::disconnected});
}

void HmdDevice::SensorLoop(const std::stop_token stop_token) {
  auto reconnect_delay = reconnect_initial_;
  input::ImuImpactDetector impact_detector(
      tap_control_config_.imu_acceleration_deviation_m_s2);
  while (!stop_token.stop_requested()) {
    using Handle = std::unique_ptr<XrealOneHandle, decltype(&xo_free)>;
    Handle handle(xo_new_with_addr(imu_address_.c_str()), &xo_free);
    if (!handle) {
      pose_store_.Publish({.state = tracking::TrackingState::disconnected});
      vr::VRDriverLog()->Log("xrealone: IMU connection failed; retrying");
      if (WaitForReconnect(stop_token, reconnect_delay)) {
        break;
      }
      reconnect_delay = std::min(reconnect_delay * 2, reconnect_max_);
      continue;
    }

    fusion_tracker_.Reset();
    impact_detector.Reset();
    pose_store_.Publish({.received_at = std::chrono::steady_clock::now(),
                         .state = tracking::TrackingState::calibrating});
    vr::VRDriverLog()->Log("xrealone: IMU connected; calibrating");
    bool reported_running = false;
    while (!stop_token.stop_requested()) {
      XOImu sample{};
      if (xo_next(handle.get(), &sample) != 0) {
        break;
      }
      const bool finite =
          std::isfinite(sample.gyro[0]) && std::isfinite(sample.gyro[1]) &&
          std::isfinite(sample.gyro[2]) && std::isfinite(sample.accel[0]) &&
          std::isfinite(sample.accel[1]) && std::isfinite(sample.accel[2]);
      if (!finite) {
        continue;
      }
      const auto acceleration_magnitude = std::sqrt(
          sample.accel[0] * sample.accel[0] +
          sample.accel[1] * sample.accel[1] +
          sample.accel[2] * sample.accel[2]);
      constexpr float kStandardGravity = 9.80665F;
      if (impact_detector.Observe(
              std::chrono::steady_clock::now(),
              std::abs(acceleration_magnitude - kStandardGravity))) {
        last_imu_impact_ns_.store(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count());
      }
      reconnect_delay = reconnect_initial_;
      auto snapshot =
          fusion_tracker_.Update(sample.gyro, sample.accel, sample.timestamp);
      if (snapshot.state == tracking::TrackingState::running &&
          !reported_running) {
        vr::VRDriverLog()->Log("xrealone: IMU tracking running");
        reported_running = true;
      }
      pose_store_.Publish(snapshot);
    }
    pose_store_.Publish({.state = tracking::TrackingState::disconnected});
    if (!stop_token.stop_requested()) {
      vr::VRDriverLog()->Log("xrealone: IMU disconnected; retrying");
    }
    if (!stop_token.stop_requested() &&
        WaitForReconnect(stop_token, reconnect_delay)) {
      break;
    }
    reconnect_delay = std::min(reconnect_delay * 2, reconnect_max_);
  }
}

bool HmdDevice::WaitForReconnect(const std::stop_token stop_token,
                                 const std::chrono::milliseconds duration) {
  std::unique_lock lock(reconnect_mutex_);
  reconnect_condition_.wait_for(lock, stop_token, duration,
                                [] { return false; });
  return stop_token.stop_requested();
}
#else
void HmdDevice::StartSensor() {}
void HmdDevice::StopSensor() {}
#endif

void HmdDevice::WriteResponse(char* response, const std::uint32_t response_size,
                              const char* text) const {
  if (response == nullptr || response_size == 0) {
    return;
  }
  const auto length = std::min<std::size_t>(std::strlen(text), response_size - 1);
  std::memcpy(response, text, length);
  response[length] = '\0';
}

}  // namespace vrealone
