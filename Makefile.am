ACLOCAL_AMFLAGS = -I build-aux/m4
AUTOMAKE_OPTIONS = serial-tests
.PHONY: gen
.INTERMEDIATE: $(GENBIN)

DIST_SUBDIRS = src/secp256k1
LIBSECP256K1=src/secp256k1/libsecp256k1.la
$(LIBSECP256K1): $(wildcard src/secp256k1/src/*) $(wildcard src/secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
SUBDIRS = $(DIST_SUBDIRS)
.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

libdogecoin: src/secp256k1

includedir = $(prefix)/include/dogecoin

lib_LTLIBRARIES = libdogecoin.la
include_HEADERS = \
    include/dogecoin/constants.h \
    include/dogecoin/dogecoin.h \
    include/dogecoin/libdogecoin.h \
    include/dogecoin/uthash.h \
    config/libdogecoin-config.h
noinst_HEADERS = \
    include/bip39/index.h \
    include/dogecoin/address.h \
    include/dogecoin/aes.h \
    include/dogecoin/arith_uint256.h \
    include/dogecoin/auxpow.h \
    include/dogecoin/base58.h \
    include/dogecoin/bip32.h \
    include/dogecoin/bip39.h \
    include/dogecoin/bip44.h \
    include/dogecoin/block.h \
    include/dogecoin/blockchain.h \
    include/dogecoin/buffer.h \
    include/dogecoin/byteswap.h \
    include/dogecoin/chainparams.h \
    include/dogecoin/common.h \
    include/dogecoin/cstr.h \
    include/dogecoin/ctaes.h \
    include/dogecoin/ecc.h \
    include/dogecoin/eckey.h \
    include/dogecoin/hash.h \
    include/dogecoin/key.h \
    include/dogecoin/koinu.h \
    include/dogecoin/map.h \
    include/dogecoin/mem.h \
    include/dogecoin/moon.h \
    include/dogecoin/portable_endian.h \
    include/dogecoin/pow.h \
    include/qr/png.h \
    include/qr/qr.h \
    include/qr/jpeg.h \
    include/dogecoin/random.h \
    include/dogecoin/rmd160.h \
    include/dogecoin/script.h \
    include/dogecoin/scrypt.h \
    include/dogecoin/sign.h \
    include/dogecoin/serialize.h \
    include/dogecoin/seal.h \
    include/dogecoin/sha2.h \
    include/dogecoin/tool.h \
    include/dogecoin/transaction.h \
    include/dogecoin/tx.h \
    include/dogecoin/utils.h \
    include/dogecoin/validation.h \
    include/dogecoin/vector.h \
    include/dogecoin/version.h \
    include/dogecoin/wow.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdogecoin.pc

libdogecoin_la_SOURCES = \
    src/address.c \
    src/aes.c \
    src/arith_uint256.c \
    src/auxpow.c \
    src/base58.c \
    src/bip32.c \
    src/bip39.c \
    src/bip44.c \
    src/block.c \
    src/buffer.c \
    src/chainparams.c \
    src/cstr.c \
    src/ctaes.c \
    src/ecc.c \
    src/eckey.c \
    src/key.c \
    src/koinu.c \
    src/map.c \
    src/mem.c \
    src/moon.c \
    src/pow.c \
    src/png.c \
    src/jpeg.c \
    src/qrengine.c \
    src/qr.c \
    src/random.c \
    src/rmd160.c \
    src/script.c \
    src/scrypt.c \
    src/seal.c \
    src/sign.c \
    src/serialize.c \
    src/sha2.c \
    src/cli/tool.c \
    src/transaction.c \
    src/tx.c \
    src/utils.c \
    src/validation.c \
    src/vector.c

# NASM assembler setup
NASM = nasm

if USE_AVX2
# Directory containing assembly source files and includes
DIR_ASM_SOURCE = src/intel/avx_t1
DIR_ASM_INCLUDE = src/intel/

# Find all assembly source files and define corresponding object files
ASM_SOURCE_FILES = $(wildcard $(DIR_ASM_SOURCE)/*.asm)
ASM_OBJECT_FILES = $(ASM_SOURCE_FILES:.asm=.o)
ASM_LIB_FILES = $(ASM_SOURCE_FILES:.asm=.a)

# Add the assembly object files to the list of dependencies
libdogecoin_la_DEPENDENCIES = $(ASM_LIB_FILES)

# Pattern rule for building .o files from .asm files
%.o: %.asm
	$(NASM) $(NASMFLAGS) -I$(DIR_ASM_INCLUDE) -o $@ $<

# Pattern rule for custom static library from .o files
%.a: %.o
	$(AR) $(ARFLAGS) $@ $<

CLEANFILES = $(ASM_OBJECT_FILES)
endif

if USE_SSE
# Directory containing assembly source files and includes
DIR_ASM_SOURCE = src/intel/sse_t1
DIR_ASM_INCLUDE = src/intel/

# Find all assembly source files and define corresponding object files
ASM_SOURCE_FILES = $(wildcard $(DIR_ASM_SOURCE)/*.asm)
ASM_OBJECT_FILES = $(ASM_SOURCE_FILES:.asm=.o)
ASM_LIB_FILES = $(ASM_SOURCE_FILES:.asm=.a)

# Add the assembly object files to the list of dependencies
libdogecoin_la_DEPENDENCIES = $(ASM_LIB_FILES)

# Pattern rule for building .o files from .asm files
%.o: %.asm
	$(NASM) $(NASMFLAGS) -I$(DIR_ASM_INCLUDE) -o $@ $<

# Pattern rule for custom static library from .o files
%.a: %.o
	$(AR) $(ARFLAGS) $@ $<

CLEANFILES = $(ASM_OBJECT_FILES) $(ASM_LIB_FILES)
endif

if USE_SSE2
libdogecoin_la_SOURCES += \
    src/scrypt-sse2.c
endif

libdogecoin_la_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/logdb/include -fPIC
libdogecoin_la_LIBADD = $(LIBSECP256K1) $(ASM_LIB_FILES)
noinst_PROGRAMS =

if USE_TESTS
noinst_PROGRAMS += tests
tests_LDADD = libdogecoin.la $(ASM_LIB_FILES)
tests_SOURCES = \
    test/address_tests.c \
    test/aes_tests.c \
    test/base58_tests.c \
    test/bip32_tests.c \
    test/bip39_tests.c \
    test/bip44_tests.c \
    test/block_tests.c \
    test/buffer_tests.c \
    test/cstr_tests.c \
    test/ecc_tests.c \
    test/hash_tests.c \
    test/key_tests.c \
    test/koinu_tests.c \
    test/mem_tests.c \
    test/moon_tests.c \
    test/opreturn_tests.c \
    test/qr_tests.c \
    test/random_tests.c \
    test/rmd160_tests.c \
    test/scrypt_tests.c \
    test/serialize_tests.c \
    test/sha2_tests.c \
    test/signmsg_tests.c \
    test/tpm_tests.c \
    test/transaction_tests.c \
    test/tx_tests.c \
    include/test/utest.h \
    test/unittester.c \
    test/utils_tests.c \
    test/vector_tests.c

tests_CFLAGS = $(libdogecoin_la_CFLAGS)
tests_CPPFLAGS = -I$(top_srcdir)/src
tests_LDADD += $(LIBSECP256K1)
tests_LDFLAGS = -static
TESTS = tests
endif

if WITH_LOGDB
libdogecoin_la_SOURCES += \
    src/logdb/logdb_core.c \
    src/logdb/logdb_memdb_llist.c \
    src/logdb/logdb_memdb_rbtree.c \
    src/logdb/logdb_rec.c \
    src/logdb/misc.c \
    src/logdb/red_black_tree.c \
    src/logdb/stack.c

logdbincludedir = $(prefix)/include/logdb
logdbinclude_HEADERS = \
    src/logdb/include/logdb/logdb_base.h \
    src/logdb/include/logdb/logdb_core.h \
    src/logdb/include/logdb/logdb_memdb_llist.h \
    src/logdb/include/logdb/logdb_memdb_rbtree.h \
    src/logdb/include/logdb/logdb_rec.h \
    src/logdb/include/logdb/logdb.h \
    src/logdb/include/logdb/misc.h \
    src/logdb/include/logdb/red_black_tree.h \
    src/logdb/include/logdb/stack.h

if USE_TESTS
tests_SOURCES += \
    src/logdb/test/logdb_tests.c \
    src/logdb/test/tests_red_black_tree.c \
    src/logdb/test/logdb_tests_sample.h
endif
endif

if WITH_BENCH
noinst_PROGRAMS += bench
bench_LDADD = libdogecoin.la $(ASM_LIB_FILES)
bench_SOURCES = \
    src/bench.c
bench_CFLAGS = $(libdogecoin_la_CFLAGS)
bench_CPPFLAGS = -I$(top_srcdir)/src
bench_LDFLAGS = -static
endif

if WITH_WALLET
noinst_HEADERS += \
    include/dogecoin/wallet.h

libdogecoin_la_SOURCES += \
    src/wallet.c

if USE_TESTS
tests_SOURCES += \
    test/wallet_tests.c
endif
endif

if WITH_NET
noinst_HEADERS += \
    include/dogecoin/headersdb.h \
    include/dogecoin/headersdb_file.h \
    include/dogecoin/protocol.h \
    include/dogecoin/net.h \
    include/dogecoin/spv.h

libdogecoin_la_SOURCES += \
    src/headersdb_file.c \
    src/net.c \
    src/protocol.c \
    src/spv.c

libdogecoin_la_LIBADD += $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS)
libdogecoin_la_CFLAGS += $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)

if USE_TESTS
tests_SOURCES += \
    test/net_tests.c \
    test/protocol_tests.c \
    test/spv_tests.c
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
such_LDADD = libdogecoin.la $(LIBSECP256K1) $(ASM_LIB_FILES)
such_SOURCES = \
    src/cli/such.c
such_CFLAGS = $(libdogecoin_la_CFLAGS)
such_CPPFLAGS = -I$(top_srcdir)/src
such_LDFLAGS = -static

if WITH_NET
inst_PROGRAMS += sendtx
sendtx_LDADD = libdogecoin.la $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(LIBSECP256K1) $(ASM_LIB_FILES)
sendtx_SOURCES = \
    src/cli/sendtx.c
sendtx_CFLAGS = $(libdogecoin_la_CFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
sendtx_CPPFLAGS = -I$(top_srcdir)/src
sendtx_LDFLAGS = -static

inst_PROGRAMS += spvnode
spvnode_LDADD = libdogecoin.la $(ASM_LIB_FILES)
spvnode_SOURCES = \
    src/cli/spvnode.c
spvnode_CFLAGS = $(libdogecoin_la_CFLAGS)
spvnode_CPPFLAGS = -I$(top_srcdir)/src
spvnode_LDFLAGS = -static
endif
endif

libdogecoin_la_LDFLAGS = -no-undefined -version-info $(LIB_VERSION_CURRENT):$(LIB_VERSION_REVISION):$(LIB_VERSION_AGE)
