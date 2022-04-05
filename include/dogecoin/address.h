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

#ifndef __LIBDOGECOIN_ADDRESS_H__
#define __LIBDOGECOIN_ADDRESS_H__

#include <stdbool.h>

#include <dogecoin/dogecoin.h>
#include <dogecoin/tool.h>

LIBDOGECOIN_BEGIN_DECL

/* generate a new private key (hex) */
LIBDOGECOIN_API int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* generate HD master key and WIF public key */
LIBDOGECOIN_API int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* generate an extended public key */
LIBDOGECOIN_API int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey);

/* verify private and public keys are valid and associated with each other*/
LIBDOGECOIN_API int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* verify private and public masters keys are valid and associated with each other */
LIBDOGECOIN_API int verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* verify address based on length and checksum */
LIBDOGECOIN_API int verifyP2pkhAddress(char* p2pkh_pubkey, uint8_t len);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_ADDRESS_H__
