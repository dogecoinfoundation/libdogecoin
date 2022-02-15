#!/usr/bin/env sh
update_subtree() {
    # set parameters
    REMOTE="$1"
    LOCAL_SUBDIR="$2"
    REMOTE_SUBDIR="$3"

    # get content from repository
    git fetch "$REMOTE"
    FETCH_HEAD="$(git describe --always FETCH_HEAD)"
    SOURCE_HEAD="$FETCH_HEAD"

    # if a remote subdir is set, split the content to only have that subdir
    if test "x$REMOTE_SUBDIR" != "x"
    then
        touch "$REMOTE_SUBDIR" # this hack lets git-subtree split a non-worktree branch
        SOURCE_HEAD="$(git subtree split --prefix "$REMOTE_SUBDIR" "$FETCH_HEAD")"
    fi

    MESSAGE="Merge subtree $LOCAL_SUBDIR from $REMOTE $REMOTE_SUBDIR $FETCH_HEAD"

    # merge or add the subtree into the worktree
    git subtree merge --squash --prefix src/"$LOCAL_SUBDIR" "$SOURCE_HEAD" --message "$MESSAGE" ||
        git subtree add --squash --prefix src/"$LOCAL_SUBDIR" "$SOURCE_HEAD" --message "$MESSAGE"
}

copy_headers() {
    # set single parameter
    SRCDIR="src/$1"
    DSTDIR="include/dogecoin"
    shift 1

    # for each header, mutate it to be like libdogecoin, and copy it in with a commit
    for HEADER in "$@"
    do
        HEADERNAME=$(basename "$HEADER")
        { sed -f /dev/stdin "$SRCDIR"/"$HEADER" > "$DSTDIR"/"$HEADERNAME" <<HEADER_MUTATION_END
            # prefix ifndef/define guard symbols with LIBDOGECOIN_ and add LIBDOGECOINL_BEGIN_DECL
            s/^\(#ifndef _\)_*\([^_]*.*[^_]\)_*_$/\1_LIBDOGECOIN_\2__/
            s/^\(#define _\)_*\([^_]*.*[^_]\)_*_\(\s.*$\|$\)/\1_LIBDOGECOIN_\2__\n&\n\n#include "dogecoin.h"\n\nLIBDOGECOIN_BEGIN_DECL/

            # prefix functions with LIBDOGECOIN_API
            s/^\(int\|void\|char\|bool\)/LIBDOGECOIN_API \1/

            # add LIBDOGECOIN_END_DECL before end
            $ s/^\(#endif\)$/LIBDOGECOIN_END_DECL\n\n\1/
HEADER_MUTATION_END
        } &&
        git add "$DSTDIR"/"$HEADERNAME" &&
        git commit -m "Update $DSTDIR/$HEADERNAME from $SRCDIR/$HEADER"
    done
}

# subtrees
update_subtree https://github.com/bitcoin-core/secp256k1 secp256k1
