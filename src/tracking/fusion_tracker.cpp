// SPDX-License-Identifier: Apache-2.0
#include "tracking/fusion_tracker.hpp"

#include <chrono>
#include <numbers>

namespace vrealone::tracking {
namespace {

constexpr float kRadiansToDegrees =
    180.0F / std::numbers::pi_v<float>;

}  // namespace

FusionTracker::FusionTracker(const float nominal_sample_rate_hz)
    : nominal_sample_rate_hz_(nominal_sample_rate_hz) {
  Reset();
}

PoseSnapshot FusionTracker::Update(const float gyro_rad_s[3],
                                   const float accel_m_s2[3],
                                   const std::uint64_t timestamp_us) {
  auto sample_period = 1.0F / nominal_sample_rate_hz_;
  if (previous_timestamp_us_ && timestamp_us > *previous_timestamp_us_) {
    const auto measured =
        static_cast<float>(timestamp_us - *previous_timestamp_us_) / 1'000'000.0F;
    if (measured >= 0.0001F && measured <= 0.1F) {
      sample_period = measured;
    }
  }
  previous_timestamp_us_ = timestamp_us;
  FusionAhrsSetSamplePeriod(&ahrs_, sample_period);

  FusionVector gyro_degrees{{gyro_rad_s[0] * kRadiansToDegrees,
                             gyro_rad_s[1] * kRadiansToDegrees,
                             gyro_rad_s[2] * kRadiansToDegrees}};
  const FusionVector acceleration{
      {accel_m_s2[0], accel_m_s2[1], accel_m_s2[2]}};
  gyro_degrees = FusionBiasUpdate(&bias_, gyro_degrees);
  FusionAhrsUpdateNoMagnetometer(&ahrs_, gyro_degrees, acceleration);
  const auto quaternion = FusionAhrsGetQuaternion(&ahrs_);

  const Quaternion orientation{quaternion.element.w, quaternion.element.x,
                               quaternion.element.y, quaternion.element.z};
  return {
      .orientation = orientation.Normalized(),
      .angular_velocity_rad_s = {gyro_rad_s[0], gyro_rad_s[1], gyro_rad_s[2]},
      .received_at = std::chrono::steady_clock::now(),
      .state = TrackingState::running,
  };
}

void FusionTracker::Reset() {
  FusionAhrsInitialise(&ahrs_);
  auto ahrs_settings = fusionAhrsDefaultSettings;
  ahrs_settings.sampleRate = nominal_sample_rate_hz_;
  ahrs_settings.convention = FusionConventionNwu;
  FusionAhrsSetSettings(&ahrs_, &ahrs_settings);

  FusionBiasInitialise(&bias_);
  auto bias_settings = fusionBiasDefaultSettings;
  bias_settings.sampleRate = nominal_sample_rate_hz_;
  FusionBiasSetSettings(&bias_, &bias_settings);
  previous_timestamp_us_.reset();
}

}  // namespace vrealone::tracking
