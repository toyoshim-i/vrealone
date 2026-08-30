// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "tracking/quaternion.hpp"

namespace vrealone::tracking {

class Recenter {
 public:
  void SetOrigin(const Quaternion& current);
  void Clear();
  [[nodiscard]] Quaternion Apply(const Quaternion& current) const;

 private:
  Quaternion offset_{};
};

}  // namespace vrealone::tracking
