// SPDX-License-Identifier: Apache-2.0
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "platform/edid.hpp"

#if VREALONE_HAS_SENSOR
#include <xreal_one_driver.h>
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
    } else {
      std::cout << ", no valid EDID";
    }
    std::cout << '\n';
  }
  return 0;
}

#if VREALONE_HAS_SENSOR
int ProbeSensor(const char* address, const long sample_count) {
  auto* handle = xo_new_with_addr(address);
  if (handle == nullptr) {
    std::cerr << "Unable to connect to " << address << ".\n";
    return 1;
  }

  std::uint64_t previous_timestamp = 0;
  long timestamp_rollbacks = 0;
  double maximum_gyro_magnitude = 0.0;
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
    previous_timestamp = sample.timestamp;
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
  return timestamp_rollbacks == 0 ? 0 : 1;
}
#endif

void PrintUsage(const char* program) {
  std::cerr << "Usage: " << program
            << " --displays | --sensor [address] [sample-count]\n";
}

}  // namespace

int main(const int argc, char** argv) {
  if (argc == 1 || std::string(argv[1]) == "--displays") {
    return ProbeDisplays();
  }
#if VREALONE_HAS_SENSOR
  if (std::string(argv[1]) == "--sensor") {
    const char* address = argc >= 3 ? argv[2] : "169.254.2.1:52998";
    const long sample_count = argc >= 4 ? std::strtol(argv[3], nullptr, 10) : 500;
    if (sample_count <= 0) {
      PrintUsage(argv[0]);
      return 2;
    }
    return ProbeSensor(address, sample_count);
  }
#endif
  PrintUsage(argv[0]);
  return 2;
}
