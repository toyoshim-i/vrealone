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
  std::uint16_t vendor_id;
  std::uint16_t product_id;
  std::string vendor_name;
};

struct DrmDisplay {
  std::filesystem::path connector;
  std::string status;
  std::optional<EdidIdentity> edid;
};

[[nodiscard]] std::optional<EdidIdentity> ParseEdid(
    const std::vector<std::uint8_t>& bytes);
[[nodiscard]] std::vector<DrmDisplay> ProbeDrmDisplays(
    const std::filesystem::path& drm_root = "/sys/class/drm");

}  // namespace vrealone::platform
