/**
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 * Copyright (c) 2022 edtubbs
 * Copyright (c) 2022 bluezr
 * Copyright (c) 2022 The Dogecoin Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include <unicode/unorm2.h>
#include <bip39/index.h>

#include <dogecoin/bip39.h>
#include <dogecoin/utils.h>
#include <dogecoin/options.h>
#include <dogecoin/random.h>
#include <dogecoin/sha2.h>

#if USE_BIP39_CACHE

static int bip39_cache_index = 0;

static CONFIDENTIAL struct {
  bool set;
  char mnemonic[256];
  char passphrase[64];
  uint8_t seed[512 / 8];
} bip39_cache[BIP39_CACHE_SIZE];

void bip39_cache_clear(void) {
  dogecoin_mem_zero(bip39_cache, sizeof(bip39_cache));
  bip39_cache_index = 0;
}

#endif

/*
 * This function reads the language file once and loads an array of words for
 * repeated use.
 */
char * wordlist[2049] = {0};

void get_custom_wordlist(const char *filepath) {
    int i = 0;
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filepath, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        strtok(line, "\n");
        wordlist[i] = malloc(read + 1);
        strcpy(wordlist[i], line);
        i++;
    }

    fclose(fp);
    if (line) free(line);
}

void get_words(const char *lang) {
    int i = 0;
    for (; i <= 2049; i++) {
      if (strcmp(lang,"spa") == 0) {
          wordlist[i]=(char*)wordlist_spa[i];
      } else if (strcmp(lang,"eng") == 0) {
          wordlist[i]=(char*)wordlist_eng[i];
      } else if (strcmp(lang,"jpn") == 0) {
          wordlist[i]=(char*)wordlist_jpn[i];
      } else if (strcmp(lang,"ita") == 0) {
          wordlist[i]=(char*)wordlist_ita[i];
      } else if (strcmp(lang,"fra") == 0) {
          wordlist[i]=(char*)wordlist_fra[i];
      } else if (strcmp(lang,"kor") == 0) {
          wordlist[i]=(char*)wordlist_kor[i];
      } else if (strcmp(lang,"sc") == 0) {
          wordlist[i]=(char*)wordlist_sc[i];
      } else if (strcmp(lang,"tc") == 0) {
          wordlist[i]=(char*)wordlist_tc[i];
      } else if (strcmp(lang,"cze") == 0) {
          wordlist[i]=(char*)wordlist_cze[i];
      } else if (strcmp(lang,"por") == 0) {
          wordlist[i]=(char*)wordlist_por[i];
      } else {
          fprintf(stderr, "Language or language file does not exist.\n");
      }
    }
}

const char *mnemonic_generate(int strength) {
  if (strength % 32 || strength < 128 || strength > 256) {
    return 0;
  }
  uint8_t data[32] = {0};
  dogecoin_cheap_random_bytes(data, 32);
  const char *r = mnemonic_from_data(data, strength / 8);
  dogecoin_mem_zero(data, sizeof(data));
  return r;
}

static CONFIDENTIAL char mnemo[24 * 10];

const char *mnemonic_from_data(const uint8_t *data, int len) {
  if (len % 4 || len < 16 || len > 32) {
    return 0;
  }

  uint8_t bits[32 + 1] = {0};

  sha256_raw(data, len, bits);
  // checksum
  bits[len] = bits[0];
  // data
  memcpy(bits, data, len);

  int mlen = len * 3 / 4;

  int i = 0, j = 0, idx = 0;
  char *p = mnemo;
  for (i = 0; i < mlen; i++) {
    idx = 0;
    for (j = 0; j < 11; j++) {
      idx <<= 1;
      idx += (bits[(i * 11 + j) / 8] & (1 << (7 - ((i * 11 + j) % 8)))) > 0;
    }
    strcpy(p, wordlist[idx]);
    p += strlen(wordlist[idx]);
    *p = (i < mlen - 1) ? ' ' : 0;
    p++;
  }
  dogecoin_mem_zero(bits, sizeof(bits));

  return mnemo;
}

void mnemonic_clear(void) { dogecoin_mem_zero(mnemo, sizeof(mnemo)); }

