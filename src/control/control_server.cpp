// SPDX-License-Identifier: Apache-2.0
#include "control/control_server.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include <array>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <utility>

namespace vrealone::control {

Command ParseCommand(std::string_view text) {
  while (!text.empty() &&
         (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
    text.remove_suffix(1);
  }
  if (text == "recenter") {
    return Command::recenter;
  }
  if (text == "click") {
    return Command::click;
  }
  if (text == "status") {
    return Command::status;
  }
  return Command::invalid;
}

std::string DefaultSocketPath() {
#if defined(_WIN32)
  return R"(\\.\pipe\vrealone-control)";
#else
  return "/run/user/" + std::to_string(getuid()) +
         "/vrealone-control.sock";
#endif
}

ControlServer::~ControlServer() { Stop(); }

bool ControlServer::Start(const std::string& path, Handler handler) {
  Stop();
#if defined(_WIN32)
  const HANDLE pipe = CreateNamedPipeA(
      path.c_str(), PIPE_ACCESS_DUPLEX,
      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT,
      1, 256, 256, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) {
    return false;
  }
  path_ = path;
  handler_ = std::move(handler);
  pipe_ = pipe;
  thread_ = std::jthread(
      [this](const std::stop_token token) { Run(token); });
  return true;
#else
  sockaddr_un address{};
  if (path.size() >= sizeof(address.sun_path)) {
    return false;
  }

  const int socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (socket_fd < 0) {
    return false;
  }

  address.sun_family = AF_UNIX;
  std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
  struct stat existing {};
  if (lstat(path.c_str(), &existing) == 0) {
    if (!S_ISSOCK(existing.st_mode) || existing.st_uid != getuid()) {
      close(socket_fd);
      return false;
    }
    unlink(path.c_str());
  }
  if (bind(socket_fd, reinterpret_cast<const sockaddr*>(&address),
           sizeof(address)) != 0 ||
      chmod(path.c_str(), S_IRUSR | S_IWUSR) != 0 ||
      listen(socket_fd, 4) != 0) {
    close(socket_fd);
    unlink(path.c_str());
    return false;
  }

  path_ = path;
  handler_ = std::move(handler);
  listen_socket_ = socket_fd;
  thread_ = std::jthread(
      [this](const std::stop_token token) { Run(token); });
  return true;
#endif
}

void ControlServer::Stop() {
  if (thread_.joinable()) {
    thread_.request_stop();
#if !defined(_WIN32)
    if (listen_socket_ >= 0) {
      shutdown(listen_socket_, SHUT_RDWR);
    }
#endif
    thread_.join();
  }
#if defined(_WIN32)
  if (pipe_ != nullptr) {
    CloseHandle(static_cast<HANDLE>(pipe_));
    pipe_ = nullptr;
  }
#else
  if (listen_socket_ >= 0) {
    close(listen_socket_);
    listen_socket_ = -1;
  }
  if (!path_.empty()) {
    unlink(path_.c_str());
    path_.clear();
  }
#endif
  path_.clear();
  handler_ = {};
}

void ControlServer::Run(const std::stop_token stop_token) {
#if defined(_WIN32)
  const HANDLE pipe = static_cast<HANDLE>(pipe_);
  while (!stop_token.stop_requested()) {
    const BOOL connected = ConnectNamedPipe(pipe, nullptr);
    const DWORD error = connected ? ERROR_SUCCESS : GetLastError();
    if (!connected && error != ERROR_PIPE_CONNECTED) {
      if (error == ERROR_PIPE_LISTENING || error == ERROR_NO_DATA) {
        if (error == ERROR_NO_DATA) {
          DisconnectNamedPipe(pipe);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        continue;
      }
      break;
    }
    std::array<char, 128> buffer{};
    DWORD received = 0;
    std::string response = "error\n";
    if (ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size() - 1),
                 &received, nullptr) && received > 0 && handler_) {
      response = handler_(ParseCommand(
          std::string_view(buffer.data(), static_cast<std::size_t>(received))));
      response.push_back('\n');
    }
    DWORD written = 0;
    [[maybe_unused]] const BOOL did_write =
        WriteFile(pipe, response.data(), static_cast<DWORD>(response.size()),
                  &written, nullptr);
    FlushFileBuffers(pipe);
    DisconnectNamedPipe(pipe);
  }
#else
  while (!stop_token.stop_requested()) {
    pollfd descriptor{listen_socket_, POLLIN, 0};
    const int poll_result = poll(&descriptor, 1, 250);
    if (poll_result <= 0 || (descriptor.revents & POLLIN) == 0) {
      continue;
    }
    const int client = accept4(listen_socket_, nullptr, nullptr, SOCK_CLOEXEC);
    if (client < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    std::array<char, 128> buffer{};
    const auto received = read(client, buffer.data(), buffer.size() - 1);
    std::string response = "error\n";
    if (received > 0 && handler_) {
      response = handler_(ParseCommand(
          std::string_view(buffer.data(), static_cast<std::size_t>(received))));
      response.push_back('\n');
    }
    [[maybe_unused]] const auto written =
        write(client, response.data(), response.size());
    close(client);
  }
#endif
}

}  // namespace vrealone::control
