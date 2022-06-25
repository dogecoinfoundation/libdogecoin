#!/bin/bash
export LC_ALL=C
set -e -o pipefail

# please note that this script should be ran as the current user but with sudo privileges as seen below:
# sudo -u $(whoami) -H bash -c "./contrib/scripts/setup.sh --host <host triplet> --depends"
# unless of course you are targetting docker which will not allow and should not be used with 'sudo'.

PROJECT_ROOT=$(git rev-parse --show-toplevel 2> /dev/null)
$PROJECT_ROOT/contrib/scripts/check_dir.sh

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

detect_os() {
    uname_out="$(uname -s)"
    case "${uname_out}" in
        Linux*)     machine=linux;;
        Darwin*)    machine=mac;;
        CYGWIN*)    machine=cygwin;;
        MINGW*)     machine=mingw;;
        *)          machine="unknown:${uname_out}"
    esac
}

TARGET_HOST_TRIPLET=""
TARGET_ARCH=""

if has_param '--host' "$@"; then
    case "$2" in
        "arm-linux-gnueabihf")
            qemu-arm -E LD_LIBRARY_PATH=/usr/arm-linux-gnueabihf/lib/ /usr/arm-linux-gnueabihf/lib/ld-linux-armhf.so.3 ./tests
        ;;
        "aarch64-linux-gnu")
            qemu-aarch64 -E LD_LIBRARY_PATH=/usr/aarch64-linux-gnu/lib/ /usr/aarch64-linux-gnu/lib/ld-linux-aarch64.so.1 ./tests
        ;;
        "x86_64-w64-mingw32")
            make check -j"$(getconf _NPROCESSORS_ONLN)" V=1
        ;;
        "i686-w64-mingw32")
            make check -j"$(getconf _NPROCESSORS_ONLN)" V=1
        ;;
        "x86_64-apple-darwin14")
            if [[ detect_os == "darwin" ]]; then
                ./tests
            fi
        ;;
        "x86_64-pc-linux-gnu") 
            make check -j"$(getconf _NPROCESSORS_ONLN)" V=1
            python3 tooltests.py
            ./wrappers/python/pytest/cython_tests.sh
            ./wrappers/golang/libdogecoin/build.sh
        ;;
        "i686-pc-linux-gnu")
            make check -j"$(getconf _NPROCESSORS_ONLN)" V=1
        ;;
    esac
fi
