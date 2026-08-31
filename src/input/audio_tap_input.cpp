// SPDX-License-Identifier: Apache-2.0
#include "input/audio_tap_input.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

#include <alsa/asoundlib.h>

namespace vrealone::input {
namespace {

constexpr unsigned int kSampleRate = 48'000;
constexpr unsigned int kChannels = 2;
constexpr snd_pcm_uframes_t kFramesPerWindow = 960;

}  // namespace

AudioTapInput::AudioTapInput(AudioTapInputConfig config)
    : config_(std::move(config)) {}

AudioTapInput::~AudioTapInput() { Stop(); }

bool AudioTapInput::Start(ActionHandler action_handler,
                          ImuImpactReader imu_impact_reader,
                          PeakHandler peak_handler) {
  Stop();
  if (snd_pcm_open(reinterpret_cast<snd_pcm_t**>(&pcm_),
                   config_.device.c_str(), SND_PCM_STREAM_CAPTURE, 0) < 0) {
    pcm_ = nullptr;
    return false;
  }
  if (snd_pcm_set_params(reinterpret_cast<snd_pcm_t*>(pcm_),
                         SND_PCM_FORMAT_S32_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                         kChannels, kSampleRate, 1, 100'000) < 0) {
    snd_pcm_close(reinterpret_cast<snd_pcm_t*>(pcm_));
    pcm_ = nullptr;
    return false;
  }
  action_handler_ = std::move(action_handler);
  imu_impact_reader_ = std::move(imu_impact_reader);
  peak_handler_ = std::move(peak_handler);
  running_.store(true);
  thread_ = std::jthread(
      [this](const std::stop_token token) { Loop(token); });
  return true;
}

void AudioTapInput::Stop() {
  if (thread_.joinable()) {
    thread_.request_stop();
    thread_.join();
  }
  running_.store(false);
  if (pcm_ != nullptr) {
    snd_pcm_close(reinterpret_cast<snd_pcm_t*>(pcm_));
    pcm_ = nullptr;
  }
}

bool AudioTapInput::Running() const { return running_.load(); }

void AudioTapInput::Loop(const std::stop_token stop_token) {
  TapDetector detector(config_.detector);
  std::array<std::int32_t, kFramesPerWindow * kChannels> samples{};
  const auto accept_after =
      std::chrono::steady_clock::now() + config_.startup_suppression;
  auto* const pcm = reinterpret_cast<snd_pcm_t*>(pcm_);
  while (!stop_token.stop_requested()) {
    const auto frames = snd_pcm_readi(pcm, samples.data(), kFramesPerWindow);
    if (frames < 0) {
      if (snd_pcm_recover(pcm, static_cast<int>(frames), 1) < 0) {
        break;
      }
      continue;
    }
    std::int64_t peak = 0;
    const auto sample_count = static_cast<std::size_t>(frames) * kChannels;
    for (std::size_t i = 0; i < sample_count; ++i) {
      const auto value = static_cast<std::int64_t>(samples[i]);
      peak = std::max(peak, value < 0 ? -value : value);
    }
    const auto now = std::chrono::steady_clock::now();
    if (now >= accept_after) {
      const auto last_imu_impact = imu_impact_reader_();
      if (peak_handler_ && peak >= config_.detector.audio_peak_threshold) {
        const auto separation = now >= last_imu_impact
                                    ? now - last_imu_impact
                                    : last_imu_impact - now;
        peak_handler_(peak,
                      std::chrono::duration_cast<std::chrono::milliseconds>(
                          separation)
                          .count());
      }
      detector.ObserveAudioWindow(now, peak, last_imu_impact);
      const auto action = detector.Poll(now);
      if (action != TapAction::none) {
        action_handler_(action);
      }
    }
  }
  running_.store(false);
}

}  // namespace vrealone::input
