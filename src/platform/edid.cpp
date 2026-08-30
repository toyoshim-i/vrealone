// SPDX-License-Identifier: Apache-2.0
#include "platform/edid.hpp"

#include <algorithm>
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
  return EdidIdentity{vendor, product, DecodeVendorName(vendor)};
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
    displays.push_back(
        {entry.path(), status, bytes.empty() ? std::nullopt : ParseEdid(bytes)});
  }
  std::ranges::sort(displays, {}, &DrmDisplay::connector);
  return displays;
}

}  // namespace vrealone::platform
