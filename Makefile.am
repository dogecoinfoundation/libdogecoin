ACLOCAL_AMFLAGS = -I build-aux/m4
.PHONY: gen
.INTERMEDIATE: $(GENBIN)

lib_LTLIBRARIES = libdogecoin.la
include_HEADERS = \
    include/dogecoin/crypto/aes.h \
    include/dogecoin/dogecoin.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdogecoin.pc

libdogecoin_la_SOURCES = \
    src/crypto/aes.c

libdogecoin_la_CFLAGS = -I$(top_srcdir)/include

if USE_TESTS
noinst_PROGRAMS = tests
tests_LDADD = libdogecoin.la
tests_SOURCES = \
    test/utest.h \
    test/unittester.c

tests_CFLAGS = $(libdogecoin_la_CFLAGS)
tests_CPPFLAGS = -I$(top_srcdir)/src
tests_LDFLAGS = -static
TESTS = tests
endif
