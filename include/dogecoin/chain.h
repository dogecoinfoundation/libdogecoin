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

#ifndef __LIBDOGECOIN_CHAIN_H__
#define __LIBDOGECOIN_CHAIN_H__

#include "dogecoin.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>

typedef struct dogecoin_chain {
    char chainname[32];
    uint8_t b58prefix_pubkey_address;
    uint8_t b58prefix_script_address;
    uint8_t b58prefix_secret_address; //!private key
    uint32_t b58prefix_bip32_privkey;
    uint32_t b58prefix_bip32_pubkey;
    const unsigned char netmagic[4];
} dogecoin_chain;

static const dogecoin_chain dogecoin_chain_main = {"main", 0x1E, 0x16, 0x9E, 0x02fac398, 0x02facafd, {0xc0, 0xc0, 0xc0, 0xc0}};
static const dogecoin_chain dogecoin_chain_test = {"testnet3", 0x71, 0xc4, 0xf1, 0x04358394, 0x043587cf, {0xfc, 0xc1, 0xb7, 0xdc}};
static const dogecoin_chain dogecoin_chain_regt = {"regtest", 0x6f, 0xc4, 0xEF, 0x04358394, 0x043587CF,{0xfa, 0xbf, 0xb5, 0xda}};

#ifdef __cplusplus
}
#endif

#endif //__LIBDOGECOIN_CHAIN_H__