int mnemonic_to_bits(const char *mnemonic, uint8_t *bits) {
  if (!mnemonic) {
    return 0;
  }

  uint32_t i = 0, n = 0;

  while (mnemonic[i]) {
    if (mnemonic[i] == ' ') {
      n++;
    }
    i++;
  }
  n++;

  // check that number of words is valid for BIP-39:
  // (a) between 128 and 256 bits of initial entropy (12 - 24 words)
  // (b) number of bits divisible by 33 (1 checksum bit per 32 input bits)
  //     - that is, (n * 11) % 33 == 0, so n % 3 == 0
  if (n < 12 || n > 24 || (n % 3)) {
    return 0;
  }

  char current_word[10] = {0};
  uint32_t j = 0, ki = 0, bi = 0;
  uint8_t result[32 + 1] = {0};

  dogecoin_mem_zero(result, sizeof(result));
  i = 0;
  while (mnemonic[i]) {
    j = 0;
    while (mnemonic[i] != ' ' && mnemonic[i] != 0) {
      if (j >= sizeof(current_word) - 1) {
        return 0;
      }
      current_word[j] = mnemonic[i];
      i++;
      j++;
    }
    current_word[j] = 0;
    if (mnemonic[i] != 0) {
      i++;
    }
    int k = mnemonic_find_word(current_word);
    if (k < 0) {  // word not found
      return 0;
    }
    for (ki = 0; ki < 11; ki++) {
      if (k & (1 << (10 - ki))) {
        result[bi / 8] |= 1 << (7 - (bi % 8));
      }
      bi++;
    }
  }
  if (bi != n * 11) {
    return 0;
  }
  memcpy(bits, result, sizeof(result));
  dogecoin_mem_zero(result, sizeof(result));

  // returns amount of entropy + checksum BITS
  return n * 11;
}

int mnemonic_check(const char *mnemonic) {
  uint8_t bits[32 + 1] = {0};
  int mnemonic_bits_len = mnemonic_to_bits(mnemonic, bits);
  if (mnemonic_bits_len != (12 * 11) && mnemonic_bits_len != (18 * 11) &&
      mnemonic_bits_len != (24 * 11)) {
    return 0;
  }
  int words = mnemonic_bits_len / 11;

  uint8_t checksum = bits[words * 4 / 3];
  sha256_raw(bits, words * 4 / 3, bits);
  if (words == 12) {
    return (bits[0] & 0xF0) == (checksum & 0xF0);  // compare first 4 bits
  } else if (words == 18) {
    return (bits[0] & 0xFC) == (checksum & 0xFC);  // compare first 6 bits
  } else if (words == 24) {
    return bits[0] == checksum;  // compare 8 bits
  }
  return 0;
}

const char *nfkd(const char *input) {
    UErrorCode status = U_ZERO_ERROR;
    const UNormalizer2 *nfkd = unorm2_getNFKDInstance(&status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "Error getting NFKD instance: %s\n", u_errorName(status));
        return NULL;
    }

    UChar *input_u = calloc(strlen(input) + 1, sizeof(UChar));
    if (input_u == NULL) {
        fprintf(stderr, "Error allocating memory for input UChar\n");
        return NULL;
    }
    u_strFromUTF8(input_u, strlen(input) + 1, NULL, input, strlen(input), &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "Error converting input to UChar: %s\n", u_errorName(status));
        free(input_u);
        return NULL;
    }

    int32_t normalized_length = unorm2_normalize(nfkd, input_u, -1, NULL, 0, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        fprintf(stderr, "Error getting length of normalized UChar: %s\n", u_errorName(status));
        free(input_u);
        return NULL;
    }
    status = U_ZERO_ERROR;

    UChar *normalized_u = calloc(normalized_length + 1, sizeof(UChar));
    if (normalized_u == NULL) {
        fprintf(stderr, "Error allocating memory for normalized UChar\n");
        free(input_u);
        return NULL;
    }

    unorm2_normalize(nfkd, input_u, -1, normalized_u, normalized_length + 1, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "Error normalizing UChar: %s\n", u_errorName(status));
        free(input_u);
        free(normalized_u);
        return NULL;
    }
    free(input_u);

    int8_t *normalized_utf8 = calloc(normalized_length * 4 + 1, sizeof(int8_t));
    if (normalized_utf8 == NULL) {
        fprintf(stderr, "Error allocating memory for normalized UTF-8\n");
        free(normalized_u);
        return NULL;
    }
    u_strToUTF8((char*)normalized_utf8, normalized_length * 4 + 1, NULL, normalized_u, normalized_length, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "Error converting normalized UChar to UTF-8: %s\n", u_errorName(status));
        free(normalized_u);
        free(normalized_utf8);
        return NULL;
    }

free(normalized_u);

return (const char *)normalized_utf8;
}

