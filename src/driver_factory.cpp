// SPDX-License-Identifier: Apache-2.0
#include <cstring>

#include <openvr_driver.h>

#include "device_provider.hpp"

#if defined(_WIN32)
#define VREALONE_EXPORT extern "C" __declspec(dllexport)
#else
#define VREALONE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

VREALONE_EXPORT void* HmdDriverFactory(const char* interface_name,
                                       int* return_code) {
  static vrealone::DeviceProvider provider;
  if (interface_name != nullptr &&
      std::strcmp(interface_name, vr::IServerTrackedDeviceProvider_Version) ==
          0) {
    if (return_code != nullptr) {
      *return_code = vr::VRInitError_None;
    }
    return &provider;
  }
  if (return_code != nullptr) {
    *return_code = vr::VRInitError_Init_InterfaceNotFound;
  }
  return nullptr;
}
