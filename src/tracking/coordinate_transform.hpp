// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>

#include "tracking/quaternion.hpp"

namespace vrealone::tracking {

// Measured physical mapping for the parser-facing XREAL One axes:
//   pitch up       parser -X -> OpenVR +X
//   yaw left       parser +Z -> OpenVR +Y
//   right-ear roll parser -Y -> OpenVR -Z
[[nodiscard]] std::array<double, 3> ParserVectorToOpenVr(
    const std::array<double, 3>& parser_vector);

[[nodiscard]] Quaternion ParserOrientationToOpenVr(
    const Quaternion& parser_orientation);

}  // namespace vrealone::tracking