// passphrase must be at most 256 characters otherwise it would be truncated
void mnemonic_to_seed(const char *mnemonic, const char *passphrase,
                      uint8_t seed[512 / 8],
                      void (*progress_callback)(uint32_t current,
                                                uint32_t total)) {
  mnemonic = nfkd(mnemonic);
  int mnemoniclen = strlen(mnemonic);
  int passphraselen = strnlen(passphrase, 256);
#if USE_BIP39_CACHE
  // check cache
  if (mnemoniclen < 256 && passphraselen < 64) {
    int i;
    for (i = 0; i < BIP39_CACHE_SIZE; i++) {
      if (!bip39_cache[i].set) continue;
      if (strcmp(bip39_cache[i].mnemonic, mnemonic) != 0) continue;
      if (strcmp(bip39_cache[i].passphrase, passphrase) != 0) continue;
      // found the correct entry
      memcpy(seed, bip39_cache[i].seed, 512 / 8);
      return;
    }
  }
#endif
  uint8_t salt[8 + 256] = {0};
  memcpy(salt, "mnemonic", 8);
  memcpy(salt + 8, passphrase, passphraselen);
  static CONFIDENTIAL pbkdf2_hmac_sha512_context pctx;
  pbkdf2_hmac_sha512_init(&pctx, (const uint8_t *)mnemonic, mnemoniclen, salt,
                          passphraselen + 8);
  if (progress_callback) {
    progress_callback(0, BIP39_PBKDF2_ROUNDS);
  }
  int i;
  for (i = 0; i < 16; i++) {
    pbkdf2_hmac_sha512_write(&pctx, BIP39_PBKDF2_ROUNDS / 16);
    if (progress_callback) {
      progress_callback((i + 1) * BIP39_PBKDF2_ROUNDS / 16,
                        BIP39_PBKDF2_ROUNDS);
    }
  }
  pbkdf2_hmac_sha512_finalize(&pctx, seed);
  dogecoin_mem_zero(salt, sizeof(salt));
#if USE_BIP39_CACHE
  // store to cache
  if (mnemoniclen < 256 && passphraselen < 64) {
    bip39_cache[bip39_cache_index].set = true;
    strcpy(bip39_cache[bip39_cache_index].mnemonic, mnemonic);
    strcpy(bip39_cache[bip39_cache_index].passphrase, passphrase);
    memcpy(bip39_cache[bip39_cache_index].seed, seed, 512 / 8);
    bip39_cache_index = (bip39_cache_index + 1) % BIP39_CACHE_SIZE;
  }
#endif
}

// binary search for finding the word in the wordlist
int mnemonic_find_word(const char *word) {
  int lo = 0, hi = BIP39_WORD_COUNT - 1;
  while (lo <= hi) {
    int mid = lo + (hi - lo) / 2;
    int cmp = strcmp(word, wordlist[mid]);
    if (cmp == 0) {
      return mid;
    }
    if (cmp > 0) {
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }
  return -1;
}

const char *mnemonic_complete_word(const char *prefix, int len) {
  // we need to perform linear search,
  // because we want to return the first match
  int i;
  for (i = 0; i < BIP39_WORD_COUNT; i++) {
    if (strncmp(wordlist[i], prefix, len) == 0) {
      return wordlist[i];
    }
  }
  return NULL;
}

const char *mnemonic_get_word(int index) {
  if (index >= 0 && index < BIP39_WORD_COUNT) {
    return wordlist[index];
  } else {
    return NULL;
  }
}

uint32_t mnemonic_word_completion_mask(const char *prefix, int len) {
  if (len <= 0) {
    return 0x3ffffff;  // all letters (bits 1-26 set)
  }
  uint32_t res = 0;
  int i;
  for (i = 0; i < BIP39_WORD_COUNT; i++) {
    const char *word = wordlist[i];
    if (strncmp(word, prefix, len) == 0 && word[len] >= 'a' &&
        word[len] <= 'z') {
      res |= 1 << (word[len] - 'a');
    }
  }
  return res;
}

/**
 * @brief This funciton generates a mnemonic for
 * a given entropy size and language
 *
 * @param entropy_size The 128, 160, 192, 224, or 256 bits of entropy
 * @param language The ISO 639-2 code for the mnemonic language
 *
 * @return mnemonic code words
*/
const char* dogecoin_generate_mnemonic (const char* entropy_size, const char* language)
{
    static char words[] = "";

    if (entropy_size != NULL && language != NULL) {
        /* load word file into memory */
        get_words(language);

        /* convert string value to long */
        long entropyBits = strtol(entropy_size, NULL, 10);

        /* actual program call */
        mnemonic_generate(entropyBits);

    }

    return words;
}
