dnl require autoconf 2.68 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.68])

# The package (a.k.a. release) version is based on semantic versioning 2.0.0 of
# the API. All changes in experimental modules are treated as
# backwards-compatible and therefore at most increase the minor version.
define(_PKG_VERSION_MAJOR, 0)
define(_PKG_VERSION_MINOR, 1)
define(_PKG_VERSION_BUILD, 5)
# true drops the suffix and yields a bare MAJOR.MINOR.BUILD; false yields -dev.
# Set this on the release branch, not on a -dev branch.  A pre-release carries
# its marker in _PKG_VERSION_SUFFIX instead, e.g. [-pre] or [-dogebox-pre].
define(_PKG_VERSION_IS_RELEASE, true)
define(_PKG_VERSION_SUFFIX, [])

# The library version is based on libtool versioning of the ABI. The set of
# rules for updating the version can be found here:
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# All changes in experimental modules are treated as if they don't affect the
# interface and therefore only increase the revision.
define(_LIB_VERSION_CURRENT, 2)
define(_LIB_VERSION_REVISION, 0)
define(_LIB_VERSION_AGE, 0)
AC_INIT([libdogecoin],m4_join([.], _PKG_VERSION_MAJOR, _PKG_VERSION_MINOR, _PKG_VERSION_BUILD)m4_if(_PKG_VERSION_IS_RELEASE, [true], _PKG_VERSION_SUFFIX, [-dev]),[https://github.com/dogecoinfoundation/libdogecoin/issues],[libdogecoin],[https://github.com/dogecoinfoundation/libdogecoin])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])

AC_CANONICAL_HOST

AH_TOP([#ifndef LIBDOGECOIN_CONFIG_H])
AH_TOP([#define LIBDOGECOIN_CONFIG_H])
AH_BOTTOM([#endif /*LIBDOGECOIN_CONFIG_H*/])

unset FAKETIME

# Require Automake 1.11.2 for AM_PROG_AR
AM_INIT_AUTOMAKE([1.11.2 no-define foreign subdir-objects])

AM_MAINTAINER_MODE([enable])

AC_HEADER_STDBOOL

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CC
# AC_PROG_CXX must be invoked unconditionally because Makefile.am references
# src/zk_carrier/zk_groth16_mcl.cpp inside an `if HAVE_MCL` block; automake
# scans all source lists at autoconf time and requires the C++ depcomp
# infrastructure (am__fastdepCXX) to be defined unconditionally.
AC_PROG_CXX
AM_PROG_AS
AM_PROG_AR

AS_UNSET(ac_cv_prog_AR)
AS_UNSET(ac_cv_prog_ac_ct_AR)

LT_INIT([pic-only win32-dll])

build_windows=no

CFLAGS="$CFLAGS -W"
case $host in
  *mingw*)
    build_windows=yes
    TARGET_OS=windows
    NASMFLAGS="-Werror -fwin64 -Xvc -gcv8 -DWIN_ABI"
    # utf8proc is vendored into libdogecoin; static MinGW links need no dllimport.
    CFLAGS="$CFLAGS -DUTF8PROC_STATIC"
    AC_CHECK_LIB([pthread],      [main],, AC_MSG_ERROR(lib missing))
    AC_CHECK_LIB([winpthread],      [main],, AC_MSG_ERROR(lib missing))
    AC_CHECK_LIB([shell32],      [main],, AC_MSG_ERROR(lib missing))
    AC_CHECK_LIB([advapi32],      [main],, AC_MSG_ERROR(lib missing))
    AC_CHECK_LIB([iphlpapi],      [main],, AC_MSG_ERROR(lib missing))
    AC_CHECK_LIB([ws2_32],      [main],, AC_MSG_ERROR(lib missing))
    AC_CHECK_LIB([bcrypt],      [main],, AC_MSG_ERROR(lib missing))
    AC_CHECK_LIB([crypt32],      [main],, AC_MSG_ERROR(lib missing))

    # -static is interpreted by libtool, where it has a different meaning.
    # In libtool-speak, it's -all-static.
    AX_CHECK_LINK_FLAG([[-static]],[LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static -no-undefined"])
    LDFLAGS="-s -static --static $LDFLAGS"
  ;;
  *darwin*)
    TARGET_OS=darwin
  ;;
  *linux*)
    TARGET_OS=linux
    NASMFLAGS="-Werror -felf64 -Xgnu -gdwarf -DLINUX -D__linux__"
  ;;
esac
warn_CFLAGS="-std=gnu99 -pedantic -Wall -Wextra -Wno-long-long -Wno-overlength-strings"
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $warn_CFLAGS"
AC_MSG_CHECKING([if ${CC} supports ${warn_CFLAGS}])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
  [ AC_MSG_RESULT([yes]) ],
  [ AC_MSG_RESULT([no])
    CFLAGS="$saved_CFLAGS"
  ])

AC_ARG_WITH([armv8-crypto],
  [AS_HELP_STRING([--with-armv8-crypto],
  [Build with armv8 crypto (default is no)])],
  [armv8_crypto=$withval],
  [armv8_crypto=no])

AC_ARG_WITH([armv82-crypto],
  [AS_HELP_STRING([--with-armv82-crypto],
  [Build with armv8.2 crypto sha512 (default is no)])],
  [armv82_crypto=$withval],
  [armv82_crypto=no])

AC_ARG_ENABLE([dit],
  [AS_HELP_STRING([--enable-dit],
  [Build with Data Independent Timing support on supported architectures (default is no)])],
  [use_dit=$enableval],
  [use_dit=no])

AC_ARG_ENABLE([intel-avx2],
  [AS_HELP_STRING([--enable-intel-avx2],
  [Build with Intel AVX2 implementation (default is no)])],
  [use_intel_avx2=$enableval],
  [use_intel_avx2=no])

AC_ARG_ENABLE([intel-sse],
  [AS_HELP_STRING([--enable-intel-sse],
  [Build with Intel SSE implementation (default is no)])],
  [use_intel_sse=$enableval],
  [use_intel_sse=no])

AC_ARG_ENABLE([intel-sse4],
  [AS_HELP_STRING([--enable-intel-sse4],
  [Build with Intel SSE4 implementation (default is no)])],
  [use_intel_sse4=$enableval],
  [use_intel_sse4=no])

AC_ARG_ENABLE([scrypt-sse2],
  [AS_HELP_STRING([--enable-scrypt-sse2],
  [Build with scrypt sse2 implementation (default is no)])],
  [use_scrypt_sse2=$enableval],
  [use_scrypt_sse2=no])

# Enable debug
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
                  [use debug compiler flags and macros (default is no)])],
  [enable_debug=$enableval],
  [enable_debug=no])

AC_ARG_ENABLE([bench],
  [AS_HELP_STRING([--disable-bench],
  [disable benchmark functions])],
  [with_bench=$enableval],
  [with_bench=yes])

AC_ARG_ENABLE([tools],
  [AS_HELP_STRING([--disable-tools],
  [disable dogecoin tool cli application])],
  [with_tools=$enableval],
  [with_tools=yes])

AC_ARG_ENABLE([net],
  [AS_HELP_STRING([--disable-net],
  [disable net functions])],
  [with_net=$enableval],
  [with_net=yes])

AC_ARG_ENABLE([logdb],
  [AS_HELP_STRING([--disable-logdb],
  [disable liblogdb])],
  [with_logdb=$enableval],
  [with_logdb=yes])

AC_ARG_ENABLE([wallet],
  [AS_HELP_STRING([--disable-wallet],
  [disable wallet/database functions])],
  [with_wallet=$enableval],
  [with_wallet=yes])

AC_ARG_ENABLE([openenclave],
  [AS_HELP_STRING([--enable-openenclave],
  [Build with openenclave (default is no)])],
  [use_openenclave=$enableval],
  [use_openenclave=no])

AC_ARG_ENABLE([optee],
  [AS_HELP_STRING([--enable-optee],
  [Build with optee (default is no)])],
  [use_optee=$enableval],
  [use_optee=no])

AC_ARG_ENABLE([yubikey],
  [AS_HELP_STRING([--enable-yubikey],
  [Build with yubikey (default is no)])],
  [use_yubikey=$enableval],
  [use_yubikey=no])

AC_ARG_ENABLE([liboqs],
  [AS_HELP_STRING([--enable-liboqs],
  [Build with upstream liboqs for Falcon-512 and Dilithium2 (default is no)])],
  [use_liboqs=$enableval],
  [use_liboqs=no])

dnl --enable-raccoon-g selects the in-tree Raccoon-G-44 backend. It is
dnl experimental and gated; byte-exact agreement with the upstream Python
dnl reference (p-11/lattice-hd-wallets) KATs is a release-blocking gate.
AC_ARG_ENABLE([raccoon-g],
  [AS_HELP_STRING([--enable-raccoon-g],
  [Build with the in-tree Raccoon-G-44 implementation backed by GMP/MPFR (default is no, experimental)])],
  [use_raccoon_g=$enableval],
  [use_raccoon_g=no])

AC_ARG_ENABLE([zk-carrier],
  [AS_HELP_STRING([--enable-zk-carrier],
  [Build with ZK carrier (Groth16/PLONK/STARK on top of PQ carrier) (default is no)])],
  [use_zk_carrier=$enableval],
  [use_zk_carrier=no])

AC_ARG_WITH([rapidsnark],
  [AS_HELP_STRING([--with-rapidsnark],
  [Link the rapidsnark Groth16 verifier into libdogecoin (default is no; verification falls back to delegated mode without it)])],
  [use_rapidsnark=$withval],
  [use_rapidsnark=no])

AC_ARG_WITH([mcl],
  [AS_HELP_STRING([--with-mcl@<:@=DIR@:>@],
  [Link the herumi/mcl BN254 pairing library + the libdogecoin Groth16 mcl verifier into libdogecoin (default is no; verification falls back to delegated mode without it).  DIR may point to a manual mcl install (uses DIR/include and DIR/lib); when omitted, the standard search path / depends-staged mcl is used.])],
  [use_mcl=$withval],
  [use_mcl=no])

AC_ARG_ENABLE([tss2],
  [AS_HELP_STRING([--enable-tss2],
  [enable tss2 functions])],
  [with_tss2=$enableval],
  [with_tss2=no])

AC_ARG_ENABLE(tests,
  AS_HELP_STRING([--enable-tests],[compile tests (default is yes)]),
  [use_tests=$enableval],
  [use_tests=yes])

AC_ARG_ENABLE(fuzz,
  AS_HELP_STRING([--enable-fuzz],[build libFuzzer harnesses, requires clang (default is no)]),
  [use_fuzz=$enableval],
  [use_fuzz=no])

AC_MSG_CHECKING([for __builtin_expect])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() {__builtin_expect(0,0);}]])],
  [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_EXPECT,1,[Define this symbol if __builtin_expect is available]) ],
  [ AC_MSG_RESULT([no])
  ])

