// SPDX-License-Identifier: Apache-2.0
#include "tracking/quaternion.hpp"

#include <cmath>
#include <stdexcept>

namespace vrealone::tracking {

double Quaternion::Norm() const {
  return std::sqrt(w * w + x * x + y * y + z * z);
}

Quaternion Quaternion::Normalized() const {
  const auto norm = Norm();
  if (!std::isfinite(norm) || norm < 1e-12) {
    throw std::invalid_argument("cannot normalize an invalid quaternion");
  }
  return {w / norm, x / norm, y / norm, z / norm};
}

Quaternion Quaternion::Inverse() const {
  const auto norm_squared = w * w + x * x + y * y + z * z;
  if (!std::isfinite(norm_squared) || norm_squared < 1e-24) {
    throw std::invalid_argument("cannot invert an invalid quaternion");
  }
  return {w / norm_squared, -x / norm_squared, -y / norm_squared,
          -z / norm_squared};
}

std::array<double, 3> Quaternion::Rotate(
    const std::array<double, 3>& vector) const {
  const auto unit = Normalized();
  const Quaternion pure{0.0, vector[0], vector[1], vector[2]};
  const auto rotated = unit * pure * unit.Inverse();
  return {rotated.x, rotated.y, rotated.z};
}

Quaternion operator*(const Quaternion& left, const Quaternion& right) {
  return {
      left.w * right.w - left.x * right.x - left.y * right.y -
          left.z * right.z,
      left.w * right.x + left.x * right.w + left.y * right.z -
          left.z * right.y,
      left.w * right.y - left.x * right.z + left.y * right.w +
          left.z * right.x,
      left.w * right.z + left.x * right.y - left.y * right.x +
          left.z * right.w,
  };
}

}  // namespace vrealone::tracking
