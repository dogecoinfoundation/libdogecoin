dnl require autoconf 2.68 (AS_ECHO/AS_ECHO_N)
AC_PREREQ([2.68])
AC_INIT([libdogecoin],[0.1],[https://github.com/dogecoinfoundation/libdogecoin/issues],[libdogecoin])
AC_CONFIG_HEADERS([src/libdogecoin-config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AC_CANONICAL_HOST
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AH_TOP([#ifndef LIBDOGECOIN_CONFIG_H])
AH_TOP([#define LIBDOGECOIN_CONFIG_H])
AH_BOTTOM([#endif /*LIBDOGECOIN_CONFIG_H*/])
AM_INIT_AUTOMAKE([no-define subdir-objects foreign])
AC_HEADER_STDBOOL
LT_INIT

PKG_PROG_PKG_CONFIG
AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)
AC_PATH_TOOL(STRIP, strip)
AM_PROG_CC_C_O
AC_PROG_CC_C99

LIBTOOL_APP_LDFLAGS="-all-static"
LIBS="$LIBS -levent -levent_core"

CFLAGS="$CFLAGS -W -fPIC"
case $host in
  *mingw*)
    CFLAGS="$CFLAGS -s -static --static"
     TARGET_OS=windows
      case $host in
       i?86-*) 
        LIBS="$LIBS -lpthread -lwinpthread -lshell32 -ladvapi32 -liphlpapi -lws2_32 -lbcrypt -lcrypt32 -DWIN32"
       ;;
       x86_64-*) 
       ;;
       *) AC_MSG_ERROR("Could not determine win32/win64 for installer") ;;
      esac
     ;;
  *darwin*)
     TARGET_OS=darwin
     ;;
  *linux*)
     TARGET_OS=linux
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

AC_SEARCH_LIBS([powl],[m],[have_math=yes],[have_math=no])

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

if test "$host" = "mingw"; then
  # -static is interpreted by libtool, where it has a different meaning.
  # In libtool-speak, it's -all-static.
  AX_CHECK_LINK_FLAG([[-static]],[LIBTOOL_APP_LDFLAGS="$LIBTOOL_APP_LDFLAGS -all-static -no-undefined"])
fi

if test x$with_net = "xyes"; then
  AC_CHECK_HEADER([event2/event.h],, AC_MSG_ERROR(libevent headers missing),)
  AC_CHECK_LIB([event],[main],EVENT_LIBS=-levent,AC_MSG_ERROR(libevent_core missing))
  AC_CHECK_LIB([event_core],[main],EVENT_LIBS=-levent_core,AC_MSG_ERROR(libevent_core missing))
  if test "$host" = "mingw"; then
    AC_CHECK_LIB([event_pthreads],[main],EVENT_PTHREADS_LIBS=-levent_pthreads,AC_MSG_ERROR(libevent_pthreads missing))
  fi
fi

AC_CONFIG_FILES([Makefile libdogecoin.pc])
AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(BUILD_EXEEXT)
AC_SUBST(EVENT_LIBS)
AC_SUBST(EVENT_PTHREADS_LIBS)
AM_CONDITIONAL([USE_TESTS], [test x"$use_tests" != x"no"])
AM_CONDITIONAL([WITH_TOOLS], [test "x$with_tools" = "xyes"])
AM_CONDITIONAL([WITH_NET], [test "x$with_net" = "xyes"])

ac_configure_args="${ac_configure_args} --disable-shared --with-pic --with-bignum=no --enable-module-recovery"
AC_CONFIG_SUBDIRS([src/secp256k1])

dnl make sure nothing new is exported so that we don't break the cache
PKGCONFIG_PATH_TEMP="$PKG_CONFIG_PATH"
unset PKG_CONFIG_PATH
PKG_CONFIG_PATH="$PKGCONFIG_PATH_TEMP"

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
