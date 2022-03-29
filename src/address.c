/*

 The MIT License (MIT)

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
#ifndef _MSC_VER
#include <getopt.h>
#endif
#ifdef HAVE_CONFIG_H
#include <src/libdogecoin-config.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <dogecoin/address.h>
#include <dogecoin/bip32.h>
#include <dogecoin/crypto/key.h>
#include <dogecoin/crypto/random.h>
#include <dogecoin/crypto/sha2.h>
#include <dogecoin/crypto/base58.h>
#include <dogecoin/tool.h>
#include <dogecoin/utils.h>

/**
 * @brief This function will populate provided private/public char* 
 * variables with a freshly generated private/public key pair, 
 * specifically for the network specified by the third parameter.
 * 
 * @param wif_privkey 
 * @param p2pkh_pubkey 
 * @param is_testnet 
 * @return int 
 */
int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet)
{
    /* internal variables */
    size_t sizeout = 100;
    char wif_privkey_internal[sizeout];
    char p2pkh_pubkey_internal[sizeout];

    /* if nothing is passed in use internal variables */
    if (wif_privkey) {
        memcpy(wif_privkey_internal, wif_privkey, sizeout);
    }
    if (p2pkh_pubkey) {
        memcpy(p2pkh_pubkey_internal, p2pkh_pubkey, sizeout);
    }

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* generate a new private key */
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    dogecoin_privkey_gen(&key);
    dogecoin_privkey_encode_wif(&key, chain, wif_privkey_internal, &sizeout);

    /* generate a new public key and export hex */
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    assert(dogecoin_pubkey_is_valid(&pubkey) == 0);
    dogecoin_pubkey_from_key(&key, &pubkey);

    /* export p2pkh_pubkey address */
    dogecoin_pubkey_getaddr_p2pkh(&pubkey, chain, p2pkh_pubkey_internal);

    if (wif_privkey) {
        memcpy(wif_privkey, wif_privkey_internal, sizeout);
    }
    if (p2pkh_pubkey) {
        memcpy(p2pkh_pubkey, p2pkh_pubkey_internal, sizeout);
    }

    /* reset internal variables */
    memset(wif_privkey_internal, 0, strlen(wif_privkey_internal));
    memset(p2pkh_pubkey_internal, 0, strlen(p2pkh_pubkey_internal));

    /* cleanup memory */
    dogecoin_pubkey_cleanse(&pubkey);
    dogecoin_privkey_cleanse(&key);
    return true;
}

/**
 * @brief This function will populate provided private/public char*
 * variables with a freshly generated master key pair for a
 * hierarchical deterministic wallet, specifically for the network
 * specified by the third parameter.
 * 
 * @param wif_privkey_master 
 * @param p2pkh_pubkey_master 
 * @param is_testnet 
 * @return int 
 */
int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)
{
    size_t strsize = 128;
    char hd_privkey_master[strsize];
    char hd_pubkey_master[strsize];

    /* if nothing is passed use internal variables */
    if (wif_privkey_master) {
        memcpy(hd_privkey_master, wif_privkey_master, strsize);
    }
    if (p2pkh_pubkey_master) {
        memcpy(hd_pubkey_master, p2pkh_pubkey_master, strsize);
    }

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* generate a new hd master key */
    hd_gen_master(chain, hd_privkey_master, strsize);

    generateDerivedHDPubkey(hd_privkey_master, hd_pubkey_master);

    if (wif_privkey_master) {
        memcpy(wif_privkey_master, hd_privkey_master, strlen(hd_privkey_master));
    }
    if (p2pkh_pubkey_master) {
        memcpy(p2pkh_pubkey_master, hd_pubkey_master, strlen(hd_pubkey_master));
    }

    /* reset internal variables */
    memset(hd_privkey_master, 0, strlen(hd_privkey_master));
    memset(hd_privkey_master, 0, strlen(hd_privkey_master));
    
    return true;
}

/**
 * @brief This function will populate the provided public char*
 * variable with a child key derived from the provided wif-
 * encoded master private key string. This input should come
 * from the result of generateHDMasterPubKeypair().
 * 
 * @param wif_privkey_master 
 * @param p2pkh_pubkey 
 * @return int 
 */
