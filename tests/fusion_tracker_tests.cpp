// SPDX-License-Identifier: Apache-2.0
#include <cmath>

#include "test_support.hpp"
#include "tracking/fusion_tracker.hpp"

void RunFusionTrackerTests() {
  vrealone::tracking::FusionTracker tracker(1000.0F);
  const float stationary_gyro[] = {0.0F, 0.0F, 0.0F};
  const float gravity_up[] = {0.0F, 0.0F, 9.80665F};
  vrealone::tracking::PoseSnapshot pose;
  for (std::uint64_t index = 1; index <= 2000; ++index) {
    pose = tracker.Update(stationary_gyro, gravity_up, index * 1000);
  }
  CheckNear(pose.orientation.Norm(), 1.0, 1e-5,
            "Fusion output quaternion must remain normalized");
  Check(pose.state == vrealone::tracking::TrackingState::running,
        "Fusion update must produce a running snapshot");
  Check(std::isfinite(pose.orientation.w) &&
            std::isfinite(pose.orientation.x) &&
            std::isfinite(pose.orientation.y) &&
            std::isfinite(pose.orientation.z),
        "Fusion output must not contain NaNs");
}
