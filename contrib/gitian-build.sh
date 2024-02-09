#!/bin/bash

# inspired by the work of @patricklodder for gitian building dogecoin core as found below:
# https://gist.github.com/patricklodder/88d6c4e3406db840963f85d95ceb44fe
# usage: ./gitian-build.sh --mem=8000 --proc=2 --commit=0.1-dev-rc1 --url=https://github.com/xanimo/libdogecoin --sign=xanimo --docker

export LC_ALL=C
set -e -o pipefail

if [ $# -eq 0 ]; then
    echo "No arguments provided"
    exit 1
fi

check_error() {
    if [ "$ERROR" ]; then
        echo "Please provide a commit or tag to build and try again."
        exit $ERROR
    fi
}

help()
{
    echo "Usage: build 
               [ -c | --commit ]
               [ --docker ]
               [ --lxc ]
               [ -m | --mem ]
               [ -p | --proc ]
               [ -s | --sign ]
               [ -t | --tag ]
               [ -u | --url ]
               [ -h | --help  ]"
    exit 2
}

BUILD_SUFFIX="`pwd`/gitian/builds"
DESCRIPTORS=('osx' 'win' 'linux')

export USE_DOCKER=0
export USE_LXC=0

for i in "$@"
do
case $i in
    -c=* | --commit=* )
      export COMMIT="${i#*=}"
    ;;
    --docker )
      export USE_DOCKER=1
    ;;
    --help )
        help
    ;;
    --lxc )
      export USE_LXC=1
    ;;
    -m=* | --mem=* )
      export MEM="${i#*=}"
      ;;
    -p=* | --proc=* )
      export PROC="${i#*=}"
      ;;
    -s=* | --sign=* )
      export SIGNER="${i#*=}"
      ;;
    -t=* | --tag=* )
      export TAG="${i#*=}"
    ;;
    -u=* | --url=* )
      export URL="${i#*=}"
      ;;
    *)
        ERROR=1
    ;;
esac
done

check_error

if [ ! "$USE_LXC" ] && [ ! "$USE_DOCKER" ]; then
    echo "Please choose either --docker or --lxc and try again."
    exit 1
fi

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
        COMMIT="v$TAG"
    fi
    BUILD_SUFFIX=$BUILD_SUFFIX/$COMMIT
fi
export COMMIT=$COMMIT

if [ ! -d "gitian" ]; then
    mkdir -p gitian
fi

pushd gitian
if [ ! -d "gitian-builder" ]; then
    git clone https://github.com/devrandom/gitian-builder.git
fi

if [ ! -d "libdogecoin" ]; then
    git clone $URL
fi

pushd libdogecoin
    git checkout ${COMMIT}
popd

pushd gitian-builder
if [ ! -d "patches" ]; then
    mkdir -p patches
fi

if [ ! -f "patches/fix-mirror_base.patch" ]; then
    wget -P patches https://gist.githubusercontent.com/xanimo/aeb7d031bc5ced761f8b2a28af3779ae/raw/241426ddcf5d272e02fe2b9fd7019afddea0f67a/fix-mirror_base.patch
    git apply patches/fix-mirror_base.patch
fi

if [ "$USE_DOCKER" ]; then
    bin/make-base-vm --docker --suite focal --arch amd64
elif [ "$USE_LXC" ]; then
    bin/make-base-vm --lxc --suite focal --arch amd64
fi

if [ ! -d "inputs" ]; then
    mkdir -p inputs
fi

if [ ! -f "inputs/osslsigncode-Backports-to-1.7.1.patch" ]; then
    wget -P inputs https://depends.dogecoincore.org/osslsigncode-Backports-to-1.7.1.patch
fi

if [ ! -f "inputs/osslsigncode_1.7.1.orig.tar.gz" ]; then
    wget -P inputs https://depends.dogecoincore.org/osslsigncode_1.7.1.orig.tar.gz
fi

if [ ! -f "inputs/Xcode-12.2-12B45b-extracted-SDK-with-libcxx-headers.tar.gz" ]; then
    wget -P inputs https://bitcoincore.org/depends-sources/sdks/Xcode-12.2-12B45b-extracted-SDK-with-libcxx-headers.tar.gz
fi

make -C ../libdogecoin/depends download SOURCES_PATH=`pwd`/cache/common

if [ ! -d "${BUILD_SUFFIX}" ]; then
    mkdir -p ${BUILD_SUFFIX}
fi

./bin/gbuild -m ${MEM} -j ${PROC} --commit libdogecoin=${COMMIT} --url libdogecoin=${URL} ../libdogecoin/contrib/gitian-descriptors/gitian-linux.yml
if [ "$SIGNER" ]; then
./bin/gsign --signer "$SIGNER" --release "$COMMIT"-"linux" \
                --destination ${BUILD_SUFFIX}/sigs/ ../libdogecoin/contrib/gitian-descriptors/gitian-linux.yml 2>&- || \
                echo "$0: Error on signature, detached signing"
fi
mv build/out/src/libdogecoin-*.tar.gz ${BUILD_SUFFIX}

./bin/gbuild -m ${MEM} -j ${PROC} --commit libdogecoin=${COMMIT} --url libdogecoin=${URL} ../libdogecoin/contrib/gitian-descriptors/gitian-win.yml
if [ "$SIGNER" ]; then
./bin/gsign --signer "$SIGNER" --release "$COMMIT"-"win" \
                --destination ${BUILD_SUFFIX}/sigs/ ../libdogecoin/contrib/gitian-descriptors/gitian-win.yml 2>&- || \
                echo "$0: Error on signature, detached signing"
fi
mv build/out/src/libdogecoin-*.zip ${BUILD_SUFFIX}

./bin/gbuild -m ${MEM} -j ${PROC} --commit libdogecoin=${COMMIT} --url libdogecoin=${URL} ../libdogecoin/contrib/gitian-descriptors/gitian-osx.yml
if [ "$SIGNER" ]; then
./bin/gsign --signer "$SIGNER" --release "$COMMIT"-"osx" \
                --destination ${BUILD_SUFFIX}/sigs/ ../libdogecoin/contrib/gitian-descriptors/gitian-osx.yml 2>&- || \
                echo "$0: Error on signature, detached signing"
fi
mv build/out/src/libdogecoin-*.tar.gz ${BUILD_SUFFIX}

popd

pushd $BUILD_SUFFIX
if [ -f "checksums.txt" ]; then
    rm checksums.txt
fi
sha256sum *.tar.gz > checksums.txt
sha256sum *.zip >> checksums.txt
cat checksums.txt
popd
