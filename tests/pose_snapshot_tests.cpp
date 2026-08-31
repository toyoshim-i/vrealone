// SPDX-License-Identifier: Apache-2.0
#include <chrono>

#include "test_support.hpp"
#include "tracking/pose_snapshot.hpp"

void RunPoseSnapshotTests() {
  using namespace std::chrono_literals;
  using vrealone::tracking::IsPoseFresh;
  using vrealone::tracking::PoseSnapshot;

  const auto now = std::chrono::steady_clock::time_point{1s};
  Check(!IsPoseFresh(PoseSnapshot{}, now, 100ms),
        "a snapshot without a receive time must be stale");

  PoseSnapshot snapshot{.received_at = now - 100ms};
  Check(IsPoseFresh(snapshot, now, 100ms),
        "a snapshot at the age limit must be fresh");
  Check(!IsPoseFresh(snapshot, now + 1ms, 100ms),
        "a snapshot beyond the age limit must be stale");
  Check(!IsPoseFresh(snapshot, now - 200ms, 100ms),
        "a receive time in the future must be rejected");
}
