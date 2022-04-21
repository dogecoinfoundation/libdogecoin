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
    include/dogecoin/address.h \
    include/dogecoin/crypto/aes.h \
    include/dogecoin/crypto/base58.h \
    include/dogecoin/bip32.h \
    include/dogecoin/block.h \
    include/dogecoin/buffer.h \
    include/dogecoin/compat/byteswap.h \
    include/dogecoin/chainparams.h \
    include/dogecoin/cstr.h \
    include/dogecoin/dogecoin.h \
    include/dogecoin/crypto/ecc.h \
    include/dogecoin/crypto/hash.h \
    include/dogecoin/crypto/key.h \
    include/dogecoin/mem.h \
    include/dogecoin/compat/portable_endian.h \
    include/dogecoin/crypto/random.h \
    include/dogecoin/crypto/rmd160.h \
    include/dogecoin/script.h \
    include/dogecoin/crypto/segwit_addr.h \
    include/dogecoin/serialize.h \
    include/dogecoin/crypto/sha2.h \
    include/dogecoin/tool.h \
    include/dogecoin/transaction.h \
    include/dogecoin/tx.h \
    contrib/uthash/uthash.h \
    include/dogecoin/utils.h \
    include/dogecoin/vector.h \
    include/dogecoin/wow.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdogecoin.pc

libdogecoin_la_SOURCES = \
    src/address.c \
    src/crypto/aes.c \
    src/crypto/base58.c \
    src/bip32.c \
    src/block.c \
    src/buffer.c \
    src/chainparams.c \
    src/cstr.c \
    src/crypto/ecc.c \
    src/crypto/key.c \
    src/mem.c \
    src/crypto/random.c \
    src/crypto/rmd160.c \
    src/script.c \
    src/crypto/segwit_addr.c \
    src/serialize.c \
    src/crypto/sha2.c \
    src/cli/such.c \
    src/cli/tool.c \
    src/transaction.c \
    src/tx.c \
    src/utils.c \
    src/vector.c

libdogecoin_la_CFLAGS = -I$(top_srcdir)/include
libdogecoin_la_LIBADD = $(LIBSECP256K1)

if USE_TESTS
noinst_PROGRAMS = tests
tests_LDADD = libdogecoin.la
tests_SOURCES = \
    test/address_tests.c \
    test/aes_tests.c \
    test/base58_tests.c \
    test/bip32_tests.c \
    test/block_tests.c \
    test/buffer_tests.c \
    test/cstr_tests.c \
    test/ecc_tests.c \
    test/hash_tests.c \
    test/key_tests.c \
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

tests_CFLAGS = $(libdogecoin_la_CFLAGS)
tests_CPPFLAGS = -I$(top_srcdir)/src
tests_LDFLAGS = -static
TESTS = tests
endif

if WITH_NET
libdogecoininclude_HEADERS += \
    include/dogecoin/net/protocol.h \
    include/dogecoin/net/net.h

libdogecoin_la_SOURCES += \
    src/net/net.c \
    src/net/protocol.c

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
such_LDADD = libdogecoin.la
such_SOURCES = \
    src/cli/such.c
such_CFLAGS = $(libdogecoin_la_CFLAGS)
such_CPPFLAGS = -I$(top_srcdir)/src
such_LDFLAGS = -static

if WITH_NET
inst_PROGRAMS += sendtx
sendtx_LDADD = libdogecoin.la $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS)
sendtx_SOURCES = \
    src/cli/sendtx.c
sendtx_CFLAGS = $(libdogecoin_la_CFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
sendtx_CPPFLAGS = -I$(top_srcdir)/src
sendtx_LDFLAGS = -static
endif
endif

clean-local:
	-$(MAKE) -C src/secp256k1 clean