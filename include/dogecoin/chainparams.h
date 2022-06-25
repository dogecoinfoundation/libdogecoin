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

#ifndef __LIBDOGECOIN_CHAINPARAMS_H__
#define __LIBDOGECOIN_CHAINPARAMS_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

typedef struct dogecoin_dns_seed_ {
    char domain[256];
} dogecoin_dns_seed;

typedef struct dogecoin_chainparams_ {
    char chainname[32];
    uint8_t b58prefix_pubkey_address;
    uint8_t b58prefix_script_address;
    const char bech32_hrp[5];
    uint8_t b58prefix_secret_address; //!private key
    uint32_t b58prefix_bip32_privkey;
    uint32_t b58prefix_bip32_pubkey;
    const unsigned char netmagic[4];
    uint256 genesisblockhash;
    int default_port;
    dogecoin_dns_seed dnsseeds[8];
} dogecoin_chainparams;

typedef struct dogecoin_checkpoint_ {
    uint32_t height;
    const char* hash;
    uint32_t timestamp;
    uint32_t target;
} dogecoin_checkpoint;

extern const dogecoin_chainparams dogecoin_chainparams_main;
extern const dogecoin_chainparams dogecoin_chainparams_test;
extern const dogecoin_chainparams dogecoin_chainparams_regtest;

// the mainnet checkpoints, needs a fix size
extern const dogecoin_checkpoint dogecoin_mainnet_checkpoint_array[21];
extern const dogecoin_checkpoint dogecoin_testnet_checkpoint_array[17];

LIBDOGECOIN_API const dogecoin_chainparams* chain_from_b58_prefix(const char* address);
LIBDOGECOIN_API int chain_from_b58_prefix_bool(char* address);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_CHAINPARAMS_H__
