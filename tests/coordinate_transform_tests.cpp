// SPDX-License-Identifier: Apache-2.0
#include <array>

#include "test_support.hpp"
#include "tracking/coordinate_transform.hpp"

void RunCoordinateTransformTests() {
  using vrealone::tracking::ParserOrientationToOpenVr;
  using vrealone::tracking::ParserVectorToOpenVr;
  using vrealone::tracking::Quaternion;
  constexpr double kSqrtHalf = 0.70710678118654752440;

  const auto pitch_up = ParserVectorToOpenVr({-1.0, 0.0, 0.0});
  CheckNear(pitch_up[0], 1.0, 1e-12, "pitch up maps to OpenVR +X");
  const auto yaw_left = ParserVectorToOpenVr({0.0, 0.0, 1.0});
  CheckNear(yaw_left[1], 1.0, 1e-12, "yaw left maps to OpenVR +Y");
  const auto roll_right = ParserVectorToOpenVr({0.0, -1.0, 0.0});
  CheckNear(roll_right[2], -1.0, 1e-12,
            "right-ear roll maps to OpenVR -Z");

  const auto pitch_orientation =
      ParserOrientationToOpenVr({kSqrtHalf, -kSqrtHalf, 0.0, 0.0});
  CheckNear(pitch_orientation.x, kSqrtHalf, 1e-12,
            "pitch quaternion maps to OpenVR +X");
  const auto yaw_orientation =
      ParserOrientationToOpenVr({kSqrtHalf, 0.0, 0.0, kSqrtHalf});
  CheckNear(yaw_orientation.y, kSqrtHalf, 1e-12,
            "yaw quaternion maps to OpenVR +Y");
  const auto roll_orientation =
      ParserOrientationToOpenVr({kSqrtHalf, 0.0, -kSqrtHalf, 0.0});
  CheckNear(roll_orientation.z, -kSqrtHalf, 1e-12,
            "roll quaternion maps to OpenVR -Z");

  const auto rotated = Quaternion{kSqrtHalf, 0.0, kSqrtHalf, 0.0}.Rotate(
      std::array<double, 3>{0.0, 0.0, -1.0});
  CheckNear(rotated[0], -1.0, 1e-12,
            "positive OpenVR yaw rotates forward toward the left");
  CheckNear(rotated[1], 0.0, 1e-12, "yaw keeps forward vector level");
  CheckNear(rotated[2], 0.0, 1e-12, "yaw rotates the forward Z component");
}
