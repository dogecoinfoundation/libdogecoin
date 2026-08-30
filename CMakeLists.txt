CMAKE_MINIMUM_REQUIRED(VERSION 3.13) # 3.13: concise relative source paths
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")

SET(CMAKE_C_STANDARD 99)
SET(CMAKE_C_STANDARD_REQUIRED TRUE)

SET(LIBDOGECOIN_NAME dogecoin)

# Read the version out of configure.ac so it is written down in exactly one
# place.  PROJECT() only accepts numeric components, so the suffix is carried
# separately and appended to PACKAGE_VERSION below.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/configure.ac" _configure_ac)
foreach(_part MAJOR MINOR BUILD)
  if(NOT _configure_ac MATCHES "define\\(_PKG_VERSION_${_part}, ([0-9]+)\\)")
    message(FATAL_ERROR "cannot read _PKG_VERSION_${_part} from configure.ac")
  endif()
  set(_v_${_part} "${CMAKE_MATCH_1}")
endforeach()
if(_configure_ac MATCHES "define\\(_PKG_VERSION_IS_RELEASE, ([a-z]+)\\)" AND CMAKE_MATCH_1 STREQUAL "true")
  if(_configure_ac MATCHES "define\\(_PKG_VERSION_SUFFIX, \\[([^]]*)\\]\\)")
    set(_v_SUFFIX "${CMAKE_MATCH_1}")
  else()
    set(_v_SUFFIX "")
  endif()
else()
  set(_v_SUFFIX "-dev")
endif()
unset(_configure_ac)

PROJECT(lib${LIBDOGECOIN_NAME} VERSION "${_v_MAJOR}.${_v_MINOR}.${_v_BUILD}")
set(LIBDOGECOIN_VERSION "${PROJECT_VERSION}${_v_SUFFIX}")
message(STATUS "libdogecoin version: ${LIBDOGECOIN_VERSION}")

option(BUILD_SHARED_LIBS "Build the shared library" OFF)
if(BUILD_SHARED_LIBS)
  set(library_type "Shared")
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  SET(CMAKE_SUPPORT_WINDOWS_EXPORT_ALL_SYMBOLS 1)
  set(visibility PUBLIC)
else()
  set(library_type "Static")
  set(visibility PRIVATE)
endif()

INCLUDE(CTest)
SET(USE_TESTS ${CMAKE_TESTING_ENABLED})
SET(WITH_BENCH TRUE CACHE BOOL "enable bench")
SET(WITH_TOOLS TRUE CACHE BOOL "enable dogecoin tool cli application")
SET(WITH_NET TRUE CACHE BOOL "enable net functions")
SET(WITH_LOGDB TRUE CACHE BOOL "enable logdb")
SET(WITH_WALLET TRUE CACHE BOOL "enable wallet")
SET(USE_AVX2 FALSE CACHE BOOL "enable intel avx2")
SET(USE_SSE FALSE CACHE BOOL "enable intel sse")
SET(USE_SSE4 FALSE CACHE BOOL "enable intel sse4")
SET(USE_SSE2 FALSE CACHE BOOL "enable scrypt sse2")
SET(USE_ARMV8 FALSE CACHE BOOL "enable armv8 crypto")
SET(USE_ARMV82 FALSE CACHE BOOL "enable armv8.2 crypto")
SET(USE_DIT FALSE CACHE BOOL "enable data independent timing support")
SET(USE_TPM2 TRUE CACHE BOOL "enable tpm2")
SET(TEST_TPM_AUTO FALSE CACHE BOOL "disable TPM UI policy for automated tests")
SET(USE_TSS2 FALSE CACHE BOOL "enable tss2 (Linux TPM2) functions")
SET(USE_OPENENCLAVE FALSE CACHE BOOL "enable openenclave")
SET(USE_YUBIKEY FALSE CACHE BOOL "enable yubikey")
SET(USE_LIBOQS FALSE CACHE BOOL "enable liboqs")
SET(USE_RACCOON_G FALSE CACHE BOOL "enable in-tree Raccoon-G-44 (experimental, requires GMP/MPFR)")
# Default OFF to match the autoconf default (--enable-zk-carrier defaults to no
# in configure.ac); flip both knobs together when the on-chain semantics are
# frozen so the two build systems produce equivalent libraries.
SET(USE_ZK_CARRIER FALSE CACHE BOOL "enable ZK carrier (Groth16/PLONK/STARK)")
SET(USE_RAPIDSNARK FALSE CACHE BOOL "link rapidsnark Groth16 verifier")
string(RANDOM LENGTH 12 ALPHABET abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 PASSWD_STR)
SET(RANDOM_DEVICE "/dev/urandom" CACHE STRING "set the device to read random data from")

