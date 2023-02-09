CMAKE_MINIMUM_REQUIRED(VERSION 3.13) # 3.13: concise relative source paths
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")

SET(CMAKE_C_STANDARD 99)
SET(CMAKE_C_STANDARD_REQUIRED TRUE)

SET(LIBDOGECOIN_NAME dogecoin)
PROJECT(lib${LIBDOGECOIN_NAME} VERSION 0.1)

INCLUDE(CTest)
SET(USE_TESTS ${CMAKE_TESTING_ENABLED})
SET(WITH_TOOLS TRUE CACHE BOOL "enable dogecoin tool cli application")
SET(WITH_NET TRUE CACHE BOOL "enable net functions")
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
        file(COPY "${PROJECT_SOURCE_DIR}/src/libevent/include" DESTINATION "${PROJECT_SOURCE_DIR}")
        find_path(LIBEVENT_INCLUDE_DIR event.h PATHS "${PROJECT_SOURCE_DIR}/include" REQUIRED)
        find_path(LIBEVENT_INCLUDE_DIR event-config.h PATHS "${PROJECT_SOURCE_DIR}/include/event2" REQUIRED)
        FIND_LIBRARY(LIBEVENT NAMES event event_core event_extras event_pthreads HINTS "${PROJECT_SOURCE_DIR}/src/libevent/build/lib/${CMAKE_BUILD_TYPE}" REQUIRED)
        FIND_LIBRARY(WINPTHREADS NAMES pthreadVC2 pthreadwin32 pthreads pthread HINTS "${PROJECT_SOURCE_DIR}/contrib/winpthreads/include/lib/x64/" REQUIRED)
    ELSE()
        FIND_LIBRARY(LIBEVENT event REQUIRED)
        FIND_LIBRARY(LIBUNISTRING unistring REQUIRED)
    ENDIF()
ENDIF()

MESSAGE(STATUS "")
MESSAGE(STATUS "Options used to compile and link:")
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
IF(WITH_NET)
    ADD_DEFINITIONS(-DWITH_NET=1)
ENDIF()
FILE(TOUCH src/libdogecoin-config.h)

ADD_LIBRARY(${LIBDOGECOIN_NAME})

INSTALL(FILES
    include/dogecoin/address.h
    include/dogecoin/aes.h
    include/dogecoin/base58.h
    include/dogecoin/bip32.h
    include/dogecoin/bip39.h
    include/dogecoin/bip44.h
    include/dogecoin/block.h
    include/dogecoin/buffer.h
    include/dogecoin/byteswap.h
    include/dogecoin/chainparams.h
    include/dogecoin/cstr.h
    include/dogecoin/ctaes.h
    include/dogecoin/dogecoin.h
    include/dogecoin/ecc.h
    include/dogecoin/hash.h
    include/dogecoin/key.h
    include/dogecoin/koinu.h
    include/dogecoin/mem.h
    include/dogecoin/portable_endian.h
    include/dogecoin/random.h
    include/dogecoin/rmd160.h
    include/dogecoin/script.h
    include/dogecoin/serialize.h
    include/dogecoin/sha2.h
    include/dogecoin/tool.h
    include/dogecoin/transaction.h
    include/dogecoin/tx.h
    include/uthash/uthash.h
    include/dogecoin/utils.h
    include/dogecoin/vector.h
    include/dogecoin/wow.h
    DESTINATION include/dogecoin
)

IF(WIN32)
    INSTALL(FILES
    include/dogecoin/winunistd.h
    contrib/getopt/wingetopt.h
    contrib/wintime/wingettime.h
    DESTINATION include/dogecoin)
ENDIF()

TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    src/address.c
    src/aes.c
    src/base58.c
    src/bip32.c
    src/bip39.c
    src/bip44.c
    src/block.c
    src/buffer.c
    src/chainparams.c
    src/cstr.c
    src/ctaes.c
    src/ecc.c
    src/key.c
    src/koinu.c
    src/mem.c
    src/random.c
    src/rmd160.c
    src/script.c
    src/serialize.c
    src/sha2.c
    src/cli/tool.c
    src/transaction.c
    src/tx.c
    src/utils.c
    src/vector.c
)

IF(WIN32)
TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
    contrib/getopt/wingetopt.c
    contrib/wintime/wingettime.c
)
ENDIF()

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
    src/secp256k1/include
    ${GMP_INSTALL_DIR}/include)

IF(USE_TESTS)
    ADD_EXECUTABLE(tests)
    TARGET_SOURCES(tests PRIVATE
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
        test/opreturn_tests.c
        test/random_tests.c
        test/rmd160_tests.c
        test/serialize_tests.c
        test/sha2_tests.c
        test/transaction_tests.c
        test/tx_tests.c
        test/utest.h
        test/unittester.c
        test/utils_tests.c
        test/vector_tests.c
)
    TARGET_LINK_LIBRARIES(tests ${LIBDOGECOIN_NAME} ${LIBUNISTRING})
    ADD_TEST(NAME ${LIBDOGECOIN_NAME}_tests COMMAND tests)
    # move test/wordlist to build
    FILE(GLOB_RECURSE WORDLISTS "${PROJECT_SOURCE_DIR}/test/wordlist/*.txt")
    FOREACH(wordlist ${WORDLISTS})
        GET_FILENAME_COMPONENT(wordlist_name ${wordlist} NAME)
        CONFIGURE_FILE(${wordlist} "${CMAKE_CURRENT_BINARY_DIR}/test/wordlist/${wordlist_name}" COPYONLY)
    ENDFOREACH()
ENDIF()

IF(WITH_NET)
    INSTALL(FILES
        include/dogecoin/protocol.h
        include/dogecoin/net.h
        DESTINATION include/dogecoin
    )
    TARGET_SOURCES(${LIBDOGECOIN_NAME} PRIVATE
        src/net.c
        src/protocol.c
    )

    TARGET_LINK_LIBRARIES(${LIBDOGECOIN_NAME} ${LIBEVENT} ${LIBEVENT_PTHREADS} ${LIBUNISTRING})

    IF(USE_TESTS)
        TARGET_SOURCES(tests PRIVATE
            test/net_tests.c
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

    ADD_EXECUTABLE(such src/cli/such.c)
    INSTALL(TARGETS such RUNTIME)
    TARGET_LINK_LIBRARIES(such ${LIBDOGECOIN_NAME} ${LIBUNISTRING})

    TARGET_INCLUDE_DIRECTORIES(such PRIVATE src)

    IF(WITH_NET)
        ADD_EXECUTABLE(sendtx src/cli/sendtx.c)
        INSTALL(TARGETS sendtx RUNTIME)
        TARGET_LINK_LIBRARIES(sendtx ${LIBDOGECOIN_NAME})

        TARGET_INCLUDE_DIRECTORIES(sendtx PRIVATE src)
    ENDIF()
ENDIF()
