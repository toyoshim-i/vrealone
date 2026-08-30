// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <openvr_driver.h>

#include "display_config.hpp"

namespace vrealone {

class DisplayComponent final : public vr::IVRDisplayComponent {
 public:
  explicit DisplayComponent(DisplayConfig config);

  void GetWindowBounds(int32_t* x, int32_t* y, std::uint32_t* width,
                       std::uint32_t* height) override;
  bool IsDisplayOnDesktop() override;
  bool IsDisplayRealDisplay() override;
  void GetRecommendedRenderTargetSize(std::uint32_t* width,
                                      std::uint32_t* height) override;
  void GetEyeOutputViewport(vr::EVREye eye, std::uint32_t* x,
                            std::uint32_t* y, std::uint32_t* width,
                            std::uint32_t* height) override;
  void GetProjectionRaw(vr::EVREye eye, float* left, float* right, float* top,
                        float* bottom) override;
  vr::DistortionCoordinates_t ComputeDistortion(vr::EVREye eye, float u,
                                                 float v) override;
  bool ComputeInverseDistortion(vr::HmdVector2_t* result, vr::EVREye eye,
                                std::uint32_t channel, float u,
                                float v) override;

 private:
  DisplayConfig config_;
};

}  // namespace vrealone
