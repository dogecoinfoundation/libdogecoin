dnl require autoconf 2.68 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.68])

# The package (a.k.a. release) version is based on semantic versioning 2.0.0 of
# the API. All changes in experimental modules are treated as
# backwards-compatible and therefore at most increase the minor version.
define(_PKG_VERSION_MAJOR, 0)
define(_PKG_VERSION_MINOR, 1)
define(_PKG_VERSION_BUILD, 5)
define(_PKG_VERSION_IS_RELEASE, false)

# The library version is based on libtool versioning of the ABI. The set of
# rules for updating the version can be found here:
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# All changes in experimental modules are treated as if they don't affect the
# interface and therefore only increase the revision.
define(_LIB_VERSION_CURRENT, 1)
define(_LIB_VERSION_REVISION, 0)
define(_LIB_VERSION_AGE, 0)
AC_INIT([libdogecoin],m4_join([.], _PKG_VERSION_MAJOR, _PKG_VERSION_MINOR, _PKG_VERSION_BUILD)m4_if(_PKG_VERSION_IS_RELEASE, [true], [], [-dev]),[https://github.com/dogecoinfoundation/libdogecoin/issues],[libdogecoin],[https://github.com/dogecoinfoundation/libdogecoin])
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

AC_ARG_ENABLE([unistring],
  [AS_HELP_STRING([--disable-unistring],
  [disable unistring functions])],
  [with_unistring=$enableval],
  [with_unistring=yes])

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

AC_ARG_ENABLE(tests,
  AS_HELP_STRING([--enable-tests],[compile tests (default is yes)]),
  [use_tests=$enableval],
  [use_tests=yes])

AC_ARG_ENABLE([test-passwd],
  [AS_HELP_STRING([--enable-test-passwd],
  [enable test password for encrypt/decrypt (default is no)])],
  [test_passwd=$enableval],
  [test_passwd=no])

AC_MSG_CHECKING([for __builtin_expect])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() {__builtin_expect(0,0);}]])],
  [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_EXPECT,1,[Define this symbol if __builtin_expect is available]) ],
  [ AC_MSG_RESULT([no])
  ])

# Check for daemon(3)
AC_CHECK_DECLS([daemon])

m4_include(m4/macros/with.m4)
ARG_WITH_SET([random-device], [/dev/urandom], [set the device to read random data from])
if test "x$random_device" = x"/dev/urandom"; then
  AC_DEFINE_UNQUOTED([FILE_RANDOM],[1],[Define to 1 to enable random retrieving over filehandle])
  AC_DEFINE([RANDOM_DEVICE],["/dev/urandom"],[Define to set random file handle])
fi
if test "x$random_device" = x"/dev/random"; then
  AC_DEFINE_UNQUOTED([FILE_RANDOM],[1],[Define to 1 to enable /dev/random as random device])
  AC_DEFINE([RANDOM_DEVICE],["/dev/random"],[Define to set random file handle])
fi

# Configure Intel AVX2
if test x$use_intel_avx2 = xyes; then
  AC_DEFINE(USE_AVX2, 1, [Define this symbol if AVX2 works])
  CFLAGS="$CFLAGS -mavx2"
fi

# Configure Intel SSE
if test x$use_intel_sse = xyes; then
  AC_DEFINE(USE_SSE, 1, [Define this symbol if SSE works])
  CFLAGS="$CFLAGS -msse4.2"
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

if test "x$test_passwd" = xyes; then
  AC_DEFINE_UNQUOTED([TEST_PASSWD],[1],[Define to 1 to enable test password])
  PASSWD_STR=`head /dev/urandom | tr -dc A-Za-z0-9 | head -c 12`
  AC_DEFINE_UNQUOTED([PASSWD_STR],["$PASSWD_STR"],[Define to set test password])
fi

