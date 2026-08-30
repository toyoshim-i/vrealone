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

}  // namespace vrealone::tracking
