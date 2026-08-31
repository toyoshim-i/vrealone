// SPDX-License-Identifier: Apache-2.0
#include "tracking/pose_snapshot.hpp"

namespace vrealone::tracking {

bool IsPoseFresh(const PoseSnapshot& snapshot,
                 const std::chrono::steady_clock::time_point now,
                 const std::chrono::steady_clock::duration maximum_age) {
  return snapshot.received_at != std::chrono::steady_clock::time_point{} &&
         now >= snapshot.received_at &&
         now - snapshot.received_at <= maximum_age;
}

void PoseStore::Publish(const PoseSnapshot& snapshot) {
  std::scoped_lock lock(mutex_);
  snapshot_ = snapshot;
}

PoseSnapshot PoseStore::Read() const {
  std::scoped_lock lock(mutex_);
  return snapshot_;
}

}  // namespace vrealone::tracking
