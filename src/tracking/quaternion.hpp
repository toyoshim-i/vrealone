// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>

namespace vrealone::tracking {

struct Quaternion {
  double w = 1.0;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  [[nodiscard]] double Norm() const;
  [[nodiscard]] Quaternion Normalized() const;
  [[nodiscard]] Quaternion Inverse() const;
  [[nodiscard]] std::array<double, 3> Rotate(
      const std::array<double, 3>& vector) const;
};

[[nodiscard]] Quaternion operator*(const Quaternion& left,
                                   const Quaternion& right);

}  // namespace vrealone::tracking
