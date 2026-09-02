// SPDX-License-Identifier: Apache-2.0
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

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
  const auto request = command + "\n";
  std::array<char, 128> response{};

#if defined(_WIN32)
  const auto path = vrealone::control::DefaultSocketPath();
  if (!WaitNamedPipeA(path.c_str(), 1000)) {
    std::cerr << "Unable to find the control pipe.\n";
    return 1;
  }
  const HANDLE pipe = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                                  0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) {
    std::cerr << "Unable to open the control pipe.\n";
    return 1;
  }
  DWORD written = 0;
  if (!WriteFile(pipe, request.data(), static_cast<DWORD>(request.size()),
                 &written, nullptr)) {
    std::cerr << "Unable to send control command.\n";
    CloseHandle(pipe);
    return 1;
  }
  DWORD received = 0;
  const BOOL did_read = ReadFile(
      pipe, response.data(), static_cast<DWORD>(response.size() - 1),
      &received, nullptr);
  CloseHandle(pipe);
  if (!did_read || received == 0) {
    std::cerr << "No response from driver.\n";
    return 1;
  }
  std::cout.write(response.data(), static_cast<std::streamsize>(received));
#else
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
  if (write(socket_fd, request.data(), request.size()) < 0) {
    std::cerr << "Unable to send control command.\n";
    close(socket_fd);
    return 1;
  }
  const auto received = read(socket_fd, response.data(), response.size() - 1);
  close(socket_fd);
  if (received <= 0) {
    std::cerr << "No response from driver.\n";
    return 1;
  }
  std::cout.write(response.data(), received);
#endif
  return 0;
}