# Check for daemon(3)
AC_CHECK_DECLS([daemon])

# Check for readpassphrase(3) (BSD/macOS); fall back to termios-based getpass otherwise
AC_CHECK_HEADERS([readpassphrase.h])
AC_CHECK_DECLS([readpassphrase], [AC_DEFINE([HAVE_READPASSPHRASE], [1], [Define if readpassphrase(3) is available])], [], [[
#ifdef HAVE_READPASSPHRASE_H
#include <readpassphrase.h>
#endif
#include <unistd.h>
]])

m4_include(m4/macros/with.m4)
ARG_WITH_SET([random-device], [/dev/urandom], [set the device to read random data from])
# Define RANDOM_DEVICE for whatever device was configured, not just for the two
# values that used to be enumerated here. Matching on exact strings meant
# --with-random-device=/dev/hwrng left RANDOM_DEVICE undefined, which is what
# pushed src/random.c into hardcoding a path and ignoring this option entirely.
AS_IF([test "x$random_device" = x],[random_device="/dev/urandom"])
AC_DEFINE_UNQUOTED([FILE_RANDOM],[1],[Define to 1 to enable random retrieving over filehandle])
AC_DEFINE_UNQUOTED([RANDOM_DEVICE],["$random_device"],[Device the POSIX entropy fallback reads from])

