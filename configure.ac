dnl require autoconf 2.68 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.68])

# The package (a.k.a. release) version is based on semantic versioning 2.0.0 of
# the API. All changes in experimental modules are treated as
# backwards-compatible and therefore at most increase the minor version.
define(_PKG_VERSION_MAJOR, 0)
define(_PKG_VERSION_MINOR, 1)
define(_PKG_VERSION_BUILD, 1)
define(_PKG_VERSION_IS_RELEASE, false)

# The library version is based on libtool versioning of the ABI. The set of
# rules for updating the version can be found here:
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
# All changes in experimental modules are treated as if they don't affect the
# interface and therefore only increase the revision.
define(_LIB_VERSION_CURRENT, 0)
define(_LIB_VERSION_REVISION, 1)
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
    AC_CHECK_LIB([unistring],[main],LIBUNISTRING=-lunistring,AC_MSG_ERROR(libunistring missing))
  ;;
  *linux*)
    TARGET_OS=linux
    AC_CHECK_LIB([unistring],[main],LIBUNISTRING=-lunistring,AC_MSG_ERROR(libunistring missing))
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

# Enable debug
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],
                  [use debug compiler flags and macros (default is no)])],
  [enable_debug=$enableval],
  [enable_debug=no])

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

AC_ARG_ENABLE(tests,
  AS_HELP_STRING([--enable-tests],[compile tests (default is yes)]),
  [use_tests=$enableval],
  [use_tests=yes])

AC_MSG_CHECKING([for __builtin_expect])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() {__builtin_expect(0,0);}]])],
  [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_EXPECT,1,[Define this symbol if __builtin_expect is available]) ],
  [ AC_MSG_RESULT([no])
  ])

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

if test "x$with_tools" = xyes; then
AC_DEFINE_UNQUOTED([WITH_TOOLS],[1],[Define to 1 to enable wallet compilation])
fi

if test "x$with_net" = xyes; then
  AC_DEFINE_UNQUOTED([WITH_NET],[1],[Define to 1 to enable net compilation])
fi

if test "x$enable_debug" = xyes; then
  CFLAGS="$CFLAGS -g3 -O0 -DDEBUG"
  CXXFLAGS="$CXXFLAGS -g3 -O0 -DDEBUG"
  AC_DEFINE_UNQUOTED([ENABLE_DEBUG],[1],[Define to 1 to enable debug output])
fi

if test x$with_net = "xyes"; then
  AC_CHECK_HEADER([event2/event.h],, AC_MSG_ERROR(libevent headers missing),)
  AC_CHECK_LIB([event],[main],EVENT_LIBS=-levent,AC_MSG_ERROR(libevent missing))
  AC_CHECK_LIB([event_core],[main],EVENT_LIBS=-levent_core,AC_MSG_ERROR(libevent_core missing))
  LIBS="$LIBS -levent -levent_core"
  if test "$host" = "mingw"; then
    AC_CHECK_LIB([event_pthreads],[main],EVENT_PTHREADS_LIBS=-levent_pthreads,AC_MSG_ERROR(libevent_pthreads missing))
  fi
fi

AC_CONFIG_HEADERS([src/libdogecoin-config.h])
AC_CONFIG_FILES([Makefile libdogecoin.pc])
AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(BUILD_EXEEXT)
AC_SUBST(EVENT_LIBS)
AC_SUBST(EVENT_PTHREADS_LIBS)
AC_SUBST(LIBUNISTRING)
AM_CONDITIONAL([USE_TESTS], [test x"$use_tests" != x"no"])
AM_CONDITIONAL([WITH_TOOLS], [test "x$with_tools" = "xyes"])
AM_CONDITIONAL([WITH_NET], [test "x$with_net" = "xyes"])
AC_SUBST(LIB_VERSION_CURRENT, _LIB_VERSION_CURRENT)
AC_SUBST(LIB_VERSION_REVISION, _LIB_VERSION_REVISION)
AC_SUBST(LIB_VERSION_AGE, _LIB_VERSION_AGE)

ac_configure_args="${ac_configure_args} --disable-shared --with-pic --with-bignum=no --enable-module-recovery"
AC_CONFIG_SUBDIRS([src/secp256k1])

AC_OUTPUT

echo 
echo "Options used to compile and link:"
echo "  with tools    = $with_tools"
echo "  with net      = $with_net"
echo
echo "  target os     = $TARGET_OS"
echo
echo "  CC            = $CC"
echo "  CFLAGS        = $CFLAGS"
echo "  CXX           = $CXX"
echo "  CXXFLAGS      = $CXXFLAGS"
echo "  LDFLAGS       = $LDFLAGS"
echo 
