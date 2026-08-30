// SPDX-License-Identifier: Apache-2.0
#include <cmath>

#include "test_support.hpp"
#include "tracking/quaternion.hpp"

void RunQuaternionTests() {
  using vrealone::tracking::Quaternion;
  constexpr double kSqrtHalf = 0.70710678118654752440;
  const Quaternion yaw90{kSqrtHalf, 0, kSqrtHalf, 0};
  const auto identity = yaw90.Inverse() * yaw90;
  CheckNear(identity.w, 1.0, 1e-12, "inverse product w");
  CheckNear(identity.x, 0.0, 1e-12, "inverse product x");
  CheckNear(identity.y, 0.0, 1e-12, "inverse product y");
  CheckNear(identity.z, 0.0, 1e-12, "inverse product z");

  const Quaternion scaled{2, 0, 0, 0};
  CheckNear(scaled.Normalized().w, 1.0, 1e-12,
            "normalization must produce a unit quaternion");
}
