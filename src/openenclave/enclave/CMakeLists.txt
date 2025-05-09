# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

# Use the edger8r to generate C bindings from the EDL file.
add_custom_command(
  OUTPUT libdogecoin_t.h libdogecoin_t.c libdogecoin_args.h
  DEPENDS ${CMAKE_SOURCE_DIR}/libdogecoin.edl
  COMMAND
    openenclave::oeedger8r --trusted ${CMAKE_SOURCE_DIR}/libdogecoin.edl
    --search-path ${OE_INCLUDEDIR} --search-path
    ${OE_INCLUDEDIR}/openenclave/edl/sgx)

add_executable(enclave enc.c ${CMAKE_CURRENT_BINARY_DIR}/libdogecoin_t.c)

if (WIN32)
  maybe_build_using_clangw(enclave)
endif ()

target_compile_definitions(enclave PUBLIC OE_API_VERSION=2)

# Need for the generated file libdogecoin_t.h
target_include_directories(enclave PRIVATE /opt/openenclave/include ${CMAKE_CURRENT_BINARY_DIR} /usr/local/include/dogecoin ${CMAKE_SOURCE_DIR}/../../src/libevent/build/include ${CMAKE_SOURCE_DIR}/../../depends/x86_64-pc-linux-gnu/include/ ${CMAKE_SOURCE_DIR}/../../depends/x86_64-pc-linux-gnu/include/dogecoin)

# Add search paths to find the enclave libraries.
target_link_directories(enclave PRIVATE ${CMAKE_SOURCE_DIR}../../ ${CMAKE_SOURCE_DIR}/../../src/libevent/build/lib ${CMAKE_SOURCE_DIR}/../../depends/x86_64-pc-linux-gnu/lib /usr/lib/x86_64-linux-gnu)

if (LVI_MITIGATION MATCHES ControlFlow)
  # Helper to enable compiler options for LVI mitigation.
  apply_lvi_mitigation(enclave)
  # Link against LVI-mitigated libraries.
  target_link_libraries(
    enclave openenclave::oeenclave-lvi-cfg
    $<TARGET_OBJECTS:openenclave::oeseal_gcmaes>
    openenclave::oecrypto${OE_CRYPTO_LIB}-lvi-cfg
    "libdogecoin.a" "libevent_core.a" "libunistring.a"
    openenclave::oelibc-lvi-cfg)
else ()
  target_link_libraries(
    enclave openenclave::oeenclave
    $<TARGET_OBJECTS:openenclave::oeseal_gcmaes>
    openenclave::oecrypto${OE_CRYPTO_LIB}
    "libdogecoin.a" "libevent_core.a" "libunistring.a"
    openenclave::oelibc)

endif ()
