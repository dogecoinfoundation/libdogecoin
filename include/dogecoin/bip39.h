/**********************************************************************
 * Copyright (c) 2022 edtubbs                                         *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef __LIBDOGECOIN_BIP39_H__
#define __LIBDOGECOIN_BIP39_H__

#include <stdint.h>
#include <stddef.h>

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API

/*
 * Defines
 */

# define LANG_WORD_CNT   2048

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

#define BYTE_TO_FIRST_FOUR_BINARY_PATTERN "%c%c%c%c"
#define BYTE_TO_FIRST_FOUR_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0')

#define BYTE_TO_FIRST_FIVE_BINARY_PATTERN "%c%c%c%c%c"
#define BYTE_TO_FIRST_FIVE_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0')

#define BYTE_TO_FIRST_SIX_BINARY_PATTERN "%c%c%c%c%c%c"
#define BYTE_TO_FIRST_SIX_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0')

#define BYTE_TO_FIRST_SEVEN_BINARY_PATTERN "%c%c%c%c%c%c%c"
#define BYTE_TO_FIRST_SEVEN_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0')

/*
 * Function declarations
 */

/* Generates mnemonic phrase */
int get_mnemonic(const int entropysize, const char* entropy, const char* wordslist[], const char* space, char *mnemonic, size_t* mnemonic_len);

/* Produces the mnemonic sentence */
int produce_mnemonic_sentence(const int segSize, const int checksumBits, const char *firstByte, const char* entropy, const char* wordlist[], const char* space, char *mnemonic, size_t *mnemonic_len);

/* Generates root seed for HD wallet */
int get_root_seed(const char *pass, const char *passphrase, uint8_t seed[64]);

/* Loads words into memory from specified language */
void get_words(const char* lang, char* wordlist[]);

/* Returns custom words from specified file as a parameter */
int get_custom_words(const char *filepath, char* wordlist[]);

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

