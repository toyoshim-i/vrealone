// SPDX-License-Identifier: Apache-2.0
#include "display_config.hpp"
#include "test_support.hpp"

void RunDisplayConfigTests() {
  const vrealone::DisplayConfig config;
  Check(config.IsValid(), "default display configuration must be valid");

  const auto left = config.EyeViewport(vrealone::Eye::left);
  const auto right = config.EyeViewport(vrealone::Eye::right);
  Check(left.x == 0 && left.width == 960 && left.height == 1080,
        "left half-SBS viewport must be 960x1080 at x=0");
  Check(right.x == 960 && right.width == 960 && right.height == 1080,
        "right half-SBS viewport must be 960x1080 at x=960");

  const auto projection = config.EyeProjection();
  CheckNear(projection.left, -projection.right, 1e-6,
            "horizontal projection must be symmetric");
  CheckNear(projection.top, -projection.bottom, 1e-6,
            "vertical projection must be symmetric");
  Check(projection.left < 0 && projection.right > 0 && projection.top < 0 &&
            projection.bottom > 0,
        "projection tangent signs must match OpenVR");

  auto full_sbs = config;
  full_sbs.width = 3840;
  const auto full_left = full_sbs.EyeViewport(vrealone::Eye::left);
  Check(full_left.width == 1920,
        "full-SBS viewport must retain native per-eye width");
  const auto full_projection = full_sbs.EyeProjection();
  CheckNear(full_projection.top, projection.top, 1e-6,
            "half and full SBS must use the same decoded optical projection");

  auto invalid = config;
  invalid.width = 3839;
  Check(!invalid.IsValid(), "odd SBS width must be rejected");
  invalid.width = 0xfffffff0U;
  Check(!invalid.IsValid(), "unreasonable display dimensions must be rejected");
}
