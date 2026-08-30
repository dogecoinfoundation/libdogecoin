ACLOCAL_AMFLAGS = -I build-aux/m4
AUTOMAKE_OPTIONS = serial-tests
.PHONY: gen fuzz
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
noinst_LTLIBRARIES =
libdogecoin_la_DEPENDENCIES =
include_HEADERS = \
    include/dogecoin/constants.h \
    include/dogecoin/dogecoin.h \
    include/dogecoin/libdogecoin.h \
    include/dogecoin/uthash.h \
    include/dogecoin/cstr.h \
    config/libdogecoin-config.h
noinst_HEADERS = \
    include/bip39/index.h \
    include/dogecoin/address.h \
    include/dogecoin/aes.h \
    include/dogecoin/arith_uint256.h \
    include/dogecoin/auxpow.h \
    include/dogecoin/base58.h \
    include/dogecoin/bip37.h \
    include/dogecoin/bip38.h \
    include/dogecoin/bip32.h \
    include/dogecoin/bip39.h \
    include/dogecoin/bip44.h \
    include/dogecoin/block.h \
    include/dogecoin/blockchain.h \
    include/dogecoin/buffer.h \
    include/dogecoin/byteswap.h \
    include/dogecoin/chacha20.h \
    include/dogecoin/chainparams.h \
    include/dogecoin/cf_checkpoints.h \
    include/dogecoin/compact_filter.h \
    include/dogecoin/common.h \
    include/dogecoin/context.h \
    include/dogecoin/ctaes.h \
    include/dogecoin/ecc.h \
    include/dogecoin/eckey.h \
    include/dogecoin/golomb.h \
    include/dogecoin/hash.h \
    include/dogecoin/key.h \
    include/dogecoin/koinu.h \
    include/dogecoin/map.h \
    include/dogecoin/mem.h \
    include/dogecoin/moon.h \
    include/dogecoin/portable_endian.h \
    include/dogecoin/pow.h \
    include/dogecoin/pqc_carrier.h \
    include/dogecoin/pqc_dilithium.h \
    include/dogecoin/pqc_falcon.h \
    include/dogecoin/pqc_raccoon.h \
    include/qr/png.h \
    include/qr/qr.h \
    include/qr/jpeg.h \
    include/dogecoin/random.h \
    include/dogecoin/rmd160.h \
    include/dogecoin/script.h \
    include/dogecoin/scrypt.h \
    include/dogecoin/sweep.h \
    include/dogecoin/sign.h \
    include/dogecoin/serialize.h \
    include/dogecoin/seal.h \
    include/dogecoin/slip0039.h \
    include/dogecoin/sha2.h \
    include/dogecoin/smpv.h \
    include/dogecoin/threadsafe.h \
    include/dogecoin/tool.h \
    include/dogecoin/psbt.h \
    include/dogecoin/transaction.h \
    include/dogecoin/tx.h \
    include/dogecoin/utf8proc.h \
    include/dogecoin/utils.h \
    include/dogecoin/validation.h \
    include/dogecoin/vector.h \
    include/dogecoin/version.h \
    include/dogecoin/wow.h \
    include/dogecoin/zk_carrier.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdogecoin.pc

libdogecoin_la_SOURCES = \
    src/address.c \
    src/aes.c \
    src/arith_uint256.c \
    src/auxpow.c \
    src/base58.c \
    src/bip38.c \
    src/bip32.c \
    src/bip39.c \
    src/bip44.c \
    src/block.c \
    src/compact_block.c \
    src/buffer.c \
    src/chacha20.c \
    src/context.c \
    src/chainparams.c \
    src/cf_checkpoints.c \
    src/compact_filter.c \
    src/cstr.c \
    src/ctaes.c \
    src/ecc.c \
    src/eckey.c \
    src/golomb.c \
    src/key.c \
    src/koinu.c \
    src/map.c \
    src/mem.c \
    src/moon.c \
    src/pow.c \
    src/psbt.c \
    src/png.c \
    src/jpeg.c \
    src/qrengine.c \
    src/qr.c \
    src/random.c \
    src/rmd160.c \
    src/script.c \
    src/scrypt.c \
    src/seal.c \
    src/slip0039.c \
    src/sign.c \
    src/sweep.c \
    src/serialize.c \
    src/sha2.c \
    src/smpv.c \
    src/cli/tool.c \
    src/transaction.c \
    src/tx.c \
    src/utf8proc.c \
    src/utils.c \
    src/validation.c \
    src/vector.c

