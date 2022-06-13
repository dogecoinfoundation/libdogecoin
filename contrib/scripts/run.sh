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

if has_param '--depends' "$@"; then
    DEPENDS=1
fi

if has_param '--host' "$@"; then
    TARGET_HOST_TRIPLET=$2
fi

if [ "$DEPENDS" = "1" ]; then
    if has_param '--clean' "$@"; then
        FILE=Makefile
        if test -f "$FILE"; then
            make clean
        fi
        git clean -xdff --exclude='/depends/SDKs/*'
    fi
    ./contrib/scripts/setup.sh --host $TARGET_HOST_TRIPLET --depends
    ./contrib/scripts/build.sh --host $TARGET_HOST_TRIPLET --depends
else
    ./contrib/scripts/setup.sh --host $TARGET_HOST_TRIPLET
    ./contrib/scripts/build.sh --host $TARGET_HOST_TRIPLET
fi
