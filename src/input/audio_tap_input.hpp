// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

#include "input/tap_detector.hpp"

struct _snd_pcm;

namespace vrealone::input {

struct AudioTapInputConfig {
  std::string device = "pulse";
  TapDetectorConfig detector;
  std::chrono::milliseconds startup_suppression{1000};
};

struct TapControlConfig {
  bool enabled = false;
  float imu_acceleration_deviation_m_s2 = 3.0F;
  AudioTapInputConfig audio;
};

class AudioTapInput {
 public:
  using ActionHandler = std::function<void(TapAction)>;
  using ImuImpactReader =
      std::function<std::chrono::steady_clock::time_point()>;
  using PeakHandler = std::function<void(std::int64_t, std::int64_t)>;

  explicit AudioTapInput(AudioTapInputConfig config = {});
  ~AudioTapInput();

  AudioTapInput(const AudioTapInput&) = delete;
  AudioTapInput& operator=(const AudioTapInput&) = delete;

  bool Start(ActionHandler action_handler, ImuImpactReader imu_impact_reader,
             PeakHandler peak_handler = {});
  void Stop();
  [[nodiscard]] bool Running() const;

 private:
  void Loop(std::stop_token stop_token);

  AudioTapInputConfig config_;
  ActionHandler action_handler_;
  ImuImpactReader imu_impact_reader_;
  PeakHandler peak_handler_;
  _snd_pcm* pcm_ = nullptr;
  std::jthread thread_;
  std::atomic<bool> running_{false};
};

}  // namespace vrealone::input
