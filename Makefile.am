ACLOCAL_AMFLAGS = -I build-aux/m4
AUTOMAKE_OPTIONS = serial-tests
.PHONY: gen
.INTERMEDIATE: $(GENBIN)

DIST_SUBDIRS = src/secp256k1

LIBSECP256K1=src/secp256k1/libsecp256k1.la

$(LIBSECP256K1): $(wildcard src/secp256k1/src/*) $(wildcard src/secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

lib_LTLIBRARIES = libdogecoin.la
libdogecoinincludedir = $(includedir)/dogecoin
libdogecoininclude_HEADERS = \
    include/dogecoin/aes256_cbc.h \
    include/dogecoin/base58.h \
    include/dogecoin/bip32.h \
    include/dogecoin/block.h \
    include/dogecoin/blockchain.h \
    include/dogecoin/dogecoin.h \
    include/dogecoin/buffer.h \
    include/dogecoin/chainparams.h \
    include/dogecoin/cstr.h \
    include/dogecoin/ctaes.h \
    include/dogecoin/ecc_key.h \
    include/dogecoin/ecc.h \
    include/dogecoin/hash.h \
    include/dogecoin/hmac.h \
    include/dogecoin/memory.h \
    include/dogecoin/portable_endian.h \
    include/dogecoin/random.h \
    include/dogecoin/ripemd160.h \
    include/dogecoin/script.h \
    include/dogecoin/segwit_addr.h \
    include/dogecoin/serialize.h \
    include/dogecoin/sha2.h \
    include/dogecoin/tool.h \
    include/dogecoin/tx.h \
    include/dogecoin/utils.h \
    include/dogecoin/vector.h

trezorcryptoincludedir = $(includedir)/trezor-crypto
trezorcryptoinclude_HEADERS = \
    src/trezor-crypto/base58.h \
    src/trezor-crypto/blake2_common.h \
    src/trezor-crypto/blake2b.h \
    src/trezor-crypto/blake256.h \
    src/trezor-crypto/groestl.h \
    src/trezor-crypto/groestl_internal.h \
    src/trezor-crypto/hasher.h \
    src/trezor-crypto/hmac.h \
    src/trezor-crypto/memzero.h \
    src/trezor-crypto/options.h \
    src/trezor-crypto/ripemd160.h \
    src/trezor-crypto/segwit_addr.h \
    src/trezor-crypto/sha2.h \
    src/trezor-crypto/sha3.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdogecoin.pc

libdogecoin_la_SOURCES = \
    src/aes256_cbc.c \
    src/base58.c \
    src/bip32.c \
    src/block.c \
    src/buffer.c \
    src/chainparams.c \
    src/commontools.c \
    src/cstr.c \
    src/ctaes.c \
    src/ecc_key.c \
    src/ecc_libsecp256k1.c \
    src/memory.c \
    src/random.c \
    src/ripemd160.c \
    src/script.c \
    src/serialize.c \
    src/tx.c \
    src/utils.c \
    src/vector.c

libdogecoin_la_SOURCES += \
    src/trezor-crypto/base58.c \
    src/trezor-crypto/blake2b.c \
    src/trezor-crypto/blake256.c \
    src/trezor-crypto/groestl.c \
    src/trezor-crypto/hasher.c \
    src/trezor-crypto/hmac.c \
    src/trezor-crypto/memzero.c \
    src/trezor-crypto/ripemd160.c \
    src/trezor-crypto/segwit_addr.c \
    src/trezor-crypto/sha2.c \
    src/trezor-crypto/sha3.c

libdogecoin_la_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/logdb/include
libdogecoin_la_LIBADD = $(LIBSECP256K1)

if USE_TESTS
noinst_PROGRAMS = tests
tests_LDADD = libdogecoin.la
tests_SOURCES = \
    test/aes_tests.c \
    test/base58check_tests.c \
    test/bip32_tests.c \
    test/block_tests.c \
    test/buffer_tests.c \
    test/cstr_tests.c \
    test/ecc_tests.c \
    test/eckey_tests.c \
    test/hash_tests.c \
    test/memory_tests.c \
    test/random_tests.c \
    test/serialize_tests.c \
    test/sha2_tests.c \
    test/utest.h \
    test/unittester.c \
    test/tx_tests.c \
    test/utils_tests.c \
    test/vector_tests.c

tests_CFLAGS = $(libdogecoin_la_CFLAGS)
tests_CPPFLAGS = -I$(top_srcdir)/src
tests_LDFLAGS = -static
TESTS = tests
endif

if WITH_LOGDB
libdogecoin_la_SOURCES += \
    src/logdb/logdb_core.c \
    src/logdb/logdb_memdb_llist.c \
    src/logdb/logdb_memdb_rbtree.c \
    src/logdb/logdb_rec.c \
    src/logdb/red_black_tree.c

logdbincludedir = $(includedir)/logdb
logdbinclude_HEADERS = \
    src/logdb/include/logdb/logdb_base.h \
    src/logdb/include/logdb/logdb_core.h \
    src/logdb/include/logdb/logdb_memdb_llist.h \
    src/logdb/include/logdb/logdb_memdb_rbtree.h \
    src/logdb/include/logdb/logdb_rec.h \
    src/logdb/include/logdb/logdb.h \
    src/logdb/include/logdb/red_black_tree.h

if USE_TESTS
tests_SOURCES += \
    src/logdb/test/logdb_tests.c \
    src/logdb/test/tests_red_black_tree.c \
    src/logdb/test/logdb_tests_sample.h
endif
endif

if WITH_WALLET
libdogecoininclude_HEADERS += \
    include/dogecoin/wallet.h

libdogecoin_la_SOURCES += \
    src/wallet.c

if USE_TESTS
tests_SOURCES += \
    test/wallet_tests.c
endif
endif

if WITH_NET
libdogecoininclude_HEADERS += \
    include/dogecoin/headersdb.h \
    include/dogecoin/headersdb_file.h \
    include/dogecoin/protocol.h \
    include/dogecoin/net.h \
    include/dogecoin/netspv.h

libdogecoin_la_SOURCES += \
    src/headersdb_file.c \
    src/net.c \
    src/netspv.c \
    src/protocol.c

libdogecoin_la_LIBADD += $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS)
libdogecoin_la_CFLAGS += $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)

if USE_TESTS
tests_SOURCES += \
    test/net_tests.c \
    test/netspv_tests.c \
    test/protocol_tests.c
tests_LDADD += $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS)
endif
endif

if WITH_TOOLS
if USE_TESTS
tests_SOURCES += \
    test/tool_tests.c
endif
instdir=$(prefix)/bin
inst_PROGRAMS = dogecointool
dogecointool_LDADD = libdogecoin.la
dogecointool_SOURCES = \
    src/tools/dogecointool.c
dogecointool_CFLAGS = $(libdogecoin_la_CFLAGS)
dogecointool_CPPFLAGS = -I$(top_srcdir)/src
dogecointool_LDFLAGS = -static

if WITH_NET
inst_PROGRAMS += dogecoin-send-tx
dogecoin_send_tx_LDADD = libdogecoin.la $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS)
dogecoin_send_tx_SOURCES = \
    src/tools/dogecoin-send-tx.c
dogecoin_send_tx_CFLAGS = $(libdogecoin_la_CFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
dogecoin_send_tx_CPPFLAGS = -I$(top_srcdir)/src
dogecoin_send_tx_LDFLAGS = -static

inst_PROGRAMS += dogecoin-spv
dogecoin_spv_LDADD = libdogecoin.la
dogecoin_spv_SOURCES = \
    src/tools/dogecoin-spv.c
dogecoin_spv_CFLAGS = $(libdogecoin_la_CFLAGS)
dogecoin_spv_CPPFLAGS = -I$(top_srcdir)/src
dogecoin_spv_LDFLAGS = -static
endif

endif

clean-local:
	-$(MAKE) -C src/secp256k1 clean
