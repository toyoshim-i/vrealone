// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace vrealone::platform {

struct EdidIdentity {
  std::uint16_t vendor_id = 0;
  std::uint16_t product_id = 0;
  std::string vendor_name;
  std::string product_name;
  std::string serial_number;
};

struct DrmMode {
  std::uint32_t width = 0;
  std::uint32_t height = 0;

  bool operator==(const DrmMode&) const = default;
};

struct DrmDisplay {
  std::filesystem::path connector;
  std::string status;
  std::optional<EdidIdentity> edid;
  std::vector<DrmMode> modes;
};

struct DirectDisplayAssessment {
  bool target_connected = false;
  bool target_has_mode = false;
  std::vector<std::filesystem::path> conflicting_connectors;

  [[nodiscard]] bool IsUnambiguous() const {
    return target_connected && target_has_mode &&
           conflicting_connectors.empty();
  }
};

[[nodiscard]] std::optional<EdidIdentity> ParseEdid(
    const std::vector<std::uint8_t>& bytes);
[[nodiscard]] bool IsXrealDisplay(const EdidIdentity& identity);
[[nodiscard]] std::vector<DrmDisplay> ProbeDrmDisplays(
    const std::filesystem::path& drm_root = "/sys/class/drm");
[[nodiscard]] DirectDisplayAssessment AssessDirectDisplay(
    const std::vector<DrmDisplay>& displays, std::uint16_t vendor_id,
    std::uint16_t product_id, std::uint32_t width, std::uint32_t height);

}  // namespace vrealone::platform
