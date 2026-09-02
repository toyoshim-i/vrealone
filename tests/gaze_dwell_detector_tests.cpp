// SPDX-License-Identifier: Apache-2.0
#include "input/gaze_dwell_detector.hpp"

#include <cmath>
#include <numbers>

#include "test_support.hpp"

namespace {

vrealone::tracking::Quaternion RotationX(const double degrees) {
  const double radians = degrees * (std::numbers::pi / 180.0);
  return {std::cos(radians / 2.0), std::sin(radians / 2.0), 0.0, 0.0};
}

void TestAngularDistance() {
  const vrealone::tracking::Quaternion identity{1.0, 0.0, 0.0, 0.0};
  const vrealone::tracking::Quaternion rot_45 = RotationX(45.0);
  const vrealone::tracking::Quaternion rot_neg_identity{-1.0, 0.0, 0.0, 0.0};

  CheckNear(vrealone::input::AngularDistanceDegrees(identity, identity), 0.0, 1e-6, "identity to identity");
  CheckNear(vrealone::input::AngularDistanceDegrees(identity, rot_45), 45.0, 1e-4, "identity to rot 45");
  CheckNear(vrealone::input::AngularDistanceDegrees(identity, rot_neg_identity), 0.0, 1e-6, "identity to -identity");
}

void TestDwellClickTrigger() {
  using namespace std::chrono_literals;
  vrealone::input::GazeDwellConfig config{
      .enabled = true,
      .dwell_time = 1000ms,
      .max_angle_deviation_deg = 1.5F,
      .cooldown_time = 1500ms,
  };
  vrealone::input::GazeDwellDetector detector(config);

  const auto t0 = std::chrono::steady_clock::time_point{};
  const vrealone::tracking::Quaternion q0{1.0, 0.0, 0.0, 0.0};

  // First sample sets anchor, no click
  Check(!detector.Observe(t0, q0), "first sample should not click");

  // Small movement within 1.5 degrees at 500ms -> no click yet
  const auto q_small = RotationX(0.8);
  Check(!detector.Observe(t0 + 500ms, q_small), "dwelling under time should not click");

  // Still within tolerance at 990ms -> no click yet
  Check(!detector.Observe(t0 + 990ms, q0), "dwelling just under threshold should not click");

  // 1000ms reached -> clicks!
  Check(detector.Observe(t0 + 1000ms, q_small), "dwell time reached should trigger click");

  // Cooldown active -> no clicks even if dwelling at 1100ms
  Check(!detector.Observe(t0 + 1100ms, q0), "cooldown should suppress immediate clicks");
  Check(!detector.Observe(t0 + 2000ms, q0), "cooldown should suppress clicks before timeout");

  // Move away beyond 2x tolerance (e.g. 5 degrees) at 2100ms -> ends cooldown
  const auto q_away = RotationX(5.0);
  Check(!detector.Observe(t0 + 2100ms, q_away), "moving away should reset anchor");

  // Dwell at new orientation for 1000ms -> should trigger another click
  Check(!detector.Observe(t0 + 2600ms, q_away), "dwelling at new anchor under time");
  Check(detector.Observe(t0 + 3100ms, q_away), "dwelling at new anchor should click");
}

void TestMovementResetsDwell() {
  using namespace std::chrono_literals;
  vrealone::input::GazeDwellConfig config{
      .enabled = true,
      .dwell_time = 1000ms,
      .max_angle_deviation_deg = 1.5F,
      .cooldown_time = 1500ms,
  };
  vrealone::input::GazeDwellDetector detector(config);

  const auto t0 = std::chrono::steady_clock::time_point{};
  const vrealone::tracking::Quaternion q0{1.0, 0.0, 0.0, 0.0};

  Check(!detector.Observe(t0, q0), "init");
  Check(!detector.Observe(t0 + 600ms, q0), "dwell 600ms");

  // Move significantly (3 degrees) at 700ms -> resets timer
  const auto q_moved = RotationX(3.0);
  Check(!detector.Observe(t0 + 700ms, q_moved), "move resets timer");

  // At 1200ms (500ms after move) -> no click
  Check(!detector.Observe(t0 + 1200ms, q_moved), "dwell 500ms after move");

  // At 1700ms (1000ms after move) -> clicks!
  Check(detector.Observe(t0 + 1700ms, q_moved), "dwell 1000ms after move clicks");
}

void TestDisabledDetector() {
  using namespace std::chrono_literals;
  vrealone::input::GazeDwellConfig config{.enabled = false};
  vrealone::input::GazeDwellDetector detector(config);

  const auto t0 = std::chrono::steady_clock::time_point{};
  const vrealone::tracking::Quaternion q0{1.0, 0.0, 0.0, 0.0};

  Check(!detector.Observe(t0, q0), "disabled detector sample 0");
  Check(!detector.Observe(t0 + 5000ms, q0), "disabled detector sample long");
}

}  // namespace

void RunGazeDwellDetectorTests() {
  TestAngularDistance();
  TestDwellClickTrigger();
  TestMovementResetsDwell();
  TestDisabledDetector();
}
