// SPDX-License-Identifier: Apache-2.0
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numbers>
#include <string>

#include "platform/edid.hpp"
#include "tracking/coordinate_transform.hpp"
#include "tracking/recenter.hpp"

#if defined(VREALONE_HAS_SENSOR)
#include <xreal_one_driver.h>

#include "tracking/fusion_tracker.hpp"
#endif

namespace {

int ProbeDisplays() {
  const auto displays = vrealone::platform::ProbeDrmDisplays();
  if (displays.empty()) {
    std::cerr << "No DRM connectors were visible under /sys/class/drm.\n";
    return 1;
  }

  for (const auto& display : displays) {
    std::cout << display.connector.filename().string() << ": "
              << display.status;
    if (display.edid) {
      std::cout << ", vendor=" << display.edid->vendor_name << " (0x"
                << std::hex << std::setw(4) << std::setfill('0')
                << display.edid->vendor_id << "), product=0x" << std::setw(4)
                << display.edid->product_id << std::dec;
      if (!display.edid->product_name.empty()) {
        std::cout << " (\"" << display.edid->product_name << "\")";
      }
      if (vrealone::platform::IsXrealDisplay(*display.edid)) {
        std::cout << " [XREAL MATCH]";
      }
    } else {
      std::cout << ", no valid EDID";
    }
    std::cout << '\n';
  }
  return 0;
}

int FindXrealConnector() {
  const auto displays = vrealone::platform::ProbeDrmDisplays();
  for (const auto& display : displays) {
    if (display.status == "connected" && display.edid &&
        vrealone::platform::IsXrealDisplay(*display.edid)) {
      std::cout << display.connector.filename().string() << '\n';
      return 0;
    }
  }
  // If no connected matching display, check any matching display
  for (const auto& display : displays) {
    if (display.edid && vrealone::platform::IsXrealDisplay(*display.edid)) {
      std::cout << display.connector.filename().string() << '\n';
      return 0;
    }
  }
  return 1;
}

int CheckDirectDisplay() {
  constexpr std::uint16_t kXrealVendorId = 0x3647;
  constexpr std::uint16_t kXrealProductId = 0x4101;
  constexpr std::uint32_t kDisplayWidth = 1920;
  constexpr std::uint32_t kDisplayHeight = 1080;
  const auto assessment = vrealone::platform::AssessDirectDisplay(
      vrealone::platform::ProbeDrmDisplays(), kXrealVendorId,
      kXrealProductId, kDisplayWidth, kDisplayHeight);

  std::cout << "target_connected="
            << (assessment.target_connected ? "yes" : "no")
            << " target_mode="
            << (assessment.target_has_mode ? "yes" : "no") << '\n';
  for (const auto& connector : assessment.conflicting_connectors) {
    std::cout << "conflict=" << connector.filename().string() << '\n';
  }
  if (assessment.IsUnambiguous()) {
    std::cout << "SAFE: XREAL One is the only connected DRM display "
                 "advertising 1920x1080.\n";
    return 0;
  }
  std::cout << "BLOCKED: direct-display selection is missing or ambiguous.\n";
  return 1;
}

#if defined(VREALONE_HAS_SENSOR)
int ProbeSensor(const char* address, const long sample_count) {
  auto* handle = xo_new_with_addr(address);
  if (handle == nullptr) {
    std::cerr << "Unable to connect to " << address << ".\n";
    return 1;
  }

  std::uint64_t previous_timestamp = 0;
  long timestamp_rollbacks = 0;
  double maximum_gyro_magnitude = 0.0;
  std::array<double, 3> integrated_gyro{};
  std::array<double, 3> peak_absolute_gyro{};
  std::array<double, 3> acceleration_sum{};
  const auto started_at = std::chrono::steady_clock::now();
  for (long index = 0; index < sample_count; ++index) {
    XOImu sample{};
    if (xo_next(handle, &sample) != 0) {
      std::cerr << "Sensor read failed after " << index << " samples.\n";
      xo_free(handle);
      return 1;
    }
    if (index > 0 && sample.timestamp <= previous_timestamp) {
      ++timestamp_rollbacks;
    }
    if (index > 0 && sample.timestamp > previous_timestamp) {
      const auto delta_us = sample.timestamp - previous_timestamp;
      if (delta_us <= 100'000) {
        const auto delta_seconds = static_cast<double>(delta_us) / 1'000'000.0;
        for (std::size_t axis = 0; axis < integrated_gyro.size(); ++axis) {
          integrated_gyro[axis] += sample.gyro[axis] * delta_seconds;
        }
      }
    }
    previous_timestamp = sample.timestamp;
    for (std::size_t axis = 0; axis < peak_absolute_gyro.size(); ++axis) {
      peak_absolute_gyro[axis] =
          std::max(peak_absolute_gyro[axis],
                   std::abs(static_cast<double>(sample.gyro[axis])));
      acceleration_sum[axis] += sample.accel[axis];
    }
    const auto gyro_magnitude = std::sqrt(
        static_cast<double>(sample.gyro[0]) * sample.gyro[0] +
        static_cast<double>(sample.gyro[1]) * sample.gyro[1] +
        static_cast<double>(sample.gyro[2]) * sample.gyro[2]);
    maximum_gyro_magnitude = std::max(maximum_gyro_magnitude, gyro_magnitude);
    if (index == 0 || index + 1 == sample_count) {
      std::cout << "sample=" << index << " timestamp=" << sample.timestamp
                << " gyro=[" << sample.gyro[0] << ", " << sample.gyro[1]
                << ", " << sample.gyro[2] << "] accel=[" << sample.accel[0]
                << ", " << sample.accel[1] << ", " << sample.accel[2]
                << "]\n";
    }
  }
  const auto elapsed = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - started_at);
  xo_free(handle);
  std::cout << "received=" << sample_count << " elapsed_s=" << elapsed.count()
            << " rate_hz=" << static_cast<double>(sample_count) / elapsed.count()
            << " timestamp_rollbacks=" << timestamp_rollbacks
            << " max_gyro_magnitude=" << maximum_gyro_magnitude << '\n';
  std::cout << "integrated_gyro_units=[" << integrated_gyro[0] << ", "
            << integrated_gyro[1] << ", " << integrated_gyro[2]
            << "] peak_abs_gyro=[" << peak_absolute_gyro[0] << ", "
            << peak_absolute_gyro[1] << ", " << peak_absolute_gyro[2]
            << "] mean_accel=["
            << acceleration_sum[0] / static_cast<double>(sample_count) << ", "
            << acceleration_sum[1] / static_cast<double>(sample_count) << ", "
            << acceleration_sum[2] / static_cast<double>(sample_count) << "]\n";
  return timestamp_rollbacks == 0 ? 0 : 1;
}

