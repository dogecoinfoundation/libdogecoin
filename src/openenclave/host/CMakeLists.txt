# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.
add_custom_command(
  OUTPUT libdogecoin_u.h libdogecoin_u.c libdogecoin_args.h
  DEPENDS ${CMAKE_SOURCE_DIR}/libdogecoin.edl
  COMMAND
    openenclave::oeedger8r --untrusted ${CMAKE_SOURCE_DIR}/libdogecoin.edl
    --search-path ${OE_INCLUDEDIR} --search-path
    ${OE_INCLUDEDIR}/openenclave/edl/sgx)

add_executable(host host.c
                               ${CMAKE_CURRENT_BINARY_DIR}/libdogecoin_u.c)

if (WIN32)
  copy_oedebugrt_target(host_oedebugrt)
  add_dependencies(host host_oedebugrt)
endif ()

target_include_directories(
  host PRIVATE # Needed for the generated file libdogecoin_u.h
                          ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(host openenclave::oehost)
