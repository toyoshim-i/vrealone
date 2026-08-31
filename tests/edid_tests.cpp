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

std::vector<std::uint8_t> MakeXrealEdid() {
  std::vector<std::uint8_t> edid(128, 0);
  const std::uint8_t header[] = {0x00, 0xff, 0xff, 0xff,
                                 0xff, 0xff, 0xff, 0x00};
  for (std::size_t index = 0; index < 8; ++index) {
    edid[index] = header[index];
  }
  // Measured XREAL One identity: EISA manufacturer MRG, product 0x4101.
  edid[8] = 0x36;
  edid[9] = 0x47;
  // product_id 0x4101 (little endian: byte[10] = 0x01, byte[11] = 0x41)
  edid[10] = 0x01;
  edid[11] = 0x41;

  // Descriptor block at offset 54 (type 0xFC = Product Name)
  edid[54] = 0x00;
  edid[55] = 0x00;
  edid[56] = 0x00;
  edid[57] = 0xfc;
  edid[58] = 0x00;
  const std::string name = "XREAL One\n";
  for (std::size_t i = 0; i < name.size(); ++i) {
    edid[59 + i] = static_cast<std::uint8_t>(name[i]);
  }

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
  Check(!vrealone::platform::IsXrealDisplay(*identity),
        "generic display must not match XREAL");

  bytes[20] ^= 1;
  Check(!vrealone::platform::ParseEdid(bytes),
        "invalid EDID checksum must be rejected");
  Check(!vrealone::platform::ParseEdid(std::vector<std::uint8_t>(12)),
        "truncated EDID must be rejected");

  const auto xreal_bytes = MakeXrealEdid();
  const auto xreal_identity = vrealone::platform::ParseEdid(xreal_bytes);
  Check(xreal_identity.has_value(), "XREAL EDID must parse");
  Check(xreal_identity->vendor_name == "MRG", "MRG vendor name must decode");
  Check(xreal_identity->product_id == 0x4101, "0x4101 product must decode");
  Check(xreal_identity->product_name == "XREAL One",
        "XREAL One product name descriptor must decode");
  Check(vrealone::platform::IsXrealDisplay(*xreal_identity),
        "XREAL EDID must match XREAL display check");

  auto vendor_only = *xreal_identity;
  vendor_only.product_id = 0x9999;
  vendor_only.product_name.clear();
  Check(!vrealone::platform::IsXrealDisplay(vendor_only),
        "MRG vendor alone must not match XREAL One");

  auto product_only = *xreal_identity;
  product_only.vendor_id = 0x0443;
  product_only.vendor_name = "ABC";
  product_only.product_name.clear();
  Check(!vrealone::platform::IsXrealDisplay(product_only),
        "product ID alone must not match XREAL One");

  auto different_model = *xreal_identity;
  different_model.product_id = 0x9999;
  different_model.product_name = "XREAL One Pro";
  Check(!vrealone::platform::IsXrealDisplay(different_model),
        "another XREAL model must not match XREAL One by substring");

  const vrealone::platform::DrmMode fhd{1920, 1080};
  const vrealone::platform::DrmMode hd{1280, 720};
  const std::vector<vrealone::platform::DrmDisplay> unambiguous_displays = {
      {"card1-DP-2", "connected", xreal_identity, {fhd}},
      {"card1-HDMI-A-1", "connected", identity, {hd}},
  };
  const auto safe = vrealone::platform::AssessDirectDisplay(
      unambiguous_displays, 0x3647, 0x4101, 1920, 1080);
  Check(safe.IsUnambiguous(),
        "a connected XREAL with a unique requested mode must be safe");

  auto ambiguous_displays = unambiguous_displays;
  ambiguous_displays[1].modes.push_back(fhd);
  const auto ambiguous = vrealone::platform::AssessDirectDisplay(
      ambiguous_displays, 0x3647, 0x4101, 1920, 1080);
  Check(!ambiguous.IsUnambiguous(),
        "another connected FHD display must make selection ambiguous");
  Check(ambiguous.conflicting_connectors.size() == 1,
        "the conflicting connector must be reported");

  ambiguous_displays[1].status = "disconnected";
  const auto retained_modes = vrealone::platform::AssessDirectDisplay(
      ambiguous_displays, 0x3647, 0x4101, 1920, 1080);
  Check(!retained_modes.IsUnambiguous(),
        "a disconnected output with a retained FHD mode must remain blocked");
  Check(retained_modes.conflicting_connectors.size() == 1,
        "a disconnected conflicting connector must be reported");

  auto disconnected_target = unambiguous_displays;
  disconnected_target[0].status = "disconnected";
  const auto missing = vrealone::platform::AssessDirectDisplay(
      disconnected_target, 0x3647, 0x4101, 1920, 1080);
  Check(!missing.target_connected,
        "a disconnected XREAL must not satisfy the safety gate");

  auto wrong_mode = unambiguous_displays;
  wrong_mode[0].modes = {hd};
  const auto unsupported = vrealone::platform::AssessDirectDisplay(
      wrong_mode, 0x3647, 0x4101, 1920, 1080);
  Check(!unsupported.target_has_mode,
        "XREAL must advertise the requested mode before direct mode starts");
}
