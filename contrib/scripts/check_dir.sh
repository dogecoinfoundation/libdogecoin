#!/bin/bash
export LC_ALL=C
set -e -o pipefail

git_root() {
    git rev-parse --show-toplevel 2> /dev/null
}

same_dir() {
    local resolved1 resolved2
    resolved1="$(git_root)"
    resolved2="$(echo `pwd`)"
    [ "$resolved1" = "$resolved2" ]
}

print_warning() {
cat << EOF
ERR: This script must be invoked from the top level of the git repository
Hint: This may look something like:
    contrib/scripts/run.sh
EOF
}

if ! same_dir "${PWD}" "$(git_root)"; then
    print_warning
    exit 1
fi
