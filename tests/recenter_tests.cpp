// SPDX-License-Identifier: Apache-2.0
#include "test_support.hpp"
#include "tracking/recenter.hpp"

void RunRecenterTests() {
  using vrealone::tracking::Quaternion;
  using vrealone::tracking::Recenter;
  constexpr double kSqrtHalf = 0.70710678118654752440;

  Recenter recenter;
  const Quaternion arbitrary_roll_pitch{kSqrtHalf, 0.5, 0.5, 0.0};
  recenter.SetOrigin(arbitrary_roll_pitch);
  const auto centered = recenter.Apply(arbitrary_roll_pitch);
  CheckNear(centered.w, 1.0, 1e-12, "recentered orientation w");
  CheckNear(centered.x, 0.0, 1e-12, "recentered orientation x");
  CheckNear(centered.y, 0.0, 1e-12, "recentered orientation y");
  CheckNear(centered.z, 0.0, 1e-12, "recentered orientation z");

  const Quaternion yaw90{kSqrtHalf, 0.0, kSqrtHalf, 0.0};
  recenter.SetOrigin(yaw90);
  const auto centered_vector =
      recenter.ApplyVector({-1.0, 0.0, 0.0});
  CheckNear(centered_vector[0], 0.0, 1e-12,
            "recentered vector x");
  CheckNear(centered_vector[1], 0.0, 1e-12,
            "recentered vector y");
  CheckNear(centered_vector[2], -1.0, 1e-12,
            "recentered vector z");
}
