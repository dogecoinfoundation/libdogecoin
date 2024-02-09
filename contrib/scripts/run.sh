#!/bin/bash

# run depends: contrib/scripts/run.sh --host <host triple> --depends
# run: contrib/scripts/run.sh --host <host triple>

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
ALL_HOST_TRIPLETS=""

if has_param '--depends' "$@"; then
    DEPENDS=1
fi

if has_param '--host' "$@"; then
    if has_param '--all' "$@"; then
        ALL_HOST_TRIPLETS=("x86_64-pc-linux-gnu" "i686-pc-linux-gnu" "aarch64-linux-gnu" "arm-linux-gnueabihf" "x86_64-apple-darwin15" "arm64-apple-darwin" "x86_64-w64-mingw32" "i686-w64-mingw32")
    else
        ALL_HOST_TRIPLETS=($2)
    fi
fi

if [[ "$TARGET_HOST_TRIPLET" == "" && "$ALL_HOST_TRIPLETS" != "" ]]; then
    echo $ALL_HOST_TRIPLETS
    END=$((${#ALL_HOST_TRIPLETS[@]} - 1))
    for i in "${!ALL_HOST_TRIPLETS[@]}"
    do
    :
        TARGET_HOST_TRIPLET="${ALL_HOST_TRIPLETS[$i]}"
        CLEAN=""
        # clean previously built files in libdogecoin and libsecp256k1
        if has_param '--clean' "$@"; then
            FILE=Makefile
            if test -f "$FILE"; then
                make clean
                make clean-local
            fi
            CLEAN=1
        fi
        
        # clean MacOS SDKs
        if [ "$DEPENDS" = "1" ]; then
            if [ "$CLEAN" = "1" ]; then
                git clean -xdff --exclude='/depends/SDKs/*'
            fi
            DEPENDS="--depends"
        fi

        DOCKER=""
        if has_param '--docker' "$@"; then
            DOCKER="--docker"
        fi

        if has_param '--setup' "$@"; then
            ./contrib/scripts/setup.sh --host $TARGET_HOST_TRIPLET $DEPENDS $DOCKER
        fi

        if has_param '--build' "$@"; then
            ./contrib/scripts/build.sh --host $TARGET_HOST_TRIPLET $DEPENDS
        fi
        
        if has_param '--test' "$@"; then
            GO=""
            CYTHON=""
            VALGRIND=""
            EXTENDED=""
            if has_param '--go' "$@"; then
                GO="--go"
            fi
            if has_param '--cython' "$@"; then
                CYTHON="--cython"
            fi
            if has_param '--valgrind' "$@"; then
                VALGRIND="--valgrind"
            fi
            if has_param '--extended' "$@"; then
                EXTENDED="--extended"
            fi
            ./contrib/scripts/test.sh --host $TARGET_HOST_TRIPLET $EXTENDED $VALGRIND $CYTHON $GO $DOCKER
        fi
    done
fi