# Configure Intel AVX2
if test x$use_intel_avx2 = xyes; then
  AC_DEFINE(USE_AVX2, 1, [Define this symbol if AVX2 works])
fi

# Configure Intel SSE
if test x$use_intel_sse = xyes; then
  AC_DEFINE(USE_SSE, 1, [Define this symbol if SSE works])
  CFLAGS="$CFLAGS -msse4.2"
fi

# Configure Intel SSE4
if test x$use_intel_sse4 = xyes; then
  AC_DEFINE(USE_SSE4, 1, [Define this symbol if SSE4 works])
fi

# Configure Scrypt SSE2
if test x$use_scrypt_sse2 = xyes; then
  AC_DEFINE(USE_SSE2, 1, [Define this symbol if SSE2 works])
  CFLAGS="$CFLAGS -msse4.2"
fi

if test x$armv8_crypto = xyes; then
  AC_MSG_CHECKING([whether to build with armv8 crypto])
  AC_MSG_RESULT(yes)
  AC_DEFINE(USE_ARMV8, 1, [Define this symbol if armv8 crypto works])
  CPPFLAGS="$CPPFLAGS -march=armv8-a+crypto"
fi

if test x$armv82_crypto = xyes; then
  CPPFLAGS="$CPPFLAGS -march=armv8.2-a+crypto+sha3"
  AC_MSG_CHECKING([whether to build with armv8.2-a crypto])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <arm_neon.h>]], [[uint64x2_t x; x = vsha512su0q_u64(x, x);]])],
                  [AC_DEFINE(USE_ARMV82, 1, [Define this symbol if armv8.2 crypto works])
                   CFLAGS="$CFLAGS -march=armv8.2-a+crypto+sha3"]
                   AC_MSG_RESULT(yes),
                  [AC_MSG_WARN([sha512 missing, using armv8-a crypto without sha512])
                   AC_DEFINE(USE_ARMV8, 1, [Define this symbol if armv8 crypto works])
                   CFLAGS=${CFLAGS/"-march=armv8.2-a+crypto+shqa3"/}
                   CFLAGS+="-march=armv8-a+crypto"])
