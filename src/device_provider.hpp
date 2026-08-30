// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

#include <openvr_driver.h>

#include "hmd_device.hpp"

namespace vrealone {

class DeviceProvider final : public vr::IServerTrackedDeviceProvider {
 public:
  vr::EVRInitError Init(vr::IVRDriverContext* driver_context) override;
  void Cleanup() override;
  const char* const* GetInterfaceVersions() override;
  void RunFrame() override;
  bool ShouldBlockStandbyMode() override;
  void EnterStandby() override;
  void LeaveStandby() override;

 private:
  std::unique_ptr<HmdDevice> hmd_;
};

}  // namespace vrealone
