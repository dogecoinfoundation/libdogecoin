CMAKE_MINIMUM_REQUIRED(VERSION 3.13) # 3.13: concise relative source paths

SET(CMAKE_C_STANDARD 99)
SET(CMAKE_C_STANDARD_REQUIRED TRUE)

SET(LIBDOGECOIN_NAME dogecoin)
PROJECT(lib${LIBDOGECOIN_NAME} VERSION 0.1)

INCLUDE(CTest)
SET(USE_TESTS ${CMAKE_TESTING_ENABLED})
SET(WITH_TOOLS TRUE CACHE BOOL "enable dogecoin tool cli application")
SET(WITH_WALLET TRUE CACHE BOOL "enable wallet/database functions")
SET(WITH_NET TRUE CACHE BOOL "enable net functions")
SET(RANDOM_DEVICE "/dev/urandom" CACHE STRING "set the device to read random data from")
IF(WITH_TOOLS OR WITH_WALLET)
    SET(WITH_LOGDB TRUE)
ELSE()
    SET(WITH_LOGDB FALSE)
ENDIF()

IF(WITH_NET)
    FIND_LIBRARY(LIBEVENT event_core REQUIRED)
    IF(WIN32)
        FIND_LIBRARY(LIBEVENT_PTHREADS event_pthreads REQUIRED)
    ENDIF()
ENDIF()

MESSAGE(STATUS "")
MESSAGE(STATUS "Options used to compile and link:")
MESSAGE(STATUS "  WITH_WALLET   = ${WITH_WALLET}")
MESSAGE(STATUS "  WITH_TOOLS    = ${WITH_TOOLS}")
MESSAGE(STATUS "  WITH_NET      = ${WITH_NET}")
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
IF(WITH_WALLET)
    ADD_DEFINITIONS(-DWITH_WALLET=1)
ENDIF()
IF(WITH_NET)
    ADD_DEFINITIONS(-DWITH_NET=1)
ENDIF()
IF(WITH_LOGDB)
    ADD_DEFINITIONS(-DWITH_LOGDB=1)
ENDIF()
FILE(TOUCH src/libdogecoin-config.h)


ADD_LIBRARY(${LIBDOGECOIN_NAME})

INSTALL(FILES
    include/dogecoin/aes256_cbc.h
    include/dogecoin/base58.h
    include/dogecoin/bip32.h
    include/dogecoin/block.h
    include/dogecoin/blockchain.h
    include/dogecoin/dogecoin.h
    include/dogecoin/buffer.h
    include/dogecoin/chainparams.h
    include/dogecoin/cstr.h
    include/dogecoin/ctaes.h
    include/dogecoin/ecc_key.h
    include/dogecoin/ecc.h
    include/dogecoin/hash.h
    include/dogecoin/hmac.h
    include/dogecoin/memory.h
    include/dogecoin/portable_endian.h
    include/dogecoin/random.h
    include/dogecoin/ripemd160.h
    include/dogecoin/script.h
    include/dogecoin/segwit_addr.h
    include/dogecoin/serialize.h
    include/dogecoin/sha2.h
    include/dogecoin/tool.h
    include/dogecoin/tx.h
    include/dogecoin/utils.h
    include/dogecoin/vector.h
    DESTINATION include/dogecoin
)

INSTALL(FILES
    src/trezor-crypto/base58.h
    src/trezor-crypto/blake2_common.h
    src/trezor-crypto/blake2b.h
    src/trezor-crypto/blake256.h
    src/trezor-crypto/groestl.h
    src/trezor-crypto/groestl_internal.h
    src/trezor-crypto/hasher.h
    src/trezor-crypto/hmac.h
    src/trezor-crypto/memzero.h
    src/trezor-crypto/options.h
    src/trezor-crypto/ripemd160.h
    src/trezor-crypto/segwit_addr.h
    src/trezor-crypto/sha2.h
    src/trezor-crypto/sha3.h
    DESTINATION include/trezor-crypto
)

TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    src/aes256_cbc.c
    src/base58.c
    src/bip32.c
    src/block.c
    src/buffer.c
    src/chainparams.c
    src/commontools.c
    src/cstr.c
    src/ctaes.c
    src/ecc_key.c
    src/ecc_libsecp256k1.c
    src/memory.c
    src/random.c
    src/ripemd160.c
    src/script.c
    src/serialize.c
    src/tx.c
    src/utils.c
    src/vector.c
)

TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    src/trezor-crypto/base58.c
    src/trezor-crypto/blake2b.c
    src/trezor-crypto/blake256.c
    src/trezor-crypto/groestl.c
    src/trezor-crypto/hasher.c
    src/trezor-crypto/hmac.c
    src/trezor-crypto/memzero.c
    src/trezor-crypto/ripemd160.c
    src/trezor-crypto/segwit_addr.c
    src/trezor-crypto/sha2.c
    src/trezor-crypto/sha3.c
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
    src/logdb/include
    src/secp256k1
    ${GMP_INSTALL_DIR}/include)

IF(USE_TESTS)
    ADD_EXECUTABLE(tests)
    TARGET_SOURCES(tests PRIVATE
        test/aes_tests.c
        test/base58check_tests.c
        test/bip32_tests.c
        test/block_tests.c
        test/buffer_tests.c
        test/cstr_tests.c
        test/ecc_tests.c
        test/eckey_tests.c
        test/hash_tests.c
        test/memory_tests.c
        test/random_tests.c
        test/serialize_tests.c
        test/sha2_tests.c
        test/unittester.c
        test/tx_tests.c
        test/utils_tests.c
        test/vector_tests.c
    )
    TARGET_LINK_LIBRARIES(tests ${LIBDOGECOIN_NAME})
    ADD_TEST(NAME ${LIBDOGECOIN_NAME}_tests COMMAND tests)
ENDIF()

IF(WITH_LOGDB)
    TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
        src/logdb/logdb_core.c
        src/logdb/logdb_memdb_llist.c
        src/logdb/logdb_memdb_rbtree.c
        src/logdb/logdb_rec.c
        src/logdb/red_black_tree.c
    )
    INSTALL(FILES
        src/logdb/include/logdb/logdb_base.h
        src/logdb/include/logdb/logdb_core.h
        src/logdb/include/logdb/logdb_memdb_llist.h
        src/logdb/include/logdb/logdb_memdb_rbtree.h
        src/logdb/include/logdb/logdb_rec.h
        src/logdb/include/logdb/logdb.h
        src/logdb/include/logdb/red_black_tree.h
        DESTINATION include/logdb
    )
    IF(USE_TESTS)
        TARGET_SOURCES(tests PRIVATE
            src/logdb/test/logdb_tests.c
            src/logdb/test/tests_red_black_tree.c
        )
    ENDIF()
ENDIF()

IF(WITH_WALLET)
    INSTALL(FILES
        include/dogecoin/wallet.h
        DESTINATION include/dogecoin
    )
    TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
        src/wallet.c
    )
    IF(USE_TESTS)
        TARGET_SOURCES(tests PRIVATE
            test/wallet_tests.c
        )
    ENDIF()
ENDIF()

IF(WITH_NET)
    INSTALL(FILES
        include/dogecoin/headersdb.h
        include/dogecoin/headersdb_file.h
        include/dogecoin/protocol.h
        include/dogecoin/net.h
        include/dogecoin/netspv.h
        DESTINATION include/dogecoin
    )
    TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
        src/headersdb_file.c
        src/net.c
        src/netspv.c
        src/protocol.c
    )

    TARGET_LINK_LIBRARIES(${LIBDOGECOIN_NAME} ${LIBEVENT} ${LIBEVENT_PTHREADS})

    IF(USE_TESTS)
        TARGET_SOURCES(tests PRIVATE
            test/net_tests.c
            test/netspv_tests.c
            test/protocol_tests.c
        )
    ENDIF()
ENDIF()

IF(WITH_TOOLS)
    IF(USE_TESTS)
        TARGET_SOURCES(tests PRIVATE
            test/tool_tests.c
        )
    ENDIF()

    ADD_EXECUTABLE(dogecointool src/tools/dogecointool.c)
    INSTALL(TARGETS dogecointool RUNTIME)
    TARGET_LINK_LIBRARIES(dogecointool ${LIBDOGECOIN_NAME})
    TARGET_INCLUDE_DIRECTORIES(dogecointool PRIVATE src)

    IF(WITH_NET)
        ADD_EXECUTABLE(dogecoin-send-tx src/tools/dogecoin-send-tx.c)
        INSTALL(TARGETS dogecoin-send-tx RUNTIME)
        TARGET_LINK_LIBRARIES(dogecoin-send-tx ${LIBDOGECOIN_NAME})
        TARGET_INCLUDE_DIRECTORIES(dogecoin-send-tx PRIVATE src)
        ADD_EXECUTABLE(dogecoin-spv src/tools/dogecoin-spv.c)
        INSTALL(TARGETS dogecoin-spv RUNTIME)
        TARGET_LINK_LIBRARIES(dogecoin-spv ${LIBDOGECOIN_NAME})
        TARGET_INCLUDE_DIRECTORIES(dogecoin-spv PRIVATE src)
    ENDIF()
ENDIF()

