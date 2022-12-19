/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David L. Whitehurst
 * Copyright (c) 2022 edtubbs
 * Copyright (c) 2022 bluezr
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
 * bip39c.c
 * A BIP-39 Implementation using C.
 *
 * Generation of varying length mnemonic words to be used
 * to create a root seed for the creation of a Hierarchical
 * Deterministic (HD) wallet (BIP-32).
 *
 * author: David L. Whitehurst
 * date: May 30, 2018
 *
 * Algorithm:
 *
 *  First Part
 *      1. Create a random sequence (entropy) of 128 to 256 bits.
 *      2. Create a checksum of the random sequence by taking the
 *          first (entropy-length/32) bits of its SHA256 hash.
 *      3. Add the checksum to the end of the random sequence.
 *      4. Split the result into 11-bit length segments.
 *      5. Map each 11-bit value to a word from the predefined
 *          dictionary of 2048 words.
 *      6. The mnemonic code is the sequence of words.
 *
 *  Second Part
 *      7. Use the mnemonic as the first parameter for the
 *          key-stretching PBKDF2 algorithm.
 *      8. The second parameter is a "salt" that's a string
 *          constant "mnemonic plus an optional user-supplied
 *          passphrase of any length.
 *      9. PBKDF2 stretches the mnemonic and salt parameters using
 *          OpenSSL and 2048 rounds of HMAC-SHA512 to produce a
 *          512-bit root seed or digest in hex-form.

 * Find this code useful? Please donate:
 *  Bitcoin: 1Mxt427mTF3XGf8BiJ8HjkhbiSVvJbkDFY
 *
 */

#include "bip39c.h"
#include "conversion.h"
#include "crypto.h"

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
#include <dogecoin/random.h>
#include <dogecoin/sha2.h>
#include <dogecoin/utils.h>

/*
 * This function implements the first part of the BIP-39 algorithm.
 * The randomness or entropy for the mnemonic must be a multiple of
 * 32 bits hence the use of 128,160,192,224,256.
 *
 * The CS values below represent a portion (in bits) of the first
 * byte of the checksum or SHA256 digest of the entropy that the user
 * chooses by program option. These checksum bits are added to the
 * entropy prior to splitting the entire random series (ENT+CS) of bits
 * into 11 bit words to be matched with the 2048 count language word
 * file chosen. The final output or mnemonic sentence consists of (MS) words.
 *
 * CS = ENT / 32
 * MS = (ENT + CS) / 11
 *
 * |  ENT  | CS | ENT+CS |  MS  |
 * +-------+----+--------+------+
 * |  128  |  4 |   132  |  12  |
 * |  160  |  5 |   165  |  15  |
 * |  192  |  6 |   198  |  18  |
 * |  224  |  7 |   231  |  21  |
 * |  256  |  8 |   264  |  24  |
 */

