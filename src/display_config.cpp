// SPDX-License-Identifier: Apache-2.0
#include "display_config.hpp"

#include <cmath>

namespace vrealone {

bool DisplayConfig::IsValid() const {
  constexpr std::uint32_t kMaximumDimension = 16384;
  return width >= 2 && width <= kMaximumDimension && width % 2 == 0 &&
         height > 0 && height <= kMaximumDimension && render_width > 0 &&
         render_width <= kMaximumDimension && render_height > 0 &&
         render_height <= kMaximumDimension && frequency_hz > 0.0F &&
         frequency_hz <= 1000.0F &&
         horizontal_fov_degrees > 0.0F && horizontal_fov_degrees < 180.0F;
}

Rect DisplayConfig::EyeViewport(const Eye eye) const {
  const auto eye_width = width / 2;
  return {.x = eye == Eye::left ? 0U : eye_width,
          .y = 0,
          .width = eye_width,
          .height = height};
}

Projection DisplayConfig::EyeProjection() const {
  constexpr float kPi = 3.14159265358979323846F;
  const auto horizontal_tangent =
      std::tan(horizontal_fov_degrees * kPi / 360.0F);
  // Projection describes the decoded optical image, not the encoded output
  // viewport. This distinction is essential for 960x1080 half SBS output that
  // the glasses expand to a 1920x1080 image for each eye.
  const auto eye_aspect = static_cast<float>(render_width) /
                          static_cast<float>(render_height);
  const auto vertical_tangent = horizontal_tangent / eye_aspect;
  return {.left = -horizontal_tangent,
          .right = horizontal_tangent,
          .top = -vertical_tangent,
          .bottom = vertical_tangent};
}

}  // namespace vrealone
