// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include <openvr_driver.h>

#include "display_component.hpp"
#include "tracking/pose_snapshot.hpp"
#include "tracking/recenter.hpp"

namespace vrealone {

class HmdDevice final : public vr::ITrackedDeviceServerDriver {
 public:
  explicit HmdDevice(DisplayConfig display_config);

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

  DisplayConfig display_config_;
  DisplayComponent display_component_;
  tracking::PoseStore pose_store_;
  tracking::Recenter recenter_;
  std::string serial_number_ = "XREALONE-UNPROBED";
  std::atomic<std::uint32_t> device_index_{vr::k_unTrackedDeviceIndexInvalid};
};

}  // namespace vrealone
