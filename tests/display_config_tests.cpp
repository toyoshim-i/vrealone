// SPDX-License-Identifier: Apache-2.0
#include "display_config.hpp"
#include "test_support.hpp"

void RunDisplayConfigTests() {
  const vrealone::DisplayConfig config;
  Check(config.IsValid(), "default display configuration must be valid");

  const auto left = config.EyeViewport(vrealone::Eye::left);
  const auto right = config.EyeViewport(vrealone::Eye::right);
  Check(left.x == 0 && left.width == 1920 && left.height == 1080,
        "left SBS viewport must be 1920x1080 at x=0");
  Check(right.x == 1920 && right.width == 1920 && right.height == 1080,
        "right SBS viewport must be 1920x1080 at x=1920");

  const auto projection = config.EyeProjection();
  CheckNear(projection.left, -projection.right, 1e-6,
            "horizontal projection must be symmetric");
  CheckNear(projection.top, -projection.bottom, 1e-6,
            "vertical projection must be symmetric");
  Check(projection.left < 0 && projection.right > 0 && projection.top < 0 &&
            projection.bottom > 0,
        "projection tangent signs must match OpenVR");

  auto invalid = config;
  invalid.width = 3839;
  Check(!invalid.IsValid(), "odd SBS width must be rejected");
  invalid.width = 0xfffffff0U;
  Check(!invalid.IsValid(), "unreasonable display dimensions must be rejected");
}
