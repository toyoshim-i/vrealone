// SPDX-License-Identifier: Apache-2.0
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <xreal_one_driver.h>

#include "input/audio_tap_input.hpp"

int main(const int argc, char** argv) {
  const int duration_seconds = argc >= 2 ? std::atoi(argv[1]) : 15;
  const std::string address =
      argc >= 3 ? argv[2] : "169.254.2.1:52998";
  const std::string audio_device = argc >= 4 ? argv[3] : "pipewire";
  if (duration_seconds < 5 || duration_seconds > 120) {
    std::cerr << "usage: " << argv[0]
              << " [duration-seconds: 5-120] [sensor-address] "
                 "[ALSA-device]\n";
    return 2;
  }

  using Handle = std::unique_ptr<XrealOneHandle, decltype(&xo_free)>;
  Handle sensor(xo_new_with_addr(address.c_str()), &xo_free);
  if (!sensor) {
    std::cerr << "Unable to connect to XREAL IMU at " << address << ".\n";
    return 1;
  }

  std::atomic<std::int64_t> last_imu_impact_ns{0};
  std::atomic<int> click_count{0};
  std::atomic<int> recenter_count{0};
  int imu_impact_count = 0;
  float maximum_acceleration_deviation = 0.0F;
  vrealone::input::ImuImpactDetector impact_detector;
  vrealone::input::AudioTapInputConfig audio_config;
  audio_config.device = audio_device;
  vrealone::input::AudioTapInput tap_input(audio_config);
  if (!tap_input.Start(
          [&](const vrealone::input::TapAction action) {
            if (action == vrealone::input::TapAction::click) {
              ++click_count;
              std::cout << "action=click\n" << std::flush;
            } else if (action == vrealone::input::TapAction::recenter) {
              ++recenter_count;
              std::cout << "action=recenter\n" << std::flush;
            }
          },
          [&] {
            return std::chrono::steady_clock::time_point(
                std::chrono::nanoseconds(last_imu_impact_ns.load()));
          },
          [](const std::int64_t peak, const std::int64_t imu_age_ms) {
            std::cout << "audio_peak=" << peak
                      << " imu_age_ms=" << imu_age_ms << '\n'
                      << std::flush;
          })) {
    std::cerr << "Unable to open ALSA capture device " << audio_device
              << ".\n";
    return 1;
  }

  std::cout << "Tap probe ready. Wait one second, tap once, wait one second, "
               "then tap twice 200-650 ms apart.\n";
  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::seconds(duration_seconds);
  while (std::chrono::steady_clock::now() < deadline) {
    XOImu sample{};
    if (xo_next(sensor.get(), &sample) != 0) {
      std::cerr << "XREAL IMU disconnected.\n";
      tap_input.Stop();
      return 1;
    }
    if (!std::isfinite(sample.accel[0]) ||
        !std::isfinite(sample.accel[1]) ||
        !std::isfinite(sample.accel[2])) {
      continue;
    }
    const auto magnitude = std::sqrt(sample.accel[0] * sample.accel[0] +
                                     sample.accel[1] * sample.accel[1] +
                                     sample.accel[2] * sample.accel[2]);
    constexpr float kStandardGravity = 9.80665F;
    const auto deviation = std::abs(magnitude - kStandardGravity);
    maximum_acceleration_deviation =
        std::max(maximum_acceleration_deviation, deviation);
    if (impact_detector.Observe(std::chrono::steady_clock::now(), deviation)) {
      ++imu_impact_count;
      last_imu_impact_ns.store(
          std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::steady_clock::now().time_since_epoch())
              .count());
    }
  }
  tap_input.Stop();
  std::cout << "clicks=" << click_count.load()
            << " recenters=" << recenter_count.load()
            << " imu_impacts=" << imu_impact_count
            << " max_accel_deviation=" << maximum_acceleration_deviation
            << '\n';
  return click_count.load() >= 1 && recenter_count.load() >= 1 ? 0 : 1;
}
