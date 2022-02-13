ACLOCAL_AMFLAGS = -I build-aux/m4
AUTOMAKE_OPTIONS = serial-tests
.PHONY: gen
.INTERMEDIATE: $(GENBIN)

DIST_SUBDIRS = src/secp256k1

LIBSECP256K1=src/secp256k1/libsecp256k1.la

$(LIBSECP256K1): $(wildcard src/secp256k1/src/*) $(wildcard src/secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

lib_LTLIBRARIES = libdogecoin.la
include_HEADERS = \
    include/dogecoin/crypto/aes.h \
    include/dogecoin/crypto/base58.h \
    include/dogecoin/buffer.h \
    include/dogecoin/cstr.h \
    include/dogecoin/dogecoin.h \
    include/dogecoin/crypto/ecc.h \
    include/dogecoin/mem.h \
    include/dogecoin/crypto/random.h \
    include/dogecoin/crypto/rmd160.h \
    include/dogecoin/crypto/sha2.h \
    include/dogecoin/utils.h \
    include/dogecoin/vector.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdogecoin.pc

libdogecoin_la_SOURCES = \
    src/crypto/aes.c \
    src/crypto/base58.c \
    src/buffer.c \
    src/cstr.c \
    src/crypto/ecc.c \
    src/mem.c \
    src/crypto/random.c \
    src/crypto/rmd160.c \
    src/crypto/sha2.c \
    src/utils.c \
    src/vector.c

libdogecoin_la_CFLAGS = -I$(top_srcdir)/include
libdogecoin_la_LIBADD = $(LIBSECP256K1)

if USE_TESTS
noinst_PROGRAMS = tests
tests_LDADD = libdogecoin.la
tests_SOURCES = \
    test/aes_tests.c \
    test/base58_tests.c \
    test/buffer_tests.c \
    test/cstr_tests.c \
    test/ecc_tests.c \
    test/mem_tests.c \
    test/random_tests.c \
    test/rmd160_tests.c \
    test/sha2_tests.c \
    test/utest.h \
    test/unittester.c \
    test/utils_tests.c \
    test/vector_tests.c

tests_CFLAGS = $(libdogecoin_la_CFLAGS)
tests_CPPFLAGS = -I$(top_srcdir)/src
tests_LDFLAGS = -static
TESTS = tests
endif

clean-local:
	-$(MAKE) -C src/secp256k1 clean
