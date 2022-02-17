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

#include <dogecoin/address.h>

#ifdef HAVE_CONFIG_H
#  include "libdogecoin-config.h"
#endif
#include <dogecoin/tool.h>
#include <dogecoin/bip32.h>
#include <dogecoin/crypto/key.h>
#include <dogecoin/crypto/random.h>
#include <dogecoin/utils.h>

#include <assert.h>
#ifndef _MSC_VER
#  include <getopt.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#  include <unistd.h>
#endif

int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet) {
    /* internal variables */
    size_t sizeout = 100;
    char wif_privkey_internal[sizeout];
    char p2pkh_pubkey_internal[sizeout];
    bool is_testnet_internal = false;

    /* if nothing is passed in use internal variables */
    if (wif_privkey) memcpy(wif_privkey_internal, wif_privkey, sizeout);
    if (p2pkh_pubkey) memcpy(p2pkh_pubkey_internal, p2pkh_pubkey, sizeout);
    if (is_testnet) is_testnet_internal = is_testnet;

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet_internal ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

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

    if (wif_privkey) memcpy(wif_privkey, wif_privkey_internal, sizeout);
    if (p2pkh_pubkey) memcpy(p2pkh_pubkey, p2pkh_pubkey_internal, sizeout);

    // printf("wif_privkey:  %s\n", wif_privkey);
    // printf("p2pkh_pubkey: %s\n\n", p2pkh_pubkey);

    /* reset internal variables */
    memset(wif_privkey_internal, 0, strlen(wif_privkey_internal));
    memset(p2pkh_pubkey_internal, 0, strlen(p2pkh_pubkey_internal));

    // TODO: evaluate how we're going to deal with key storage and cleansing memory
    // /* cleanup memory */
    // dogecoin_pubkey_cleanse(&pubkey);
    // dogecoin_privkey_cleanse(&key);
    return true;
}

int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet) {
    size_t strsize = 128;
    char hd_privkey_master[strsize];
    char hd_pubkey_master[strsize];
    bool is_testnet_internal = false;

    /* if nothing is passed use internal variables */
    if (wif_privkey_master) memcpy(hd_privkey_master, wif_privkey_master, strsize);
    if (p2pkh_pubkey_master) memcpy(hd_pubkey_master, p2pkh_pubkey_master, strsize);
    if (is_testnet) is_testnet_internal = is_testnet; 

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;
    /* generate a new hd master key */
    hd_gen_master(chain, hd_privkey_master, strsize);

    generateDerivedHDPubkey(hd_privkey_master, hd_pubkey_master);

    if (wif_privkey_master) memcpy(wif_privkey_master, hd_privkey_master, strlen(hd_privkey_master));
    if (p2pkh_pubkey_master) memcpy(p2pkh_pubkey_master, hd_pubkey_master, strlen(hd_pubkey_master));

    /* reset internal variables */
    memset(hd_privkey_master, 0, strlen(hd_privkey_master));
    memset(hd_privkey_master, 0, strlen(hd_privkey_master));

    // printf("wif_privkey_master:   %s\n", wif_privkey_master);
    // printf("p2pkh_pubkey_master:  %s\n", p2pkh_pubkey_master);

    // TODO: evaluate how we're going to deal with key storage and cleansing memory
    // memset(wif_privkey_master, 0, strlen(wif_privkey_master));
    // memset(p2pkh_pubkey_master, 0, strlen(p2pkh_pubkey_master));
    return true;
}

int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey) {
    /* require master key */
    if (!wif_privkey_master) return false;

    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;

    size_t strsize = 128;
    char str[strsize];

    /* if nothing is passed in use internal variables */
    if (p2pkh_pubkey) memcpy(str, p2pkh_pubkey, strsize);

    dogecoin_hdnode node;
    dogecoin_hdnode_deserialize(wif_privkey_master, chain, &node);

    dogecoin_hdnode_get_p2pkh_address(&node, chain, str, strsize);

    /* pass back to external variable if exists */
    if (p2pkh_pubkey) memcpy(p2pkh_pubkey, str, strsize);

    /* reset internal variables */
    memset(str, 0, strlen(str));

    return true;
}