int get_mnemonic(const int entropysize, const char* entropy, const char* wordlist[], const char* space, char *mnemonic, size_t* mnemonic_len) {

    /* Check entropy size per BIP-39 */
    if (!(entropysize >= 128 && entropysize <= 256 && entropysize % 32 == 0)) {
        fprintf(stderr,
                "ERROR: Only the following values for entropy bit sizes may be used: 128, 160, 192, 224, and 256\n");
        return -1;
    }


    int entBytes = entropysize / 8; // bytes instead of bits
    int csAdd = entropysize / 32; // portion in bits of a single byte

    /*
     * ENT (Entropy)
     */

    char entropyBits[entropysize + 1];
    entropyBits[0] = '\0';
    memset(entropyBits, 0, sizeof(entropyBits));  // Initialize entropyBits to all zeros

    char binaryByte[9] = "";

    /* OpenSSL */
    /* Gather entropy bytes locally unless optionally provided */
    unsigned char local_entropy[entBytes];
    if (entropy == NULL) {
        int rc = (int) dogecoin_random_bytes(local_entropy, sizeof(local_entropy), 0);
        if (rc != 1) {
            fprintf(stderr, "ERROR: Failed to generate random entropy\n");
            return -1;
        }
    }
    else {
        /* Convert optional entropy string to bytes */
        unsigned char* entropy_bytes = hexstr_to_char(entropy);
        if (entropy_bytes == NULL) {
            fprintf(stderr, "ERROR: Failed to convert entropy string to bytes\n");
            return -1;
        }
        memcpy(local_entropy, entropy_bytes, sizeof(local_entropy));
        free(entropy_bytes);
    }

    /* Concatenate string of bits from entropy bytes */
    for (size_t i = 0; i < sizeof(local_entropy); i++) {

        /* Convert valid byte to string of bits */
        sprintf(binaryByte, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(local_entropy[i]));
        binaryByte[8] = '\0';  // null-terminate the binary byte

        /* Concatentate the bits */
        if (strcat(entropyBits, binaryByte) == NULL) {
            fprintf(stderr, "ERROR: Failed to concatenate entropy\n");
            return -1;
        }
    }

    /*
     * ENT SHA256 checksum
     */
    static char checksum[SHA256_DIGEST_STRING_LENGTH];
    memset(checksum, 0, sizeof(checksum));
    checksum[0] = '\0';

    /* SHA256 of entropy bytes */
    unsigned char hash[SHA256_DIGEST_LENGTH];
    sha256_raw(local_entropy, sizeof(local_entropy), hash);

    /* Checksum from SHA256 */
    memset(checksum, 0, sizeof(checksum));  // Initialize checksum to all zeros
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(&checksum[i * 2], "%02x", hash[i]);
    }
    checksum[SHA256_DIGEST_STRING_LENGTH - 1] = '\0';  // null-terminate the checksum string

    /* Copy the checksum */
    char hexStr[3];
    memset(hexStr, 0, sizeof(hexStr));
    strncpy(hexStr, &checksum[0], 2);
    hexStr[2] = '\0';

    /*
     * CS (Checksum portion) to add to entropy
     */

    int ret = produce_mnemonic_sentence(csAdd * 33 + 1, csAdd + 1, hexStr, entropyBits, wordlist, space, mnemonic, mnemonic_len);
    if (ret != 0) {
        fprintf(stderr, "ERROR: Failed to generate mnemonic sentence\n");
        return -1;
    }

    return 0;
}


/*
 * This function converts the input string to a Unicode format,
 * normalizes it using the NFKD normalization form, and converts
 * it back to a UTF-8 encoded string.
 */
char *nfkd(const char *input) {
    /* Initialize error code */
    UErrorCode status = U_ZERO_ERROR;

    /* Get an instance of the NFKD normalizer */
    const UNormalizer2 *nfkd = unorm2_getNFKDInstance(&status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ERROR: Failed getting NFKD instance: %s\n", u_errorName(status));
        u_cleanup();
        return NULL;
    }

    /* Allocate memory for the input in Unicode format */
    UChar *input_u = calloc(strlen(input) + 1, sizeof(UChar));
    if (input_u == NULL) {
        fprintf(stderr, "ERROR: Failed allocating memory for input UChar\n");
        u_cleanup();
        return NULL;
    }

    /* Convert the input to Unicode format */
    u_strFromUTF8(input_u, strlen(input) + 1, NULL, input, strlen(input), &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ERROR: Failed converting input to UChar: %s\n", u_errorName(status));
        free(input_u);
        u_cleanup();
        return NULL;
    }

    /* Get the length of the normalized string */
    int32_t normalized_length = unorm2_normalize(nfkd, input_u, -1, NULL, 0, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        fprintf(stderr, "ERROR: Failed getting length of normalized UChar: %s\n", u_errorName(status));
        free(input_u);
        u_cleanup();
        return NULL;
    }
    /* Reset the status flag */
    status = U_ZERO_ERROR;

    /* Allocate memory for the normalized string */
    UChar *normalized_u = calloc(normalized_length + 1, sizeof(UChar));
    if (normalized_u == NULL) {
        fprintf(stderr, "ERROR: Failed allocating memory for normalized UChar\n");
        free(input_u);
        u_cleanup();
        return NULL;
    }

    /* Perform the normalization of the input string */
    unorm2_normalize(nfkd, input_u, -1, normalized_u, normalized_length + 1, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ERROR: Failed normalizing UChar: %s\n", u_errorName(status));
        free(input_u);
        free(normalized_u);
        u_cleanup();
        return NULL;
    }
    free(input_u);

   /* Allocate memory for the normalized string in UTF-8 format */
    char *normalized_utf8 = calloc(normalized_length * 4 + 1, sizeof(int8_t));
    if (normalized_utf8 == NULL) {
        fprintf(stderr, "ERROR: Failed allocating memory for normalized UTF-8\n");
        free(normalized_u);
        u_cleanup();
       return NULL;
    }

    /* Convert the normalized UChar to UTF-8 and return it. */
    u_strToUTF8(normalized_utf8, normalized_length * 4 + 1, NULL, normalized_u, normalized_length, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "ERROR: Failed converting normalized UChar to UTF-8: %s\n", u_errorName(status));
        free(normalized_u);
        free(normalized_utf8);
        u_cleanup();
        return NULL;
    }

    free(normalized_u);
    u_cleanup();

    return normalized_utf8;
}


