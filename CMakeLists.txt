CMAKE_MINIMUM_REQUIRED(VERSION 3.13) # 3.13: concise relative source paths
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")

SET(CMAKE_C_STANDARD 99)
SET(CMAKE_C_STANDARD_REQUIRED TRUE)

SET(LIBDOGECOIN_NAME dogecoin)
PROJECT(lib${LIBDOGECOIN_NAME} VERSION 0.1.3)

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
SET(WITH_TOOLS TRUE CACHE BOOL "enable dogecoin tool cli application")
SET(WITH_NET TRUE CACHE BOOL "enable net functions")
SET(WITH_LOGDB TRUE CACHE BOOL "enable logdb")
SET(WITH_UNISTRING TRUE CACHE BOOL "enable unistring functions")
SET(WITH_WALLET TRUE CACHE BOOL "enable wallet")
SET(USE_SSE2 FALSE CACHE BOOL "enable scrypt sse2")
SET(USE_TPM2 TRUE CACHE BOOL "enable tpm2")
SET(USE_OPENENCLAVE FALSE CACHE BOOL "enable openenclave")
SET(TEST_PASSWD FALSE CACHE BOOL "enable test password")
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
        execute_process(COMMAND cmake --build .
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/libevent/build)
        file(COPY "${PROJECT_SOURCE_DIR}/src/libevent/build/include" DESTINATION "${PROJECT_SOURCE_DIR}")
        file(COPY "${PROJECT_SOURCE_DIR}/src/libevent/include/event2" DESTINATION "${PROJECT_SOURCE_DIR}/include")
        find_path(LIBEVENT_INCLUDE_DIR event.h PATHS "${PROJECT_SOURCE_DIR}/include/event2" REQUIRED)
        find_path(LIBEVENT_INCLUDE_DIR event-config.h PATHS "${PROJECT_SOURCE_DIR}/include/event2" REQUIRED)
        FIND_LIBRARY(LIBEVENT NAMES event event_core event_extras event_pthreads HINTS "${PROJECT_SOURCE_DIR}/src/libevent/build/lib/${CMAKE_BUILD_TYPE}" REQUIRED)
        FIND_LIBRARY(WINPTHREADS NAMES pthreadVC2 pthreadwin32 pthreads pthread HINTS "${PROJECT_SOURCE_DIR}/contrib/winpthreads/include/lib/x64/" REQUIRED)
    ELSE()
        FIND_LIBRARY(LIBEVENT event REQUIRED)
        FIND_LIBRARY(LIBUNISTRING unistring REQUIRED)
    ENDIF()
ENDIF()

ADD_DEFINITIONS(
    -DPACKAGE_NAME="${PROJECT_NAME}"
    -DPACKAGE_VERSION="${PROJECT_VERSION}"
    -DRANDOM_DEVICE="${RANDOM_DEVICE}"
)
IF(BUILD_SHARED_LIBS)
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

IF(USE_TESTS)
    ADD_DEFINITIONS(-DUSE_TESTS=1)
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
IF(WITH_UNISTRING)
    ADD_DEFINITIONS(-DUSE_UNISTRING=1)
ENDIF()
IF(WITH_WALLET)
    ADD_DEFINITIONS(-DWITH_WALLET=1)
ENDIF()
IF(USE_SSE2)
    ADD_DEFINITIONS(-DUSE_SSE2=1)
ENDIF()
IF(USE_TPM2)
    ADD_DEFINITIONS(-DUSE_TPM2=1)
ENDIF()
IF(USE_OPENENCLAVE)
    ADD_DEFINITIONS(-DUSE_OPENENCLAVE=1)
ENDIF()
IF(TEST_PASSWD)
    ADD_DEFINITIONS(-DTEST_PASSWD=1)
    ADD_DEFINITIONS(-DPASSWD_STR="${PASSWD_STR}")
ENDIF()

MESSAGE(STATUS "")
MESSAGE(STATUS "Options used to compile and link:")
MESSAGE(STATUS "  USE_TESTS      = ${USE_TESTS}")
MESSAGE(STATUS "  WITH_TOOLS     = ${WITH_TOOLS}")
MESSAGE(STATUS "  WITH_NET       = ${WITH_NET}")
MESSAGE(STATUS "  WITH_LOGDB     = ${WITH_LOGDB}")
MESSAGE(STATUS "  WITH_UNISTRING = ${WITH_UNISTRING}")
MESSAGE(STATUS "  WITH_WALLET    = ${WITH_WALLET}")
MESSAGE(STATUS "")
MESSAGE(STATUS "  USE_SSE2       = ${USE_SSE2}")
MESSAGE(STATUS "  USE_TPM2       = ${USE_TPM2}")
MESSAGE(STATUS "  TEST_PASSWD    = ${TEST_PASSWD}")
IF(TEST_PASSWD)
MESSAGE(STATUS "                   ${PASSWD_STR}")
ENDIF()
MESSAGE(STATUS "")
MESSAGE(STATUS "  openenclave    = ${USE_OPENENCLAVE}")
MESSAGE(STATUS "")
message(STATUS "  library type   = ${library_type}")
MESSAGE(STATUS "")

