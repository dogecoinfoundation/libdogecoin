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


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dogecoin/chainparams.h>
#include <dogecoin/base58.h>
#include <dogecoin/ecc.h>
#include <dogecoin/hash.h>
#include <dogecoin/key.h>
#include <dogecoin/random.h>
#include <dogecoin/rmd160.h>
#include <dogecoin/script.h>
#include <dogecoin/serialize.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>

void dogecoin_privkey_init(dogecoin_key* privkey)
{
    dogecoin_mem_zero(&privkey->privkey, DOGECOIN_ECKEY_PKEY_LENGTH);
}

dogecoin_bool dogecoin_privkey_is_valid(const dogecoin_key* privkey)
{
    if (!privkey)
        return false;
    return dogecoin_ecc_verify_privatekey(privkey->privkey);
}

void dogecoin_privkey_cleanse(dogecoin_key* privkey)
{
    dogecoin_mem_zero(&privkey->privkey, DOGECOIN_ECKEY_PKEY_LENGTH);
}

dogecoin_bool dogecoin_privkey_gen(dogecoin_key* privkey)
{
    if (privkey == NULL)
        return false;
    do {
        const dogecoin_bool res = dogecoin_random_bytes(privkey->privkey, DOGECOIN_ECKEY_PKEY_LENGTH, 0);
        if (!res)
            return false;
    } while (dogecoin_ecc_verify_privatekey(privkey->privkey) == 0);
    return true;
}

dogecoin_bool dogecoin_privkey_verify_pubkey(dogecoin_key* privkey, dogecoin_pubkey* pubkey)
{
    uint256 rnddata, hash;
    const dogecoin_bool res = dogecoin_random_bytes(rnddata, DOGECOIN_HASH_LENGTH, 0);
    if (!res)
        return false;
    dogecoin_hash(rnddata, DOGECOIN_HASH_LENGTH, hash);
    unsigned char sig[74];
    size_t siglen = 74;
    if (!dogecoin_key_sign_hash(privkey, hash, sig, &siglen))
        return false;
    return dogecoin_pubkey_verify_sig(pubkey, hash, sig, siglen);
}

void dogecoin_privkey_encode_wif(const dogecoin_key* privkey, const dogecoin_chainparams* chain, char* privkey_wif, size_t* strsize_inout)
{
    uint8_t pkeybase58c[34];
    pkeybase58c[0] = chain->b58prefix_secret_address;
    pkeybase58c[33] = 1; /* always use compressed keys */
    memcpy_safe(&pkeybase58c[1], privkey->privkey, DOGECOIN_ECKEY_PKEY_LENGTH);
    assert(dogecoin_base58_encode_check(pkeybase58c, 34, privkey_wif, *strsize_inout) != 0);
    dogecoin_mem_zero(&pkeybase58c, 34);
}

dogecoin_bool dogecoin_privkey_decode_wif(const char* privkey_wif, const dogecoin_chainparams* chain, dogecoin_key* privkey)
{
    if (!privkey_wif || strlen(privkey_wif) < 50)
        return false;
    const size_t privkey_len = strlen(privkey_wif);
    uint8_t* privkey_data = (uint8_t*)dogecoin_calloc(1, privkey_len);
    dogecoin_mem_zero(privkey_data, privkey_len);
    size_t outlen = 0;
    outlen = dogecoin_base58_decode_check(privkey_wif, privkey_data, privkey_len);
    if (!outlen) {
        dogecoin_free(privkey_data);
        return false;
    }
    if (privkey_data[0] != chain->b58prefix_secret_address) {
        dogecoin_free(privkey_data);
        return false;
    }
    memcpy_safe(privkey->privkey, &privkey_data[1], DOGECOIN_ECKEY_PKEY_LENGTH);
    dogecoin_mem_zero(privkey_data, sizeof(privkey_data));
    dogecoin_free(privkey_data);
    return true;
}

void dogecoin_pubkey_init(dogecoin_pubkey* pubkey)
{
    if (pubkey == NULL)
        return;
    dogecoin_mem_zero(pubkey->pubkey, DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH);
    pubkey->compressed = false;
}

unsigned int dogecoin_pubkey_get_length(unsigned char ch_header)
{
    if (ch_header == 2 || ch_header == 3)
        return DOGECOIN_ECKEY_COMPRESSED_LENGTH;
    if (ch_header == 4 || ch_header == 6 || ch_header == 7)
        return DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH;
    return 0;
}

