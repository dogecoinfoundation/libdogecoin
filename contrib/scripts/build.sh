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
PREFIX=""

if [ -f "`pwd`/such*" ]; then
    rm "`pwd`/such*"
fi

if [ -f "`pwd`/sendtx*" ]; then
    rm "`pwd`/sendtx*"
fi

if [ -f "`pwd`/tests*" ]; then
    rm "`pwd`/tests*"
fi

if [ -d "`pwd`/.libs" ]; then
    rm -rf "`pwd`/.libs"
    make clean
    make clean-local
fi

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
        ;;
        "i686-w64-mingw32")
            TARGET_ARCH="i386"
        ;;
        "x86_64-apple-darwin15")
            TARGET_ARCH="amd64"
        ;;
        "arm64-apple-darwin")
            TARGET_ARCH="arm64"
        ;;
        "x86_64-pc-linux-gnu")
            TARGET_ARCH="amd64"
        ;;
        "i686-pc-linux-gnu")
            TARGET_ARCH="i386"
        ;;
    esac
fi

if has_param '--depends' "$@"; then
    DEPENDS=1
    export PREFIX=`pwd`/depends/$TARGET_HOST_TRIPLET
    export CFLAGS+="-I`pwd`/depends/$TARGET_HOST_TRIPLET/include/"
    export LDFLAGS+="-I`pwd`/depends/$TARGET_HOST_TRIPLET/lib/"
    export LD_LIBRARY_PATH+="`pwd`/depends/$TARGET_HOST_TRIPLET/lib"
    export PKG_CONFIG_PATH+="`pwd`/depends/$TARGET_HOST_TRIPLET/lib/pkgconfig"
fi

./autogen.sh
if [ "$DEPENDS" ]; then
    ./configure \
    --prefix="${PREFIX}" \
    --disable-maintainer-mode \
    --disable-dependency-tracking \
    --enable-static \
    --disable-shared
else
    ./configure
fi
make
