// SPDX-License-Identifier: Apache-2.0
#include "tracking/pose_snapshot.hpp"

namespace vrealone::tracking {

void PoseStore::Publish(const PoseSnapshot& snapshot) {
  std::scoped_lock lock(mutex_);
  snapshot_ = snapshot;
}

PoseSnapshot PoseStore::Read() const {
  std::scoped_lock lock(mutex_);
  return snapshot_;
}

}  // namespace vrealone::tracking
