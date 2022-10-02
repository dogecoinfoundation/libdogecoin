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

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dogecoin/bip32.h>
#include <dogecoin/base58.h>
#include <dogecoin/ecc.h>
#include <dogecoin/hash.h>
#include <dogecoin/key.h>
#include <dogecoin/rmd160.h>
#include <dogecoin/sha2.h>
#include <dogecoin/mem.h>
#include <dogecoin/utils.h>


/**
 * @brief This function writes 4 big endian bytes.
 * 
 * @param data The buffer being written to.
 * @param x The 4 bytes to be written, bundled as int.
 * 
 * @return Nothing.
 */
static void write_be(uint8_t* data, uint32_t x)
{
    data[0] = x >> 24;
    data[1] = x >> 16;
    data[2] = x >> 8;
    data[3] = x;
}


/**
 * @brief This function reads 4 big endian bytes.
 * 
 * @param data The bytes to be read.
 * 
 * @return The data buffer in int format.
 */
static uint32_t read_be(const uint8_t* data)
{
    return (((uint32_t)data[0]) << 24) |
           (((uint32_t)data[1]) << 16) |
           (((uint32_t)data[2]) << 8) |
           (((uint32_t)data[3]));
}


/**
 * @brief This function allocates memory for a new 
 * HD node object and returns a pointer to it.
 * 
 * @return The pointer to the new HD node object.
 */
dogecoin_hdnode* dogecoin_hdnode_new()
{
    dogecoin_hdnode* hdnode;
    hdnode = dogecoin_calloc(1, sizeof(*hdnode));
    return hdnode;
}


/**
 * @brief The function creates a new HD node object 
 * and copies all information from the old to the new.
 * 
 * @param hdnode The HD node to be copied.
 * 
 * @return The pointer to the new node.
 */
dogecoin_hdnode* dogecoin_hdnode_copy(const dogecoin_hdnode* hdnode)
{
    dogecoin_hdnode* newnode = dogecoin_hdnode_new();

    newnode->depth = hdnode->depth;
    newnode->fingerprint = hdnode->fingerprint;
    newnode->child_num = hdnode->child_num;
    memcpy_safe(newnode->chain_code, hdnode->chain_code, sizeof(hdnode->chain_code));
    memcpy_safe(newnode->private_key, hdnode->private_key, sizeof(hdnode->private_key));
    memcpy_safe(newnode->public_key, hdnode->public_key, sizeof(hdnode->public_key));

    return newnode;
}


/**
 * @brief This function frees an HD node object in memory.
 * 
 * @param hdnode The HD node to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_hdnode_free(dogecoin_hdnode* hdnode)
{
    dogecoin_mem_zero(hdnode->chain_code, sizeof(hdnode->chain_code));
    dogecoin_mem_zero(hdnode->private_key, sizeof(hdnode->private_key));
    dogecoin_mem_zero(hdnode->public_key, sizeof(hdnode->public_key));
    dogecoin_free(hdnode);
}


/**
 * @brief This function generates a private and public
 * keypair along with chain_code for a hierarchical 
 * deterministic wallet. This is derived from a seed 
 * which usually consists of 12 random mnemonic words.
 * 
 * @param seed The byte string containing the seed phrase.
 * @param seed_len The length of the phrase in characters.
 * @param out The master node which stores the generated data.
 * 
 * @return Nothing.
 */
dogecoin_bool dogecoin_hdnode_from_seed(const uint8_t* seed, int seed_len, dogecoin_hdnode* out)
{
    uint8_t I[DOGECOIN_ECKEY_PKEY_LENGTH + DOGECOIN_BIP32_CHAINCODE_SIZE];
    dogecoin_mem_zero(out, sizeof(dogecoin_hdnode));
    out->depth = 0;
    out->fingerprint = 0x00000000;
    out->child_num = 0;
    hmac_sha512((const uint8_t*)"Dogecoin seed", 12, seed, seed_len, I);
    memcpy_safe(out->private_key, I, DOGECOIN_ECKEY_PKEY_LENGTH);

    if (!dogecoin_ecc_verify_privatekey(out->private_key)) {
        dogecoin_mem_zero(I, sizeof(I));
        return false;
    }

    memcpy_safe(out->chain_code, I + DOGECOIN_ECKEY_PKEY_LENGTH, DOGECOIN_BIP32_CHAINCODE_SIZE);
    dogecoin_hdnode_fill_public_key(out);
    dogecoin_mem_zero(I, sizeof(I));
    return true;
}