std::array<double, 3> RotationVectorDegrees(
    vrealone::tracking::Quaternion quaternion) {
  quaternion = quaternion.Normalized();
  if (quaternion.w < 0.0) {
    quaternion = {-quaternion.w, -quaternion.x, -quaternion.y,
                  -quaternion.z};
  }
  const auto vector_norm = std::sqrt(
      quaternion.x * quaternion.x + quaternion.y * quaternion.y +
      quaternion.z * quaternion.z);
  if (vector_norm < 1e-12) {
    return {};
  }
  const auto angle_degrees =
      2.0 * std::atan2(vector_norm, quaternion.w) *
      180.0 / std::numbers::pi;
  return {quaternion.x / vector_norm * angle_degrees,
          quaternion.y / vector_norm * angle_degrees,
          quaternion.z / vector_norm * angle_degrees};
}

int ProbeTracking(const char* address, const long sample_count) {
  using Handle = std::unique_ptr<XrealOneHandle, decltype(&xo_free)>;
  Handle handle(xo_new_with_addr(address), &xo_free);
  if (!handle) {
    std::cerr << "Unable to connect to " << address << ".\n";
    return 1;
  }

  vrealone::tracking::FusionTracker tracker;
  vrealone::tracking::Recenter recenter;
  bool recentered = false;
  std::cout << "Keep the glasses still while tracking=calibrating. Move only "
               "after recentered=yes appears.\n";
  for (long index = 0; index < sample_count; ++index) {
    XOImu sample{};
    if (xo_next(handle.get(), &sample) != 0) {
      std::cerr << "Sensor read failed after " << index << " samples.\n";
      return 1;
    }
    const auto snapshot =
        tracker.Update(sample.gyro, sample.accel, sample.timestamp);
    const auto orientation = vrealone::tracking::ParserOrientationToOpenVr(
        snapshot.orientation);
    if (!recentered &&
        snapshot.state == vrealone::tracking::TrackingState::running) {
      recenter.SetOrigin(orientation);
      recentered = true;
    }
    if (index % 250 == 0 || index + 1 == sample_count) {
      const auto rotation =
          RotationVectorDegrees(recenter.Apply(orientation));
      std::cout << "sample=" << index << " tracking="
                << (snapshot.state ==
                            vrealone::tracking::TrackingState::running
                        ? "running"
                        : "calibrating")
                << " recentered=" << (recentered ? "yes" : "no")
                << " rotation_deg[pitch_x,yaw_y,roll_z]=[" << rotation[0]
                << ", " << rotation[1] << ", " << rotation[2] << "]\n";
    }
  }
  return recentered ? 0 : 1;
}
#endif

void PrintUsage(const char* program) {
  std::cerr << "Usage: " << program
            << " [--displays | --find-xreal | --check-direct-display | "
               "--sensor [address] [sample-count] | "
               "--tracking [address] [sample-count]]\n";
}

}  // namespace

int main(const int argc, char** argv) {
  if (argc == 1 || std::string(argv[1]) == "--displays") {
    return ProbeDisplays();
  }
  if (std::string(argv[1]) == "--find-xreal") {
    return FindXrealConnector();
  }
  if (std::string(argv[1]) == "--check-direct-display") {
    return CheckDirectDisplay();
  }
#if defined(VREALONE_HAS_SENSOR)
  if (std::string(argv[1]) == "--sensor") {
    const char* address = argc >= 3 ? argv[2] : "169.254.2.1:52998";
    const long sample_count = argc >= 4 ? std::strtol(argv[3], nullptr, 10) : 500;
    if (sample_count <= 0) {
      PrintUsage(argv[0]);
      return 2;
    }
    return ProbeSensor(address, sample_count);
  }
  if (std::string(argv[1]) == "--tracking") {
    const char* address = argc >= 3 ? argv[2] : "169.254.2.1:52998";
    const long sample_count =
        argc >= 4 ? std::strtol(argv[3], nullptr, 10) : 10'000;
    if (sample_count <= 0) {
      PrintUsage(argv[0]);
      return 2;
    }
    return ProbeTracking(address, sample_count);
  }
#endif
  PrintUsage(argv[0]);
  return 2;
}
