// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <optional>

#include <Fusion.h>

#include "tracking/pose_snapshot.hpp"

namespace vrealone::tracking {

class FusionTracker {
 public:
  explicit FusionTracker(float nominal_sample_rate_hz = 1000.0F);

  // The parser-facing contract is explicit: gyroscope input is radians per
  // second, acceleration is metres per second squared, and timestamp is us.
  [[nodiscard]] PoseSnapshot Update(const float gyro_rad_s[3],
                                    const float accel_m_s2[3],
                                    std::uint64_t timestamp_us);
  void Reset();

 private:
  float nominal_sample_rate_hz_;
  FusionAhrs ahrs_{};
  FusionBias bias_{};
  std::optional<std::uint64_t> previous_timestamp_us_;
};

}  // namespace vrealone::tracking
