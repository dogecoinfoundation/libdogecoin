/**
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 * Copyright (c) 2015 Douglas J. Bakkumk
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


#include <dogecoin/bip32.h>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dogecoin/base58.h>
#include <dogecoin/ecc.h>
#include <dogecoin/ecc_key.h>
#include <dogecoin/hash.h>
#include <dogecoin/hmac.h>
#include <dogecoin/memory.h>
#include <dogecoin/ripemd160.h>
#include <dogecoin/sha2.h>
#include <dogecoin/utils.h>

// write 4 big endian bytes
static void write_be(uint8_t* data, uint32_t x)
{
    data[0] = x >> 24;
    data[1] = x >> 16;
    data[2] = x >> 8;
    data[3] = x;
}


// read 4 big endian bytes
static uint32_t read_be(const uint8_t* data)
{
    return (((uint32_t)data[0]) << 24) |
           (((uint32_t)data[1]) << 16) |
           (((uint32_t)data[2]) << 8) |
           (((uint32_t)data[3]));
}

dogecoin_hdnode* dogecoin_hdnode_new()
{
    dogecoin_hdnode* hdnode;
    hdnode = dogecoin_calloc(1, sizeof(*hdnode));
    return hdnode;
}

dogecoin_hdnode* dogecoin_hdnode_copy(const dogecoin_hdnode* hdnode)
{
    dogecoin_hdnode* newnode = dogecoin_hdnode_new();

    newnode->depth = hdnode->depth;
    newnode->fingerprint = hdnode->fingerprint;
    newnode->child_num = hdnode->child_num;
    memcpy(newnode->chain_code, hdnode->chain_code, sizeof(hdnode->chain_code));
    memcpy(newnode->private_key, hdnode->private_key, sizeof(hdnode->private_key));
    memcpy(newnode->public_key, hdnode->public_key, sizeof(hdnode->public_key));

    return newnode;
}

void dogecoin_hdnode_free(dogecoin_hdnode* hdnode)
{
    memset(hdnode->chain_code, 0, sizeof(hdnode->chain_code));
    memset(hdnode->private_key, 0, sizeof(hdnode->private_key));
    memset(hdnode->public_key, 0, sizeof(hdnode->public_key));
    dogecoin_free(hdnode);
}

dogecoin_bool dogecoin_hdnode_from_seed(const uint8_t* seed, int seed_len, dogecoin_hdnode* out)
{
    uint8_t I[DOGECOIN_ECKEY_PKEY_LENGTH + DOGECOIN_BIP32_CHAINCODE_SIZE];
    memset(out, 0, sizeof(dogecoin_hdnode));
    out->depth = 0;
    out->fingerprint = 0x00000000;
    out->child_num = 0;
    hmac_sha512((const uint8_t*)"Bitcoin seed", 12, seed, seed_len, I);
    memcpy(out->private_key, I, DOGECOIN_ECKEY_PKEY_LENGTH);

    if (!dogecoin_ecc_verify_privatekey(out->private_key)) {
        memset(I, 0, sizeof(I));
        return false;
    }

    memcpy(out->chain_code, I + DOGECOIN_ECKEY_PKEY_LENGTH, DOGECOIN_BIP32_CHAINCODE_SIZE);
    dogecoin_hdnode_fill_public_key(out);
    memset(I, 0, sizeof(I));
    return true;
}


dogecoin_bool dogecoin_hdnode_public_ckd(dogecoin_hdnode* inout, uint32_t i)
{
    uint8_t data[1 + 32 + 4];
    uint8_t I[32 + DOGECOIN_BIP32_CHAINCODE_SIZE];
    uint8_t fingerprint[32];

    if (i & 0x80000000) { // private derivation
        return false;
    } else { // public derivation
        memcpy(data, inout->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    }
    write_be(data + DOGECOIN_ECKEY_COMPRESSED_LENGTH, i);

    sha256_Raw(inout->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH, fingerprint);
    dogecoin_ripemd160(fingerprint, 32, fingerprint);
    inout->fingerprint = (fingerprint[0] << 24) + (fingerprint[1] << 16) + (fingerprint[2] << 8) + fingerprint[3];

    memset(inout->private_key, 0, 32);

    int failed = 0;
    hmac_sha512(inout->chain_code, 32, data, sizeof(data), I);
    memcpy(inout->chain_code, I + 32, DOGECOIN_BIP32_CHAINCODE_SIZE);


    if (!dogecoin_ecc_public_key_tweak_add(inout->public_key, I))
        failed = false;

    if (!failed) {
        inout->depth++;
        inout->child_num = i;
    }

    // Wipe all stack data.
    memset(data, 0, sizeof(data));
    memset(I, 0, sizeof(I));
    memset(fingerprint, 0, sizeof(fingerprint));

    return failed ? false : true;
}


dogecoin_bool dogecoin_hdnode_private_ckd(dogecoin_hdnode* inout, uint32_t i)
{
    uint8_t data[1 + DOGECOIN_ECKEY_PKEY_LENGTH + 4];
    uint8_t I[DOGECOIN_ECKEY_PKEY_LENGTH + DOGECOIN_BIP32_CHAINCODE_SIZE];
    uint8_t fingerprint[DOGECOIN_BIP32_CHAINCODE_SIZE];
    uint8_t p[DOGECOIN_ECKEY_PKEY_LENGTH], z[DOGECOIN_ECKEY_PKEY_LENGTH];

    if (i & 0x80000000) { // private derivation
        data[0] = 0;
        memcpy(data + 1, inout->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    } else { // public derivation
        memcpy(data, inout->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    }
    write_be(data + DOGECOIN_ECKEY_COMPRESSED_LENGTH, i);

    sha256_Raw(inout->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH, fingerprint);
    dogecoin_ripemd160(fingerprint, 32, fingerprint);
    inout->fingerprint = (fingerprint[0] << 24) + (fingerprint[1] << 16) +
                         (fingerprint[2] << 8) + fingerprint[3];

    memset(fingerprint, 0, sizeof(fingerprint));
    memcpy(p, inout->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);

    hmac_sha512(inout->chain_code, DOGECOIN_BIP32_CHAINCODE_SIZE, data, sizeof(data), I);
    memcpy(inout->chain_code, I + DOGECOIN_ECKEY_PKEY_LENGTH, DOGECOIN_BIP32_CHAINCODE_SIZE);
    memcpy(inout->private_key, I, DOGECOIN_ECKEY_PKEY_LENGTH);

    memcpy(z, inout->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);

    int failed = 0;
    if (!dogecoin_ecc_verify_privatekey(z)) {
        failed = 1;
        return false;
    }

    memcpy(inout->private_key, p, DOGECOIN_ECKEY_PKEY_LENGTH);
    if (!dogecoin_ecc_private_key_tweak_add(inout->private_key, z)) {
        failed = 1;
    }

    if (!failed) {
        inout->depth++;
        inout->child_num = i;
        dogecoin_hdnode_fill_public_key(inout);
    }

    memset(data, 0, sizeof(data));
    memset(I, 0, sizeof(I));
    memset(p, 0, sizeof(p));
    memset(z, 0, sizeof(z));
    return true;
}


void dogecoin_hdnode_fill_public_key(dogecoin_hdnode* node)
{
    size_t outsize = DOGECOIN_ECKEY_COMPRESSED_LENGTH;
    dogecoin_ecc_get_pubkey(node->private_key, node->public_key, &outsize, true);
}


static void dogecoin_hdnode_serialize(const dogecoin_hdnode* node, uint32_t version, char use_public, char* str, int strsize)
{
    uint8_t node_data[78];
    write_be(node_data, version);
    node_data[4] = node->depth;
    write_be(node_data + 5, node->fingerprint);
    write_be(node_data + 9, node->child_num);
    memcpy(node_data + 13, node->chain_code, DOGECOIN_BIP32_CHAINCODE_SIZE);
    if (use_public) {
        memcpy(node_data + 45, node->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    } else {
        node_data[45] = 0;
        memcpy(node_data + 46, node->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    }
    dogecoin_base58_encode_check(node_data, 78, str, strsize);
}


void dogecoin_hdnode_serialize_public(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, int strsize)
{
    dogecoin_hdnode_serialize(node, chain->b58prefix_bip32_pubkey, 1, str, strsize);
}


void dogecoin_hdnode_serialize_private(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, int strsize)
{
    dogecoin_hdnode_serialize(node, chain->b58prefix_bip32_privkey, 0, str, strsize);
}


void dogecoin_hdnode_get_hash160(const dogecoin_hdnode* node, uint160 hash160_out)
{
    uint256 hashout;
    dogecoin_hash_sngl_sha256(node->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH, hashout);
    dogecoin_ripemd160(hashout, sizeof(hashout), hash160_out);
}

void dogecoin_hdnode_get_p2pkh_address(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, int strsize)
{
    uint8_t hash160[sizeof(uint160)+1];
    hash160[0] = chain->b58prefix_pubkey_address;
    dogecoin_hdnode_get_hash160(node, hash160 + 1);
    dogecoin_base58_encode_check(hash160, sizeof(hash160), str, strsize);
}

dogecoin_bool dogecoin_hdnode_get_pub_hex(const dogecoin_hdnode* node, char* str, size_t* strsize)
{
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    memcpy(&pubkey.pubkey, node->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    pubkey.compressed = true;

    return dogecoin_pubkey_get_hex(&pubkey, str, strsize);
}


// check for validity of curve point in case of public data not performed
dogecoin_bool dogecoin_hdnode_deserialize(const char* str, const dogecoin_chainparams* chain, dogecoin_hdnode* node)
{
    const size_t ndlen = sizeof(uint8_t) * strlen(str);
    uint8_t *node_data = (uint8_t *)dogecoin_malloc(ndlen);
    memset(node, 0, sizeof(dogecoin_hdnode));
    size_t outlen = 0;

    outlen = dogecoin_base58_decode_check(str, node_data, ndlen);
    if (!outlen) {
        dogecoin_free(node_data);
        return false;
    }
    uint32_t version = read_be(node_data);
    if (version == chain->b58prefix_bip32_pubkey) { // public node
        memcpy(node->public_key, node_data + 45, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    } else if (version == chain->b58prefix_bip32_privkey) { // private node
        if (node_data[45]) {                                // invalid data
            dogecoin_free(node_data);
            return false;
        }
        memcpy(node->private_key, node_data + 46, DOGECOIN_ECKEY_PKEY_LENGTH);
        dogecoin_hdnode_fill_public_key(node);
    } else {
        dogecoin_free(node_data);
        return false; // invalid version
    }
    node->depth = node_data[4];
    node->fingerprint = read_be(node_data + 5);
    node->child_num = read_be(node_data + 9);
    memcpy(node->chain_code, node_data + 13, DOGECOIN_BIP32_CHAINCODE_SIZE);
    dogecoin_free(node_data);
    return true;
}

dogecoin_bool dogecoin_hd_generate_key(dogecoin_hdnode* node, const char* keypath, const uint8_t* keymaster, const uint8_t* chaincode, dogecoin_bool usepubckd)
{
    static char delim[] = "/";
    static char prime[] = "phH\'";
    static char digits[] = "0123456789";
    uint64_t idx = 0;
    assert(strlens(keypath) < 1024);
    char *pch, *kp = dogecoin_malloc(strlens(keypath) + 1);

    if (!kp) {
        return false;
    }

    if (strlens(keypath) < strlens("m/")) {
        goto err;
    }

    memset(kp, 0, strlens(keypath) + 1);
    memcpy(kp, keypath, strlens(keypath));

    if (kp[0] != 'm' || kp[1] != '/') {
        goto err;
    }

    node->depth = 0;
    node->child_num = 0;
    node->fingerprint = 0;
    memcpy(node->chain_code, chaincode, DOGECOIN_BIP32_CHAINCODE_SIZE);
    if (usepubckd == true) {
        memcpy(node->public_key, keymaster, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    } else {
        memcpy(node->private_key, keymaster, DOGECOIN_ECKEY_PKEY_LENGTH);
        dogecoin_hdnode_fill_public_key(node);
    }

    pch = strtok(kp + 2, delim);
    while (pch != NULL) {
        size_t i = 0;
        int prm = 0;
        for (; i < strlens(pch); i++) {
            if (strchr(prime, pch[i])) {
                if ((i != strlens(pch) - 1) || usepubckd == true) {
                    goto err;
                }
                prm = 1;
            } else if (!strchr(digits, pch[i])) {
                goto err;
            }
        }

        idx = strtoull(pch, NULL, 10);
        if (idx > UINT32_MAX) {
            goto err;
        }

        if (prm) {
            if (dogecoin_hdnode_private_ckd_prime(node, idx) != true) {
                goto err;
            }
        } else {
            if ((usepubckd == true ? dogecoin_hdnode_public_ckd(node, idx) : dogecoin_hdnode_private_ckd(node, idx)) != true) {
                goto err;
            }
        }
        pch = strtok(NULL, delim);
    }
    dogecoin_free(kp);
    return true;

err:
    dogecoin_free(kp);
    return false;
}

dogecoin_bool dogecoin_hdnode_has_privkey(dogecoin_hdnode* node)
{
    int i;
    for (i = 0; i < DOGECOIN_ECKEY_PKEY_LENGTH; ++i) {
        if (node->private_key[i] != 0)
            return true;
    }
    return false;
}
