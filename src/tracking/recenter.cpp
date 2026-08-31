// SPDX-License-Identifier: Apache-2.0
#include "tracking/recenter.hpp"

namespace vrealone::tracking {

void Recenter::SetOrigin(const Quaternion& current) {
  offset_ = current.Normalized().Inverse();
}

void Recenter::Clear() { offset_ = {}; }

Quaternion Recenter::Apply(const Quaternion& current) const {
  return (offset_ * current.Normalized()).Normalized();
}

std::array<double, 3> Recenter::ApplyVector(
    const std::array<double, 3>& current) const {
  return offset_.Rotate(current);
}

}  // namespace vrealone::tracking
