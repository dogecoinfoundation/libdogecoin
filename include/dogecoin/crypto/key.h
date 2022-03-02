/*

 The MIT License (MIT)

 Copyright (c) 2015 Jonas Schnelli
 Copyright (c) 2022 bluezr
 Copyright (c) 2022 The Dogecoin Foundation

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef __LIBDOGECOIN_CRYPTO_KEY_H__
#define __LIBDOGECOIN_CRYPTO_KEY_H__

#include <dogecoin/chainparams.h>
#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

typedef struct dogecoin_key_ {
    uint8_t privkey[DOGECOIN_ECKEY_PKEY_LENGTH];
} dogecoin_key;

typedef struct dogecoin_pubkey_ {
    dogecoin_bool compressed;
    uint8_t pubkey[DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH];
} dogecoin_pubkey;

LIBDOGECOIN_API void dogecoin_privkey_init(dogecoin_key* privkey);
LIBDOGECOIN_API dogecoin_bool dogecoin_privkey_is_valid(const dogecoin_key* privkey);
LIBDOGECOIN_API void dogecoin_privkey_cleanse(dogecoin_key* privkey);
LIBDOGECOIN_API dogecoin_bool dogecoin_privkey_gen(dogecoin_key* privkey);
LIBDOGECOIN_API dogecoin_bool dogecoin_privkey_verify_pubkey(dogecoin_key* privkey, dogecoin_pubkey* pubkey);

// form a WIF encoded string from the given pubkey, make sure privkey_wif is large enough and strsize_inout contains the size of the buffer
LIBDOGECOIN_API void dogecoin_privkey_encode_wif(const dogecoin_key* privkey, const dogecoin_chainparams* chain, char* privkey_wif, size_t* strsize_inout);
LIBDOGECOIN_API dogecoin_bool dogecoin_privkey_decode_wif(const char* privkey_wif, const dogecoin_chainparams* chain, dogecoin_key* privkey);

LIBDOGECOIN_API void dogecoin_pubkey_init(dogecoin_pubkey* pubkey);

// Compute the length of a pubkey with a given first byte.
LIBDOGECOIN_API unsigned int dogecoin_pubkey_get_length(unsigned char ch_header);

LIBDOGECOIN_API dogecoin_bool dogecoin_pubkey_is_valid(const dogecoin_pubkey* pubkey);
LIBDOGECOIN_API void dogecoin_pubkey_cleanse(dogecoin_pubkey* pubkey);
LIBDOGECOIN_API void dogecoin_pubkey_from_key(const dogecoin_key* privkey, dogecoin_pubkey* pubkey_inout);

//get the hash160 (single SHA256 + RIPEMD160)
LIBDOGECOIN_API void dogecoin_pubkey_get_hash160(const dogecoin_pubkey* pubkey, uint160 hash160);

//get the hex representation of a pubkey, strsize must be at leat 66 bytes
LIBDOGECOIN_API dogecoin_bool dogecoin_pubkey_get_hex(const dogecoin_pubkey* pubkey, char* str, size_t* strsize);

//sign a 32byte message/hash and returns a DER encoded signature (through *sigout)
LIBDOGECOIN_API dogecoin_bool dogecoin_key_sign_hash(const dogecoin_key* privkey, const uint256 hash, unsigned char* sigout, size_t* outlen);

//sign a 32byte message/hash and returns a 64 byte compact signature (through *sigout)
LIBDOGECOIN_API dogecoin_bool dogecoin_key_sign_hash_compact(const dogecoin_key* privkey, const uint256 hash, unsigned char* sigout, size_t* outlen);

//sign a 32byte message/hash and returns a 64 byte compact signature (through *sigout) plus a 1byte recovery id
LIBDOGECOIN_API dogecoin_bool dogecoin_key_sign_hash_compact_recoverable(const dogecoin_key* privkey, const uint256 hash, unsigned char* sigout, size_t* outlen, int* recid);

LIBDOGECOIN_API dogecoin_bool dogecoin_key_sign_recover_pubkey(const unsigned char* sig, const uint256 hash, int recid, dogecoin_pubkey* pubkey);

//verifies a DER encoded signature with given pubkey and return true if valid
LIBDOGECOIN_API dogecoin_bool dogecoin_pubkey_verify_sig(const dogecoin_pubkey* pubkey, const uint256 hash, unsigned char* sigder, int len);

LIBDOGECOIN_API dogecoin_bool dogecoin_pubkey_getaddr_p2sh_p2wpkh(const dogecoin_pubkey* pubkey, const dogecoin_chainparams* chain, char* addrout);
LIBDOGECOIN_API dogecoin_bool dogecoin_pubkey_getaddr_p2pkh(const dogecoin_pubkey* pubkey, const dogecoin_chainparams* chain, char* addrout);
LIBDOGECOIN_API dogecoin_bool dogecoin_pubkey_getaddr_p2wpkh(const dogecoin_pubkey* pubkey, const dogecoin_chainparams* chain, char* addrout);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_CRYPTO_KEY_H__
