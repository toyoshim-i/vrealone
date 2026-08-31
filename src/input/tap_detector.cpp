// SPDX-License-Identifier: Apache-2.0
#include "input/tap_detector.hpp"

namespace vrealone::input {

TapDetector::TapDetector(TapDetectorConfig config) : config_(config) {}

void TapDetector::ObserveAudioWindow(
    const std::chrono::steady_clock::time_point now, const std::int64_t peak,
    const std::chrono::steady_clock::time_point last_imu_impact) {
  if (peak < config_.audio_peak_threshold ||
      last_imu_impact == std::chrono::steady_clock::time_point{}) {
    return;
  }
  const auto separation = now >= last_imu_impact ? now - last_imu_impact
                                                 : last_imu_impact - now;
  if (separation <= config_.imu_correlation_window &&
      (!last_confirmed_imu_impact_ ||
       *last_confirmed_imu_impact_ != last_imu_impact)) {
    last_confirmed_imu_impact_ = last_imu_impact;
    ObserveConfirmedTap(now);
  }
}

TapAction TapDetector::Poll(const std::chrono::steady_clock::time_point now) {
  if (recenter_due_ && now >= *recenter_due_) {
    recenter_due_.reset();
    return TapAction::recenter;
  }
  if (first_tap_ && now >= *first_tap_ + config_.double_tap_window) {
    first_tap_.reset();
    return TapAction::click;
  }
  return TapAction::none;
}

void TapDetector::Reset() {
  last_confirmed_imu_impact_.reset();
  last_tap_.reset();
  first_tap_.reset();
  recenter_due_.reset();
}

void TapDetector::ObserveConfirmedTap(
    const std::chrono::steady_clock::time_point now) {
  if (last_tap_ && now - *last_tap_ < config_.tap_refractory_period) {
    return;
  }
  last_tap_ = now;
  if (first_tap_ && now - *first_tap_ <= config_.double_tap_window) {
    first_tap_.reset();
    recenter_due_ = now + config_.post_double_tap_delay;
    return;
  }
  first_tap_ = now;
}

ImuImpactDetector::ImuImpactDetector(
    const float trigger_deviation_m_s2,
    const std::chrono::milliseconds quiet_period)
    : trigger_deviation_m_s2_(trigger_deviation_m_s2),
      quiet_period_(quiet_period) {}

bool ImuImpactDetector::Observe(
    const std::chrono::steady_clock::time_point now,
    const float acceleration_deviation_m_s2) {
  if (acceleration_deviation_m_s2 <= trigger_deviation_m_s2_ / 2.0F) {
    if (!quiet_since_) {
      quiet_since_ = now;
    }
    if (now - *quiet_since_ >= quiet_period_) {
      armed_ = true;
    }
  } else {
    quiet_since_.reset();
  }
  if (armed_ && acceleration_deviation_m_s2 >= trigger_deviation_m_s2_) {
    armed_ = false;
    quiet_since_.reset();
    return true;
  }
  return false;
}

void ImuImpactDetector::Reset() {
  armed_ = true;
  quiet_since_.reset();
}

}  // namespace vrealone::input
