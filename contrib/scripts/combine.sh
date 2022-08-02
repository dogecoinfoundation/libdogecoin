#!/bin/bash
export LC_ALL=C
set -e -o pipefail

# helper script to combine static libraries together. basic usage:
# ./contrib/scripts/combine.sh --target .libs/libdogecoin.a --append "src/secp256k1/.libs/libsecp256k1.a src/secp256k1/.libs/libsecp256k1_precomputed.a"

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

TARGET_LIB=""
LIBS_TO_APPEND=""
LIBS=""

if has_param '--target' "$@"; then
    TARGET_LIB="$2"
fi

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

detect_os
OS=$machine
echo $OS
if has_param '--append' "$@"; then
    LIBS_TO_APPEND=($4)
    END=$((${#LIBS_TO_APPEND[@]} - 1))
    for i in "${!LIBS_TO_APPEND[@]}"
    do
    :
        if [ $OS == "mac" ]; then
            LIBS+="${LIBS_TO_APPEND[$i]} "
        else
            LIBS+="ADDLIB ${LIBS_TO_APPEND[$i]}"
            if [ "$i" != "$END" ]; then
                LIBS+="\n"
            fi
        fi
    done
fi

case $OS in 
"mac")
libtool -static -o $TARGET_LIB $TARGET_LIB $LIBS
;;
"linux") 
ar -M << EOM 
    OPEN $(echo $TARGET_LIB)
    $(echo -e $LIBS)
    SAVE
    END
EOM
;;
esac
