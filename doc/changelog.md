# Changelog

## [Released]

## [0.1.5] - 2026-08-25
## What's Changed
* src: added utf8proc, updated bip39 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/271
* spv, wallet, ci: quit on q/Q, skip prompts with address, refresh macOS cmake by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/272
* wallet: update init params to opts, utxo height on reorg by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/273
* src, doc: added entropy size to such by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/281
* rest: added getRawTx and viewTx by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/274
* rest, spv, doc: added /stats and /chainStats for metrics by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/275
* ci: update TPL_BIN to v1.19 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/276
* ci: update to macos-14 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/283
* headersdb_file, wallet: add conversion, prompt text by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/279
* spv: added smpv for dashb0rd by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/282
* better Swift support by @radmakr in https://github.com/dogecoinfoundation/libdogecoin/pull/285
* src, doc, test: added mempool confirmations by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/286
* build, depends, src, doc, test: add liboqs PQC support and draft BIP by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/288
* build, src, doc, test: add bip37, spv filtering and smpv watchers by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/287
* rest: fix tps window by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/289
* build, validation, test: treat nVersion as signed and add tests by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/290
* scrypt: fix MSVC cpuid by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/291
* src, include, tests: use arith_uint256 for chainwork by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/292
* build, ci, doc, seal, test: make test password follow test builds by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/293
* ci, contrib: add gitian build with windows and macos code signing by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/295
* build, contrib, pqc: add PQC carrier support and Raccoon-G by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/294
* zk_carrier: add module, codec, commit/reveal helpers, tests and proof assets by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/305
* build, ci: use YUBIKEY flag, add ci target, drop gitian overrides by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/296
* seal: free memory in all paths, use dogecoin_free by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/299
* utils, ecc, ci: add DIT support, enable arm64 macOS by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/304
* depends, build, ci: add native nasm and test in ci by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/307
* hd, contrib, doc: align getDerivedHDAddressByPath with P2PKH output by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/300
* libdogecoin: expand APIs for smpv, spv, hd wallet and crypto/ecc by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/297
* ci, docs: stage via make install, add wallet API doc by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/298
* rest, tx, wallet: add /getSpends, fix /getBalance and /getTransactions by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/302
* multisig: add P2SH workflow, signing support, docs and E2E tests by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/306
* build, ci, docs, src, test: add linux TPM seal variants and swtpm by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/308
* net: break event loop on shutdown by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/309
* contrib: fix CFLAGS and LDFLAGS in build script by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/301
* src: update tx validation, use memcpy_safe, P2PKHLEN and shared constants by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/311
* spv: use SIGINT shutdown, drop stdin polling by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/312
* build, ci, src, test: add avx2 and sse double-hash by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/313
* ci: remove nixos target by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/314
* ci: bump rk3588 DDR TPL_BIN from v1.19 to v1.21 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/316
* ci, build, depends: normalize depends option flags by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/315
* psbt: add BIP174 PSBT support (all 6 roles, C API, CLI commands, docs) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/317
* test: free the height-100 wtx in reorg utxo update test by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/318
* tx: fix sigder_out guard and widen DER length check by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/319
* fix: const-correct utils_bin_to_hex decl in umbrella header by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/320
* update: apply upstream ctaes uint16_t cast hardening by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/321
* fix: widen base58_decode_check buffer hint to accept valid short payl… by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/322
* seal: use constant-time comparison for password verification hashes by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/349
* serialize: prevent length truncation in ser_str / ser_varstr by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/350
* psbt: bound key/value lengths before allocation in deser_psbt_kv by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/352
* psbt: require exact 33-byte pubkey in bip32 derivation handlers by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/353
* block: fix error-path leak in dogecoin_block_header_deserialize by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/354
* sha2: fix OOB write in hmac_sha256_prepare by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/324
* transaction: fix heap overflow in sign_raw_transaction in-place write by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/331
* logdb: fix stack overflow and unbounded allocation in record deserialization by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/345
* seal: remove redundant free in software seed decryption by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/343
* bip32/bip37: fix signed-overflow UB in big-endian word assembly by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/325
* sha2: avoid unaligned word loads in sha256_transform by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/326
* arith_uint256/jpeg: fix signed-overflow UB in bit assembly by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/327
* cstr: fix integer overflow in cstr_alloc_min_sz buffer sizing by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/344
* fix: eliminate function-pointer type-mismatch UB via typed trampolines by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/361
* ci: add ASAN+UBSAN make check gate by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/328
* ci: match bare *-dev branches in push triggers by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/371
* build, ci, docs, include, spv, src, test: stateless thread-safe refactor by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/330
* transaction: mint never-reused registry ids; stop evicting live entries by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/333
* block: fix UB and count truncation in auxpow deserialization by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/334
* wallet: mint never-reused utxo ids; stop evicting live utxos by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/335
* eckey: mint never-reused key ids; stop evicting (and leaking) live keys by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/336
* map: mint never-reused hash/map ids; stop evicting live entries by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/337
* protocol: bound getheaders locator count before allocating by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/339
* script: bound OP_PUSHDATA before allocating in copy_without_op_codeseperator by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/338
* wallet: bound transaction record length when loading from disk by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/342
* transaction: fail cleanly on malformed WIF in sign_raw_transaction by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/332
* net: dispatch only this message's payload to command handlers by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/340
* tx: dogecoin_tx_add_address_out returns false when no output is added… by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/368
* such: fix nondeterministic wrong amount when editing an output amount by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/369
* key/eckey/bip32: zeroize private-key material after use (CWE-226) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/362
* wallet: zeroize master seed, hdnode, and mnemonic in wallet_init (CWE… by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/367
* ci: force IPv4 in the openenclave build container by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/373
* net: verify message payload checksum before dispatch by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/341
* ci: phase 0 static analysis — CodeQL security-extended, cppcheck, clang-tidy by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/359
* build: add libFuzzer harness infrastructure and make fuzz target by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/351
* fuzz: PSBT (BIP174) deserializer harness + regression corpus by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/357
* coverage: measure fuzz harness reachability (stacked on #351) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/360
* cmake: build and run the Raccoon-G test suite by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/346
* cmake: link liboqs when USE_LIBOQS is enabled by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/347
* test: assert PQC wrapper output lengths and reject tampered inputs by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/348
* build, doc, src, test: add SLIP-0039 Shamir mnemonic secret sharing by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/323
* contrib: add constant-time verification tests (dudect) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/365
* seal: zeroize derived keys and hardware-path plaintext (CWE-226) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/363
* docs: security assurance case, audit plan, and sanitizer sweep by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/366
* Bip38 Support by @qlpqlp in https://github.com/dogecoinfoundation/libdogecoin/pull/277
* cli: declare option arguments consistently in spvnode and sendtx by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/374
* rpctest: stop discarding the SPV test exit code and bound its runtime by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/375
* ci: move actions off the deprecated Node.js 20 runtime by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/376
* random: fail closed on a short read from /dev/urandom (CWE-330) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/382
* ci: bound every job with timeout-minutes by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/385
* ci: build CMake natively, both WITH_NET settings by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/388
* spv: parallel genesis header download over checkpoint segments by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/378
* bip152: compact block types, serialization and negotiation (v1, pre-SegWit) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/379
* ci: fix the cppcheck gate, and run cppcheck and clang-tidy on PRs at all by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/403
* eckey, test: copy measured lengths instead of strcpy by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/418
* wallet, utils, headersdb: put null guards above the dereference by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/404
* utils, random: fix slice(), and make the TESTING RNG unbuildable in release by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/407
* wallet, seal: create private files 0600 instead of at the umask by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/412
* block: let a header own its AuxPoW proof by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/392
* block: separate parsing a header from validating it (stacked on #392) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/393
* block: serialize a header with its AuxPoW (stacked on #393) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/394
* block: serialize a whole block (stacked on #394) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/395
* block: compute merkle roots, with mutation detection (stacked on #395) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/399
* golomb: add GCS encoding for compact block filters (BIP158) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/380
* compact_filter: BIP157 messages, filter header chain and validation (stacked on #380) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/381
* cf_checkpoints: populate testnet filter headers at 10000-block spacing (stacked on #381) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/383
* cli: add YubiKey backend (-u/--yubikey) to such encrypted-key commands by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/364
* ci: document the branch naming convention, run CodeQL on every PR, cancel superseded runs by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/377
* context: move context definitions into dogecoin/context.h by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/396
* pow, validation: report the condition that actually failed by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/397
* chainparams: sync the public dogecoin_chainparams with the internal one by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/400
* bip39, qr: bound the wordlist token read; don't rely on assert for indexing by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/401
* doc: refresh the assurance status tables after the merge wave by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/402
* cfheadersdb: persist filter headers and filter data per chain (stacked on #381) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/384
* fuzz: BIP152 compact block deserializer harness (stacked on #351, #379) by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/391
* random: the Windows RNG must report failure, not -1 by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/405
* depends, libevent: update to 2.1.13-stable by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/406
* rpctest: filter tar members on extraction; document two API contracts by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/409
* such: stop writing through a string literal in the -e path by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/411
* ci: scope CodeQL to first-party code by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/413
* test: add RFC 6979 deterministic nonce regression tests by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/414
* random, optee, openenclave: set the generator through the mapper, not set_rng by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/419
* spvnode: retry peer discovery, and stop instead of hanging by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/426
* ci: drop the VS2019 build tools install from the native Windows jobs by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/427
* utils: add reentrant time formatting by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/424
* rest, spv: spell the timestamp format in specifiers Windows has by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/425
* bip38, tool, bench, random: check dogecoin_random_bytes everywhere by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/408
* test: assert RNG and keygen output entropy, not just return codes by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/415
* random: drop dogecoin_cheap_random_bytes, which was broken under OP-TEE by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/416
* ecc: report 64 bytes from the recoverable signer, which is what it writes by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/417
* spv: verify the checkpoint header's work before flushing a segment by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/420
* scripts: don't assume a git checkout, and derive combine.sh's target from the triplet by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/423
* ci: update the deprecated action versions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/428
* ci: bound the apt downloads so a bad mirror fails fast by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/429
* chainparams, test: store pow_limit in internal byte order by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/398
* all: print unsigned values with unsigned conversions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/410
* utils, qrengine, headersdb: stop creating files at the umask by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/433
* spv, openenclave: audit follow-ups by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/432
* mem, bip39, seal: allocate through the mapper that frees these pointers by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/434
* cfheadersdb: create the datadir, and keep the cfilters store private by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/430
* net: give a bare -i peer the chain's default port by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/431
* rest, base58: check the bound before dereferencing at it by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/435
* add missing file by @radmakr in https://github.com/dogecoinfoundation/libdogecoin/pull/436
* tx: size the scriptPubKey output from the constant that describes it by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/439
* address: correct the declared output buffer size for getDerivedHDAddress by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/442
* tx: free the stripped-script buffer on the conversion failure path by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/440
* tx: make getAddrFromPubkeyHash take a pubkey hash by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/441
* doc: document the address-returning HD derivation functions by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/443
* test: bound test_spv so an offline runner stops instead of hanging by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/444

## New Contributors
* @radmakr made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/285

**Full Changelog**: https://github.com/dogecoinfoundation/libdogecoin/compare/v0.1.5-pre...v0.1.5


## [0.1.5-pre] - 2025-06-27
## What's Changed
* merge v0.1.4 into main by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/253
* ci: updated to 22.04 runner by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/255
* ci: remove obsolete packages by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/256
* open 0.1.5-dev for development by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/254
* ql: updated to v3 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/257
* get entropy size instead of default 256 by @joijuke in https://github.com/dogecoinfoundation/libdogecoin/pull/258
* bip39: updated to verify checksum by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/259
* tests: added test case for large transactions to transaction_tests.c by @raffecat in https://github.com/dogecoinfoundation/libdogecoin/pull/261
* utils: updated buffer length for max tx by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/262
* added TXHEXMAXLEN by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/263
* transaction: added check, replaced memcpy by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/265
* transaction: added external buffer functions by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/266
* transaction: added finalize_transaction_ex by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/267
* transaction: added wrappers by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/268
* fixate 0.1.5 pre by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/269

## New Contributors
* @raffecat made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/261

**Full Changelog**: https://github.com/dogecoinfoundation/libdogecoin/compare/v0.1.4...v0.1.5-pre


## [0.1.4] - 2025-04-10
## What's Changed
* merge v0.1.3 into main by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/204
* added 'tbs' library link by @UsaRandom in https://github.com/dogecoinfoundation/libdogecoin/pull/205
* net: added disconnected state for shutdown by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/207
* src: added intel assembly for sha algs by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/208
* ci: comment out arm64-macos due to gh billing by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/209
* build: require secp256k1 build first in Makefile by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/211
* ci: pin macos version to 12 instead of latest by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/214
* net: added http server by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/212
* blockchain: moved chainwork to blockindex by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/213
* crypto: adds chacha20 and fast_random_context by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/210
* fix DragonFlyBSD 6.4-RELEASE support by @movepointsolutions in https://github.com/dogecoinfoundation/libdogecoin/pull/182
* Make getDerivedHDAddress return address, not key by @chromatic in https://github.com/dogecoinfoundation/libdogecoin/pull/218
* spvnode: move http callback function to it's own respective file by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/220
* fixate 0.1.4-dogebox-pre by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/221
* [feat] Add Key Management Enclaves with YubiKey and NanoPC-T6 Support by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/223
* config: add -levent_core to AC_CHECK_LIB for event_extra by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/225
* ci: added 'tags' to 'on:' for sign actions by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/227
* ci: updated runner to macos-13 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/229
* optee, openenclave: added custom key path parameter by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/238
* depends: disable libunistring for mingw32 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/228
* src: updated reference and error handling in rest by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/226
* such: updated usage for public child keys by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/230
* chainparams: added backup dns seed by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/239
* add _t suffix to uint256, uint160 and vector types by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/240
* sha2: added armv8 and armv8.2 crypto by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/231
* rest: added getTimestamp endpoint by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/234
* rest: added getLastBlockInfo endpoint  by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/235
* rest: added utxo confirmations by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/236
* validation: check auxpow PoW before other checks by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/237
* ci: updated Windows signing certs and root by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/241
* optee: enable TRNG, add LIBDIR overrides, and switch fortify flag by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/242
* sha2: moved armv8 and armv82 guards by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/243
* ci: added windows native release and no tpm builds by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/244
* wallet: restore logic check by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/245
* cmake: restore flag settings by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/246
* docs: update changelog.md by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/247
* fixate 0.1.4 by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/248
* fixate gitian descriptors by @xanimo in https://github.com/dogecoinfoundation/libdogecoin/pull/249
* docs: re-update changelog.md by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/250
* depends: make yubikey depends selectable by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/251
* docs: finalize changelog.md by @edtubbs in https://github.com/dogecoinfoundation/libdogecoin/pull/252

## New Contributors
* @UsaRandom made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/205
* @movepointsolutions made their first contribution in https://github.com/dogecoinfoundation/libdogecoin/pull/182

**Full Changelog**: https://github.com/dogecoinfoundation/libdogecoin/compare/v0.1.3...v0.1.4


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