# Set a default build type if none was specified
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    MESSAGE(STATUS "git dir ${CMAKE_SOURCE_DIR}")
    set(default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

IF(WITH_NET)
    IF(WIN32)
        # build libevent
        execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/src/libevent/build)
        execute_process(COMMAND cmake ..
            -DEVENT__DISABLE_OPENSSL=ON
            -DEVENT__DISABLE_SAMPLES=ON
            -DEVENT__LIBRARY_TYPE=STATIC
            -DEVENT__DISABLE_TESTS=ON
            -DEVENT__DISABLE_THREAD_SUPPORT=OFF
            -DEVENT__DISABLE_MM_REPLACEMENT=ON
            -DEVENT__DISABLE_REGRESS=ON
            -DEVENT__DISABLE_BENCHMARK=ON
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/libevent/build)
        execute_process(COMMAND cmake --build . --config ${CMAKE_BUILD_TYPE}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/libevent/build)
        file(COPY "${PROJECT_SOURCE_DIR}/src/libevent/build/include" DESTINATION "${PROJECT_SOURCE_DIR}")
        file(COPY "${PROJECT_SOURCE_DIR}/src/libevent/include/event2" DESTINATION "${PROJECT_SOURCE_DIR}/include")
        find_path(LIBEVENT_INCLUDE_DIR event.h PATHS "${PROJECT_SOURCE_DIR}/include/event2" REQUIRED)
        find_path(LIBEVENT_INCLUDE_DIR event-config.h PATHS "${PROJECT_SOURCE_DIR}/include/event2" REQUIRED)
        FIND_LIBRARY(LIBEVENT NAMES event HINTS "${PROJECT_SOURCE_DIR}/src/libevent/build/lib/${CMAKE_BUILD_TYPE}" REQUIRED)
        SET(LIBEVENT_PTHREADS "")
        FIND_LIBRARY(WINPTHREADS NAMES pthreadVC2 pthreadwin32 pthreads pthread HINTS "${PROJECT_SOURCE_DIR}/contrib/winpthreads/include/lib/x64/" REQUIRED)
    ELSE()
        # build libevent
        execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/src/libevent/build)
        execute_process(COMMAND cmake ..
            -DCMAKE_C_FLAGS=-U_FORTIFY_SOURCE
            -DEVENT__DISABLE_OPENSSL=ON
            -DEVENT__DISABLE_SAMPLES=ON
            -DEVENT__LIBRARY_TYPE=STATIC
            -DEVENT__DISABLE_TESTS=ON
            -DEVENT__DISABLE_THREAD_SUPPORT=OFF
            -DEVENT__DISABLE_MM_REPLACEMENT=ON
            -DEVENT__DISABLE_REGRESS=ON
            -DEVENT__DISABLE_BENCHMARK=ON
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/libevent/build)
        execute_process(COMMAND cmake --build .
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/libevent/build)
        file(COPY "${PROJECT_SOURCE_DIR}/src/libevent/build/include" DESTINATION "${PROJECT_SOURCE_DIR}")
        file(COPY "${PROJECT_SOURCE_DIR}/src/libevent/include/event2" DESTINATION "${PROJECT_SOURCE_DIR}/include")
        find_path(LIBEVENT_INCLUDE_DIR event.h PATHS "${PROJECT_SOURCE_DIR}/include/event2" REQUIRED)
        find_path(LIBEVENT_INCLUDE_DIR event-config.h PATHS "${PROJECT_SOURCE_DIR}/include/event2" REQUIRED)
        # Multi-config generators (MSVC) put the library under lib/<CONFIG>;
        # single-config ones (Makefiles, Ninja) put it in lib/ directly. Search
        # both, or the Linux build cannot find the libevent it just built and
        # silently depends on a system copy being installed instead.
        FIND_LIBRARY(LIBEVENT NAMES event
            HINTS "${PROJECT_SOURCE_DIR}/src/libevent/build/lib/${CMAKE_BUILD_TYPE}"
                  "${PROJECT_SOURCE_DIR}/src/libevent/build/lib"
            REQUIRED)
        FIND_LIBRARY(LIBEVENT_PTHREADS NAMES event_pthreads
            HINTS "${PROJECT_SOURCE_DIR}/src/libevent/build/lib/${CMAKE_BUILD_TYPE}"
                  "${PROJECT_SOURCE_DIR}/src/libevent/build/lib"
            REQUIRED)
    ENDIF()
ENDIF()

