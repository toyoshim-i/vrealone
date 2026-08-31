// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>

#include "tracking/quaternion.hpp"

namespace vrealone::tracking {

class Recenter {
 public:
  void SetOrigin(const Quaternion& current);
  void Clear();
  [[nodiscard]] Quaternion Apply(const Quaternion& current) const;
  [[nodiscard]] std::array<double, 3> ApplyVector(
      const std::array<double, 3>& current) const;

 private:
  Quaternion offset_{};
};

}  // namespace vrealone::tracking