if test "x$with_net" = xyes; then
  AC_CHECK_HEADER([event2/event.h],, AC_MSG_ERROR(libevent headers missing),)
  AC_CHECK_LIB([event],[main],EVENT_LIBS=-levent,AC_MSG_ERROR(libevent missing))
  AC_CHECK_LIB([event_core],[main],EVENT_CORE_LIBS=-levent_core,AC_MSG_ERROR(libevent_core missing),[-levent_core])
  AC_CHECK_LIB([event_extra],[main],EVENT_EXTRA_LIBS=-levent_extra,AC_MSG_ERROR(libevent_extra missing))
  if test "$host" = "mingw"; then
    AC_CHECK_LIB([event_pthreads],[main],EVENT_PTHREADS_LIBS=-levent_pthreads,AC_MSG_ERROR(libevent_pthreads missing))
  fi
  LIBS="$LIBS $EVENT_LIBS $EVENT_CORE_LIBS $EVENT_EXTRA_LIBS $EVENT_PTHREADS_LIBS"
fi

if test "x$with_logdb" = xyes; then
  AC_DEFINE_UNQUOTED([WITH_LOGDB],[1],[Define to 1 to enable logdb compilation])
fi

if test "x$with_unistring" = xyes; then
  AC_CHECK_LIB([unistring],[main],LIBUNISTRING=-lunistring,AC_MSG_ERROR(libunistring missing))
  LIBS="$LIBS $LIBUNISTRING"
  AC_DEFINE(USE_UNISTRING, 1, [Define this symbol if libunistring works])
fi

if test "x$use_openenclave" = xyes; then
  AC_DEFINE(USE_OPENENCLAVE, 1, [Define this symbol if openenclave works])
fi

if test "x$use_optee" = xyes; then
  AC_DEFINE_UNQUOTED([USE_OPTEE],[1],[Define this symbol if optee works])
fi

if test "x$use_yubikey" = xyes; then
  AC_CHECK_LIB([ykpiv],[main],LIBYKPIV=-lykpiv,AC_MSG_ERROR(libpkpiv missing))
  LIBS="$LIBS $LIBYKPIV"
  AC_DEFINE(USE_YUBIKEY, 1, [Define this symbol if Yubikey works])
fi

AC_CONFIG_HEADERS([config/libdogecoin-config.h])
AC_CONFIG_FILES([Makefile libdogecoin.pc])
AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(BUILD_EXEEXT)
AC_SUBST(EVENT_LIBS)
AC_SUBST(EVENT_CORE_LIBS)
AC_SUBST(EVENT_EXTRA_LIBS)
AC_SUBST(EVENT_PTHREADS_LIBS)
AC_SUBST(LIBUNISTRING)
AC_SUBST(LIBYKPIV)
AC_SUBST(NASMFLAGS)
AM_CONDITIONAL([USE_TESTS], [test x"$use_tests" != x"no"])
AM_CONDITIONAL([WITH_BENCH], [test "x$with_bench" = "xyes"])
AM_CONDITIONAL([WITH_TOOLS], [test "x$with_tools" = "xyes"])
AM_CONDITIONAL([WITH_NET], [test "x$with_net" = "xyes"])
AM_CONDITIONAL([WITH_LOGDB], [test "x$with_logdb" = "xyes"])
AM_CONDITIONAL([WITH_WALLET], [test "x$with_wallet" = "xyes"])
AM_CONDITIONAL([USE_AVX2], [test "x$use_intel_avx2" = "xyes"])
AM_CONDITIONAL([USE_SSE], [test "x$use_intel_sse" = "xyes"])
AM_CONDITIONAL([USE_SSE2], [test "x$use_scrypt_sse2" = "xyes"])
AM_CONDITIONAL([USE_OPENENCLAVE], [test "x$use_openenclave" = "xyes"])
AM_CONDITIONAL([USE_OPTEE], [test "x$use_optee" = "xyes"])
AM_CONDITIONAL([USE_YUBIKEY], [test "x$use_yubikey" = "xyes"])
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
echo "  with unistring  = $with_unistring"
echo
echo "  Intel AVX2      = $use_intel_avx2"
echo "  Intel SSE       = $use_intel_sse"
echo "  SSE2 Scrypt     = $use_scrypt_sse2"
echo "  ARMv8 crypto    = $armv8_crypto"
echo "  ARMv8.2 crypto  = $armv82_crypto"
echo
echo "  test password   = $test_passwd"
if test "x$test_passwd" = xyes; then
echo "                    $PASSWD_STR"
fi
echo
echo "  openenclave     = $use_openenclave"
echo "  optee           = $use_optee"
echo "  Yubikey         = $use_yubikey"
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