/*
 * This function implements the second part of the BIP-39 algorithm.
 */

int get_root_seed(const char *pass, const char *passphrase, uint8_t seed[64]) {

        /* initialize variables */
        unsigned char digest[64];

        /* create salt, passphrase could be empty string */
        char *salt = malloc(strlen(passphrase) + 9);
        if (salt == NULL) {
            fprintf(stderr, "ERROR: Failed to allocate memory for salt\n");
            return -1;
        }
        *salt = '\0';

        if (strcat(salt, "mnemonic") == NULL || strcat(salt, passphrase) == NULL) {
            fprintf(stderr, "ERROR: Failed to concatenate salt\n");
            return -1;
        }

        /* normalize the passphrase and salt */
        char *norm_pass =  nfkd(pass);
        char *norm_salt =  nfkd(salt);

        /* pbkdf2 hmac sha512 */
        pbkdf2_hmac_sha512((const unsigned char*) norm_pass, strlen(norm_pass), (const unsigned char*) norm_salt, strlen(norm_salt), LANG_WORD_CNT, digest);

        /* we're done with salt */
        free(salt);
        free(norm_pass);
        free(norm_salt);

        /* copy the digest into seed*/
        memcpy(seed, digest, SHA512_DIGEST_LENGTH);

        return 0;
}

/*
 * This function reads the language file once and loads an array of words for
 * repeated use.
 */

int get_custom_words(const char *filepath, char* wordlist[]) {
    int i = 0;
    FILE * fp;
    char * line = NULL;
    size_t line_len = 0;
    ssize_t read;

    fp = fopen(filepath, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: file read error\n");
        return -1;
    }

    while ((read = getline(&line, &line_len, fp)) != -1) {
        strtok(line, "\n");
        if (i >= LANG_WORD_CNT) {
            fprintf(stderr, "ERROR: too many words in file\n");
            return -1;
        }
        wordlist[i] = malloc(read + 1);
        if (wordlist[i] == NULL) {
            fprintf(stderr, "ERROR: cannot allocate memory\n");
            return -1;
        }
        strncpy(wordlist[i], line, read);
        wordlist[i][read] = '\0';
        i++;
    }

    fclose(fp);
    if (line) free(line);

    if (i != LANG_WORD_CNT) {
        fprintf(stderr, "ERROR: not 2048 words\n");
        return -1;
    }

    return 0;
}

/*
 * This function reads a wordlist and loads an array of words for
 * repeated use.
 */

void get_words(const char *lang, char* wordlist[]) {
    int i = 0;
    for (; i < 2048; i++) {
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
          fprintf(stderr, "ERROR: Language or language file does not exist.\n");
          exit(EXIT_FAILURE);
      }
    }
}

