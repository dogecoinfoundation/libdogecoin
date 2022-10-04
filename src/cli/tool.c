/*
 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
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
#ifdef HAVE_CONFIG_H
#  include "libdogecoin-config.h"
#endif
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <getopt.h>
#include <unistd.h>
#endif

#include <dogecoin/bip32.h>
#include <dogecoin/base58.h>
#include <dogecoin/ecc.h>
#include <dogecoin/key.h>
#include <dogecoin/random.h>
#include <dogecoin/serialize.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

/**
 * @brief Given a public key, it returns the addresses that correspond to it
 * 
 * @param chain The chainparams struct for the chain you want to generate addresses for.
 * @param pubkey_hex The hexadecimal representation of the public key.
 * @param p2pkh_address The address in legacy format (DRnwTww6YB2zw1TTjiPfeFkfuabojfyaB8)
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool addresses_from_pubkey(const dogecoin_chainparams* chain, const char* pubkey_hex, char* p2pkh_address)
{
    if (!pubkey_hex)
        return false;
    if (strlen(pubkey_hex) != 66)
        return false;
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    pubkey.compressed = 1;
    size_t outlen = 0;
    utils_hex_to_bin(pubkey_hex, pubkey.pubkey, strlen(pubkey_hex), &outlen);
    assert(dogecoin_pubkey_is_valid(&pubkey) == 1);
    dogecoin_pubkey_getaddr_p2pkh(&pubkey, chain, p2pkh_address);
    return true;
}

/**
 * @brief Given a private key in WIF format, convert it to a public key and return it in hex format
 * 
 * @param chain The chainparams structure.
 * @param privkey_wif The private key in WIF format.
 * @param pubkey_hex The public key in hexadecimal format.
 * @param sizeout The size of the output buffer.
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool pubkey_from_privatekey(const dogecoin_chainparams* chain, const char* privkey_wif, char* pubkey_hex, size_t* sizeout)
{
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    if (!dogecoin_privkey_decode_wif(privkey_wif, chain, &key))
        return false;
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    assert(dogecoin_pubkey_is_valid(&pubkey) == 0);
    dogecoin_pubkey_from_key(&key, &pubkey);
    dogecoin_privkey_cleanse(&key);
    dogecoin_pubkey_get_hex(&pubkey, pubkey_hex, sizeout);
    dogecoin_pubkey_cleanse(&pubkey);
    return true;
}

/**
 * @brief Generate a private key and export it in both WIF and hex format
 * 
 * @param chain the chain parameters
 * @param privkey_wif the private key in WIF format
 * @param strsize_wif the size of the buffer to hold the WIF key
 * @param privkey_hex_or_null If you pass in a valid pointer, the function will export the private key
 * in hex format.
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool gen_privatekey(const dogecoin_chainparams* chain, char* privkey_wif, size_t strsize_wif, char* privkey_hex_or_null)
{
    uint8_t pkeybase58c[34];
    pkeybase58c[0] = chain->b58prefix_secret_address;
    pkeybase58c[33] = 1; /* always use compressed keys */
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    dogecoin_privkey_gen(&key);
    memcpy_safe(&pkeybase58c[1], key.privkey, DOGECOIN_ECKEY_PKEY_LENGTH);
    assert(dogecoin_base58_encode_check(pkeybase58c, 34, privkey_wif, strsize_wif) != 0);
    // also export the hex privkey if use had passed in a valid pointer
    // will always export 32 bytes
    if (privkey_hex_or_null != NULL)
        utils_bin_to_hex(key.privkey, DOGECOIN_ECKEY_PKEY_LENGTH, privkey_hex_or_null);
    dogecoin_privkey_cleanse(&key);
    return true;
}

/**
 * @brief Generate a master key from a random seed
 * 
 * @param chain The chain parameters to use.
 * @param masterkeyhex The master key in hexadecimal format.
 * @param strsize The size of the buffer to be returned.
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool hd_gen_master(const dogecoin_chainparams* chain, char* masterkeyhex, size_t strsize)
{
    dogecoin_hdnode node;
    uint8_t seed[32];
    dogecoin_random_bytes(seed, 32, true);
    dogecoin_hdnode_from_seed(seed, 32, &node);
    dogecoin_mem_zero(seed, 32);
    dogecoin_hdnode_serialize_private(&node, chain, masterkeyhex, strsize);
    dogecoin_mem_zero(&node, sizeof(node));
    return true;
}

/**
 * @brief It takes in a serialized extended public key and prints out all the information about it
 * 
 * @param chain The chain parameters.
 * @param nodeser The serialized extended public key.
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool hd_print_node(const dogecoin_chainparams* chain, const char* nodeser)
{
    dogecoin_hdnode node;
    if (!dogecoin_hdnode_deserialize(nodeser, chain, &node))
        return false;

    char str[128];
    size_t strsize = sizeof(str);
    printf("ext key:             %s\n", nodeser);

    uint8_t pkeybase58c[34];
    dogecoin_hdnode_serialize_public(&node, chain, str, strsize);
    printf("extended pubkey:     %s\n", str);

    if (!dogecoin_hdnode_get_pub_hex(&node, str, &strsize))
        return false;
    printf("pubkey hex:          %s\n", str);

    pkeybase58c[0] = chain->b58prefix_secret_address;
    pkeybase58c[33] = 1; /* always use compressed keys */
    char privkey_wif[128];
    memcpy_safe(&pkeybase58c[1], node.private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    assert(dogecoin_base58_encode_check(pkeybase58c, sizeof(pkeybase58c), privkey_wif, sizeof(privkey_wif)) != 0);
    if (dogecoin_hdnode_has_privkey(&node)) {
        printf("privatekey WIF:      %s\n", privkey_wif);
    }
    printf("depth:               %d\n", node.depth);
    printf("child index:         %d\n", node.child_num);
    char addr[34 + 1];
    addresses_from_pubkey(&dogecoin_chainparams_main, str, addr);
    printf("p2pkh address:       %s\n", addr);
    return true;
}

/**
 * @brief It takes a master key, a path, and an output buffer. It derives the child key at the path and writes
 * it to the output buffer
 * 
 * @param chain The chain parameters.
 * @param masterkey The master key to derive the child key from.
 * @param derived_path The path to derive the key from.
 * @param extkeyout The output buffer to write the extended key to.
 * @param extkeyout_size The size of the extkeyout buffer.
 * 
 * @return dogecoin_bool (uint8_t)
 */
dogecoin_bool hd_derive(const dogecoin_chainparams* chain, const char* masterkey, const char* derived_path, char* extkeyout, size_t extkeyout_size)
{
    if (!derived_path || !masterkey || !extkeyout)
        return false;
    dogecoin_hdnode node, nodenew;
    if (!dogecoin_hdnode_deserialize(masterkey, chain, &node))
        return false;
    //check if we only have the publickey
    bool pubckd = !dogecoin_hdnode_has_privkey(&node);
    //derive child key, use pubckd or privckd
    if (!dogecoin_hd_generate_key(&nodenew, derived_path, pubckd ? node.public_key : node.private_key, node.chain_code, pubckd))
        return false;
    if (pubckd)
        dogecoin_hdnode_serialize_public(&nodenew, chain, extkeyout, extkeyout_size);
    else
        dogecoin_hdnode_serialize_private(&nodenew, chain, extkeyout, extkeyout_size);
    return true;
}
