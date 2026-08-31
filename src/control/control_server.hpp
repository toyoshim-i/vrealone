// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <thread>

namespace vrealone::control {

enum class Command { recenter, click, status, invalid };

[[nodiscard]] Command ParseCommand(std::string_view text);
[[nodiscard]] std::string DefaultSocketPath();

class ControlServer {
 public:
  using Handler = std::function<std::string(Command)>;

  ControlServer() = default;
  ~ControlServer();
  ControlServer(const ControlServer&) = delete;
  ControlServer& operator=(const ControlServer&) = delete;

  [[nodiscard]] bool Start(const std::string& path, Handler handler);
  void Stop();

 private:
  void Run(std::stop_token stop_token);

  std::string path_;
  Handler handler_;
  int listen_socket_ = -1;
  std::jthread thread_;
};

}  // namespace vrealone::control