dogecoin_bool dogecoin_pubkey_is_valid(const dogecoin_pubkey* pubkey)
{
    return dogecoin_ecc_verify_pubkey(pubkey->pubkey, pubkey->compressed);
}

void dogecoin_pubkey_cleanse(dogecoin_pubkey* pubkey)
{
    if (pubkey == NULL)
        return;
    dogecoin_mem_zero(pubkey->pubkey, DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH);
}

void dogecoin_pubkey_get_hash160(const dogecoin_pubkey* pubkey, uint160 hash160)
{
    uint256 hashout;
    dogecoin_hash_sngl_sha256(pubkey->pubkey, pubkey->compressed ? DOGECOIN_ECKEY_COMPRESSED_LENGTH : DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH, hashout);
    rmd160(hashout, sizeof(hashout), hash160);
}

dogecoin_bool dogecoin_pubkey_get_hex(const dogecoin_pubkey* pubkey, char* str, size_t* strsize)
{
    if (*strsize < DOGECOIN_ECKEY_COMPRESSED_LENGTH * 2 + 1)
        return false;
    utils_bin_to_hex((unsigned char*)pubkey->pubkey, DOGECOIN_ECKEY_COMPRESSED_LENGTH, str);
    *strsize = DOGECOIN_ECKEY_COMPRESSED_LENGTH * 2;
    return true;
}

void dogecoin_pubkey_from_key(const dogecoin_key* privkey, dogecoin_pubkey* pubkey_inout)
{
    if (pubkey_inout == NULL || privkey == NULL)
        return;
    size_t in_out_len = DOGECOIN_ECKEY_COMPRESSED_LENGTH;
    dogecoin_ecc_get_pubkey(privkey->privkey, pubkey_inout->pubkey, &in_out_len, true);
    pubkey_inout->compressed = true;
}

dogecoin_bool dogecoin_key_sign_hash(const dogecoin_key* privkey, const uint256 hash, unsigned char* sigout, size_t* outlen)
{
    return dogecoin_ecc_sign(privkey->privkey, hash, sigout, outlen);
}

dogecoin_bool dogecoin_key_sign_hash_compact(const dogecoin_key* privkey, const uint256 hash, unsigned char* sigout, size_t* outlen)
{
    return dogecoin_ecc_sign_compact(privkey->privkey, hash, sigout, outlen);
}

dogecoin_bool dogecoin_key_sign_hash_compact_recoverable(const dogecoin_key* privkey, const uint256 hash, unsigned char* sigout, size_t* outlen, int* recid)
{
    return dogecoin_ecc_sign_compact_recoverable(privkey->privkey, hash, sigout, outlen, recid);
}

dogecoin_bool dogecoin_key_sign_recover_pubkey(const unsigned char* sig, const uint256 hash, int recid, dogecoin_pubkey* pubkey)
{
    uint8_t pubkeybuf[128];
    size_t outlen = 128;
    if (!dogecoin_ecc_recover_pubkey(sig, hash, recid, pubkeybuf, &outlen) || outlen > DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH)
        return 0;
    dogecoin_mem_zero(pubkey->pubkey, sizeof(pubkey->pubkey));
    memcpy_safe(pubkey->pubkey, pubkeybuf, outlen);
    if (outlen == DOGECOIN_ECKEY_COMPRESSED_LENGTH)
        pubkey->compressed = true;
    return 1;
}

dogecoin_bool dogecoin_pubkey_verify_sig(const dogecoin_pubkey* pubkey, const uint256 hash, unsigned char* sigder, size_t len)
{
    return dogecoin_ecc_verify_sig(pubkey->pubkey, pubkey->compressed, hash, sigder, len);
}

dogecoin_bool dogecoin_pubkey_getaddr_p2pkh(const dogecoin_pubkey* pubkey, const dogecoin_chainparams* chain, char* addrout)
{
    uint8_t hash160[sizeof(uint160) + 1];
    hash160[0] = chain->b58prefix_pubkey_address;
    dogecoin_pubkey_get_hash160(pubkey, hash160 + 1);
    dogecoin_base58_encode_check(hash160, sizeof(hash160), addrout, 100);
    return true;
}
