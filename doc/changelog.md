# Changelog

## [Released]

## [0.1.2] - 2023-03-22

## What's Changed
* doc: update transaction signing definitions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/82
* build: fix up cmake on linux by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/90
* libdogecoin: added wrapper for bip39 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/91
* ci: bump i686-pc-linux-gnu from bionic to focal by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/95
* build: add msvs support with cmake by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/93
* docs: added bip39 seedphrases and libunistring by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/96
* 0.1.2 dev QR support by @michilumin in https://github.com/dogecoinfoundation/libdogecoin/pull/94
* utils: add dogecoin_network_enabled function by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/87
* utils: fix missing libdogecoin-config header by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/97
* added jpeg qr functionality using a modified version of jpec by @michilumin in https://github.com/dogecoinfoundation/libdogecoin/pull/100
* docs: finalize derived hd address functions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/99
* constants: add header with address definitions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/101
* doc: updated guidance on bip39 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/102
* build: add extra line to eof's by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/103
* Add Moon Files by @qlpqlp in https://github.com/dogecoinfoundation/libdogecoin/pull/98
* sign: add message signing and verification by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/104
* Add key to signing-keys by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/106
* build: combine libunistring.a in gitian descriptors by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/107
* docs: update changelog.md by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/108
* fixate 0.1.2 as release by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/109

## New Contributors
* @edtubbs made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/91
* @qlpqlp made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/98

**Full Changelog**: https://github.com/dogecoinfoundation/libdogecoin/compare/v0.1.1...v0.1.2


## [Released]

## [0.1.1] - 2022-10-03

## What's Changed
* fixate 0.1.0 by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/59
* open 0.1.1-dev for development by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/60
* Fix all go get errors caused by rename by @quackduck in https://github.com/dogecoinfoundation/libdogecoin/pull/64
* fix bad path for python wrapper in docs by @just-an-dev in https://github.com/dogecoinfoundation/libdogecoin/pull/68
* Fix for dogecoin_script_copy_without_op_codeseperator todo by @nooperation in https://github.com/dogecoinfoundation/libdogecoin/pull/72
* Remove VLAs (variable-length-arrays) from the code.  Fix some allocations. by @michilumin in https://github.com/dogecoinfoundation/libdogecoin/pull/75
* Fixed memory cleanup issue in dogecoin_base58_encode_check and updated its declaration by @nooperation in https://github.com/dogecoinfoundation/libdogecoin/pull/76
* address: fix memleaks caused from excessive key lengths by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/77
* address: adds getDerivedHDAddress functions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/78
* Fixed command and ouputs for HD by @nformant1 in https://github.com/dogecoinfoundation/libdogecoin/pull/79
* (API Change) Fixed the truncation of size_t's to int's in some places by @nooperation in https://github.com/dogecoinfoundation/libdogecoin/pull/80

## New Contributors
* @quackduck made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/64
* @just-an-dev made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/68
* @nooperation made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/72
* @nformant1 made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/79

**Full Changelog**: https://github.com/dogecoinfoundation/libdogecoin/compare/v0.1.0...v0.1.1


## [Released]

## [0.1.0] - 2022-08-05

## What's Changed
* docs: mv diagrams/ to doc/ and amend README.md by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/6
* 0.1-dev-autoreconf by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/8
* qa: omit python from codeql by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/9
* crypto: sha2, rmd160 by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/11
* deps: bitcoin-core/secp256k1 subtree by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/12
* feature: address by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/13
* Example doc format by @tjstebbing in https://github.com/dogecoinfoundation/libdogecoin/pull/15
* Creating first Python wrapper PR by @jaxlotl in https://github.com/dogecoinfoundation/libdogecoin/pull/14
* qa: address_test by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/18
* contrib: formatting by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/21
* Verify address by @jaxlotl in https://github.com/dogecoinfoundation/libdogecoin/pull/22
* mem: fix memleaks by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/24
* Python module refactoring by @jaxlotl in https://github.com/dogecoinfoundation/libdogecoin/pull/25
* Fixing compiler warnings - new_line by @DrinoSan in https://github.com/dogecoinfoundation/libdogecoin/pull/29
* Documentation by @jaxlotl in https://github.com/dogecoinfoundation/libdogecoin/pull/27
* Fixing unit_tests - Increase size of char array by @DrinoSan in https://github.com/dogecoinfoundation/libdogecoin/pull/32
* C improved tests by @jaxlotl in https://github.com/dogecoinfoundation/libdogecoin/pull/31
* Setting fixed size for priv and pubkeys in generatePrivPubKeypair and… by @DrinoSan in https://github.com/dogecoinfoundation/libdogecoin/pull/34
* such: transaction by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/33
* security: refactor koinu conversion functions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/42
* ci: fix apt-get update step for i686-w64-mingw32 by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/45
* issue template updated to prevent spam in repository by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/44
* transaction: remove all refs to segwit and bech32 by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/46
* security: implement refactored conversion functions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/43
* trivial: fix up headers by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/47
* include: delete valgrind/valgrind.h by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/48
* cmake: add koinu to CMakeLists.txt by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/49
* crypto: fix mismatched bound on sha256/512_finalize by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/51
* tx: remove bloat from dogecoin_tx_sign_input by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/52
* net: move broadcast_tx from tx to net by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/50
* trivial: fix remaining GCC warnings/errors by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/53
* contrib: update expired signing key for xanimo by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/55
* build: backport autotools/gitian build system by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/54
* doc: update changelog by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/58

## Contributors
* @DrinoSan
* @jaxlotl
* @michilumin
* @tjstebbing
* @xanimo

**Full Changelog**: https://github.com/dogecoinfoundation/libdogecoin/commits/main
