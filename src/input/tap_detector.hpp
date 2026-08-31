// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace vrealone::input {

enum class TapAction { none, click, recenter };

struct TapDetectorConfig {
  std::int64_t audio_peak_threshold = 80'000'000;
  std::chrono::milliseconds imu_correlation_window{200};
  std::chrono::milliseconds double_tap_window{650};
  std::chrono::milliseconds tap_refractory_period{180};
  std::chrono::milliseconds post_double_tap_delay{250};
};

class TapDetector {
 public:
  explicit TapDetector(TapDetectorConfig config = {});

  void ObserveAudioWindow(std::chrono::steady_clock::time_point now,
                          std::int64_t peak,
                          std::chrono::steady_clock::time_point last_imu_impact);
  [[nodiscard]] TapAction Poll(std::chrono::steady_clock::time_point now);
  void Reset();

 private:
  void ObserveConfirmedTap(std::chrono::steady_clock::time_point now);

  TapDetectorConfig config_;
  std::optional<std::chrono::steady_clock::time_point>
      last_confirmed_imu_impact_;
  std::optional<std::chrono::steady_clock::time_point> last_tap_;
  std::optional<std::chrono::steady_clock::time_point> first_tap_;
  std::optional<std::chrono::steady_clock::time_point> recenter_due_;
};

class ImuImpactDetector {
 public:
  explicit ImuImpactDetector(float trigger_deviation_m_s2 = 3.0F,
                             std::chrono::milliseconds quiet_period =
                                 std::chrono::milliseconds(50));
  [[nodiscard]] bool Observe(std::chrono::steady_clock::time_point now,
                             float acceleration_deviation_m_s2);
  void Reset();

 private:
  float trigger_deviation_m_s2_;
  std::chrono::milliseconds quiet_period_;
  bool armed_ = true;
  std::optional<std::chrono::steady_clock::time_point> quiet_since_;
};

}  // namespace vrealone::input
