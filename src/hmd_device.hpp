// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <openvr_driver.h>

#include "control/control_server.hpp"
#include "display_component.hpp"
#include "input/audio_tap_input.hpp"
#include "tracking/pose_snapshot.hpp"
#include "tracking/recenter.hpp"

#if VREALONE_HAS_SENSOR
#include "tracking/fusion_tracker.hpp"
#endif

namespace vrealone {

class HmdDevice final : public vr::ITrackedDeviceServerDriver {
 public:
  HmdDevice(DisplayConfig display_config, std::string imu_address,
            std::chrono::milliseconds stale_pose_timeout,
            std::chrono::milliseconds reconnect_initial,
            std::chrono::milliseconds reconnect_max,
            input::TapControlConfig tap_control_config);
  ~HmdDevice();

  vr::EVRInitError Activate(std::uint32_t object_id) override;
  void Deactivate() override;
  void EnterStandby() override;
  void* GetComponent(const char* name_and_version) override;
  void DebugRequest(const char* request, char* response,
                    std::uint32_t response_size) override;
  vr::DriverPose_t GetPose() override;

  void RunFrame();
  [[nodiscard]] const std::string& SerialNumber() const;

 private:
  void WriteResponse(char* response, std::uint32_t response_size,
                     const char* text) const;
  void StartSensor();
  void StopSensor();
  void StartTapInput();
  void StopTapInput();
  void QueueSelectClick();
  [[nodiscard]] std::string HandleControlCommand(control::Command command);
#if VREALONE_HAS_SENSOR
  void SensorLoop(std::stop_token stop_token);
  bool WaitForReconnect(std::stop_token stop_token,
                        std::chrono::milliseconds duration);
#endif

  DisplayConfig display_config_;
  DisplayComponent display_component_;
  tracking::PoseStore pose_store_;
  tracking::Recenter recenter_;
  std::string imu_address_;
  std::chrono::milliseconds stale_pose_timeout_;
  std::chrono::milliseconds reconnect_initial_;
  std::chrono::milliseconds reconnect_max_;
  input::TapControlConfig tap_control_config_;
  std::string serial_number_ = "XREALONE-UNPROBED";
  std::atomic<std::uint32_t> device_index_{vr::k_unTrackedDeviceIndexInvalid};
  control::ControlServer control_server_;
  std::atomic<bool> recenter_requested_{false};
  std::atomic<std::int64_t> select_click_until_ns_{0};
  std::atomic<std::int64_t> last_imu_impact_ns_{0};
  vr::VRInputComponentHandle_t select_click_handle_ =
      vr::k_ulInvalidInputComponentHandle;
  input::AudioTapInput audio_tap_input_;
#if VREALONE_HAS_SENSOR
  tracking::FusionTracker fusion_tracker_;
  std::jthread sensor_thread_;
  std::mutex reconnect_mutex_;
  std::condition_variable_any reconnect_condition_;
#endif
};

}  // namespace vrealone
