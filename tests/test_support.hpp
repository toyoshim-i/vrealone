// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cmath>
#include <stdexcept>
#include <string>

inline void Check(const bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

inline void CheckNear(const double actual, const double expected,
                      const double tolerance, const std::string& message) {
  Check(std::abs(actual - expected) <= tolerance,
        message + ": got " + std::to_string(actual));
}

void RunDisplayConfigTests();
void RunEdidTests();
void RunQuaternionTests();
void RunRecenterTests();
#if VREALONE_HAS_FUSION
void RunFusionTrackerTests();
#endif