FILE(TOUCH config/libdogecoin-config.h)

ADD_LIBRARY(${LIBDOGECOIN_NAME})
INSTALL(TARGETS
    ${LIBDOGECOIN_NAME}
    LIBRARY DESTINATION lib
)

INSTALL(FILES
    include/dogecoin/constants.h
    include/dogecoin/dogecoin.h
    include/dogecoin/libdogecoin.h
    include/dogecoin/uthash.h
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
    src/bip39.c
    src/bip44.c
    src/block.c
    src/buffer.c
    src/cstr.c
    src/ctaes.c
    src/ecc.c
    src/eckey.c
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
    src/random.c
    src/rmd160.c
    src/script.c
    src/scrypt.c
    src/seal.c
    src/serialize.c
    src/sha2.c
    src/sign.c
    src/cli/tool.c
    src/transaction.c
    src/tx.c
    src/utils.c
    src/validation.c
    src/vector.c
)
TARGET_SOURCES(${LIBDOGECOIN_NAME} PUBLIC
    src/chainparams.c
)

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
        test/base58_tests.c
        test/bip32_tests.c
        test/bip39_tests.c
        test/bip44_tests.c
        test/block_tests.c
        test/buffer_tests.c
        test/cstr_tests.c
        test/ecc_tests.c
        test/hash_tests.c
        test/key_tests.c
        test/koinu_tests.c
        test/mem_tests.c
        test/moon_tests.c
        test/opreturn_tests.c
        test/qr_tests.c
        test/random_tests.c
        test/rmd160_tests.c
        test/scrypt_tests.c
        test/serialize_tests.c
        test/sha2_tests.c
        test/signmsg_tests.c
        test/tpm_tests.c
        test/transaction_tests.c
        test/tx_tests.c
        include/test/utest.h
        test/unittester.c
        test/utils_tests.c
        test/vector_tests.c
)
IF (WIN32 AND USE_TPM2)
    TARGET_LINK_LIBRARIES(tests ${LIBDOGECOIN_NAME} ${LIBUNISTRING} tbs ncrypt crypt32)
ELSE()
    TARGET_LINK_LIBRARIES(tests ${LIBDOGECOIN_NAME} ${LIBUNISTRING})
ENDIF()
    ADD_TEST(NAME ${LIBDOGECOIN_NAME}_tests COMMAND tests)
    # move test/wordlist to build
    FILE(GLOB_RECURSE WORDLISTS "${PROJECT_SOURCE_DIR}/test/wordlist/*.txt")
    FOREACH(wordlist ${WORDLISTS})
        GET_FILENAME_COMPONENT(wordlist_name ${wordlist} NAME)
        CONFIGURE_FILE(${wordlist} "${CMAKE_CURRENT_BINARY_DIR}/test/wordlist/${wordlist_name}" COPYONLY)
    ENDFOREACH()
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
    TARGET_SOURCES(${LIBDOGECOIN_NAME} ${visibility}
        src/headersdb_file.c
        src/net.c
        src/protocol.c
        src/spv.c
    )

    IF(WIN32 AND USE_TPM2)
        TARGET_LINK_LIBRARIES(${LIBDOGECOIN_NAME} ${LIBEVENT} ${LIBEVENT_PTHREADS} ${LIBUNISTRING} ncrypt crypt32)
    ELSE()
        TARGET_LINK_LIBRARIES(${LIBDOGECOIN_NAME} ${LIBEVENT} ${LIBEVENT_PTHREADS} ${LIBUNISTRING})
    ENDIF()
    IF(USE_TESTS)
        TARGET_SOURCES(tests ${visibility}
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
        TARGET_LINK_LIBRARIES(such ${LIBDOGECOIN_NAME} ${LIBUNISTRING} tbs ncrypt crypt32)
    ELSE()
        TARGET_LINK_LIBRARIES(such ${LIBDOGECOIN_NAME} ${LIBUNISTRING})
    ENDIF()
    TARGET_INCLUDE_DIRECTORIES(such ${visibility} src)

    IF(WITH_NET)
        ADD_EXECUTABLE(sendtx src/cli/sendtx.c)
        INSTALL(TARGETS sendtx RUNTIME)
        TARGET_LINK_LIBRARIES(sendtx ${LIBDOGECOIN_NAME})
        TARGET_INCLUDE_DIRECTORIES(sendtx ${visibility} src)
        ADD_EXECUTABLE(spvnode src/cli/spvnode.c)
        INSTALL(TARGETS spvnode RUNTIME)

        IF (WIN32 AND USE_TPM2)
            TARGET_LINK_LIBRARIES(spvnode ${LIBDOGECOIN_NAME} tbs ncrypt crypt32)
        ELSE()
            TARGET_LINK_LIBRARIES(spvnode ${LIBDOGECOIN_NAME})
        ENDIF()
        TARGET_INCLUDE_DIRECTORIES(spvnode ${visibility} src)
    ENDIF()
ENDIF()
