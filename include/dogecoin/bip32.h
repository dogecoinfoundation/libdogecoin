/**
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 * Copyright (c) 2015 Douglas J. Bakkumk
 * Copyright (c) 2015 Jonas Schnelli
 * Copyright (c) 2022 bluezr
 * Copyright (c) 2022 The Dogecoin Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __LIBDOGECOIN_BIP32_H__
#define __LIBDOGECOIN_BIP32_H__

#include <stdint.h>

#include <dogecoin/chainparams.h>
#include <dogecoin/dogecoin.h>

#define DOGECOIN_BIP32_CHAINCODE_SIZE 32

LIBDOGECOIN_BEGIN_DECL

typedef struct
{
    uint32_t depth;
    uint32_t fingerprint;
    uint32_t child_num;
    uint8_t chain_code[DOGECOIN_BIP32_CHAINCODE_SIZE];
    uint8_t private_key[DOGECOIN_ECKEY_PKEY_LENGTH];
    uint8_t public_key[DOGECOIN_ECKEY_COMPRESSED_LENGTH];
} dogecoin_hdnode;

#define dogecoin_hdnode_private_ckd_prime(X, I) dogecoin_hdnode_private_ckd((X), ((I) | 0x80000000))

LIBDOGECOIN_API dogecoin_hdnode* dogecoin_hdnode_new();
LIBDOGECOIN_API dogecoin_hdnode* dogecoin_hdnode_copy(const dogecoin_hdnode* hdnode);
LIBDOGECOIN_API void dogecoin_hdnode_free(dogecoin_hdnode* node);
LIBDOGECOIN_API dogecoin_bool dogecoin_hdnode_public_ckd(dogecoin_hdnode* inout, uint32_t i);
LIBDOGECOIN_API dogecoin_bool dogecoin_hdnode_from_seed(const uint8_t* seed, int seed_len, dogecoin_hdnode* out);
LIBDOGECOIN_API dogecoin_bool dogecoin_hdnode_private_ckd(dogecoin_hdnode* inout, uint32_t i);
LIBDOGECOIN_API void dogecoin_hdnode_fill_public_key(dogecoin_hdnode* node);
LIBDOGECOIN_API void dogecoin_hdnode_serialize_public(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, int strsize);
LIBDOGECOIN_API void dogecoin_hdnode_serialize_private(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, int strsize);

/* gives out the raw sha256/ripemd160 hash */
LIBDOGECOIN_API void dogecoin_hdnode_get_hash160(const dogecoin_hdnode* node, uint160 hash160_out);
LIBDOGECOIN_API void dogecoin_hdnode_get_p2pkh_address(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, int strsize);
LIBDOGECOIN_API dogecoin_bool dogecoin_hdnode_get_pub_hex(const dogecoin_hdnode* node, char* str, size_t* strsize);
LIBDOGECOIN_API dogecoin_bool dogecoin_hdnode_deserialize(const char* str, const dogecoin_chainparams* chain, dogecoin_hdnode* node);

//!derive dogecoin_hdnode from extended private or extended public key orkey
//if you use pub child key derivation, pass usepubckd=true
LIBDOGECOIN_API dogecoin_bool dogecoin_hd_generate_key(dogecoin_hdnode* node, const char* keypath, const uint8_t* keymaster, const uint8_t* chaincode, dogecoin_bool usepubckd);

//!checks if a node has the according private key (or if its a pubkey only node)
LIBDOGECOIN_API dogecoin_bool dogecoin_hdnode_has_privkey(dogecoin_hdnode* node);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_BIP32_H__