fi

if test x$use_dit = xyes; then
  AC_DEFINE(USE_DIT, 1, [Define this symbol if DIT support is enabled])
fi

if test "x$with_bench" = xyes; then
  AC_DEFINE_UNQUOTED([WITH_BENCH],[1],[Define to 1 to enable bench compilation])
fi

if test "x$with_tools" = xyes; then
  AC_DEFINE_UNQUOTED([WITH_TOOLS],[1],[Define to 1 to enable tool compilation])
fi

if test "x$with_net" = xyes; then
  AC_DEFINE_UNQUOTED([WITH_NET],[1],[Define to 1 to enable net compilation])
fi

if test "x$with_wallet" = xyes; then
  AC_DEFINE_UNQUOTED([WITH_WALLET],[1],[Define to 1 to enable wallet compilation])
fi

if test "x$enable_debug" = xyes; then
  CFLAGS="$CFLAGS -g3 -O0 -DDEBUG"
  CXXFLAGS="$CXXFLAGS -g3 -O0 -DDEBUG"
  AC_DEFINE_UNQUOTED([ENABLE_DEBUG],[1],[Define to 1 to enable debug output])
fi

if test "x$use_tests" = xyes; then
  PASSWD_STR=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 12`
  AC_DEFINE_UNQUOTED([PASSWD_STR],["$PASSWD_STR"],[Define to set test password])
fi

case "$host_os" in
  android*)
    PTHREAD_LIBS=
    ;;
  *)
    AC_CHECK_LIB([pthread], [pthread_create], [PTHREAD_LIBS=-lpthread], [PTHREAD_LIBS=])
    ;;
esac
LIBS="$LIBS $PTHREAD_LIBS"

if test "x$with_net" = xyes; then
  AC_CHECK_HEADER([event2/event.h],, AC_MSG_ERROR(libevent headers missing),)
  AC_CHECK_LIB([event],[main],EVENT_LIBS=-levent,AC_MSG_ERROR(libevent missing))
  AC_CHECK_LIB([event_core],[main],EVENT_CORE_LIBS=-levent_core,AC_MSG_ERROR(libevent_core missing),[-levent_core])
  AC_CHECK_LIB([event_extra],[main],EVENT_EXTRA_LIBS=-levent_extra,AC_MSG_ERROR(libevent_extra missing))
  AC_CHECK_LIB([event_pthreads],[main],EVENT_PTHREADS_LIBS=-levent_pthreads,[
    case "$host_os" in
      mingw*|msys*|cygwin*)
        EVENT_PTHREADS_LIBS=
        ;;
      *)
        AC_MSG_ERROR(libevent_pthreads missing)
        ;;
    esac
  ])
  LIBS="$LIBS $EVENT_LIBS $EVENT_CORE_LIBS $EVENT_EXTRA_LIBS $EVENT_PTHREADS_LIBS"
fi

if test "x$with_logdb" = xyes; then
  AC_DEFINE_UNQUOTED([WITH_LOGDB],[1],[Define to 1 to enable logdb compilation])
fi

if test "x$use_openenclave" = xyes; then
  AC_DEFINE(USE_OPENENCLAVE, 1, [Define this symbol if openenclave works])
fi

if test "x$use_optee" = xyes; then
  AC_DEFINE_UNQUOTED([USE_OPTEE],[1],[Define this symbol if optee works])
fi

if test "x$use_yubikey" = xyes; then
  AC_CHECK_LIB([ykpiv],[ykpiv_init],LIBYKPIV=-lykpiv,AC_MSG_ERROR(libykpiv missing))
  LIBS="$LIBS $LIBYKPIV"
  AC_DEFINE(USE_YUBIKEY, 1, [Define this symbol if Yubikey works])
fi

if test "x$use_liboqs" = xyes; then
  AC_CHECK_LIB([oqs],[main],[LIBOQS=-loqs],AC_MSG_ERROR(liboqs missing))
  LIBS="$LIBS $LIBOQS"
  AC_DEFINE(USE_LIBOQS, 1, [Define this symbol if liboqs works])
fi

if test "x$use_raccoon_g" = xyes; then
  AC_CHECK_HEADER([gmp.h],,[AC_MSG_ERROR([gmp.h missing (required by --enable-raccoon-g; build via depends RACCOON_G=1 or install libgmp-dev)])])
  AC_CHECK_HEADER([mpfr.h],,[AC_MSG_ERROR([mpfr.h missing (required by --enable-raccoon-g; build via depends RACCOON_G=1 or install libmpfr-dev)])])
  AC_CHECK_LIB([gmp],[__gmpz_init],[LIBGMP=-lgmp],AC_MSG_ERROR(libgmp missing))
  AC_CHECK_LIB([mpfr],[mpfr_init],[LIBMPFR=-lmpfr],AC_MSG_ERROR(libmpfr missing),[$LIBGMP])
  AC_CHECK_LIB([m],[sqrt],[LIBM=-lm],AC_MSG_ERROR(libm missing))
  LIBS="$LIBS $LIBMPFR $LIBGMP $LIBM"
  AC_DEFINE(USE_RACCOON_G, 1, [Define this symbol to enable the in-tree Raccoon-G-44 backend])
fi

if test "x$use_zk_carrier" = xyes; then
  AC_DEFINE(USE_ZK_CARRIER, 1, [Define this symbol to enable the ZK carrier module])
fi

if test "x$use_rapidsnark" != xno -a "x$use_rapidsnark" != x; then
  RAPIDSNARK_LIBS="-lrapidsnark -lfr -lfq -lgmp -lstdc++ -lpthread"
  ac_check_lib_save_LIBS="$LIBS"
  LIBS="$RAPIDSNARK_LIBS $LIBS"
  AC_MSG_CHECKING([for groth16_verify in -lrapidsnark])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    extern int groth16_verify(const char* proof, const char* inputs,
                              const char* vk, char* err, unsigned long max);
  ]],[[
    return groth16_verify(0, 0, 0, 0, 0);
  ]])],[
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_RAPIDSNARK], [1], [Define if rapidsnark is available for in-process Groth16 verification])
  ],[
    AC_MSG_RESULT([no])
    AC_MSG_WARN([--with-rapidsnark requested but librapidsnark not found; falling back to delegated verification])
    LIBS="$ac_check_lib_save_LIBS"
  ])
fi

have_mcl=no
if test "x$use_mcl" != xno -a "x$use_mcl" != x; then
  if test "x$use_mcl" = xyes; then
    MCL_CFLAGS=""
  else
    MCL_CFLAGS="-I$use_mcl/include"
    MCL_LIBS="-L$use_mcl/lib"
  fi
  MCL_LIBS="$MCL_LIBS -lmcl -lstdc++"
  # mcl is a C++ library and exports no `main`; AC_CHECK_LIB([mcl],[main],…)
  # was unreliable (passed/failed depending on which static libs the linker
  # happened to pull in).  Use an explicit C++ link test against a known
  # mcl symbol instead — this is the canonical way upstream recommends
  # detecting mcl from autoconf.
  AC_LANG_PUSH([C++])
  ac_check_mcl_save_LIBS="$LIBS"
  ac_check_mcl_save_CXXFLAGS="$CXXFLAGS"
  LIBS="$MCL_LIBS $LIBS"
  CXXFLAGS="$MCL_CFLAGS $CXXFLAGS"
  AC_MSG_CHECKING([for mcl::bn::initPairing in $MCL_LIBS])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
    #include <mcl/bn.hpp>
  ]],[[
    mcl::bn::initPairing(mcl::BN_SNARK1);
    return 0;
  ]])],[
    AC_MSG_RESULT([yes])
    have_mcl=yes
    AC_DEFINE([HAVE_MCL], [1], [Define if herumi/mcl BN254 pairing is available for Groth16 verification])
    AC_SUBST([MCL_CFLAGS])
    AC_SUBST([MCL_LIBS])
  ],[
    AC_MSG_RESULT([no])
    AC_MSG_WARN([--with-mcl requested but libmcl not found; falling back to delegated verification])
    MCL_CFLAGS=""
    MCL_LIBS=""
  ])
  LIBS="$ac_check_mcl_save_LIBS"
  CXXFLAGS="$ac_check_mcl_save_CXXFLAGS"
  AC_LANG_POP([C++])
fi

if test "x$with_tss2" = xyes; then
  # Minimum recommended libtss2-esys version: 2.4.0
  # Prefer pkg-config (handles dependency libs and version), fall back to a
  # plain library check. Require the public Esys header so a misconfigured
  # libtss2-dev install fails loudly at configure time rather than at link.
  m4_ifdef([PKG_CHECK_MODULES], [
    PKG_CHECK_MODULES([TSS2_ESYS], [tss2-esys >= 2.4.0],
      [LIBTSS2="$TSS2_ESYS_LIBS"
       CFLAGS="$CFLAGS $TSS2_ESYS_CFLAGS"],
      [AC_CHECK_LIB([tss2-esys],[Esys_Initialize],[LIBTSS2=-ltss2-esys],
        [AC_MSG_ERROR([libtss2-esys missing or too old (need >= 2.4.0); install libtss2-dev])])])
  ], [
    AC_CHECK_LIB([tss2-esys],[Esys_Initialize],[LIBTSS2=-ltss2-esys],
      [AC_MSG_ERROR([libtss2-esys missing; install libtss2-dev])])
  ])
  AC_CHECK_HEADER([tss2/tss2_esys.h], [],
    [AC_MSG_ERROR([tss2/tss2_esys.h missing; install libtss2-dev])])
  LIBS="$LIBS $LIBTSS2"
  AC_DEFINE(USE_TSS2, 1, [Define this symbol if libtss2 works])
fi

AC_CONFIG_HEADERS([config/libdogecoin-config.h])
AC_CONFIG_FILES([Makefile libdogecoin.pc])
AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(BUILD_EXEEXT)
AC_SUBST(EVENT_LIBS)
AC_SUBST(EVENT_CORE_LIBS)
AC_SUBST(EVENT_EXTRA_LIBS)
AC_SUBST(EVENT_PTHREADS_LIBS)
AC_SUBST(LIBTSS2)
AC_SUBST(LIBYKPIV)
AC_SUBST(NASMFLAGS)

# Resolve nasm to an absolute path at configure time so make does not depend on
# nasm being on $PATH. When building against ./depends, config.site prepends
# depends/${host}/native/bin to PATH, so this picks up the depends-built nasm.
if test x$use_intel_avx2 = xyes || test x$use_intel_sse = xyes; then
  AC_PATH_PROG([NASM], [nasm], [nasm])
  if test "x$NASM" = "xnasm"; then
    AC_MSG_ERROR([nasm assembler is required for --enable-intel-avx2/--enable-intel-sse but was not found; build it via depends with NASM=1 or install it (for example: apt-get install nasm, or brew install nasm)])
  fi
fi
AC_SUBST([NASM])
AM_CONDITIONAL([USE_TESTS], [test x"$use_tests" != x"no"])
AM_CONDITIONAL([USE_FUZZ], [test x"$use_fuzz" = x"yes"])
AM_CONDITIONAL([WITH_BENCH], [test "x$with_bench" = "xyes"])
AM_CONDITIONAL([WITH_TOOLS], [test "x$with_tools" = "xyes"])
AM_CONDITIONAL([WITH_NET], [test "x$with_net" = "xyes"])
AM_CONDITIONAL([WITH_LOGDB], [test "x$with_logdb" = "xyes"])
AM_CONDITIONAL([WITH_WALLET], [test "x$with_wallet" = "xyes"])
AM_CONDITIONAL([USE_AVX2], [test "x$use_intel_avx2" = "xyes"])
AM_CONDITIONAL([USE_SSE], [test "x$use_intel_sse" = "xyes"])
AM_CONDITIONAL([USE_SSE4], [test "x$use_intel_sse4" = "xyes"])
AM_CONDITIONAL([USE_SSE2], [test "x$use_scrypt_sse2" = "xyes"])
AM_CONDITIONAL([USE_OPENENCLAVE], [test "x$use_openenclave" = "xyes"])
AM_CONDITIONAL([USE_OPTEE], [test "x$use_optee" = "xyes"])
AM_CONDITIONAL([USE_YUBIKEY], [test "x$use_yubikey" = "xyes"])
AM_CONDITIONAL([USE_LIBOQS], [test "x$use_liboqs" = "xyes"])
AM_CONDITIONAL([USE_RACCOON_G], [test "x$use_raccoon_g" = "xyes"])
AM_CONDITIONAL([USE_ZK_CARRIER], [test "x$use_zk_carrier" = "xyes"])
AM_CONDITIONAL([HAVE_MCL], [test "x$have_mcl" = "xyes"])
AM_CONDITIONAL([USE_PQC_CARRIER], [test "x$use_liboqs" = "xyes" -o "x$use_raccoon_g" = "xyes" -o "x$use_zk_carrier" = "xyes"])
AM_CONDITIONAL([USE_ARMV8], [test "x$armv8_crypto" = "xyes"])
AM_CONDITIONAL([USE_ARMV82], [test "x$armv82_crypto" = "xyes"])
AC_SUBST(LIB_VERSION_CURRENT, _LIB_VERSION_CURRENT)
AC_SUBST(LIB_VERSION_REVISION, _LIB_VERSION_REVISION)
AC_SUBST(LIB_VERSION_AGE, _LIB_VERSION_AGE)

ac_configure_args="${ac_configure_args} --disable-shared --with-pic --with-bignum=no --enable-module-recovery"
AC_CONFIG_SUBDIRS([src/secp256k1])

AC_OUTPUT

echo
echo "Options used to compile and link:"
echo "  with tests      = $use_tests"
echo "  with bench      = $with_bench"
echo "  with tools      = $with_tools"
echo "  with net        = $with_net"
echo "  with logdb      = $with_logdb"
echo "  with wallet     = $with_wallet"
echo "  with TSS2       = $with_tss2"
echo
echo "  Intel AVX2      = $use_intel_avx2"
echo "  Intel SSE       = $use_intel_sse"
echo "  Intel SSE4      = $use_intel_sse4"
echo "  SSE2 Scrypt     = $use_scrypt_sse2"
echo "  ARMv8 crypto    = $armv8_crypto"
echo "  ARMv8.2 crypto  = $armv82_crypto"
echo "  DIT             = $use_dit"
echo
if test "x$use_tests" = xyes; then
echo "  test password   = $PASSWD_STR"
fi
echo
echo "  openenclave     = $use_openenclave"
echo "  optee           = $use_optee"
echo "  Yubikey         = $use_yubikey"
echo "  liboqs          = $use_liboqs"
echo "  raccoon-g       = $use_raccoon_g (experimental, in-tree)"
echo "  zk-carrier      = $use_zk_carrier"
echo "  rapidsnark      = $use_rapidsnark"
echo "  mcl             = $have_mcl"
echo
echo "  host            = $host"
echo "  target os       = $TARGET_OS"
echo "  nasm flags      = $NASMFLAGS"
echo
echo "  CC              = $CC"
echo "  CFLAGS          = $CFLAGS"
echo "  CXX             = $CXX"
echo "  CXXFLAGS        = $CXXFLAGS"
echo "  LDFLAGS         = $LDFLAGS"
echo "  LIBS            = $LIBS"
echo
