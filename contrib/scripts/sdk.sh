#!/bin/bash
export LC_ALL=C
set -e -o pipefail

PROJECT_ROOT=$(git rev-parse --show-toplevel 2> /dev/null)

$PROJECT_ROOT/contrib/scripts/check_dir.sh

SDK_URL=https://bitcoincore.org/depends-sources/sdks
SDK_SHASUM="df75d30ecafc429e905134333aeae56ac65fac67cb4182622398fd717df77619"
SDK_VERSION=12.2
SDK_BUILD="12B45b"
SDK_FILENAME=Xcode-$SDK_VERSION-$SDK_BUILD-extracted-SDK-with-libcxx-headers.tar.gz

echo $SDK_FILENAME
mkdir -p ./depends/sdk-sources
mkdir -p ./depends/SDKs
echo "$SDK_SHASUM depends/sdk-sources/$SDK_FILENAME" | sha256sum -c || \
curl --location --fail $SDK_URL/$SDK_FILENAME -o depends/sdk-sources/$SDK_FILENAME && \
echo "$SDK_SHASUM depends/sdk-sources/$SDK_FILENAME" | sha256sum -c
tar -C depends/SDKs -xf depends/sdk-sources/$SDK_FILENAME
