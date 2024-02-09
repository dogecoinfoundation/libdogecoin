#!/bin/bash
export LC_ALL=C
set -e -o pipefail

if [ $# -eq 0 ]; then
    echo "No arguments provided"
    exit 1
fi

check_tools() {
    for cmd in "$@"; do
        if ! command -v "$cmd" > /dev/null 2>&1; then
            echo "ERR: This script requires that '$cmd' is installed and available in your \$PATH"
            exit 1
        fi
    done
}

check_tools ar

git_root() {
    git rev-parse --show-toplevel 2> /dev/null
}

same_dir() {
    local resolved1 resolved2
    resolved1="$(git_root)"
    resolved2="$(echo `pwd`)"
    [ "$resolved1" = "$resolved2" ]
}

if ! same_dir "${PWD}" "$(git_root)"; then
cat << EOF
ERR: This script must be invoked from the top level of the git repository
Hint: This may look something like:
    contrib/scripts/combine_lib.sh
EOF
exit 1
fi

check_error() {
    if [ "$ERROR" ]; then
        echo "Please provide a host to build and try again."
        exit $ERROR
    fi
}

BUILD_PREFIX=""
BUILD_SUFFIX=""
COMMIT=""
DOCUMENTATION="`pwd`/doc/*.md README.md"
DOCKER=""
ERROR=""
HOST=""
LIB="`pwd`/.libs/libdogecoin.a `pwd`/include/dogecoin/dogecoin.h `pwd`/include/dogecoin/libdogecoin.h `pwd`/include/dogecoin/constants.h `pwd`/include/dogecoin/uthash.h"
SHA=sha256sum
BINARY_SUFFIX=""
for i in "$@"
do
case $i in
    -c=*|--commit=*)
        COMMIT="${i#*=}"
    ;;
    -h=*|--host=*)
        HOST="${i#*=}"
    ;;
    -p=*|--prefix=*)
        BUILD_PREFIX="${i#*=}"
    ;;
    -t=*|--tag=*)
        TAG="${i#*=}"
    ;;
    *)
        ERROR=1
    ;;
esac
done

check_error

# set up build directory if it doesn't exist
if [ ! -d "$BUILD_PREFIX" ]; then
    mkdir -p `pwd`/$BUILD_PREFIX
fi

# set binary file extension and simulataneously bounds check host
if [ "$HOST" ]; then
    case "$HOST" in
        "arm-linux-gnueabihf")
        ;;
        "aarch64-linux-gnu")
        ;;
        "x86_64-w64-mingw32")
            BINARY_SUFFIX=".exe"
        ;;
        "i686-w64-mingw32")
            BINARY_SUFFIX=".exe"
        ;;
        "x86_64-apple-darwin15")
        ;;
        "arm64-apple-darwin")
        ;;
        "x86_64-pc-linux-gnu")
        ;;
        "i686-pc-linux-gnu")
        ;;
        *)
            ERROR=1
        ;;
    esac
    BUILD_SUFFIX="libdogecoin"
    EXE="`pwd`/such$BINARY_SUFFIX `pwd`/sendtx$BINARY_SUFFIX `pwd`/spvnode$BINARY_SUFFIX"
    FILES="$LIB $EXE"
fi

check_error

# allow either tag or commit for git repositories
if [ "$TAG" ] && [ "$COMMIT" ]; then
    echo "Please specify only a commit or a tag and try again."
    exit 1
else
    if [ "$TAG" ]; then
        # logic may need refining
        if git rev-parse -q --verify "refs/tags/$TAG" >/dev/null; then
            echo "$TAG is a legitimate commit"
            # git tag -f -s $TAG -m "$TAG"
        else
            echo "$TAG does not exist. Exiting..."
            exit 1
            # git tag -s $TAG -m "$TAG"
        fi
        BUILD_SUFFIX=$BUILD_SUFFIX-$TAG
    else 
        if [ "$COMMIT" ]; then
            BUILD_SUFFIX=$BUILD_SUFFIX-$COMMIT
        fi
    fi
fi
BUILD_SUFFIX=$BUILD_SUFFIX-$HOST

