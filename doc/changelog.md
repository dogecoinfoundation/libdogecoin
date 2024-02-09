# Changelog

## [Released]

## [0.1.3] - 2024-02-07
* logdb: adds files and tests for spv node wallet database by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/110.
* depends: add build support for arm64-apple-darwin by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/111.
* added libdogecoin-config.h to install by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/112.
* ci/codeql: bump node.js actions from 12 to 16 by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/113.
* config: added config flag for unistring by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/115.
* spv/wallet: add files and tests by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/117.
* cmake: added use_unistring symbol by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/118.
* additional secp256k1 source exclusions by @Eshnek in https://github.com/dogecoinfoundation/libdogecoin/pull/119.
* spvnode/wallet: support multiple watch addresses per init by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/120.
* qa: omit p2wpkh section from test_wallet by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/121.
* build: enable building shared lib via cmake by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/122.
* fix minor typo in readme by @themagic314 in https://github.com/dogecoinfoundation/libdogecoin/pull/123.
* ci: update mac osx sdk checksum by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/124.
* scrypt: add files and test by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/125.
* bug-fix: python wrapper missing unistring by @joijuke in https://github.com/dogecoinfoundation/libdogecoin/pull/126.
* python wrapper better setup practice by @joijuke in https://github.com/dogecoinfoundation/libdogecoin/pull/127.
* wallet: add get vout and amount functions and expose koinu str funcs by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/128.
* restruct python wrapper by @joijuke in https://github.com/dogecoinfoundation/libdogecoin/pull/129.
* map: add files and use in deserialize_dogecoin_auxpow_block by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/130.
* wallet: fix rehydration of waddr_rbtree and route wtx to proper vector by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/131.
* spvnode: added wallet files by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/132.
* wallet: make dogecoin_wallet_scrape_utxos account for edge case by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/133.
* map: remove extraneous swap_bytes function from map.c by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/134.
* qa/spv: shorten block duration on ibd and switch to testnet for spv_test by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/135.
* wrappers: remove wrappers dir, decouple from ci/codeql by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/136.
* spvnode: added headers files by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/137.
* wallet: prevent duplicate utxos from being added to unspent vector by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/138.
* depends: add support for android by @alamshafil in https://github.com/dogecoinfoundation/libdogecoin/pull/140.
* wallet: fix dogecoin_wallet_unregister_watch_address_with_node by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/142.
* 0.1.3 dev expose tools by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/143.
* validation: adds block and header checks by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/144.
* lib: expose p2pkh utility functions by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/145.
* dogecoin_tx_out function in header by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/147.
* Added bip and private key utilities by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/148.
* address: adds getHDNodeAndExtKeyByPath, getHDNodePrivateKeyWIFByPath by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/151.
* TPM2 crypto for mnemonics, seeds and keys on windows by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/153.
* Improve HD address derivation by @chromatic in https://github.com/dogecoinfoundation/libdogecoin/pull/154.
* lib: added key string constants, chainparams and bip32/44 wrappers by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/158.
* 0.1.3 dev openenclave by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/159.
* cli: addressed compiler warnings in such and spvnode by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/160.
* global updates to constants by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/162.
* software encrypt/decrypt with cli tools by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/163.
* validation: updated scrypt and pow by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/164.
* seal: added test_passwd to tpm functions by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/165.
* seal: added dogecoin_free and dogecoin_mem_zero of passwords by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/166.
* spvnode: updated usage by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/167.
* docs: updated tools.md for spvnode by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/168.
* utils: added getpass by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/169.
* seal: added encrypted store directory by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/170.
* vector: updated memory allocation in deserialize by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/171.
* validation: added scrypt-sse2 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/172.
* block: added parent merkle check for auxpow by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/173.
* hash: added dogecoin_hashwriter_free by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/174.
* such: fix mantissa during tx edit by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/175.
* ci/ql: added enable-test-passwd option by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/176.
* tx: emulate tx_in witness_stack vector in tx deser by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/177.
* wallet: free waddrs in dogecoin_wallet_init by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/178.
* headersdb_file: updated dogecoin_headers_db_connect_hdr to reorg by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/179.
* wallet: redesign utxo and radio doge functions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/180.
* net: updated check to connect nodes by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/183.
* ci: added sign jobs for windows and macos by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/184.
* wallet: added prompt to dogecoin_wallet_load by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/185.
* chainparams: update chain_from_b58_prefix to detect testnet and regtest by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/187.
* wallet: clear memory leaks from radio doge functions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/188.
* spv: removed reject on invalid block by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/189.
* headersdb_file: updated reorg to find common ancestor with memcmp by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/190.
* ci: added tag check to sign actions by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/191.
* ci: added test for aarch64-android by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/192.
* bip39: added fclose to error conditions by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/193.
* spv: optimize initial block download by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/194.
* ci: reduced uploads for signed builds by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/195.
* tool: updated pubkey_from_privatekey param by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/196.
* gitian: bump build system to focal from bionic by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/197.
* cmake: added build type for msvc by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/198.
* trivial: add copyright script and update copyrights by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/199.
* doc: update changelog.md authored by @edtubbs and committed by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/201.
* ci: bump to actions/cache@v4 for android by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/202.
* ci: config arm64-apple-darwin runner by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/203.
* fixate v0.1.3 by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/200.

## New Contributors
* @alamshafil made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/140
* @Eshnek made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/119
* @joijuke made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/126
* @chromatic made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/154

**Full Changelog**: https://github.com/dogecoinfoundation/libdogecoin/compare/v0.1.2...v0.1.3


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
