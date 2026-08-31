// SPDX-License-Identifier: Apache-2.0
#include "input/tap_detector.hpp"

#include <chrono>

#include "test_support.hpp"

void RunTapDetectorTests() {
  using namespace std::chrono_literals;
  using vrealone::input::TapAction;
  const auto start = std::chrono::steady_clock::time_point(1s);

  vrealone::input::TapDetector single;
  single.ObserveAudioWindow(start, 100'000'000, start - 20ms);
  Check(single.Poll(start + 649ms) == TapAction::none,
        "single tap should wait for the double-tap window");
  Check(single.Poll(start + 650ms) == TapAction::click,
        "single correlated tap should click");

  vrealone::input::TapDetector double_tap;
  double_tap.ObserveAudioWindow(start, 100'000'000, start);
  double_tap.ObserveAudioWindow(start + 250ms, 100'000'000, start + 240ms);
  Check(double_tap.Poll(start + 499ms) == TapAction::none,
        "double tap should wait for settling");
  Check(double_tap.Poll(start + 500ms) == TapAction::recenter,
        "double correlated tap should recenter");

  vrealone::input::TapDetector audio_only;
  audio_only.ObserveAudioWindow(start, 200'000'000, start - 300ms);
  Check(audio_only.Poll(start + 1s) == TapAction::none,
        "audio without a recent IMU impact must be ignored");

  vrealone::input::TapDetector impact_only;
  impact_only.ObserveAudioWindow(start, 1'000, start);
  Check(impact_only.Poll(start + 1s) == TapAction::none,
        "IMU impact without a loud audio peak must be ignored");

  vrealone::input::TapDetector same_impact;
  same_impact.ObserveAudioWindow(start, 100'000'000, start);
  same_impact.ObserveAudioWindow(start + 250ms, 100'000'000, start);
  Check(same_impact.Poll(start + 650ms) == TapAction::click,
        "one IMU impact must not be consumed twice during microphone ringing");

  vrealone::input::ImuImpactDetector impacts;
  Check(impacts.Observe(start, 4.0F), "first impact should trigger");
  Check(!impacts.Observe(start + 10ms, 4.0F),
        "one sustained impact must trigger only once");
  Check(!impacts.Observe(start + 20ms, 0.5F),
        "quiet period should not immediately rearm");
  Check(!impacts.Observe(start + 60ms, 0.5F),
        "quiet period must fully elapse");
  Check(!impacts.Observe(start + 70ms, 0.5F),
        "rearming itself is not an impact");
  Check(impacts.Observe(start + 80ms, 4.0F),
        "a new impact after sustained quiet should trigger");
}
