#!/bin/bash
export LC_ALL=C
set -e -o pipefail

PROJECT_ROOT=$(git rev-parse --show-toplevel 2> /dev/null)

$PROJECT_ROOT/contrib/scripts/check_dir.sh

SDK_URL=https://bitcoincore.org/depends-sources/sdks
SDK_SHASUM="66c4d0756e3f5c6303643410c99821db8ec91b600f423b29788d1d7b9f35e2c1"
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