int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey)
{
    /* require master key */
    if (!wif_privkey_master) {
        return false;
    }

    /* determine address prefix for network chainparams */
    char prefix[1];
    memcpy(prefix, wif_privkey_master, 1);
    const dogecoin_chainparams* chain = memcmp(prefix, "d", 1) == 0 ? &dogecoin_chainparams_main : &dogecoin_chainparams_test;

    size_t strsize = 128;
    char str[strsize];

    /* if nothing is passed in use internal variables */
    if (p2pkh_pubkey) {
        memcpy(str, p2pkh_pubkey, strsize);
    }

    dogecoin_hdnode* node = dogecoin_hdnode_new();
    dogecoin_hdnode_deserialize(wif_privkey_master, chain, node);

    dogecoin_hdnode_get_p2pkh_address(node, chain, str, strsize);

    /* pass back to external variable if exists */
    if (p2pkh_pubkey) {
        memcpy(p2pkh_pubkey, str, strsize);
    }

    /* reset internal variables */
    dogecoin_hdnode_free(node);
    memset(str, 0, strlen(str));

    return true;
}

/**
 * @brief This function validates that a given private key
 * matches a given public key. This could be useful prior
 * to signing, or in some kind of wallet recovery tool to
 * match keys.
 * 
 * @param wif_privkey 
 * @param p2pkh_pubkey 
 * @param is_testnet 
 * @return int 
 */
int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet) {
    /* require both private and public key */
    if (!wif_privkey || !p2pkh_pubkey) return false;

    /* set chain */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;
    size_t sizeout = 100;

    /* verify private key */
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    dogecoin_privkey_decode_wif(wif_privkey, chain, &key);
    if (!dogecoin_privkey_is_valid(&key)) return false;
    char new_wif_privkey[sizeout];
    dogecoin_privkey_encode_wif(&key, chain, new_wif_privkey, &sizeout);

    /* verify public key */
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    dogecoin_pubkey_from_key(&key, &pubkey);
    if (!dogecoin_pubkey_is_valid(&pubkey)) return false;

    /* verify address derived matches provided address */
    char new_p2pkh_pubkey[sizeout];
    dogecoin_pubkey_getaddr_p2pkh(&pubkey, chain, new_p2pkh_pubkey);
    if (strcmp(p2pkh_pubkey, new_p2pkh_pubkey)) return false;

    dogecoin_pubkey_cleanse(&pubkey);
    dogecoin_privkey_cleanse(&key);
    return true;
}

/**
 * @brief This function will populate provided private/public char*
 * variables with a freshly generated master key pair for a
 * hierarchical deterministic wallet, specifically for the network
 * specified by the third parameter.
 * 
 * @param wif_privkey_master 
 * @param p2pkh_pubkey_master 
 * @param is_testnet 
 * @return int 
 */
int verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet) {
    /* require both private and public key */
    if (!wif_privkey_master || !p2pkh_pubkey_master) return false;

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* calculate master pubkey from master privkey */
    dogecoin_hdnode node;
    dogecoin_hdnode_deserialize(wif_privkey_master, chain, &node);
    size_t sizeout = 128;
    char new_p2pkh_pubkey_master[sizeout];
    dogecoin_hdnode_get_p2pkh_address(&node, chain, new_p2pkh_pubkey_master, sizeout);

    /* compare derived and given pubkeys */
    if (strcmp(p2pkh_pubkey_master, new_p2pkh_pubkey_master)) return false;

    return true;
}

/**
 * @brief This function accepts a public key and does some basic
 * validation to determine if it looks like a valid Dogecoin Address.
 * This is useful for wallets that want to check that a recipient
 * address looks legitimate.
 * 
 * @param p2pkh_pubkey 
 * @param len 
 * @return int 
 */
int verifyP2pkhAddress(char* p2pkh_pubkey, uint8_t len) {
    if (!p2pkh_pubkey || !len) return false;
    /* check length */
    unsigned char dec[len], d1[SHA256_DIGEST_LENGTH], d2[SHA256_DIGEST_LENGTH];
    if (!dogecoin_base58_decode_check(p2pkh_pubkey, dec, len)) {
        return false;
    }
    /* check validity */
    sha256_raw(dec, 21, d1);
    sha256_raw(d1, SHA256_DIGEST_LENGTH, d2);
    if (memcmp(dec + 21, d2, 4) != 0) {
        return false;
    }
    return true;
}