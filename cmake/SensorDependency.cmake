# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

set(VREALONE_XREAL_DRIVER_REVISION
    "3e912315ec16e94e9a47688770d649cfef293624"
    CACHE STRING "Reviewed xreal_one_driver revision")

if(VREALONE_XREAL_DRIVER_SOURCE_DIR)
  set(xreal_driver_SOURCE_DIR "${VREALONE_XREAL_DRIVER_SOURCE_DIR}")
else()
  FetchContent_Declare(xreal_driver
    GIT_REPOSITORY https://github.com/rohitsangwan01/xreal_one_driver.git
    GIT_TAG "${VREALONE_XREAL_DRIVER_REVISION}"
    GIT_SHALLOW FALSE
  )
  FetchContent_GetProperties(xreal_driver)
  if(NOT xreal_driver_POPULATED)
    FetchContent_Populate(xreal_driver)
  endif()
endif()

find_program(CARGO_EXECUTABLE cargo REQUIRED)
set(XREAL_CARGO_TARGET_DIR "${CMAKE_BINARY_DIR}/cargo")
if(WIN32)
  set(XREAL_RUST_TARGET "x86_64-pc-windows-msvc")
  set(XREAL_STATIC_LIBRARY
      "${XREAL_CARGO_TARGET_DIR}/${XREAL_RUST_TARGET}/release/xreal_one_driver.lib")
  set(XREAL_CARGO_TARGET_ARGS --target "${XREAL_RUST_TARGET}")
else()
  set(XREAL_STATIC_LIBRARY
      "${XREAL_CARGO_TARGET_DIR}/release/libxreal_one_driver.a")
  set(XREAL_CARGO_TARGET_ARGS)
endif()

add_custom_command(
  OUTPUT "${XREAL_STATIC_LIBRARY}"
  COMMAND "${CMAKE_COMMAND}" -E env
          "CARGO_TARGET_DIR=${XREAL_CARGO_TARGET_DIR}"
          "${CARGO_EXECUTABLE}" build --locked --release
          ${XREAL_CARGO_TARGET_ARGS}
          --manifest-path "${xreal_driver_SOURCE_DIR}/Cargo.toml"
  DEPENDS
    "${xreal_driver_SOURCE_DIR}/Cargo.toml"
    "${xreal_driver_SOURCE_DIR}/Cargo.lock"
    "${xreal_driver_SOURCE_DIR}/src/lib.rs"
  COMMENT "Building pinned xreal_one_driver Rust static library"
  VERBATIM
)
add_custom_target(xreal_one_driver_build DEPENDS "${XREAL_STATIC_LIBRARY}")
add_library(xreal_one_driver STATIC IMPORTED GLOBAL)
set_target_properties(xreal_one_driver PROPERTIES
  IMPORTED_LOCATION "${XREAL_STATIC_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${xreal_driver_SOURCE_DIR}/include"
)
add_dependencies(xreal_one_driver xreal_one_driver_build)
if(WIN32)
  target_link_libraries(xreal_one_driver INTERFACE
    ws2_32
    ntdll
    userenv
    bcrypt
    advapi32
  )
endif()
