// SPDX-License-Identifier: Apache-2.0
#include "platform/edid.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <iterator>
#include <numeric>
#include <ranges>

namespace vrealone::platform {
namespace {

constexpr std::array<std::uint8_t, 8> kEdidHeader = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

std::string DecodeVendorName(const std::uint16_t vendor_id) {
  std::string result(3, '?');
  for (int index = 0; index < 3; ++index) {
    const auto shift = 10 - index * 5;
    const auto letter = static_cast<unsigned>((vendor_id >> shift) & 0x1fU);
    if (letter >= 1 && letter <= 26) {
      result[static_cast<std::size_t>(index)] =
          static_cast<char>('A' + letter - 1);
    }
  }
  return result;
}

std::string DecodeDescriptorString(const std::uint8_t* data,
                                   const std::size_t length) {
  std::string result;
  for (std::size_t index = 0; index < length; ++index) {
    const auto byte = data[index];
    if (byte == 0x0a) {
      break;
    }
    if (byte >= 0x20 && byte <= 0x7e) {
      result.push_back(static_cast<char>(byte));
    }
  }
  while (!result.empty() && result.back() == ' ') {
    result.pop_back();
  }
  return result;
}

bool CaseInsensitiveEquals(const std::string& left, const std::string& right) {
  return left.size() == right.size() &&
         std::equal(left.begin(), left.end(), right.begin(),
                    [](const char left_char, const char right_char) {
                      return std::tolower(
                                 static_cast<unsigned char>(left_char)) ==
                             std::tolower(
                                 static_cast<unsigned char>(right_char));
                    });
}

std::string ReadText(const std::filesystem::path& path) {
  std::ifstream stream(path);
  std::string value;
  std::getline(stream, value);
  return value;
}

std::vector<std::uint8_t> ReadBinary(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(stream),
          std::istreambuf_iterator<char>()};
}

std::vector<DrmMode> ReadModes(const std::filesystem::path& path) {
  std::ifstream stream(path);
  std::vector<DrmMode> modes;
  std::string line;
  while (std::getline(stream, line)) {
    const auto separator = line.find('x');
    if (separator == std::string::npos) {
      continue;
    }
    DrmMode mode;
    const auto width_result = std::from_chars(
        line.data(), line.data() + separator, mode.width);
    const auto height_result = std::from_chars(
        line.data() + separator + 1, line.data() + line.size(), mode.height);
    if (width_result.ec == std::errc{} &&
        width_result.ptr == line.data() + separator &&
        height_result.ec == std::errc{} &&
        height_result.ptr == line.data() + line.size()) {
      modes.push_back(mode);
    }
  }
  return modes;
}

}  // namespace

std::optional<EdidIdentity> ParseEdid(
    const std::vector<std::uint8_t>& bytes) {
  if (bytes.size() < 128 ||
      !std::equal(kEdidHeader.begin(), kEdidHeader.end(), bytes.begin())) {
    return std::nullopt;
  }
  const auto checksum = std::accumulate(
      bytes.begin(), bytes.begin() + 128, 0U,
      [](const unsigned sum, const std::uint8_t byte) { return sum + byte; });
  if ((checksum & 0xffU) != 0U) {
    return std::nullopt;
  }

  const auto vendor = static_cast<std::uint16_t>(
      static_cast<std::uint16_t>(bytes[8]) << 8U | bytes[9]);
  const auto product = static_cast<std::uint16_t>(
      static_cast<std::uint16_t>(bytes[11]) << 8U | bytes[10]);
  EdidIdentity identity{vendor, product, DecodeVendorName(vendor), "", ""};

  // Parse 18-byte descriptor blocks at offsets 54, 72, 90, 108.
  constexpr std::array<std::size_t, 4> kDescriptorOffsets = {54, 72, 90, 108};
  for (const auto offset : kDescriptorOffsets) {
    if (bytes[offset] == 0x00 && bytes[offset + 1] == 0x00 &&
        bytes[offset + 2] == 0x00) {
      const auto tag = bytes[offset + 3];
      if (tag == 0xfc) {
        identity.product_name =
            DecodeDescriptorString(&bytes[offset + 5], 13);
      } else if (tag == 0xff) {
        identity.serial_number =
            DecodeDescriptorString(&bytes[offset + 5], 13);
      }
    }
  }

  return identity;
}

bool IsXrealDisplay(const EdidIdentity& identity) {
  if (identity.vendor_id == 0x3647 && identity.product_id == 0x4101) {
    return true;
  }
  return identity.vendor_name == "MRG" && !identity.product_name.empty() &&
         (CaseInsensitiveEquals(identity.product_name, "xreal one") ||
          CaseInsensitiveEquals(identity.product_name, "nreal one"));
}

std::vector<DrmDisplay> ProbeDrmDisplays(
    const std::filesystem::path& drm_root) {
  std::vector<DrmDisplay> displays;
  std::error_code error;
  for (const auto& entry : std::filesystem::directory_iterator(drm_root, error)) {
    if (error) {
      break;
    }
    if (!entry.is_directory(error) ||
        !std::filesystem::exists(entry.path() / "status", error)) {
      continue;
    }
    const auto status = ReadText(entry.path() / "status");
    auto bytes = ReadBinary(entry.path() / "edid");
    displays.push_back({entry.path(), status,
                        bytes.empty() ? std::nullopt : ParseEdid(bytes),
                        ReadModes(entry.path() / "modes")});
  }
  std::ranges::sort(displays, {}, &DrmDisplay::connector);
  return displays;
}

DirectDisplayAssessment AssessDirectDisplay(
    const std::vector<DrmDisplay>& displays, const std::uint16_t vendor_id,
    const std::uint16_t product_id, const std::uint32_t width,
    const std::uint32_t height) {
  DirectDisplayAssessment assessment;
  const DrmMode requested_mode{width, height};
  for (const auto& display : displays) {
    const bool is_target = display.edid &&
                           display.edid->vendor_id == vendor_id &&
                           display.edid->product_id == product_id;
    const bool has_mode =
        std::ranges::find(display.modes, requested_mode) != display.modes.end();
    if (is_target) {
      if (display.status == "connected") {
        assessment.target_connected = true;
        assessment.target_has_mode = assessment.target_has_mode || has_mode;
      }
    } else if (has_mode) {
      // SteamVR 2.16.7's X11 direct-display path scans advertised modes even
      // when RandR calls the output disconnected.  Treat a retained mode as a
      // collision until it disappears from the connector's mode list.
      assessment.conflicting_connectors.push_back(display.connector);
    }
  }
  return assessment;
}

}  // namespace vrealone::platform
