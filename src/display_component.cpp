// SPDX-License-Identifier: Apache-2.0
#include "display_component.hpp"

#include <stdexcept>

namespace vrealone {

DisplayComponent::DisplayComponent(DisplayConfig config) : config_(config) {
  if (!config_.IsValid()) {
    throw std::invalid_argument("invalid display configuration");
  }
}

void DisplayComponent::GetWindowBounds(int32_t* x, int32_t* y,
                                       std::uint32_t* width,
                                       std::uint32_t* height) {
  *x = 0;
  *y = 0;
  *width = config_.width;
  *height = config_.height;
}

bool DisplayComponent::IsDisplayOnDesktop() { return !config_.direct_mode; }

bool DisplayComponent::IsDisplayRealDisplay() { return config_.direct_mode; }

void DisplayComponent::GetRecommendedRenderTargetSize(std::uint32_t* width,
                                                       std::uint32_t* height) {
  *width = config_.render_width;
  *height = config_.render_height;
}

void DisplayComponent::GetEyeOutputViewport(vr::EVREye eye, std::uint32_t* x,
                                             std::uint32_t* y,
                                             std::uint32_t* width,
                                             std::uint32_t* height) {
  const auto viewport =
      config_.EyeViewport(eye == vr::Eye_Left ? Eye::left : Eye::right);
  *x = viewport.x;
  *y = viewport.y;
  *width = viewport.width;
  *height = viewport.height;
}

void DisplayComponent::GetProjectionRaw(vr::EVREye, float* left, float* right,
                                         float* top, float* bottom) {
  const auto projection = config_.EyeProjection();
  *left = projection.left;
  *right = projection.right;
  *top = projection.top;
  *bottom = projection.bottom;
}

vr::DistortionCoordinates_t DisplayComponent::ComputeDistortion(vr::EVREye,
                                                                 float u,
                                                                 float v) {
  vr::DistortionCoordinates_t result{};
  result.rfRed[0] = result.rfGreen[0] = result.rfBlue[0] = u;
  result.rfRed[1] = result.rfGreen[1] = result.rfBlue[1] = v;
  return result;
}

bool DisplayComponent::ComputeInverseDistortion(vr::HmdVector2_t* result,
                                                 vr::EVREye, std::uint32_t,
                                                 float u, float v) {
  result->v[0] = u;
  result->v[1] = v;
  return true;
}

}  // namespace vrealone
