# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

set(VREALONE_OPENVR_REVISION
    "0924064316de3effbcd1acf1e309182a2deb1c05"
    CACHE STRING "Reviewed OpenVR revision")

if(VREALONE_OPENVR_SOURCE_DIR)
  set(OPENVR_INCLUDE_DIR "${VREALONE_OPENVR_SOURCE_DIR}/headers")
else()
  FetchContent_Declare(openvr_headers
    GIT_REPOSITORY https://github.com/ValveSoftware/openvr.git
    GIT_TAG "${VREALONE_OPENVR_REVISION}"
    GIT_SHALLOW FALSE
  )
  FetchContent_GetProperties(openvr_headers)
  if(NOT openvr_headers_POPULATED)
    FetchContent_Populate(openvr_headers)
  endif()
  set(OPENVR_INCLUDE_DIR "${openvr_headers_SOURCE_DIR}/headers")
endif()

if(NOT EXISTS "${OPENVR_INCLUDE_DIR}/openvr_driver.h")
  message(FATAL_ERROR "openvr_driver.h was not found below ${OPENVR_INCLUDE_DIR}")
endif()