if [ ! -d "output" ]; then
    mkdir -p "`pwd`/output"
fi

if [ -d "output/$BUILD_SUFFIX" ]; then
    rm -rf "output/$BUILD_SUFFIX"
fi

if [ ! -d "$BUILD_PREFIX/$BUILD_SUFFIX" ]; then
    mkdir -p "$BUILD_PREFIX/$BUILD_SUFFIX"
fi

cp -r $FILES "$BUILD_PREFIX/$BUILD_SUFFIX"

if [ ! -f "$BUILD_PREFIX/$BUILD_SUFFIX/checksums.txt" ]; then
    pushd $BUILD_PREFIX/$BUILD_SUFFIX
        $SHA * > checksums.txt
        if [ ! -d "`pwd`/bin" ]; then
            mkdir -p `pwd`/bin
        fi
        mv such* sendtx* spvnode* "`pwd`/bin"
    popd
fi

if [ ! -d "$BUILD_PREFIX/$BUILD_SUFFIX/lib" ]; then
    mkdir -p "$BUILD_PREFIX/$BUILD_SUFFIX/lib"
fi

mv "$BUILD_PREFIX/$BUILD_SUFFIX/libdogecoin.a" "$BUILD_PREFIX/$BUILD_SUFFIX/lib/libdogecoin.a"
mv `pwd`/depends/$HOST/lib/libunistring.a "$BUILD_PREFIX/$BUILD_SUFFIX/lib/libunistring.a"
mv `pwd`/depends/$HOST/lib/libevent_core.a "$BUILD_PREFIX/$BUILD_SUFFIX/lib/libevent_core.a"

if [ ! -d "$BUILD_PREFIX/$BUILD_SUFFIX/include" ]; then
    mkdir -p "$BUILD_PREFIX/$BUILD_SUFFIX/include"
fi

mv "$BUILD_PREFIX/$BUILD_SUFFIX/dogecoin.h" "$BUILD_PREFIX/$BUILD_SUFFIX/include/dogecoin.h"
mv "$BUILD_PREFIX/$BUILD_SUFFIX/libdogecoin.h" "$BUILD_PREFIX/$BUILD_SUFFIX/include/libdogecoin.h"
mv "$BUILD_PREFIX/$BUILD_SUFFIX/constants.h" "$BUILD_PREFIX/$BUILD_SUFFIX/include/constants.h"
mv "$BUILD_PREFIX/$BUILD_SUFFIX/uthash.h" "$BUILD_PREFIX/$BUILD_SUFFIX/include/uthash.h"

if [ ! -d "$BUILD_PREFIX/$BUILD_SUFFIX/docs" ]; then
    mkdir -p "$BUILD_PREFIX/$BUILD_SUFFIX/docs"
fi

cp -r $DOCUMENTATION "$BUILD_PREFIX/$BUILD_SUFFIX/docs"
pushd "$BUILD_PREFIX/$BUILD_SUFFIX/docs"
    rm project_roadmap.md release-process.md
popd

if [ ! -d "$BUILD_PREFIX/$BUILD_SUFFIX/docs" ]; then
    mkdir -p "$BUILD_PREFIX/$BUILD_SUFFIX/docs"
fi

if [ ! -d "$BUILD_PREFIX/$BUILD_SUFFIX/examples" ]; then
    mkdir -p "$BUILD_PREFIX/$BUILD_SUFFIX/examples"
fi

cp `pwd`/contrib/examples/example.c "$BUILD_PREFIX/$BUILD_SUFFIX/examples/example.c"
cp `pwd`/LICENSE "$BUILD_PREFIX/$BUILD_SUFFIX/LICENSE"

./contrib/scripts/archive.sh --host $HOST

if [ -f `pwd`/$BUILD_PREFIX/*.tar.gz ]; then
    cp -r `pwd`/$BUILD_PREFIX/*.tar.gz `pwd`/output
fi

if [ -f `pwd`/$BUILD_PREFIX/*.zip ]; then
    cp -r `pwd`/$BUILD_PREFIX/*.zip `pwd`/output
fi

# clean up
rm -rf $BUILD_PREFIX
