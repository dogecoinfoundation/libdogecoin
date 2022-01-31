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

#ifdef HAVE_CONFIG_H
#  include "libdogecoin-config.h"
#endif
#include <dogecoin/base58.h>
#include <dogecoin/bip32.h>
#include <dogecoin/ecc.h>
#include <dogecoin/ecc_key.h>
#include <dogecoin/random.h>
#include <dogecoin/serialize.h>
#include <dogecoin/utils.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _MSC_VER
#  include <getopt.h>
#  include <unistd.h>
#endif

dogecoin_bool address_from_pubkey(const dogecoin_chain* chain, const char *pubkey_hex, char *address)
{
    if (!pubkey_hex) return false;
    if (strlen(pubkey_hex) != 66) return false;

    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    pubkey.compressed = 1;

    size_t outlen = 0;
    utils_hex_to_bin(pubkey_hex, pubkey.pubkey, strlen(pubkey_hex), (int *)&outlen);
    assert(dogecoin_pubkey_is_valid(&pubkey) == 1);

    uint8_t hash160[21];
    hash160[0] = chain->b58prefix_pubkey_address;
    dogecoin_pubkey_get_hash160(&pubkey, hash160+1);

    dogecoin_base58_encode_check(hash160, 21, address, 98);

    return true;
}

dogecoin_bool pubkey_from_privatekey(const dogecoin_chain* chain, const char *privkey_wif, char *pubkey_hex, size_t *sizeout)
{
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    if (!dogecoin_privkey_decode_wif(privkey_wif, chain, &key)) {
        return false;
    }

    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    assert(dogecoin_pubkey_is_valid(&pubkey) == 0);
    dogecoin_pubkey_from_key(&key, &pubkey);
    dogecoin_privkey_cleanse(&key);

    dogecoin_pubkey_get_hex(&pubkey, pubkey_hex, sizeout);
    dogecoin_pubkey_cleanse(&pubkey);
    
    return true;
}

dogecoin_bool gen_privatekey(const dogecoin_chain* chain, char *privkey_wif, size_t strsize_wif, char *privkey_hex_or_null)
{
    uint8_t pkeybase58c[34];
    pkeybase58c[0] = chain->b58prefix_secret_address;
    pkeybase58c[33] = 1; /* always use compressed keys */

    dogecoin_key key;
    dogecoin_privkey_init(&key);
    dogecoin_privkey_gen(&key);
    memcpy(&pkeybase58c[1], key.privkey, DOGECOIN_ECKEY_PKEY_LENGTH);
    assert(dogecoin_base58_encode_check(pkeybase58c, 34, privkey_wif, strsize_wif) != 0);

    // also export the hex privkey if use had passed in a valid pointer
    // will always export 32 bytes
    if (privkey_hex_or_null != NULL)
        utils_bin_to_hex(key.privkey, DOGECOIN_ECKEY_PKEY_LENGTH, privkey_hex_or_null);
    dogecoin_privkey_cleanse(&key);
    return true;
}

dogecoin_bool hd_gen_master(const dogecoin_chain* chain, char *masterkeyhex, size_t strsize)
{
    dogecoin_hdnode node;
    uint8_t seed[32];
    random_bytes(seed, 32, true);
    dogecoin_hdnode_from_seed(seed, 32, &node);
    memset(seed, 0, 32);
    dogecoin_hdnode_serialize_private(&node, chain, masterkeyhex, strsize);
    memset(&node, 0, sizeof(node));
    return true;
}

dogecoin_bool hd_print_node(const dogecoin_chain* chain, const char *nodeser)
{
    dogecoin_hdnode node;
    if (!dogecoin_hdnode_deserialize(nodeser, chain, &node))
        return false;

    size_t strsize = 128;
    char str[strsize];
    dogecoin_hdnode_get_p2pkh_address(&node, chain, str, strsize);

    printf("ext key: %s\n", nodeser);

    printf("depth: %d\n", node.depth);
    printf("p2pkh address: %s\n", str);

    if (!dogecoin_hdnode_get_pub_hex(&node, str, &strsize))
        return false;
    printf("pubkey hex: %s\n", str);

    strsize = 128;
    dogecoin_hdnode_serialize_public(&node, chain, str, strsize);
    printf("extended pubkey: %s\n", str);
    return true;
}

dogecoin_bool hd_derive(const dogecoin_chain* chain, const char *masterkey, const char *derived_path, char *extkeyout, size_t extkeyout_size)
{
    if (!derived_path || !masterkey || !extkeyout) return false;
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