# The TSS2 (Linux TPM2) seal backend does not depend on net, so detect it
# independently of WITH_NET. This lets the seal API be built without net
# (e.g. a TPM-only configuration), matching the autotools behaviour where
# --enable-tss2 is handled separately from --enable-net.
IF(USE_TSS2 AND NOT WIN32)
    # Minimum recommended libtss2-esys version: 2.4.0
    # Prefer pkg-config (handles transitive deps and version checking),
    # but fall back to a plain library lookup so builds work even when
    # pkg-config metadata for tss2-esys is missing.
    FIND_PACKAGE(PkgConfig QUIET)
    IF(PkgConfig_FOUND)
        PKG_CHECK_MODULES(TSS2_ESYS QUIET tss2-esys>=2.4.0)
    ENDIF()
    IF(TSS2_ESYS_FOUND)
        SET(LIBTSS2 ${TSS2_ESYS_LIBRARIES})
        INCLUDE_DIRECTORIES(${TSS2_ESYS_INCLUDE_DIRS})
    ELSE()
        FIND_LIBRARY(LIBTSS2 tss2-esys)
        IF(NOT LIBTSS2)
            MESSAGE(FATAL_ERROR
                "USE_TSS2 requested but libtss2-esys was not found. "
                "Install libtss2-dev (>= 2.4.0) or disable USE_TSS2.")
        ENDIF()
    ENDIF()
    FIND_PATH(TSS2_ESYS_HEADER tss2/tss2_esys.h)
    IF(NOT TSS2_ESYS_HEADER)
        MESSAGE(FATAL_ERROR
            "tss2/tss2_esys.h not found; install libtss2-dev.")
    ENDIF()
ENDIF()

