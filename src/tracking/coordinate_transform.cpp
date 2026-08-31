// SPDX-License-Identifier: Apache-2.0
#include "tracking/coordinate_transform.hpp"

namespace vrealone::tracking {

std::array<double, 3> ParserVectorToOpenVr(
    const std::array<double, 3>& parser_vector) {
  return {-parser_vector[0], parser_vector[2], parser_vector[1]};
}

Quaternion ParserOrientationToOpenVr(const Quaternion& parser_orientation) {
  // This is the quaternion form of M * R * inverse(M), where M maps parser
  // basis vectors into the measured OpenVR basis.  Under a proper orthogonal
  // basis change, the quaternion scalar is unchanged and its axial vector is
  // transformed by M.
  return {parser_orientation.w, -parser_orientation.x, parser_orientation.z,
          parser_orientation.y};
}

}  // namespace vrealone::tracking
