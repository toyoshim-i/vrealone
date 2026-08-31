# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.24)

project(vrealone VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
find_package(ALSA REQUIRED)

option(VREALONE_BUILD_DRIVER "Build the SteamVR driver" ON)
option(VREALONE_ENABLE_SENSOR "Build XREAL One sensor support" ON)
set(VREALONE_OPENVR_SOURCE_DIR "" CACHE PATH "Path to a pinned OpenVR source tree")
set(VREALONE_XREAL_DRIVER_SOURCE_DIR "" CACHE PATH
    "Path to a pinned xreal_one_driver source tree")
set(VREALONE_FUSION_SOURCE_DIR "" CACHE PATH "Path to a pinned Fusion source tree")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(vrealone_core STATIC
  src/display_config.cpp
  src/control/control_server.cpp
  src/input/tap_detector.cpp
  src/platform/edid.cpp
  src/tracking/coordinate_transform.cpp
  src/tracking/pose_snapshot.cpp
  src/tracking/quaternion.cpp
  src/tracking/recenter.cpp
)
target_include_directories(vrealone_core PUBLIC src)
target_compile_options(vrealone_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(vrealone_core PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
)

add_executable(xrealone_probe tools/xrealone_probe.cpp)
target_link_libraries(xrealone_probe PRIVATE vrealone_core)
target_compile_options(xrealone_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(xrealone_ctl tools/xrealone_ctl.cpp)
target_link_libraries(xrealone_ctl PRIVATE vrealone_core)
target_compile_options(xrealone_ctl PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_library(vrealone_audio_input STATIC src/input/audio_tap_input.cpp)
target_link_libraries(vrealone_audio_input PUBLIC vrealone_core ALSA::ALSA)
target_include_directories(vrealone_audio_input PUBLIC src)
target_compile_options(vrealone_audio_input PRIVATE
  -Wall -Wextra -Wpedantic -Werror
)
set_target_properties(vrealone_audio_input PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(VREALONE_ENABLE_SENSOR)
  include(cmake/SensorDependency.cmake)
  include(cmake/FusionDependency.cmake)
  add_library(vrealone_fusion_tracker STATIC src/tracking/fusion_tracker.cpp)
  target_link_libraries(vrealone_fusion_tracker
    PUBLIC vrealone_core
    PUBLIC vrealone_fusion_upstream
  )
  target_include_directories(vrealone_fusion_tracker PUBLIC src)
  target_compile_options(vrealone_fusion_tracker PRIVATE
    -Wall -Wextra -Wpedantic -Werror
  )
  set_target_properties(vrealone_fusion_tracker PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )
  target_link_libraries(xrealone_probe PRIVATE
    vrealone_fusion_tracker
    xreal_one_driver
  )
  target_compile_definitions(xrealone_probe PRIVATE VREALONE_HAS_SENSOR=1)

  add_executable(xrealone_tap_probe tools/xrealone_tap_probe.cpp)
  target_link_libraries(xrealone_tap_probe PRIVATE
    vrealone_audio_input
    xreal_one_driver
  )
  target_compile_options(xrealone_tap_probe PRIVATE
    -Wall -Wextra -Wpedantic -Werror
  )
endif()

if(VREALONE_BUILD_DRIVER)
  include(cmake/Dependencies.cmake)

  add_library(driver_xrealone SHARED
    src/device_provider.cpp
    src/display_component.cpp
    src/driver_factory.cpp
    src/hmd_device.cpp
  )
  target_link_libraries(driver_xrealone PRIVATE
    vrealone_core
    vrealone_audio_input
  )
  if(VREALONE_ENABLE_SENSOR)
    target_link_libraries(driver_xrealone PRIVATE
      vrealone_fusion_tracker
      xreal_one_driver
    )
    target_compile_definitions(driver_xrealone PRIVATE VREALONE_HAS_SENSOR=1)
  endif()
  target_include_directories(driver_xrealone SYSTEM PRIVATE "${OPENVR_INCLUDE_DIR}")
  target_compile_definitions(driver_xrealone PRIVATE LINUX POSIX)
  target_compile_options(driver_xrealone PRIVATE
    -Wall -Wextra -Wpedantic -Werror -fvisibility=hidden
  )
  set_target_properties(driver_xrealone PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/xrealone/bin/linux64"
  )

  file(MAKE_DIRECTORY
    "${CMAKE_BINARY_DIR}/xrealone/resources/settings"
    "${CMAKE_BINARY_DIR}/xrealone/resources/input"
    "${CMAKE_BINARY_DIR}/xrealone/examples"
  )
  configure_file(
    resources/xrealone/driver.vrdrivermanifest
    "${CMAKE_BINARY_DIR}/xrealone/driver.vrdrivermanifest"
    COPYONLY
  )
  configure_file(
    resources/xrealone/resources/settings/default.vrsettings
    "${CMAKE_BINARY_DIR}/xrealone/resources/settings/default.vrsettings"
    COPYONLY
  )
  configure_file(
    resources/xrealone/resources/input/xrealone_hmd_profile.json
    "${CMAKE_BINARY_DIR}/xrealone/resources/input/xrealone_hmd_profile.json"
    COPYONLY
  )
  configure_file(
    resources/xrealone/resources/input/xrealone_dashboard_binding.json
    "${CMAKE_BINARY_DIR}/xrealone/resources/input/xrealone_dashboard_binding.json"
    COPYONLY
  )
  configure_file(
    resources/xrealone/examples/full-sbs.vrsettings
    "${CMAKE_BINARY_DIR}/xrealone/examples/full-sbs.vrsettings"
    COPYONLY
  )
endif()

if(BUILD_TESTING)
  add_executable(vrealone_tests
    tests/display_config_tests.cpp
    tests/edid_tests.cpp
    tests/main.cpp
    tests/coordinate_transform_tests.cpp
    tests/control_server_tests.cpp
    tests/pose_snapshot_tests.cpp
    tests/quaternion_tests.cpp
    tests/recenter_tests.cpp
    tests/tap_detector_tests.cpp
  )
  target_link_libraries(vrealone_tests PRIVATE vrealone_core)
  target_include_directories(vrealone_tests PRIVATE tests)
  target_compile_options(vrealone_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME vrealone_tests COMMAND vrealone_tests)
  if(VREALONE_ENABLE_SENSOR)
    target_sources(vrealone_tests PRIVATE tests/fusion_tracker_tests.cpp)
    target_link_libraries(vrealone_tests PRIVATE vrealone_fusion_tracker)
    target_compile_definitions(vrealone_tests PRIVATE VREALONE_HAS_FUSION=1)
  endif()
endif()
