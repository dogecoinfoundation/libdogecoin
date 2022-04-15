CMAKE_MINIMUM_REQUIRED(VERSION 3.13) # 3.13: concise relative source paths

SET(CMAKE_C_STANDARD 99)
SET(CMAKE_C_STANDARD_REQUIRED TRUE)

SET(LIBDOGECOIN_NAME dogecoin)
PROJECT(lib${LIBDOGECOIN_NAME} VERSION 0.1)

INCLUDE(CTest)
SET(USE_TESTS ${CMAKE_TESTING_ENABLED})
SET(WITH_TOOLS TRUE CACHE BOOL "enable dogecoin tool cli application")
SET(RANDOM_DEVICE "/dev/urandom" CACHE STRING "set the device to read random data from")

MESSAGE(STATUS "")
MESSAGE(STATUS "Options used to compile and link:")
MESSAGE(STATUS "  WITH_TOOLS    = ${WITH_TOOLS}")
MESSAGE(STATUS "")

ADD_DEFINITIONS(
    -DPACKAGE_NAME="${PROJECT_NAME}"
    -DPACKAGE_VERSION="${PROJECT_VERSION}"
    -DRANDOM_DEVICE="${RANDOM_DEVICE}"
)
IF(USE_TESTS)
    ADD_DEFINITIONS(-DUSE_TESTS=1)
ENDIF()
IF(WITH_TOOLS)
    ADD_DEFINITIONS(-DWITH_TOOLS=1)
ENDIF()
FILE(TOUCH src/libdogecoin-config.h)

ADD_LIBRARY(${LIBDOGECOIN_NAME})

INSTALL(FILES
    include/dogecoin/address.h
    include/dogecoin/crypto/aes.h
    include/dogecoin/crypto/base58.h
    include/dogecoin/bip32.h
    include/dogecoin/block.h
    include/dogecoin/blockchain.h
    include/dogecoin/buffer.h
    include/dogecoin/compat/byteswap.h
    include/dogecoin/chainparams.h
    include/dogecoin/common.h
    include/dogecoin/cstr.h
    include/dogecoin/crypto/ctaes/ctaes.h
    include/dogecoin/dogecoin.h
    include/dogecoin/crypto/ecc.h
    include/dogecoin/crypto/hash.h
    include/dogecoin/crypto/key.h
    include/dogecoin/mem.h
    include/dogecoin/compat/portable_endian.h
    include/dogecoin/crypto/random.h
    include/dogecoin/crypto/rmd160.h
    include/dogecoin/script.h
    include/dogecoin/crypto/segwit_addr.h
    include/dogecoin/serialize.h
    include/dogecoin/crypto/sha2.h
    include/dogecoin/tool.h
    include/dogecoin/tx.h
    include/dogecoin/utils.h
    include/dogecoin/vector.h
    DESTINATION include/dogecoin
)

TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    src/address.c
    src/crypto/aes.c
    src/crypto/base58.c
    src/bip32.c
    src/block.c
    src/buffer.c
    src/chainparams.c
    src/cstr.c
    src/crypto/ctaes/ctaes.c
    src/crypto/ecc.c
    src/crypto/key.c
    src/mem.c
    src/crypto/random.c
    src/crypto/rmd160.c
    src/script.c
    src/crypto/segwit_addr.c
    src/serialize.c
    src/crypto/sha2.c
    src/cli/tool.c
    src/tx.c
    src/utils.c
    src/vector.c
)

FILE(GLOB SECP256K1 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    src/secp256k1/src/*.c)
LIST(REMOVE_ITEM SECP256K1
    src/secp256k1/src/tests.c
    src/secp256k1/src/tests_exhaustive.c
    src/secp256k1/src/bench_ecdh.c
    src/secp256k1/src/bench_ecmult.c
    src/secp256k1/src/bench_recover.c
    src/secp256k1/src/bench_sign.c
    src/secp256k1/src/bench_verify.c
    src/secp256k1/src/bench_ecdh.c
    src/secp256k1/src/bench_internal.c
    src/secp256k1/src/bench_schnorr_verify.c
    src/secp256k1/src/valgrind_ctime_test.c)
ADD_DEFINITIONS(
    -DUSE_NUM_GMP
    -DUSE_FIELD_10X26
    -DUSE_FIELD_INV_BUILTIN
    -DUSE_SCALAR_8X32
    -DUSE_SCALAR_INV_BUILTIN
    -DENABLE_MODULE_RECOVERY
    -DECMULT_WINDOW_SIZE=15
    -DECMULT_GEN_PREC_BITS=4)
TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE ${SECP256K1})

INCLUDE_DIRECTORIES(
    include
    src/secp256k1
    ${GMP_INSTALL_DIR}/include)

IF(USE_TESTS)
    ADD_EXECUTABLE(tests)
    TARGET_SOURCES(tests PRIVATE
        test/address_tests.c
        test/aes_tests.c
        test/base58_tests.c
        test/bip32_tests.c
        test/block_tests.c
        test/buffer_tests.c
        test/cstr_tests.c
        test/ecc_tests.c
        test/hash_tests.c
        test/key_tests.c
        test/mem_tests.c
        test/random_tests.c
        test/rmd160_tests.c
        test/serialize_tests.c
        test/sha2_tests.c
        test/tx_tests.c
        test/utest.h
        test/unittester.c
        test/utils_tests.c
        test/vector_tests.c
    )
    TARGET_LINK_LIBRARIES(tests ${LIBDOGECOIN_NAME})
    ADD_TEST(NAME ${LIBDOGECOIN_NAME}_tests COMMAND tests)
ENDIF()

IF(WITH_TOOLS)
    IF(USE_TESTS)
        TARGET_SOURCES(tests PRIVATE
            test/tool_tests.c
        )
    ENDIF()

    ADD_EXECUTABLE(such src/cli/such.c)
    INSTALL(TARGETS such RUNTIME)
    TARGET_LINK_LIBRARIES(such ${LIBDOGECOIN_NAME})
    TARGET_INCLUDE_DIRECTORIES(such PRIVATE src)
ENDIF()

