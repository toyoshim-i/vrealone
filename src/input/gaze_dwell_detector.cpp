// SPDX-License-Identifier: Apache-2.0
#include "input/gaze_dwell_detector.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace vrealone::input {

double AngularDistanceDegrees(const tracking::Quaternion& q1,
                              const tracking::Quaternion& q2) {
  const double dot = std::abs(q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z);
  const double clamped_dot = std::clamp(dot, 0.0, 1.0);
  const double angle_rad = 2.0 * std::acos(clamped_dot);
  return angle_rad * (180.0 / std::numbers::pi);
}

GazeDwellDetector::GazeDwellDetector(GazeDwellConfig config)
    : config_(std::move(config)) {}

void GazeDwellDetector::Reset() {
  has_anchor_ = false;
  in_cooldown_ = false;
  dwell_start_time_ = TimePoint{};
  last_click_time_ = TimePoint{};
}

bool GazeDwellDetector::Observe(const TimePoint now,
                               const tracking::Quaternion& orientation) {
  if (!config_.enabled) {
    return false;
  }

  tracking::Quaternion normalized;
  try {
    normalized = orientation.Normalized();
  } catch (...) {
    return false;
  }

  if (!has_anchor_) {
    anchor_orientation_ = normalized;
    dwell_start_time_ = now;
    has_anchor_ = true;
    return false;
  }

  const double angle = AngularDistanceDegrees(anchor_orientation_, normalized);

  if (in_cooldown_) {
    const bool moved_away = angle > (config_.max_angle_deviation_deg * 2.0F);
    const bool cooldown_expired = (now - last_click_time_) >= config_.cooldown_time;
    if (moved_away || cooldown_expired) {
      in_cooldown_ = false;
      anchor_orientation_ = normalized;
      dwell_start_time_ = now;
    }
    return false;
  }

  if (angle <= config_.max_angle_deviation_deg) {
    if ((now - dwell_start_time_) >= config_.dwell_time) {
      in_cooldown_ = true;
      last_click_time_ = now;
      return true;
    }
  } else {
    anchor_orientation_ = normalized;
    dwell_start_time_ = now;
  }

  return false;
}

}  // namespace vrealone::input
