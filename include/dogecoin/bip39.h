/**********************************************************************
 * Copyright (c) 2022 edtubbs                                         *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef __LIBDOGECOIN_BIP39_H__
#define __LIBDOGECOIN_BIP39_H__

#include <stdint.h>

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API

/* Generate a mnemonic for a given entropy size and language */
/* 128, 160, 192, 224, or 256 bits of entropy */
/* ISO 639-2 code for the mnemonic language */
/* en.wikipedia.org/wiki/List_of_ISO_639-2_codes */
/* space character to seperate mnemonic */
/* entropy from the caller (optional) */
/* length of the generated mnemonic in bytes (output) */
/* generated mnemonic (output) */
/* returns 0 (success), -1 (fail) */
int dogecoin_generate_mnemonic (const char* entropy_size, const char* language, const char* space, const char* entropy, const char* filename, size_t* length, char* words);

/* Derive the seed from the mnemonic */
/* mnemonic code words */
/* passphrase (optional) */
/* 512-bit seed */
/* returns 0 (success), -1 (fail) */
int dogecoin_seed_from_mnemonic (const char* mnemonic, const char* passphrase, uint8_t seed[64]);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_BIP39_H__