if USE_LIBOQS
libdogecoin_la_SOURCES += \
    src/pqc_dilithium.c \
    src/pqc_falcon.c
endif

if USE_PQC_CARRIER
libdogecoin_la_SOURCES += \
    src/pqc_carrier.c
endif

if USE_RACCOON_G
libdogecoin_la_SOURCES += \
    src/pqc_raccoon.c \
    src/raccoon_g/polyr.c \
    src/raccoon_g/ntt.c \
    src/raccoon_g/gaussian.c \
    src/raccoon_g/shake256.c \
    src/raccoon_g/keygen_kdf.c \
    src/raccoon_g/thrc.c \
    src/raccoon_g/raccoong.c
endif

if USE_ZK_CARRIER
libdogecoin_la_SOURCES += \
    src/zk_carrier/zk_carrier.c \
    src/zk_carrier/zk_commit.c \
    src/zk_carrier/zk_groth16.c \
    src/zk_carrier/zk_plonk.c \
    src/zk_carrier/zk_stark.c

if HAVE_MCL
# Groth16 BN254 verifier via herumi/mcl.
libdogecoin_la_SOURCES += src/zk_carrier/zk_groth16_mcl.cpp
endif
endif

# NASM assembler setup (absolute path resolved by configure, e.g. from
# depends/${host}/native/bin via config.site, falling back to system nasm)
NASM = @NASM@
CLEANFILES =

%.o: %.asm
	$(NASM) $(NASMFLAGS) -I$(DIR_ASM_INCLUDE) -o $@ $<

%.a: %.o
	$(AR) $(ARFLAGS) $@ $<

if USE_AVX2
DIR_ASM_SOURCE = src/intel/avx_t1
DIR_ASM_INCLUDE = src/intel/