/*
 * This function prints the mnemonic sentence of size based on the segment
 * size and number of checksum bits appended to the entropy bits.
 */

int produce_mnemonic_sentence(const int segSize, const int checksumBits, const char *firstByte, const char* entropy, const char* wordlist[], const char* space, char *mnemonic, size_t *mnemonic_len) {

    /* Check if the input arguments are valid */
    if (segSize <= 0 || checksumBits <= 0 || !firstByte || !entropy || !wordlist || !mnemonic || !mnemonic_len) {
        fprintf(stderr, "ERROR: invalid input arguments\n");
        return -1;
    }

    /* Check that wordlist is valid */
    if (wordlist == NULL || *wordlist == NULL) {
        fprintf(stderr, "ERROR: invalid value of wordlist\n");
        return -1;
    }

    /* Define and initialize segment and csBits */
    char segment[segSize];
    strcpy(segment, "");

    char csBits[checksumBits];
    strcpy(csBits, "");

    /* Convert the checksum string to a byte */
    unsigned char *bytes = hexstr_to_char(firstByte);
    if (bytes == NULL) {
        /* Invalid byte, return from the function */
        fprintf(stderr, "ERROR: Failed to convert first byte\n");
        return -1;
    }

    /* Convert the byte to bits */
    switch(checksumBits) {
        case 5:
            sprintf(csBits, BYTE_TO_FIRST_FOUR_BINARY_PATTERN, BYTE_TO_FIRST_FOUR_BINARY(bytes[0]));
            break;
        case 6:
            sprintf(csBits, BYTE_TO_FIRST_FIVE_BINARY_PATTERN, BYTE_TO_FIRST_FIVE_BINARY(bytes[0]));
            break;
        case 7:
            sprintf(csBits, BYTE_TO_FIRST_SIX_BINARY_PATTERN, BYTE_TO_FIRST_SIX_BINARY(bytes[0]));
            break;
        case 8:
            sprintf(csBits, BYTE_TO_FIRST_SEVEN_BINARY_PATTERN, BYTE_TO_FIRST_SEVEN_BINARY(bytes[0]));
            break;
        case 9:
            sprintf(csBits, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(bytes[0]));
            break;
        default:
            return -1;
            free (bytes);
            break;
    }
    free (bytes);
    csBits[checksumBits - 1] = '\0';   // null-terminate the checksum string

    /* Concatenate the entropy and checksum bits onto the segment array,
     * ensuring that the segment array does not overflow.
     */

    strncat(segment, entropy, segSize - strlen(segment) - 1);
    strncat(segment, csBits, segSize - strlen(segment) - 1);

    char elevenBits[12] = {""};

    int elevenBitIndex = 0;

    for (int i = 0; i < segSize; i++) {

        if (elevenBitIndex == 11) {
            elevenBits[11] = '\0';
            /* Compute the decimal value of the 11-bit binary chunk */
            long real = 0;
            for (int j = 0; j < 11; j++) {
                real = (real << 1) | (elevenBits[j] - '0');
            }

            /* Check that real is a valid index into the wordlist array */
            if (real < 0 || real >= LANG_WORD_CNT) {
                fprintf(stderr, "ERROR: invalid 11-bit binary chunk\n");
                return -1;
            }

            /* Concatenate the word from the wordlist to the mnemonic */
            const char *word = wordlist[real];
            strcat(mnemonic, word);

            /* Concatenate a space to the mnemonic */
            strcat(mnemonic, space);

            elevenBitIndex = 0;
        }

        elevenBits[elevenBitIndex] = segment[i];
        elevenBitIndex++;
    }

    /* Remove the trailing space from the mnemonic sentence */
    mnemonic[strlen(mnemonic) - strlen(space)] = '\0';

    /* Update the mnemonic_len output parameter to reflect the length of the generated mnemonic */
    *mnemonic_len = strlen(mnemonic);

    return 0;
}

