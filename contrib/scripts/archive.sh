#!/bin/bash
export LC_ALL=C
set -e -o pipefail

if [ $# -eq 0 ]; then
    echo "No arguments provided (archive.sh)"
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

check_tools tar

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

ERROR=""
TARGET_HOST_TRIPLET=""
if has_param '--host' "$@"; then
    case "$2" in
        "arm-linux-gnueabihf")
            TARGET_HOST_TRIPLET=$2
            shift 2
        ;;
        "aarch64-linux-gnu")
            TARGET_HOST_TRIPLET=$2
            shift 2
        ;;
        "x86_64-w64-mingw32")
            TARGET_HOST_TRIPLET=$2
            shift 2
        ;;
        "i686-w64-mingw32")
            TARGET_HOST_TRIPLET=$2
            shift 2
        ;;
        "x86_64-apple-darwin15")
            TARGET_HOST_TRIPLET=$2
            shift 2
        ;;
        "arm64-apple-darwin")
            TARGET_HOST_TRIPLET=$2
            shift 2
        ;;
        "x86_64-pc-linux-gnu")
            TARGET_HOST_TRIPLET=$2
            shift 2
        ;;
        "i686-pc-linux-gnu")
            TARGET_HOST_TRIPLET=$2
            shift 2
        ;;
        *)
            ERROR=1
        ;;
    esac
    export TARGET_HOST_TRIPLET
fi

if [ "$ERROR" == 1 ]; then
    echo "Unrecognized host. Please provide a known host and try again."
    exit 1
fi

TARGET_DIRECTORY=`find . -maxdepth 2 -type d -regex ".*libdogecoin.*$TARGET_HOST_TRIPLET"`
if [ "$TARGET_DIRECTORY" != "" ]; then
    IFS="/" read -ra PARTS <<< "$TARGET_DIRECTORY"
    basedir="${PARTS[1]}"
    targetdir=$( echo ${TARGET_DIRECTORY##*/} )
    if [[ "$TARGET_HOST_TRIPLET" == *-w64-mingw32 ]]; then
        pushd $basedir/
            find $targetdir -not -name "*.dbg" | sort | zip -X@ $targetdir.zip
        popd
    else
        pushd $basedir
            find $targetdir -not -name "*.dbg" | sort | tar --no-recursion --mode='u+rw,go+r-w,a+X' --owner=0 --group=0 -c -T - | gzip -9n > $targetdir.tar.gz
        popd
    fi
fi
