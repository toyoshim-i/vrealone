// SPDX-License-Identifier: Apache-2.0
#include <cstdint>
#include <vector>

#include "platform/edid.hpp"
#include "test_support.hpp"

namespace {

std::vector<std::uint8_t> MakeEdid() {
  std::vector<std::uint8_t> edid(128, 0);
  const std::uint8_t header[] = {0x00, 0xff, 0xff, 0xff,
                                 0xff, 0xff, 0xff, 0x00};
  for (std::size_t index = 0; index < 8; ++index) {
    edid[index] = header[index];
  }
  // EISA manufacturer code "ABC" and little-endian product 0x1234.
  edid[8] = 0x04;
  edid[9] = 0x43;
  edid[10] = 0x34;
  edid[11] = 0x12;
  unsigned sum = 0;
  for (std::size_t index = 0; index < 127; ++index) {
    sum += edid[index];
  }
  edid[127] = static_cast<std::uint8_t>((256U - (sum & 0xffU)) & 0xffU);
  return edid;
}

}  // namespace

void RunEdidTests() {
  auto bytes = MakeEdid();
  const auto identity = vrealone::platform::ParseEdid(bytes);
  Check(identity.has_value(), "valid EDID must parse");
  Check(identity->vendor_id == 0x0443, "raw EDID vendor code must parse");
  Check(identity->vendor_name == "ABC", "EISA vendor name must decode");
  Check(identity->product_id == 0x1234,
        "little-endian EDID product code must parse");

  bytes[20] ^= 1;
  Check(!vrealone::platform::ParseEdid(bytes),
        "invalid EDID checksum must be rejected");
  Check(!vrealone::platform::ParseEdid(std::vector<std::uint8_t>(12)),
        "truncated EDID must be rejected");
}