/**
 * @brief This function derives a child key from the
 * public key of the specified master HD node.
 * 
 * @param inout The node to derive the child key from.
 * @param i The index of the child key, specified in derivation path.
 * 
 * @return Nothing. 
 */
dogecoin_bool dogecoin_hdnode_public_ckd(dogecoin_hdnode* inout, uint32_t i)
{
    uint8_t data[1 + 32 + 4];
    uint8_t I[32 + DOGECOIN_BIP32_CHAINCODE_SIZE];
    uint8_t fingerprint[32];

    if (i & 0x80000000) { // private derivation
        return false;
    } else { // public derivation
        memcpy_safe(data, inout->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    }
    write_be(data + DOGECOIN_ECKEY_COMPRESSED_LENGTH, i);

    sha256_raw(inout->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH, fingerprint);
    rmd160(fingerprint, 32, fingerprint);
    inout->fingerprint = (fingerprint[0] << 24) + (fingerprint[1] << 16) + (fingerprint[2] << 8) + fingerprint[3];

    dogecoin_mem_zero(inout->private_key, 32);

    int failed = 0;
    hmac_sha512(inout->chain_code, 32, data, sizeof(data), I);
    memcpy_safe(inout->chain_code, I + 32, DOGECOIN_BIP32_CHAINCODE_SIZE);


    if (!dogecoin_ecc_public_key_tweak_add(inout->public_key, I))
        failed = false;

    if (!failed) {
        inout->depth++;
        inout->child_num = i;
    }

    // Wipe all stack data.
    dogecoin_mem_zero(data, sizeof(data));
    dogecoin_mem_zero(I, sizeof(I));
    dogecoin_mem_zero(fingerprint, sizeof(fingerprint));

    return failed ? false : true;
}


/**
 * @brief This function derives a child key from the
 * private key of the specified master HD node.
 * 
 * @param inout The node to derive the child key from.
 * @param i The index of the child key, specified in derivation path.
 * 
 * @return Nothing. 
 */
dogecoin_bool dogecoin_hdnode_private_ckd(dogecoin_hdnode* inout, uint32_t i)
{
    uint8_t data[1 + DOGECOIN_ECKEY_PKEY_LENGTH + 4];
    uint8_t I[DOGECOIN_ECKEY_PKEY_LENGTH + DOGECOIN_BIP32_CHAINCODE_SIZE];
    uint8_t fingerprint[DOGECOIN_BIP32_CHAINCODE_SIZE];
    uint8_t p[DOGECOIN_ECKEY_PKEY_LENGTH], z[DOGECOIN_ECKEY_PKEY_LENGTH];

    if (i & 0x80000000) { // private derivation
        data[0] = 0;
        memcpy_safe(data + 1, inout->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    } else { // public derivation
        memcpy_safe(data, inout->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    }
    write_be(data + DOGECOIN_ECKEY_COMPRESSED_LENGTH, i);

    sha256_raw(inout->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH, fingerprint);
    rmd160(fingerprint, 32, fingerprint);
    inout->fingerprint = (fingerprint[0] << 24) + (fingerprint[1] << 16) +
                         (fingerprint[2] << 8) + fingerprint[3];

    dogecoin_mem_zero(fingerprint, sizeof(fingerprint));
    memcpy_safe(p, inout->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);

    hmac_sha512(inout->chain_code, DOGECOIN_BIP32_CHAINCODE_SIZE, data, sizeof(data), I);
    memcpy_safe(inout->chain_code, I + DOGECOIN_ECKEY_PKEY_LENGTH, DOGECOIN_BIP32_CHAINCODE_SIZE);
    memcpy_safe(inout->private_key, I, DOGECOIN_ECKEY_PKEY_LENGTH);

    memcpy_safe(z, inout->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);

    int failed = 0;
    if (!dogecoin_ecc_verify_privatekey(z)) {
        failed = 1;
    }

    memcpy_safe(inout->private_key, p, DOGECOIN_ECKEY_PKEY_LENGTH);
    if (!dogecoin_ecc_private_key_tweak_add(inout->private_key, z)) {
        failed = 1;
    }

    if (!failed) {
        inout->depth++;
        inout->child_num = i;
        dogecoin_hdnode_fill_public_key(inout);
    }

    dogecoin_mem_zero(data, sizeof(data));
    dogecoin_mem_zero(I, sizeof(I));
    dogecoin_mem_zero(z, sizeof(z));
    return true;
}


/**
 * @brief This function derives a public key from a private 
 * key taken from the HD node provided.
 * 
 * @param node The HD node which contains the private key.
 * 
 * @return Nothing.
 */
void dogecoin_hdnode_fill_public_key(dogecoin_hdnode* node)
{
    size_t outsize = DOGECOIN_ECKEY_COMPRESSED_LENGTH;
    dogecoin_ecc_get_pubkey(node->private_key, node->public_key, &outsize, true);
}


/**
 * @brief This function serializes the information inside an 
 * HD node and loads it into string variable str using WIF-
 * encoding to represent the public or private key.
 * 
 * @param node The HD node containing the key.
 * @param version The version byte.
 * @param use_public Whether to use public or private key.
 * @param str The string to contain the encoded output.
 * @param strsize The size of the string that will be returned.
 * 
 * @return Nothing.
 */
static void dogecoin_hdnode_serialize(const dogecoin_hdnode* node, uint32_t version, char use_public, char* str, size_t strsize)
{
    uint8_t node_data[78];
    write_be(node_data, version);
    node_data[4] = node->depth;
    write_be(node_data + 5, node->fingerprint);
    write_be(node_data + 9, node->child_num);
    memcpy_safe(node_data + 13, node->chain_code, DOGECOIN_BIP32_CHAINCODE_SIZE);
    if (use_public) {
        memcpy_safe(node_data + 45, node->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    } else {
        node_data[45] = 0;
        memcpy_safe(node_data + 46, node->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    }
    dogecoin_base58_encode_check(node_data, 78, str, strsize);
}


/**
 * @brief This function serializes the HD node information 
 * into a string using the public key.
 * 
 * @param node The HD node containing the key.
 * @param chain The preset chain parameters to use.
 * @param str The string to contain the encoded public key.
 * @param strsize The size of the string to be returned.
 * 
 * @return Nothing.
 */
void dogecoin_hdnode_serialize_public(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, size_t strsize)
{
    dogecoin_hdnode_serialize(node, chain->b58prefix_bip32_pubkey, 1, str, strsize);
}


/**
 * @brief This function serializes the HD node information
 * into a string using the private key.
 * 
 * @param node The HD node containing the key.
 * @param chain The chain parameters to use.
 * @param str The string to contain the encoded private key.
 * @param strsize The size of the string to be returned.
 * 
 * @return Nothing.
 */
void dogecoin_hdnode_serialize_private(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, size_t strsize)
{
    dogecoin_hdnode_serialize(node, chain->b58prefix_bip32_privkey, 0, str, strsize);
}


/**
 * @brief This function applies the sha256 and rmd160 hash
 * functions to public key and loads variable hash160_out
 * with result.
 * 
 * @param node The node containing the public key and its chain code.
 * @param hash160_out The hash160 of the public key.
 * 
 * @return Nothing.
 */
void dogecoin_hdnode_get_hash160(const dogecoin_hdnode* node, uint160 hash160_out)
{
    uint256 hashout;
    dogecoin_hash_sngl_sha256(node->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH, hashout);
    rmd160(hashout, sizeof(hashout), hash160_out);
}


/**
 * @brief This function produces a dogecoin pay-to-
 * public-key-hash (p2pkh) address from the public 
 * key of the given node.
 * 
 * @param node The HD node containing the public key.
 * @param chain The chain parameters to use.
 * @param str The string containing the p2pkh address.
 * @param strsize The size of the string to be returned.
 * 
 * @return Nothing.
 */
void dogecoin_hdnode_get_p2pkh_address(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, size_t strsize)
{
    uint8_t hash160[sizeof(uint160) + 1];
    hash160[0] = chain->b58prefix_pubkey_address;
    dogecoin_hdnode_get_hash160(node, hash160 + 1);
    dogecoin_base58_encode_check(hash160, sizeof(hash160), str, strsize);
}


/**
 * @brief This function converts a public key from binary
 * to hexadecimal string format.
 * 
 * @param node The HD node containing the public key.
 * @param str The hexadecimal string to be returned.
 * @param strsize The size of the string to be returned.
 * 
 * @return Nothing.
 */
dogecoin_bool dogecoin_hdnode_get_pub_hex(const dogecoin_hdnode* node, char* str, size_t* strsize)
{
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    memcpy_safe(&pubkey.pubkey, node->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    pubkey.compressed = true;

    return dogecoin_pubkey_get_hex(&pubkey, str, strsize);
}

/**
 * @brief This function takes a string buffer containing HD
 * node data and loads an HD node object with that data. The
 * function will copy either a private key or a public key, 
 * depending on the prefix of the given buffer.
 * 
 * @param str The buffer containing the node information.
 * @param chain The chain parameters to use.
 * @param node The HD node to be filled.
 * 
 * @return Nothing.
 */
dogecoin_bool dogecoin_hdnode_deserialize(const char* str, const dogecoin_chainparams* chain, dogecoin_hdnode* node)
{
    if (!str || !chain || !node) return false;
    const size_t ndlen = sizeof(uint8_t) * sizeof(dogecoin_hdnode);
    uint8_t* node_data = (uint8_t*)dogecoin_calloc(1, ndlen);
    dogecoin_mem_zero(node, sizeof(dogecoin_hdnode));
    if (!dogecoin_base58_decode_check(str, node_data, ndlen)) {
        dogecoin_free(node_data);
        return false;
    }
    uint32_t version = read_be(node_data);
    if (version == chain->b58prefix_bip32_pubkey) { // public node
        memcpy_safe(node->public_key, node_data + 45, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    } else if (version == chain->b58prefix_bip32_privkey) { // private node
        if (node_data[45]) {                                // invalid data
            dogecoin_free(node_data);
            return false;
        }
        memcpy_safe(node->private_key, node_data + 46, DOGECOIN_ECKEY_PKEY_LENGTH);
        dogecoin_hdnode_fill_public_key(node);
    } else {
        dogecoin_free(node_data);
        return false; // invalid version
    }
    node->depth = node_data[4];
    node->fingerprint = read_be(node_data + 5);
    node->child_num = read_be(node_data + 9);
    memcpy_safe(node->chain_code, node_data + 13, DOGECOIN_BIP32_CHAINCODE_SIZE);
    dogecoin_free(node_data);
    return true;
}


/**
 * @brief This function generates a child key from a given
 * master key and loads it into an HD node.
 * 
 * @param node The HD node to be filled with the derived key.
 * @param keypath The derivation path of the desired key (e.g. "m/0h/0/0")
 * @param keymaster The master key to derive the child from.
 * @param chaincode A 32-byte value that is used to generate the child key.
 * @param usepubckd Whether to use public or private key derivation function.
 * 
 * @return Nothing.
 */
dogecoin_bool dogecoin_hd_generate_key(dogecoin_hdnode* node, const char* keypath, const uint8_t* keymaster, const uint8_t* chaincode, dogecoin_bool usepubckd)
{
    static char delim[] = "/";
    static char prime[] = "phH\'";
    static char digits[] = "0123456789";
    uint64_t idx = 0;
    assert(strlens(keypath) < 1024);
    char *pch, *kp = dogecoin_malloc(strlens(keypath) + 1);
    char *saveptr; /* for strtok_r calls - fix for concurrent calls error*/

    if (!kp) {
        return false;
    }

    if (strlens(keypath) < strlens("m/")) {
        goto err;
    }

    dogecoin_mem_zero(kp, strlens(keypath) + 1);
    memcpy_safe(kp, keypath, strlens(keypath));

    if (kp[0] != 'm' || kp[1] != '/') {
        goto err;
    }

    node->depth = 0;
    node->child_num = 0;
    node->fingerprint = 0;
    memcpy_safe(node->chain_code, chaincode, DOGECOIN_BIP32_CHAINCODE_SIZE);
    if (usepubckd == true) {
        memcpy_safe(node->public_key, keymaster, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    } else {
        memcpy_safe(node->private_key, keymaster, DOGECOIN_ECKEY_PKEY_LENGTH);
        dogecoin_hdnode_fill_public_key(node);
    }

    pch = strtok_r(kp + 2, delim, &saveptr);
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
        pch = strtok_r(NULL, delim, &saveptr);
    }
    dogecoin_free(kp);
    return true;

err:
    dogecoin_free(kp);
    return false;
}


/**
 * @brief This function checks if the HD node has a private key.
 * 
 * @param node The HD node to check.
 * 
 * @return A boolean value, true if node has a private key.
 */
dogecoin_bool dogecoin_hdnode_has_privkey(dogecoin_hdnode* node)
{
    int i;
    for (i = 0; i < DOGECOIN_ECKEY_PKEY_LENGTH; ++i) {
        if (node->private_key[i] != 0)
            return true;
    }
    return false;
}
