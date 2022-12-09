ACLOCAL_AMFLAGS = -I build-aux/m4
AUTOMAKE_OPTIONS = serial-tests
.PHONY: gen
.INTERMEDIATE: $(GENBIN)

DIST_SUBDIRS = src/secp256k1

LIBSECP256K1=src/secp256k1/libsecp256k1.la

$(LIBSECP256K1): $(wildcard src/secp256k1/src/*) $(wildcard src/secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

lib_LTLIBRARIES = libdogecoin.la
include_HEADERS = include/dogecoin/libdogecoin.h
noinst_HEADERS = \
    include/bip39/chinese_simplified.h \
    include/bip39/chinese_traditional.h \
    include/bip39/czech.h \
    include/bip39/english.h \
    include/bip39/french.h \
    include/bip39/index.h \
    include/bip39/italian.h \
    include/bip39/japanese.h \
    include/bip39/korean.h \
    include/bip39/portuguese.h \
    include/bip39/spanish.h \
    include/dogecoin/address.h \
    include/dogecoin/aes.h \
    include/dogecoin/base58.h \
    include/dogecoin/bip32.h \
    include/dogecoin/bip39.h \
    include/dogecoin/block.h \
    include/dogecoin/buffer.h \
    include/dogecoin/byteswap.h \
    include/dogecoin/chainparams.h \
    include/dogecoin/cstr.h \
    include/dogecoin/ctaes.h \
    include/dogecoin/dogecoin.h \
    include/dogecoin/ecc.h \
    include/dogecoin/hash.h \
    include/dogecoin/key.h \
    include/dogecoin/koinu.h \
    include/dogecoin/mem.h \
    include/dogecoin/portable_endian.h \
    include/dogecoin/random.h \
    include/dogecoin/rmd160.h \
    include/dogecoin/script.h \
    include/dogecoin/serialize.h \
    include/dogecoin/sha2.h \
    include/dogecoin/tool.h \
    include/dogecoin/transaction.h \
    include/dogecoin/tx.h \
    include/uthash/uthash.h \
    include/dogecoin/utils.h \
    include/dogecoin/vector.h \
    include/dogecoin/wow.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdogecoin.pc

libdogecoin_la_SOURCES = \
    src/address.c \
    src/aes.c \
    src/base58.c \
    src/bip32.c \
    src/bip39/bip39c.c \
    src/bip39/bip39.c \
    src/bip39/crypto.c \
    src/bip39/print_util.c \
    src/bip39/conversion.c \
    src/block.c \
    src/buffer.c \
    src/chainparams.c \
    src/cstr.c \
    src/ctaes.c \
    src/ecc.c \
    src/key.c \
    src/koinu.c \
    src/mem.c \
    src/random.c \
    src/rmd160.c \
    src/script.c \
    src/serialize.c \
    src/sha2.c \
    src/cli/tool.c \
    src/transaction.c \
    src/tx.c \
    src/utils.c \
    src/vector.c

libdogecoin_la_CFLAGS = -I$(top_srcdir)/include -fPIC
libdogecoin_la_LIBADD = $(LIBSECP256K1)

if USE_TESTS
noinst_PROGRAMS = tests
tests_LDADD = libdogecoin.la
tests_SOURCES = \
    test/address_tests.c \
    test/aes_tests.c \
    test/base58_tests.c \
    test/bip32_tests.c \
    test/bip39_tests.c \
    test/block_tests.c \
    test/buffer_tests.c \
    test/cstr_tests.c \
    test/ecc_tests.c \
    test/hash_tests.c \
    test/key_tests.c \
    test/koinu_tests.c \
    test/mem_tests.c \
    test/opreturn_tests.c \
    test/random_tests.c \
    test/rmd160_tests.c \
    test/serialize_tests.c \
    test/sha2_tests.c \
    test/transaction_tests.c \
    test/tx_tests.c \
    test/utest.h \
    test/unittester.c \
    test/utils_tests.c \
    test/vector_tests.c

tests_CFLAGS = $(libdogecoin_la_CFLAGS) -O0
tests_CPPFLAGS = -I$(top_srcdir)/src
tests_LDADD += $(LIBSECP256K1) -lssl -lcrypto -licuuc
tests_LDFLAGS = -static
TESTS = tests
endif

if WITH_NET
noinst_HEADERS += \
    include/dogecoin/protocol.h \
    include/dogecoin/net.h

libdogecoin_la_SOURCES += \
    src/net.c \
    src/protocol.c

libdogecoin_la_LIBADD += $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS)
libdogecoin_la_CFLAGS += $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)

if USE_TESTS
tests_SOURCES += \
    test/net_tests.c \
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
inst_PROGRAMS = such
such_LDADD = libdogecoin.la $(LIBSECP256K1)
such_SOURCES = \
    src/cli/such.c
such_CFLAGS = $(libdogecoin_la_CFLAGS)
such_CPPFLAGS = -I$(top_srcdir)/src
such_LDFLAGS = -static

if WITH_NET
inst_PROGRAMS += sendtx
sendtx_LDADD = libdogecoin.la $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(LIBSECP256K1)
sendtx_SOURCES = \
    src/cli/sendtx.c
sendtx_CFLAGS = $(libdogecoin_la_CFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
sendtx_CPPFLAGS = -I$(top_srcdir)/src
sendtx_LDFLAGS = -static
endif
endif

libdogecoin_la_LDFLAGS = -no-undefined -version-info $(LIB_VERSION_CURRENT):$(LIB_VERSION_REVISION):$(LIB_VERSION_AGE)

clean-local:
	-$(MAKE) -C src/secp256k1 clean