# NASM integration for AVX2/SSE assembly
if(USE_AVX2 OR USE_SSE)
    # Prefer depends-built nasm when present, so the build does not rely on nasm
    # being on $PATH. HOST may be passed in by callers; otherwise also scan the
    # usual depends/*/native/bin outputs.
    set(LIBDOGECOIN_NASM_HINTS
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/native/bin")
    if(DEFINED HOST AND NOT "${HOST}" STREQUAL "")
        list(PREPEND LIBDOGECOIN_NASM_HINTS
            "${CMAKE_CURRENT_SOURCE_DIR}/depends/${HOST}/native/bin")
    endif()
    file(GLOB LIBDOGECOIN_DEPENDS_NATIVE_BIN_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/depends/*/native/bin")
    list(APPEND LIBDOGECOIN_NASM_HINTS ${LIBDOGECOIN_DEPENDS_NATIVE_BIN_DIRS})

    if(CMAKE_ASM_NASM_COMPILER AND NOT EXISTS "${CMAKE_ASM_NASM_COMPILER}")
        unset(CMAKE_ASM_NASM_COMPILER CACHE)
        unset(LIBDOGECOIN_NASM_COMPILER CACHE)
    elseif(NOT CMAKE_ASM_NASM_COMPILER AND LIBDOGECOIN_NASM_COMPILER AND NOT EXISTS "${LIBDOGECOIN_NASM_COMPILER}")
        unset(CMAKE_ASM_NASM_COMPILER CACHE)
        unset(LIBDOGECOIN_NASM_COMPILER CACHE)
    endif()

    if(NOT CMAKE_ASM_NASM_COMPILER OR NOT EXISTS "${CMAKE_ASM_NASM_COMPILER}")
        find_program(LIBDOGECOIN_NASM_COMPILER
            NAMES nasm
            HINTS ${LIBDOGECOIN_NASM_HINTS}
        )
        if(LIBDOGECOIN_NASM_COMPILER)
            set(CMAKE_ASM_NASM_COMPILER "${LIBDOGECOIN_NASM_COMPILER}" CACHE FILEPATH "NASM assembler" FORCE)
        endif()
    endif()
    if(NOT CMAKE_ASM_NASM_COMPILER)
        message(FATAL_ERROR "Can't find NASM assembler")
    endif()

    enable_language(ASM_NASM)
    if(NOT CMAKE_ASM_NASM_COMPILER_LOADED)
        message(FATAL_ERROR "Can't find NASM assembler")
    endif()

    set(NASM_VERSION_REQUIRED "2.14")
    execute_process(
        COMMAND ${CMAKE_ASM_NASM_COMPILER} --version
        OUTPUT_VARIABLE NASM_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "NASM version ([0-9]*.[0-9]*)" NASM_VERSION "${NASM_VERSION_OUTPUT}")
    if(NOT NASM_VERSION OR NASM_VERSION_REQUIRED VERSION_GREATER ${CMAKE_MATCH_1})
        message(FATAL_ERROR "NASM version must be at least ${NASM_VERSION_REQUIRED}!")
    endif()

    # Define directories with assembly files (modify paths as needed)
    set(DIR_ASM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intel)

    # Define directories with include files (modify paths as needed)
    set(DIR_ASM_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/intel)

    # Find all assembly files in the defined directory
    file(GLOB_RECURSE ASM_SOURCES "${DIR_ASM_SOURCES}/*.asm")

    # Add include directories to the NASM compiler
    set_property(SOURCE ${ASM_SOURCES} PROPERTY INCLUDE_DIRECTORIES ${DIR_ASM_INCLUDES})

endif()

ADD_DEFINITIONS(
    -DPACKAGE_NAME="${PROJECT_NAME}"
    -DPACKAGE_VERSION="${LIBDOGECOIN_VERSION}"
    -DRANDOM_DEVICE="${RANDOM_DEVICE}"
)
IF(BUILD_SHARED_LIBS)
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

IF(USE_TESTS)
    ADD_DEFINITIONS(-DUSE_TESTS=1)
ENDIF()
IF(WITH_BENCH)
    ADD_DEFINITIONS(-DWITH_BENCH=1)
ENDIF()
IF(WITH_TOOLS)
    ADD_DEFINITIONS(-DWITH_TOOLS=1)
ENDIF()
IF(WITH_NET)
    ADD_DEFINITIONS(-DWITH_NET=1)
ENDIF()
IF(WITH_LOGDB)
    ADD_DEFINITIONS(-DWITH_LOGDB=1)
ENDIF()
IF(WITH_WALLET)
    ADD_DEFINITIONS(-DWITH_WALLET=1)
ENDIF()
IF(USE_AVX2)
    ADD_DEFINITIONS(-DUSE_AVX2=1)
ENDIF()
IF(USE_SSE)
    ADD_DEFINITIONS(-DUSE_SSE=1)
ENDIF()
IF(USE_SSE4)
    ADD_DEFINITIONS(-DUSE_SSE4=1)
ENDIF()
IF(USE_SSE2)
    ADD_DEFINITIONS(-DUSE_SSE2=1)
ENDIF()
IF(USE_ARMV8)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+crypto")

    ADD_DEFINITIONS(-DUSE_ARMV8=1)
ENDIF()
IF(USE_ARMV82)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8.2-a+crypto+sha3")
    message(STATUS "Checking whether to build with armv8.2-a crypto")

    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS "-march=armv8.2-a+crypto+sha3")
    check_cxx_source_compiles("
        #include <arm_neon.h>
        int main() {
            uint64x2_t x;
            x = vsha512su0q_u64(x, x);
            return 0;
        }
    " ARMV82_CRYPTO_WORKS)

    if (ARMV82_CRYPTO_WORKS)
        add_definitions(-DUSE_ARMV82)
        message(STATUS "yes")
    else()
        message(WARNING "sha512 missing, using armv8-a crypto without sha512")
        string(REPLACE "-march=armv8.2-a+crypto+sha3" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+crypto")
    endif()

    ADD_DEFINITIONS(-DUSE_ARMV82=1)
ENDIF()
IF(USE_DIT)
    ADD_DEFINITIONS(-DUSE_DIT=1)
ENDIF()
IF(USE_TPM2)
    ADD_DEFINITIONS(-DUSE_TPM2=1)
ENDIF()
IF(USE_TSS2)
    ADD_DEFINITIONS(-DUSE_TSS2=1)
ENDIF()
IF(USE_OPENENCLAVE)
    ADD_DEFINITIONS(-DUSE_OPENENCLAVE=1)
ENDIF()
IF(USE_YUBIKEY)
    FIND_LIBRARY(LIBYKPIV NAMES ykpiv REQUIRED HINTS /usr/lib/x86_64-linux-gnu)
    ADD_DEFINITIONS(-DUSE_YUBIKEY=1)
ENDIF()
IF(USE_LIBOQS)
    ADD_DEFINITIONS(-DUSE_LIBOQS=1)
    FIND_LIBRARY(LIBOQS NAMES oqs REQUIRED)
ENDIF()
IF(USE_RACCOON_G)
    FIND_LIBRARY(LIBGMP NAMES gmp REQUIRED)
    FIND_LIBRARY(LIBMPFR NAMES mpfr REQUIRED)
    ADD_DEFINITIONS(-DUSE_RACCOON_G=1)
ENDIF()
IF(USE_ZK_CARRIER)
    ADD_DEFINITIONS(-DUSE_ZK_CARRIER=1)
ENDIF()
IF(USE_RAPIDSNARK)
    ADD_DEFINITIONS(-DHAVE_RAPIDSNARK=1)
ENDIF()
IF(USE_TESTS)
    ADD_DEFINITIONS(-DPASSWD_STR="${PASSWD_STR}")
ENDIF()

MESSAGE(STATUS "")
MESSAGE(STATUS "Options used to compile and link:")
MESSAGE(STATUS "  USE_TESTS      = ${USE_TESTS}")
MESSAGE(STATUS "  WITH_BENCH     = ${WITH_BENCH}")
MESSAGE(STATUS "  WITH_TOOLS     = ${WITH_TOOLS}")
MESSAGE(STATUS "  WITH_NET       = ${WITH_NET}")
MESSAGE(STATUS "  WITH_LOGDB     = ${WITH_LOGDB}")
MESSAGE(STATUS "  WITH_WALLET    = ${WITH_WALLET}")
MESSAGE(STATUS "")
MESSAGE(STATUS "  USE_AVX2       = ${USE_AVX2}")
MESSAGE(STATUS "  USE_SSE        = ${USE_SSE}")
MESSAGE(STATUS "  USE_SSE4       = ${USE_SSE4}")
MESSAGE(STATUS "  USE_SSE2       = ${USE_SSE2}")
MESSAGE(STATUS "  USE_ARMV8      = ${USE_ARMV8}")
MESSAGE(STATUS "  USE_ARMV82     = ${USE_ARMV82}")
MESSAGE(STATUS "  USE_DIT        = ${USE_DIT}")
MESSAGE(STATUS "  USE_TPM2       = ${USE_TPM2}")
MESSAGE(STATUS "  TEST_TPM_AUTO  = ${TEST_TPM_AUTO}")
MESSAGE(STATUS "  USE_TSS2       = ${USE_TSS2}")
MESSAGE(STATUS "")
IF(USE_TESTS)
MESSAGE(STATUS "  test password  = ${PASSWD_STR}")
ENDIF()
MESSAGE(STATUS "")
MESSAGE(STATUS "  openenclave    = ${USE_OPENENCLAVE}")
MESSAGE(STATUS "  yubikey        = ${USE_YUBIKEY}")
MESSAGE(STATUS "  liboqs         = ${USE_LIBOQS}")
MESSAGE(STATUS "  raccoon_g      = ${USE_RACCOON_G} (experimental, in-tree)")
MESSAGE(STATUS "  zk_carrier     = ${USE_ZK_CARRIER}")
MESSAGE(STATUS "  rapidsnark     = ${USE_RAPIDSNARK}")
MESSAGE(STATUS "")
message(STATUS "  library type   = ${library_type}")
MESSAGE(STATUS "")

FILE(TOUCH config/libdogecoin-config.h)

ADD_LIBRARY(${LIBDOGECOIN_NAME} ${ASM_SOURCES})
IF(WIN32 AND NOT BUILD_SHARED_LIBS)
    TARGET_COMPILE_DEFINITIONS(${LIBDOGECOIN_NAME} PRIVATE UTF8PROC_STATIC)
ENDIF()
INSTALL(TARGETS
    ${LIBDOGECOIN_NAME}
    LIBRARY DESTINATION lib
)

INSTALL(FILES
    include/dogecoin/constants.h
    include/dogecoin/dogecoin.h
    include/dogecoin/libdogecoin.h
    include/dogecoin/uthash.h
    include/dogecoin/cstr.h
    config/libdogecoin-config.h
    DESTINATION include/dogecoin
)

TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    src/address.c
    src/aes.c
    src/arith_uint256.c
    src/auxpow.c
    src/base58.c
    src/bip32.c
    src/bip38.c
    src/bip39.c
    src/bip44.c
    src/block.c
    src/buffer.c
    src/chacha20.c
    src/compact_filter.c
    src/context.c
    src/compact_block.c
    src/cstr.c
    src/ctaes.c
    src/ecc.c
    src/eckey.c
    src/golomb.c
    src/key.c
    src/koinu.c
    src/map.c
    src/mem.c
    src/moon.c
    src/qr.c
    src/qrengine.c
    src/pow.c
    src/png.c
    src/jpeg.c
    src/psbt.c
    src/random.c
    src/rmd160.c
    src/script.c
    src/scrypt.c
    src/seal.c
    src/slip0039.c
    src/serialize.c
    src/sha2.c
    src/sign.c
    src/sweep.c
    src/smpv.c
    src/cli/tool.c
    src/transaction.c
    src/tx.c
    src/utf8proc.c
    src/utils.c
    src/validation.c
    src/vector.c
)
IF(USE_LIBOQS)
TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    src/pqc_dilithium.c
    src/pqc_falcon.c
)
ENDIF()
# pqc_carrier.c provides the P2SH carrier helpers shared by the PQ and ZK
# carriers; build it whenever either is enabled.  pqc_falcon.c (Falcon-512
# helpers) is only needed when LIBOQS is on — the ZK carrier no longer
# depends on it now that dogecoin_tx_sighash32 lives in src/tx.c.
IF(USE_LIBOQS OR USE_ZK_CARRIER)
TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    src/pqc_carrier.c
)
ENDIF()
IF(USE_RACCOON_G)
TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    src/pqc_raccoon.c
    src/raccoon_g/polyr.c
    src/raccoon_g/ntt.c
    src/raccoon_g/gaussian.c
    src/raccoon_g/shake256.c
    src/raccoon_g/thrc.c
    src/raccoon_g/keygen_kdf.c
    src/raccoon_g/raccoong.c
)
TARGET_LINK_LIBRARIES(${LIBDOGECOIN_NAME} ${LIBMPFR} ${LIBGMP})
ENDIF()
IF(USE_ZK_CARRIER)
TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    src/zk_carrier/zk_carrier.c
    src/zk_carrier/zk_commit.c
    src/zk_carrier/zk_groth16.c
    src/zk_carrier/zk_plonk.c
    src/zk_carrier/zk_stark.c
)
IF(USE_RAPIDSNARK)
    TARGET_LINK_LIBRARIES(${LIBDOGECOIN_NAME} PRIVATE rapidsnark)
ENDIF()
ENDIF()
TARGET_SOURCES(${LIBDOGECOIN_NAME} PUBLIC
    src/chainparams.c
    # PUBLIC for the same reason as chainparams: these are const data
    # referenced from the CLI targets, and a PRIVATE source is compiled only
    # into the library. On a Windows DLL build the executables then cannot
    # reach the arrays -- data symbols do not resolve across the DLL boundary
    # without dllimport, and LIBDOGECOIN_API expands to nothing for consumers.
    src/cf_checkpoints.c
)

IF(WIN32 AND USE_TPM2)
    TARGET_LINK_LIBRARIES(${LIBDOGECOIN_NAME} tbs ncrypt crypt32)
ENDIF()

# Link liboqs into the core library when enabled. pqc_dilithium.c / pqc_falcon.c
# call OQS_SIG_* directly, so without this link any executable that pulls in the
# PQC wrappers (the test runner, such, spvnode) fails to link. The autotools
# build adds -loqs to LIBS in configure.ac; this mirrors that for CMake.
IF(USE_LIBOQS)
    TARGET_LINK_LIBRARIES(${LIBDOGECOIN_NAME} ${LIBOQS})
ENDIF()

# Link the Linux TPM2 (TSS2) backend into the core library regardless of net,
# so the seal API works in a net-less build. When WITH_NET is enabled the net
# link rule below also adds ${LIBTSS2}; linking the same library twice is
# harmless.
IF(USE_TSS2)
    TARGET_LINK_LIBRARIES(${LIBDOGECOIN_NAME} ${LIBTSS2})
ENDIF()

IF(USE_AVX2)
    TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE src/crypto/sha256_avx2.c)
    SET_SOURCE_FILES_PROPERTIES(src/crypto/sha256_avx2.c PROPERTIES COMPILE_OPTIONS "-mavx2")
ENDIF()
IF(USE_SSE4)
    TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE src/crypto/sha256_sse41.c)
    SET_SOURCE_FILES_PROPERTIES(src/crypto/sha256_sse41.c PROPERTIES COMPILE_OPTIONS "-msse4.1")
    TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE src/crypto/sha256_sse4.c)
    SET_SOURCE_FILES_PROPERTIES(src/crypto/sha256_sse4.c PROPERTIES COMPILE_OPTIONS "-msse4.1")
ENDIF()

IF(USE_SSE2)
TARGET_SOURCES(${LIBDOGECOIN_NAME} ${visibility}
    src/scrypt-sse2.c
)
ENDIF()

IF(WIN32)
TARGET_SOURCES(${LIBDOGECOIN_NAME} ${visibility}
    contrib/getopt/wingetopt.c
    contrib/wintime/wingettime.c
)
ENDIF()

FILE(GLOB SECP256K1 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    src/secp256k1/src/*.c)
LIST(REMOVE_ITEM SECP256K1
    src/secp256k1/src/tests.c
    src/secp256k1/src/tests_exhaustive.c
    src/secp256k1/src/bench.c
    src/secp256k1/src/bench.h
    src/secp256k1/src/bench_ecdh.c
    src/secp256k1/src/bench_ecmult.c
    src/secp256k1/src/bench_recover.c
    src/secp256k1/src/bench_sign.c
    src/secp256k1/src/bench_verify.c
    src/secp256k1/src/bench_ecdh.c
    src/secp256k1/src/bench_internal.c
    src/secp256k1/src/bench_schnorr_verify.c
    src/secp256k1/src/valgrind_ctime_test.c
    src/secp256k1/src/precompute_ecmult.c
    src/secp256k1/src/precompute_ecmult_gen.c)
ADD_DEFINITIONS(
    -DUSE_NUM_GMP
    -DUSE_FIELD_10X26
    -DUSE_FIELD_INV_BUILTIN
    -DUSE_SCALAR_8X32
    -DUSE_SCALAR_INV_BUILTIN
    -DENABLE_MODULE_RECOVERY
    -DECMULT_WINDOW_SIZE=15
    -DECMULT_GEN_PREC_BITS=4)
TARGET_SOURCES(${LIBDOGECOIN_NAME} ${visibility} ${SECP256K1})

INCLUDE_DIRECTORIES(
    include
    src/logdb/include
    src/secp256k1
    src/secp256k1/include
    ${GMP_INSTALL_DIR}/include)

IF(USE_TESTS)
    ADD_EXECUTABLE(tests)
    TARGET_SOURCES(tests ${visibility}
        test/address_tests.c
        test/aes_tests.c
        test/arith_uint256_tests.c
        test/base58_tests.c
        test/bip32_tests.c
        test/bip39_tests.c
        test/bip44_tests.c
        test/block_tests.c
        test/buffer_tests.c
        test/cf_checkpoints_tests.c
        test/chacha20_tests.c
        test/chainparams_abi_tests.c
        test/sighash_surface_tests.c
        test/checkpoints_tests.c
        test/compact_block_tests.c
        test/context_tests.c
        test/cstr_tests.c
        test/compact_filter_tests.c
        test/golomb_tests.c
        test/ecc_tests.c
        test/hash_tests.c
        test/key_tests.c
        test/koinu_tests.c
        test/map_tests.c
        test/mem_tests.c
        test/moon_tests.c
        test/opreturn_tests.c
        test/psbt_tests.c
        test/rfc6979_tests.c
        test/rng_entropy_tests.c
        test/qr_tests.c
        test/random_tests.c
        test/rmd160_tests.c
        test/scrypt_tests.c
        test/serialize_tests.c
        test/sweep_tests.c
        test/sha1_tests.c
        test/sha2_tests.c
        test/signmsg_tests.c
        test/slip0039_tests.c
        test/smpv_tests.c
        test/tpm_tests.c
        test/transaction_tests.c
        test/validation_tests.c
        test/tx_tests.c
        include/test/utest.h
        test/unittester.c
        test/utils_tests.c
        test/vector_tests.c
)
IF(USE_ZK_CARRIER)
TARGET_SOURCES(tests ${visibility}
        test/zk_carrier_tests.c
)
ENDIF()
IF(USE_RACCOON_G)
TARGET_SOURCES(tests ${visibility}
        test/raccoong_polyr_tests.c
        test/raccoong_ntt_tests.c
        test/raccoong_shake_tests.c
        test/raccoong_xof_sample_q_tests.c
        test/raccoong_matvec_tests.c
        test/raccoong_keygen_t_tests.c
        test/raccoong_keypair_tests.c
        test/raccoong_gaussian_tests.c
        test/raccoong_hd_derive_tests.c
        test/raccoong_signature_serialize_tests.c
        test/raccoong_buff_mu_tests.c
        test/raccoong_chal_poly_tests.c
        test/raccoong_hash_vec_tests.c
        test/raccoong_sign_tests.c
)
TARGET_INCLUDE_DIRECTORIES(tests ${visibility} ${CMAKE_SOURCE_DIR})
ENDIF()
IF (USE_YUBIKEY)
    SET(LIBS ${LIBDOGECOIN_NAME} ${LIBYKPIV})
ELSE()
    SET(LIBS ${LIBDOGECOIN_NAME})
ENDIF()

IF (WIN32 AND USE_TPM2)
    IF (TEST_TPM_AUTO)
        TARGET_COMPILE_DEFINITIONS(tests PRIVATE TEST_TPM_AUTO=1)
    ENDIF()
    TARGET_LINK_LIBRARIES(tests ${LIBS} tbs ncrypt crypt32)
ELSEIF (USE_TSS2)
    TARGET_LINK_LIBRARIES(tests ${LIBS} ${LIBTSS2})
ELSE()
    TARGET_LINK_LIBRARIES(tests ${LIBS})
ENDIF()
IF(USE_RACCOON_G)
    TARGET_LINK_LIBRARIES(tests m ${LIBMPFR} ${LIBGMP})
ENDIF()
    ADD_TEST(NAME ${LIBDOGECOIN_NAME}_tests COMMAND tests)
    # move test/wordlist to build
    FILE(GLOB_RECURSE WORDLISTS "${PROJECT_SOURCE_DIR}/test/wordlist/*.txt")
    FOREACH(wordlist ${WORDLISTS})
        GET_FILENAME_COMPONENT(wordlist_name ${wordlist} NAME)
        CONFIGURE_FILE(${wordlist} "${CMAKE_CURRENT_BINARY_DIR}/test/wordlist/${wordlist_name}" COPYONLY)
    ENDFOREACH()
ENDIF()

IF (WITH_BENCH)
    ADD_EXECUTABLE(bench)
    TARGET_SOURCES(bench ${visibility}
        src/bench.c
    )
    TARGET_LINK_LIBRARIES(bench ${LIBDOGECOIN_NAME})
ENDIF()

IF(WITH_LOGDB)
    TARGET_SOURCES(${LIBDOGECOIN_NAME} ${visibility}
        src/logdb/logdb_core.c
        src/logdb/logdb_memdb_llist.c
        src/logdb/logdb_memdb_rbtree.c
        src/logdb/logdb_rec.c
        src/logdb/misc.c
        src/logdb/red_black_tree.c
        src/logdb/stack.c
    )
    IF(USE_TESTS)
        TARGET_SOURCES(tests ${visibility}
            src/logdb/test/logdb_tests.c
            src/logdb/test/tests_red_black_tree.c
            src/logdb/test/logdb_tests_sample.h
        )
    ENDIF()
ENDIF()

IF(WITH_WALLET)
    INSTALL(FILES
        include/dogecoin/wallet.h
        DESTINATION include/dogecoin
    )
    TARGET_SOURCES(${LIBDOGECOIN_NAME} ${visibility}
        src/wallet.c
    )
    IF(USE_TESTS)
        TARGET_SOURCES(tests ${visibility}
            test/wallet_tests.c
        )
    ENDIF()
ENDIF()

IF(WITH_NET)
    INSTALL(FILES
        include/dogecoin/rest.h
        DESTINATION include/dogecoin
    )
    TARGET_SOURCES(${LIBDOGECOIN_NAME} ${visibility}
        src/bip37.c
        src/cfheadersdb_file.c
        src/headersdb_file.c
        src/net.c
        src/protocol.c
        src/rest.c
        src/spv.c
    )

    IF(WIN32 AND USE_TPM2)
        TARGET_LINK_LIBRARIES(${LIBS} ${LIBEVENT} tbs ncrypt crypt32)
    ELSEIF(USE_TSS2)
        TARGET_LINK_LIBRARIES(${LIBS} ${LIBEVENT} ${LIBEVENT_PTHREADS} ${LIBTSS2})
    ELSE()
        TARGET_LINK_LIBRARIES(${LIBS} ${LIBEVENT} ${LIBEVENT_PTHREADS})
    ENDIF()
    IF(USE_TESTS)
        TARGET_SOURCES(tests ${visibility}
            test/cfheadersdb_tests.c
            test/net_tests.c
            test/protocol_tests.c
            test/spv_tests.c
        )
    ENDIF()
ENDIF()

IF(WITH_TOOLS)
    IF(USE_TESTS)
        TARGET_SOURCES(tests ${visibility}
            test/tool_tests.c
        )
    ENDIF()

    ADD_EXECUTABLE(such src/cli/such.c)
    INSTALL(TARGETS such RUNTIME)
    IF (WIN32 AND USE_TPM2)
        TARGET_LINK_LIBRARIES(such ${LIBS} tbs ncrypt crypt32)
    ELSEIF (USE_TSS2)
        TARGET_LINK_LIBRARIES(such ${LIBS} ${LIBTSS2})
    ELSE()
        TARGET_LINK_LIBRARIES(such ${LIBS})
    ENDIF()
    TARGET_INCLUDE_DIRECTORIES(such ${visibility} src)

    # Thread-safe variant: same sources, DOGECOIN_TS routes through the `_ts`
    # library APIs (see doc/thread_safety.md).
    ADD_EXECUTABLE(such_ts src/cli/such.c)
    INSTALL(TARGETS such_ts RUNTIME)
    IF (WIN32 AND USE_TPM2)
        TARGET_LINK_LIBRARIES(such_ts ${LIBS} tbs ncrypt crypt32)
    ELSEIF (USE_TSS2)
        TARGET_LINK_LIBRARIES(such_ts ${LIBS} ${LIBTSS2})
    ELSE()
        TARGET_LINK_LIBRARIES(such_ts ${LIBS})
    ENDIF()
    TARGET_INCLUDE_DIRECTORIES(such_ts ${visibility} src)
    TARGET_COMPILE_DEFINITIONS(such_ts ${visibility} DOGECOIN_TS=1)

    IF(WITH_NET)
        ADD_EXECUTABLE(sendtx src/cli/sendtx.c)
        INSTALL(TARGETS sendtx RUNTIME)
        TARGET_LINK_LIBRARIES(sendtx ${LIBDOGECOIN_NAME})
        TARGET_INCLUDE_DIRECTORIES(sendtx ${visibility} src)

        ADD_EXECUTABLE(sendtx_ts src/cli/sendtx.c)
        INSTALL(TARGETS sendtx_ts RUNTIME)
        TARGET_LINK_LIBRARIES(sendtx_ts ${LIBDOGECOIN_NAME})
        TARGET_INCLUDE_DIRECTORIES(sendtx_ts ${visibility} src)
        TARGET_COMPILE_DEFINITIONS(sendtx_ts ${visibility} DOGECOIN_TS=1)

        ADD_EXECUTABLE(spvnode src/cli/spvnode.c)
        INSTALL(TARGETS spvnode RUNTIME)

        IF (WIN32 AND USE_TPM2)
            TARGET_LINK_LIBRARIES(spvnode ${LIBDOGECOIN_NAME} ${LIBEVENT} tbs ncrypt crypt32)
        ELSE()
            TARGET_LINK_LIBRARIES(spvnode ${LIBDOGECOIN_NAME} ${LIBEVENT} ${LIBEVENT_PTHREADS})
        ENDIF()
        TARGET_INCLUDE_DIRECTORIES(spvnode ${visibility} src)

        ADD_EXECUTABLE(spvnode_ts src/cli/spvnode.c)
        INSTALL(TARGETS spvnode_ts RUNTIME)

        IF (WIN32 AND USE_TPM2)
            TARGET_LINK_LIBRARIES(spvnode_ts ${LIBDOGECOIN_NAME} ${LIBEVENT} tbs ncrypt crypt32)
        ELSE()
            TARGET_LINK_LIBRARIES(spvnode_ts ${LIBDOGECOIN_NAME} ${LIBEVENT} ${LIBEVENT_PTHREADS})
        ENDIF()
        TARGET_INCLUDE_DIRECTORIES(spvnode_ts ${visibility} src)
        TARGET_COMPILE_DEFINITIONS(spvnode_ts ${visibility} DOGECOIN_TS=1)
    ENDIF()
ENDIF()
