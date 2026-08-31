// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <chrono>
#include <mutex>

#include "tracking/quaternion.hpp"

namespace vrealone::tracking {

enum class TrackingState { disconnected, calibrating, running };

struct PoseSnapshot {
  Quaternion orientation{};
  double angular_velocity_rad_s[3]{};
  std::chrono::steady_clock::time_point received_at{};
  TrackingState state = TrackingState::disconnected;
};

[[nodiscard]] bool IsPoseFresh(
    const PoseSnapshot& snapshot,
    std::chrono::steady_clock::time_point now,
    std::chrono::steady_clock::duration maximum_age);

class PoseStore {
 public:
  void Publish(const PoseSnapshot& snapshot);
  [[nodiscard]] PoseSnapshot Read() const;

 private:
  mutable std::mutex mutex_;
  PoseSnapshot snapshot_{};
};

}  // namespace vrealone::tracking
