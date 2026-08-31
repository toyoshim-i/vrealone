// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace vrealone {

enum class Eye { left, right };

struct Rect {
  std::uint32_t x;
  std::uint32_t y;
  std::uint32_t width;
  std::uint32_t height;
};

struct Projection {
  float left;
  float right;
  float top;
  float bottom;
};

struct DisplayConfig {
  // Encoded display width. The development default is half SBS: two 960-wide
  // viewports in a 1920-wide mode, expanded horizontally by the glasses.
  std::uint32_t width = 1920;
  std::uint32_t height = 1080;
  std::uint32_t render_width = 1920;
  std::uint32_t render_height = 1080;
  float frequency_hz = 60.0F;
  float horizontal_fov_degrees = 50.0F;
  bool direct_mode = true;
  std::uint32_t edid_vendor_id = 0x3647;
  std::uint32_t edid_product_id = 0x4101;

  [[nodiscard]] bool IsValid() const;
  [[nodiscard]] Rect EyeViewport(Eye eye) const;
  [[nodiscard]] Projection EyeProjection() const;
};

}  // namespace vrealone
