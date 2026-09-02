# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

set(VREALONE_FUSION_REVISION
    "9325424011892abacc0ce42b8bb1a8ae20264b9b"
    CACHE STRING "Reviewed Fusion revision")

if(VREALONE_FUSION_SOURCE_DIR)
  set(fusion_SOURCE_DIR "${VREALONE_FUSION_SOURCE_DIR}")
else()
  FetchContent_Declare(fusion
    GIT_REPOSITORY https://github.com/xioTechnologies/Fusion.git
    GIT_TAG "${VREALONE_FUSION_REVISION}"
    GIT_SHALLOW FALSE
  )
  FetchContent_GetProperties(fusion)
  if(NOT fusion_POPULATED)
    FetchContent_Populate(fusion)
  endif()
endif()

add_library(vrealone_fusion_upstream STATIC
  "${fusion_SOURCE_DIR}/Fusion/FusionAhrs.c"
  "${fusion_SOURCE_DIR}/Fusion/FusionBias.c"
  "${fusion_SOURCE_DIR}/Fusion/FusionCompass.c"
  "${fusion_SOURCE_DIR}/Fusion/FusionConvention.c"
  "${fusion_SOURCE_DIR}/Fusion/FusionRemap.c"
)
target_include_directories(vrealone_fusion_upstream SYSTEM
  PUBLIC "${fusion_SOURCE_DIR}/Fusion"
)
if(NOT MSVC)
  target_link_libraries(vrealone_fusion_upstream PUBLIC m)
endif()
set_target_properties(vrealone_fusion_upstream PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
