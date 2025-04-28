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
                          ${CMAKE_CURRENT_BINARY_DIR} /usr/local/include /usr/local/include/dogecoin /usr/include/ykpers-1 ${CMAKE_SOURCE_DIR}/../../depends/x86_64-pc-linux-gnu/host/include/ ${CMAKE_SOURCE_DIR}/../../depends/x86_64-pc-linux-gnu/host/include/dogecoin/ ${CMAKE_SOURCE_DIR}/../../depends/x86_64-pc-linux-gnu/host/include/ykpers-1 ${CMAKE_SOURCE_DIR}/../../src/libevent/build/include)

# Add search paths to find the enclave libraries.
target_link_directories(host PRIVATE ${CMAKE_SOURCE_DIR}../../ ${CMAKE_SOURCE_DIR}/../../src/libevent/build/lib ${CMAKE_SOURCE_DIR}/../../depends/x86_64-pc-linux-gnu/host/lib)

target_link_libraries(host openenclave::oehost "libdogecoin.a" "libevent.a" "libunistring.a" "libykpers-1.so" "libyubikey.so" "libusb-1.0.so")
