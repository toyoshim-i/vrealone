// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <chrono>

#include "tracking/quaternion.hpp"

namespace vrealone::input {

struct GazeDwellConfig {
  bool enabled = true;
  std::chrono::milliseconds dwell_time{1200};
  float max_angle_deviation_deg = 1.5F;
  std::chrono::milliseconds cooldown_time{1500};
};

class GazeDwellDetector {
 public:
  using TimePoint = std::chrono::steady_clock::time_point;

  explicit GazeDwellDetector(GazeDwellConfig config = {});

  void Reset();

  /// Observes a new orientation at the given timestamp.
  /// Returns true if a dwell click should be triggered.
  bool Observe(TimePoint now, const tracking::Quaternion& orientation);

  [[nodiscard]] const GazeDwellConfig& Config() const { return config_; }
  void SetConfig(const GazeDwellConfig& config) {
    config_ = config;
    Reset();
  }

 private:
  GazeDwellConfig config_;
  tracking::Quaternion anchor_orientation_{1.0, 0.0, 0.0, 0.0};
  TimePoint dwell_start_time_{};
  TimePoint last_click_time_{};
  bool has_anchor_ = false;
  bool in_cooldown_ = false;
};

/// Calculates the angular distance in degrees between two quaternions.
[[nodiscard]] double AngularDistanceDegrees(
    const tracking::Quaternion& q1, const tracking::Quaternion& q2);

}  // namespace vrealone::input
