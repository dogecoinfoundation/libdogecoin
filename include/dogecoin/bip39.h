/**********************************************************************
 * Copyright (c) 2023 edtubbs                                         *
 * Copyright (c) 2023 The Dogecoin Foundation                         *
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

/* number of words in the language wordlist used for mnemonics */
#define LANG_WORD_CNT 2048

/* Indicates the number of entropy bits supported */
#define MAX_ENTROPY_BITS 256

/* Specifies the number of hex characters needed to represent a single byte */
#define HEX_CHARS_PER_BYTE 2

/* Maximum size of a hex string representing entropy in bytes */
#define MAX_ENTROPY_STRING_SIZE (MAX_ENTROPY_BITS / 8) * HEX_CHARS_PER_BYTE + 1

/* Maximum number of bits supported for checksum */
#define MAX_CHECKSUM_BITS 8

/* Maximum number of words in a mnemonic phrase */
#define MAX_WORDS_IN_MNEMONIC (MAX_ENTROPY_BITS + MAX_CHECKSUM_BITS) / 11

/* Maximum number of characters in a single word of a mnemonic phrase */
#define MAX_CHARS_IN_MNEMONIC_WORD 16

/* Maximum size of a mnemonic phrase string in bytes */
#define MAX_MNEMONIC_STRING_SIZE (MAX_WORDS_IN_MNEMONIC * MAX_CHARS_IN_MNEMONIC_WORD * HEX_CHARS_PER_BYTE) + 1

/* Maximum number of characters in a passphrase */
#define MAX_CHARS_IN_PASSPHRASE 256

/* Maximum size of a passphrase string in bytes */
#define MAX_PASSPHRASE_STRING_SIZE (MAX_CHARS_IN_PASSPHRASE * HEX_CHARS_PER_BYTE) + 1

/* Maximum size of a seed in bytes */
#define MAX_SEED_SIZE 64

/* Specifies the number of decimal characters needed to represent entropy size */
#define ENTROPY_SIZE_STRING_SIZE 3

/* format string to print the binary representation of a byte */
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"

/* macro to pass a byte to printf with the BYTE_TO_BINARY_PATTERN format */
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

/* format string to print the first 4 bits of a byte in binary representation */
#define BYTE_TO_FIRST_FOUR_BINARY_PATTERN "%c%c%c%c"

/* macro to pass a byte to printf with the BYTE_TO_FIRST_FOUR_BINARY_PATTERN format */
#define BYTE_TO_FIRST_FOUR_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0')

/* format string to print the first 5 bits of a byte in binary representation */
#define BYTE_TO_FIRST_FIVE_BINARY_PATTERN "%c%c%c%c%c"

/* macro to pass a byte to printf with the BYTE_TO_FIRST_FIVE_BINARY_PATTERN format */
#define BYTE_TO_FIRST_FIVE_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0')

/* format string to print the first 6 bits of a byte in binary representation */
#define BYTE_TO_FIRST_SIX_BINARY_PATTERN "%c%c%c%c%c%c"

/* macro to pass a byte to printf with the BYTE_TO_FIRST_SIX_BINARY_PATTERN format */
#define BYTE_TO_FIRST_SIX_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0')

/* format string to print the first 7 bits of a byte in binary representation */
#define BYTE_TO_FIRST_SEVEN_BINARY_PATTERN "%c%c%c%c%c%c%c"

/* macro to pass a byte to printf with the BYTE_TO_FIRST_SEVEN_BINARY_PATTERN format */
#define BYTE_TO_FIRST_SEVEN_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0')

/*
 * Type definitions
 */

/* A string representation of entropy used to generate a BIP39 mnemonic */
/* The entropy should be a hexadecimal string with a maximum size of MAX_ENTROPY_STRING_SIZE */
typedef char HEX_ENTROPY[MAX_ENTROPY_STRING_SIZE];

/* A string used as an additional input to the PBKDF2 function when generating a BIP39 mnemonic */
/* The passphrase should be a string with a maximum size of MAX_PASSPHRASE_STRING_SIZE */
typedef char PASSPHRASE[MAX_PASSPHRASE_STRING_SIZE];

/* A string representation of a BIP39 mnemonic phrase, used as a seed to generate private and public keys */
/* The mnemonic should be a space-separated string with a maximum size of MAX_MNEMONIC_STRING_SIZE */
typedef char MNEMONIC[MAX_MNEMONIC_STRING_SIZE];

/* A binary representation of the BIP39 seed, used to generate private and public keys */
/* The seed should be an unsigned integer with a maximum size of MAX_SEED_SIZE */
typedef uint8_t SEED[MAX_SEED_SIZE];

/* A string representation of the entropy size to generate a BIP39 mnemonic */
/* The entropy size should be a decimal string with a size of ENTROPY_SIZE_STRING_SIZE */
typedef char ENTROPY_SIZE [ENTROPY_SIZE_STRING_SIZE];

/*
 * Function declarations
 */

/* Generates mnemonic phrase */
int get_mnemonic(const int entropysize, const char* entropy, const char* wordslist[], const char* space, char* entropy_out, char* mnemonic, size_t* mnemonic_size);

/* Produces the mnemonic sentence */
int produce_mnemonic_sentence(const int segSize, const int checksumBits, const char* firstByte, const char* entropy, const char* wordlist[], const char* space, char* mnemonic, size_t* mnemonic_size);

/* Generates root seed for HD wallet */
int get_root_seed(const char* pass, const char* passphrase, SEED seed);

/* Loads words into memory from specified language */
int get_words(const char* lang, char* wordlist[]);

/* Returns custom words from specified file as a parameter */
int get_custom_words(const char* filepath, char* wordlist[]);

/* Generate a mnemonic for a given entropy size and language */
/* "128", "160", "192", "224", or "256" bits of entropy */
/* ISO 639-2 code for the mnemonic language */
/* en.wikipedia.org/wiki/List_of_ISO_639-2_codes */
/* space character to seperate mnemonic */
/* entropy from the caller (optional) */
/* path to custom word list file (optional, language ignored) */
/* entropy of given size as hex string (output) */
/* size of the generated mnemonic in bytes (output) */
/* generated mnemonic (output) */
/* returns 0 (success), -1 (fail) */
LIBDOGECOIN_API int dogecoin_generate_mnemonic (const ENTROPY_SIZE entropy_size, const char* language, const char* space, const char* entropy, const char* filename, char* entropy_out, size_t* size, char* words);

/* Derive the seed from the mnemonic and optional passphrase */
/* mnemonic code words */
/* passphrase (optional) */
/* 512-bit seed */
/* returns 0 (success), -1 (fail) */
LIBDOGECOIN_API int dogecoin_seed_from_mnemonic (const char* mnemonic, const char* passphrase, SEED seed);

/* Generates a random (256-bit) English mnemonic phrase */
/* size of the entropy in bits */
/* output buffer for the generated mnemonic */
/* return: 0 (success), -1 (fail) */
LIBDOGECOIN_API int generateRandomEnglishMnemonic(const ENTROPY_SIZE size, MNEMONIC mnemonic);

/* Generates an English mnemonic phrase from given hex string of entropy */
/* hex string of the entropy to use for generating the mnemonic */
/* size of the entropy in bits */
/* output buffer for the generated mnemonic */
/* return: 0 (success), -1 (fail) */
LIBDOGECOIN_API int generateEnglishMnemonic(const HEX_ENTROPY entropy, const ENTROPY_SIZE size, MNEMONIC mnemonic);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_BIP39_H__
