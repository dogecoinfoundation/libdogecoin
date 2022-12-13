/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David L. Whitehurst
 * Copyright (c) 2022 edtubbs
 * Copyright (c) 2022 The Dogecoin Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * bip39c.h
 * A BIP-39 Implementation using C.
 *
 * author: David L. Whitehurst
 * date: May 30, 2018
 *
 * Find this code useful? Please donate:
 *  Bitcoin: 1Mxt427mTF3XGf8BiJ8HjkhbiSVvJbkDFY
 *
 */

#include <stdint.h>
#include <stddef.h>

#ifndef BIP39C_BIP39C_H
#define BIP39C_BIP39C_H



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

/* Define to the full name of this package. */
#define PACKAGE_NAME "bip39c"

/*
 * Function declarations
 */

/* Generates mnemonic phrase */
int get_mnemonic(const int entropysize, const char* wordslist[], char *mnemonic, size_t* mnemonic_len);

/* Produces the mnemonic sentence */
int produce_mnemonic_sentence(const int segSize, const int checksumBits, const char *firstByte, const char entropy[], const char* wordlist[], char *mnemonic, size_t *mnemonic_len);

/* Generates root seed for HD wallet */
int get_root_seed(const char *pass, const char *passphrase, uint8_t seed[64]);

/* Loads words into memory from specified language */
void get_words(const char* lang, char* wordlist[]);

/* Returns custom words from specified file as a parameter */
int get_custom_words(const char *filepath, char* wordlist[]);

#endif //BIP39C_BIP39C_H
