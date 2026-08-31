// SPDX-License-Identifier: Apache-2.0
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <iostream>
#include <string>

#include "control/control_server.hpp"

int main(const int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " recenter|click|status\n";
    return 2;
  }
  const std::string command = argv[1];
  if (vrealone::control::ParseCommand(command) ==
      vrealone::control::Command::invalid) {
    std::cerr << "Unknown command: " << command << '\n';
    return 2;
  }

  const int socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (socket_fd < 0) {
    std::cerr << "Unable to create control socket.\n";
    return 1;
  }
  const auto path = vrealone::control::DefaultSocketPath();
  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
  if (connect(socket_fd, reinterpret_cast<const sockaddr*>(&address),
              sizeof(address)) != 0) {
    std::cerr << "Unable to connect to " << path << ".\n";
    close(socket_fd);
    return 1;
  }
  const auto request = command + "\n";
  if (write(socket_fd, request.data(), request.size()) < 0) {
    std::cerr << "Unable to send control command.\n";
    close(socket_fd);
    return 1;
  }
  std::array<char, 128> response{};
  const auto received = read(socket_fd, response.data(), response.size() - 1);
  close(socket_fd);
  if (received <= 0) {
    std::cerr << "No response from driver.\n";
    return 1;
  }
  std::cout.write(response.data(), received);
  return 0;
}