ASM_SOURCE_FILES = $(wildcard $(DIR_ASM_SOURCE)/*.asm)
ASM_OBJECT_FILES = $(ASM_SOURCE_FILES:.asm=.o)
ASM_LIB_FILES = $(ASM_SOURCE_FILES:.asm=.a)

libdogecoin_la_DEPENDENCIES += $(ASM_LIB_FILES)

CLEANFILES += $(ASM_OBJECT_FILES) $(ASM_LIB_FILES)
endif

if USE_SSE
if !USE_AVX2
# SSE only when AVX2 is off: src/sha2.c's C preprocessor path checks USE_AVX2
# first, then USE_SSE. If both --enable-intel-avx2 and --enable-intel-sse are
# specified, AVX2 takes precedence; building SSE asm alongside AVX2-defined C
# would produce a library whose internal calls don't match the assembled symbols.
# In that case, only AVX2 assembly is built and linked into the library.
DIR_ASM_SOURCE = src/intel/sse_t1
DIR_ASM_INCLUDE = src/intel/

ASM_SOURCE_FILES = $(wildcard $(DIR_ASM_SOURCE)/*.asm)
ASM_OBJECT_FILES = $(ASM_SOURCE_FILES:.asm=.o)
ASM_LIB_FILES = $(ASM_SOURCE_FILES:.asm=.a)

libdogecoin_la_DEPENDENCIES += $(ASM_LIB_FILES)

CLEANFILES += $(ASM_OBJECT_FILES) $(ASM_LIB_FILES)
endif
endif

if USE_AVX2
noinst_LTLIBRARIES += libdogecoin_sha256_avx2.la
libdogecoin_sha256_avx2_la_SOURCES = src/crypto/sha256_avx2.c
libdogecoin_sha256_avx2_la_CFLAGS = $(libdogecoin_la_CFLAGS) -mavx2
endif

if USE_SSE4
noinst_LTLIBRARIES += libdogecoin_sha256_sse4.la
libdogecoin_sha256_sse4_la_SOURCES = \
    src/crypto/sha256_sse41.c \
    src/crypto/sha256_sse4.c
libdogecoin_sha256_sse4_la_CFLAGS = $(libdogecoin_la_CFLAGS) -msse4.1
endif

if USE_SSE2
libdogecoin_la_SOURCES += \
    src/scrypt-sse2.c
endif

libdogecoin_la_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/logdb/include -fPIC -DUTF8PROC_STATIC
# Linking $(ASM_LIB_FILES) here triggers a libtool "non-portable" warning
# but is needed so libdogecoin.so ships the asm symbols. Only one of the
# USE_AVX2 / USE_SSE asm blocks above is active at a time (the SSE block
# is gated by !USE_AVX2), so $(ASM_LIB_FILES) is unambiguous.
libdogecoin_la_LIBADD = $(LIBSECP256K1) $(ASM_LIB_FILES)
if USE_AVX2
libdogecoin_la_LIBADD += libdogecoin_sha256_avx2.la
libdogecoin_la_DEPENDENCIES += libdogecoin_sha256_avx2.la
endif
if USE_SSE4
libdogecoin_la_LIBADD += libdogecoin_sha256_sse4.la
libdogecoin_la_DEPENDENCIES += libdogecoin_sha256_sse4.la
endif

if HAVE_MCL
# mcl BN254 flags.
libdogecoin_la_CFLAGS    += @MCL_CFLAGS@
libdogecoin_la_CXXFLAGS   = -I$(top_srcdir)/include -fPIC @MCL_CFLAGS@ -std=c++14
libdogecoin_la_LIBADD    += @MCL_LIBS@
endif

noinst_PROGRAMS =

if USE_TESTS
noinst_PROGRAMS += tests
tests_LDADD = libdogecoin.la $(ASM_LIB_FILES)
tests_SOURCES = \
    test/arith_uint256_tests.c \
    test/address_tests.c \
    test/aes_tests.c \
    test/base58_tests.c \
    test/bip32_tests.c \
    test/bip39_tests.c \
    test/bip44_tests.c \
    test/block_tests.c \
    test/compact_block_tests.c \
    test/buffer_tests.c \
    test/cf_checkpoints_tests.c \
    test/chacha20_tests.c \
    test/chainparams_abi_tests.c \
    test/sighash_surface_tests.c \
    test/checkpoints_tests.c \
    test/context_tests.c \
    test/cstr_tests.c \
    test/compact_filter_tests.c \
    test/golomb_tests.c \
    test/ecc_tests.c \
    test/hash_tests.c \
    test/key_tests.c \
    test/koinu_tests.c \
    test/map_tests.c \
    test/mem_tests.c \
    test/moon_tests.c \
    test/opreturn_tests.c \
    test/qr_tests.c \
    test/random_tests.c \
    test/rmd160_tests.c \
    test/scrypt_tests.c \
    test/serialize_tests.c \
    test/sweep_tests.c \
    test/sha1_tests.c \
    test/sha2_tests.c \
    test/signmsg_tests.c \
    test/slip0039_tests.c \
    test/smpv_tests.c \
    test/tpm_tests.c \
    test/psbt_tests.c \
    test/rfc6979_tests.c \
    test/rng_entropy_tests.c \
    test/transaction_tests.c \
    test/validation_tests.c \
    test/tx_tests.c \
    include/test/utest.h \
    test/unittester.c \
    test/utils_tests.c \
    test/vector_tests.c

if USE_ZK_CARRIER
tests_SOURCES += test/zk_carrier_tests.c
endif

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

if USE_RACCOON_G
if USE_TESTS
tests_SOURCES += \
    test/raccoong_polyr_tests.c \
    test/raccoong_ntt_tests.c \
    test/raccoong_gaussian_tests.c \
    test/raccoong_shake_tests.c \
    test/raccoong_xof_sample_q_tests.c \
    test/raccoong_matvec_tests.c \
    test/raccoong_keygen_t_tests.c \
    test/raccoong_keypair_tests.c \
    test/raccoong_hd_derive_tests.c \
    test/raccoong_signature_serialize_tests.c \
    test/raccoong_chal_poly_tests.c \
    test/raccoong_hash_vec_tests.c \
    test/raccoong_buff_mu_tests.c \
    test/raccoong_sign_tests.c \
    test/data/auxpow_block_371338.h \
    test/data/bip152_vectors.h \
    test/data/bip158_vectors.h \
    test/data/raccoong_polyr_vectors.h \
    test/data/raccoong_ntt_vectors.h \
    test/data/raccoong_gaussian_vectors.h \
    test/data/raccoong_xof_sample_q_vectors.h \
    test/data/raccoong_matvec_vectors.h \
    test/data/raccoong_keygen_t_vectors.h \
    test/data/raccoong_keypair_vectors.h \
    test/data/raccoong_hd_derive_vectors.h \
    test/data/raccoong_signature_serialize_vectors.h \
    test/data/raccoong_chal_poly_vectors.h \
    test/data/raccoong_hash_vec_vectors.h \
    test/data/raccoong_buff_mu_vectors.h \
    test/data/raccoong_sign_vectors.h
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
    include/dogecoin/cfheadersdb_file.h \
    include/dogecoin/headersdb.h \
    include/dogecoin/headersdb_file.h \
    include/dogecoin/protocol.h \
    include/dogecoin/net.h \
    include/dogecoin/rest.h \
    include/dogecoin/spv.h

libdogecoin_la_SOURCES += \
    src/cfheadersdb_file.c \
    src/headersdb_file.c \
    src/bip37.c \
    src/net.c \
    src/protocol.c \
    src/rest.c \
    src/spv.c

libdogecoin_la_LIBADD += $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS)
libdogecoin_la_CFLAGS += $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)

if USE_TESTS
tests_SOURCES += \
    test/cfheadersdb_tests.c \
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

# Thread-safe variant: same sources compiled with DOGECOIN_TS so the CLI routes
# through the thread-safe (`_ts`) library APIs. See doc/thread_safety.md.
inst_PROGRAMS += such_ts
such_ts_LDADD = $(such_LDADD)
such_ts_SOURCES = $(such_SOURCES)
such_ts_CFLAGS = $(such_CFLAGS)
such_ts_CPPFLAGS = $(such_CPPFLAGS) -DDOGECOIN_TS=1
such_ts_LDFLAGS = $(such_LDFLAGS)

if WITH_NET
inst_PROGRAMS += sendtx
sendtx_LDADD = libdogecoin.la $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(LIBSECP256K1) $(ASM_LIB_FILES)
sendtx_SOURCES = \
    src/cli/sendtx.c
sendtx_CFLAGS = $(libdogecoin_la_CFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
sendtx_CPPFLAGS = -I$(top_srcdir)/src
sendtx_LDFLAGS = -static

inst_PROGRAMS += sendtx_ts
sendtx_ts_LDADD = $(sendtx_LDADD)
sendtx_ts_SOURCES = $(sendtx_SOURCES)
sendtx_ts_CFLAGS = $(sendtx_CFLAGS)
sendtx_ts_CPPFLAGS = $(sendtx_CPPFLAGS) -DDOGECOIN_TS=1
sendtx_ts_LDFLAGS = $(sendtx_LDFLAGS)

inst_PROGRAMS += spvnode
spvnode_LDADD = libdogecoin.la $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ASM_LIB_FILES)
spvnode_SOURCES = \
    src/cli/spvnode.c
spvnode_CFLAGS = $(libdogecoin_la_CFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
spvnode_CPPFLAGS = -I$(top_srcdir)/src
spvnode_LDFLAGS = -static

inst_PROGRAMS += spvnode_ts
spvnode_ts_LDADD = $(spvnode_LDADD)
spvnode_ts_SOURCES = $(spvnode_SOURCES)
spvnode_ts_CFLAGS = $(spvnode_CFLAGS)
spvnode_ts_CPPFLAGS = $(spvnode_CPPFLAGS) -DDOGECOIN_TS=1
spvnode_ts_LDFLAGS = $(spvnode_LDFLAGS)
endif
endif

libdogecoin_la_LDFLAGS = -no-undefined -version-info $(LIB_VERSION_CURRENT):$(LIB_VERSION_REVISION):$(LIB_VERSION_AGE)

if USE_FUZZ
noinst_PROGRAMS += fuzz/fuzz_tx
fuzz_fuzz_tx_SOURCES = fuzz/fuzz_tx.c
fuzz_fuzz_tx_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include
fuzz_fuzz_tx_LDADD = libdogecoin.la $(ASM_LIB_FILES)
fuzz_fuzz_tx_CFLAGS = $(AM_CFLAGS) -fsanitize=fuzzer,address,undefined
fuzz_fuzz_tx_LDFLAGS = -fsanitize=fuzzer,address,undefined -static

noinst_PROGRAMS += fuzz/fuzz_compact_block
fuzz_fuzz_compact_block_SOURCES = fuzz/fuzz_compact_block.c
fuzz_fuzz_compact_block_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include
fuzz_fuzz_compact_block_LDADD = libdogecoin.la $(ASM_LIB_FILES)
fuzz_fuzz_compact_block_CFLAGS = $(AM_CFLAGS) -fsanitize=fuzzer,address,undefined
fuzz_fuzz_compact_block_LDFLAGS = -fsanitize=fuzzer,address,undefined -static

noinst_PROGRAMS += fuzz/fuzz_block
fuzz_fuzz_block_SOURCES = fuzz/fuzz_block.c
fuzz_fuzz_block_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include
fuzz_fuzz_block_LDADD = libdogecoin.la $(ASM_LIB_FILES)
fuzz_fuzz_block_CFLAGS = $(AM_CFLAGS) -fsanitize=fuzzer,address,undefined
fuzz_fuzz_block_LDFLAGS = -fsanitize=fuzzer,address,undefined -static

# fuzz_protocol drives p2p message deserializers from protocol.c, which is
# only compiled into the library under WITH_NET (and pulls in libevent).
# Gate the harness on the same condition so `make fuzz` still builds the
# net-independent targets when networking is disabled.
if WITH_NET
noinst_PROGRAMS += fuzz/fuzz_protocol
fuzz_fuzz_protocol_SOURCES = fuzz/fuzz_protocol.c
fuzz_fuzz_protocol_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include
fuzz_fuzz_protocol_LDADD = libdogecoin.la $(ASM_LIB_FILES)
fuzz_fuzz_protocol_CFLAGS = $(AM_CFLAGS) -fsanitize=fuzzer,address,undefined
fuzz_fuzz_protocol_LDFLAGS = -fsanitize=fuzzer,address,undefined -static
endif

noinst_PROGRAMS += fuzz/fuzz_wtx
fuzz_fuzz_wtx_SOURCES = fuzz/fuzz_wtx.c
fuzz_fuzz_wtx_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include
fuzz_fuzz_wtx_LDADD = libdogecoin.la $(ASM_LIB_FILES)
fuzz_fuzz_wtx_CFLAGS = $(AM_CFLAGS) -fsanitize=fuzzer,address,undefined
fuzz_fuzz_wtx_LDFLAGS = -fsanitize=fuzzer,address,undefined -static

noinst_PROGRAMS += fuzz/fuzz_logdb
fuzz_fuzz_logdb_SOURCES = fuzz/fuzz_logdb.c
fuzz_fuzz_logdb_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/src/logdb/include
fuzz_fuzz_logdb_LDADD = libdogecoin.la $(ASM_LIB_FILES)
fuzz_fuzz_logdb_CFLAGS = $(AM_CFLAGS) -fsanitize=fuzzer,address,undefined
fuzz_fuzz_logdb_LDFLAGS = -fsanitize=fuzzer,address,undefined -static

noinst_PROGRAMS += fuzz/fuzz_psbt
fuzz_fuzz_psbt_SOURCES = fuzz/fuzz_psbt.c
fuzz_fuzz_psbt_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include
fuzz_fuzz_psbt_LDADD = libdogecoin.la $(ASM_LIB_FILES)
fuzz_fuzz_psbt_CFLAGS = $(AM_CFLAGS) -fsanitize=fuzzer,address,undefined
fuzz_fuzz_psbt_LDFLAGS = -fsanitize=fuzzer,address,undefined -static

# Corpus seed generator for fuzz_logdb. NOT a libFuzzer target: it links
# the plain (uninstrumented) library and emits checksum-valid single
# records so the fuzzer can mutate past the per-record SHA256 gate.
noinst_PROGRAMS += fuzz/seed_logdb_corpus
fuzz_seed_logdb_corpus_SOURCES = fuzz/seed_logdb_corpus.c
fuzz_seed_logdb_corpus_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_srcdir)/src/logdb/include
fuzz_seed_logdb_corpus_LDADD = libdogecoin.la $(ASM_LIB_FILES)

# Generate a starter corpus: make fuzz-corpus CORPUS=path (default ./corpus_logdb)
CORPUS ?= corpus_logdb
fuzz-corpus: fuzz/seed_logdb_corpus
	$(MKDIR_P) $(CORPUS)
	./fuzz/seed_logdb_corpus $(CORPUS)


FUZZ_TARGETS = fuzz/fuzz_tx fuzz/fuzz_block fuzz/fuzz_wtx fuzz/fuzz_logdb fuzz/fuzz_psbt fuzz/fuzz_compact_block
if WITH_NET
FUZZ_TARGETS += fuzz/fuzz_protocol
endif

fuzz: $(FUZZ_TARGETS)
endif
