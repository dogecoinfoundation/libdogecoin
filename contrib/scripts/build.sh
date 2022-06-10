#!/bin/bash

# build depends: contrib/scripts/build.sh --host <host triple> --depends
# build: contrib/scripts/build.sh --host <host triple>

export LC_ALL=C
set -e -o pipefail

if [ $# -eq 0 ]; then
    echo "No arguments provided"
    exit 1
fi

has_param() {
    local term="$1"
    shift
    for arg; do
        if [[ $arg == "$term" ]]; then
            return 0
        fi
    done
    return 1
}

DEPENDS=""
TARGET_HOST_TRIPLET=""
TARGET_ARCH=""
CONFIGURE_OPTIONS=""
PREFIX="/usr/local"

if has_param '--host' "$@"; then
    TARGET_HOST_TRIPLET=$2
    case "$2" in
        "arm-linux-gnueabihf") 
            TARGET_ARCH="armhf"
        ;;
        "aarch64-linux-gnu")
            TARGET_ARCH="arm64"
        ;;
        "x86_64-w64-mingw32")
            TARGET_ARCH="amd64"
            LDFLAGS+=-no-undefined
        ;;
        "i686-w64-mingw32")
            TARGET_ARCH="i386"
            LDFLAGS+=-no-undefined
            LIBS+="-lpthread -lwinpthread -lshell32 -ladvapi32 -liphlpapi -lws2_32 -lbcrypt -lcrypt32 -DWIN32"
        ;;
        "x86_64-apple-darwin14")
            TARGET_ARCH="amd64"
        ;;
        "x86_64-pc-linux-gnu")
            TARGET_ARCH="amd64"
        ;;
        "i686-pc-linux-gnu")
            TARGET_ARCH="i386"
        ;;
    esac
    CONFIGURE_OPTIONS="--enable-static --disable-shared"
fi

if has_param '--depends' "$@"; then
    DEPENDS=1
    PREFIX=`pwd`/depends/$TARGET_HOST_TRIPLET
    CFLAGS+="-I`pwd`/depends/$TARGET_HOST_TRIPLET/include/"
    LDFLAGS+="-L`pwd`/depends/$TARGET_HOST_TRIPLET/lib/"
    LD_LIBRARY_PATH+=`pwd`/depends/$TARGET_HOST_TRIPLET/lib/
    PKG_CONFIG_PATH+=`pwd`/depends/$TARGET_HOST_TRIPLET/lib/pkgconfig
    LIBS+="-levent -levent_core -lm -lsecp256k1 -lsecp256k1_precomputed"
fi

./autogen.sh
if [ $DEPENDS ]; then
    echo $PREFIX
    ./configure \
    --prefix=$PREFIX \
    CFLAGS=$CFLAGS \
    LIBTOOL_APP_LDFLAGS="-all-static" \
    LDFLAGS="-s -static --static -static-libgcc -static-libstdc++" \
    LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
    PKG_CONFIG_PATH=$PKG_CONFIG_PATH \
    $CONFIGURE_OPTIONS
else
    ./configure
fi
