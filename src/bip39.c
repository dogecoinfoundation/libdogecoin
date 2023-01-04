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

#include <stdio.h>
#include <uninorm.h>
#include <bip39/index.h>
#include <dogecoin/bip39.h>
#include <dogecoin/utils.h>
#include <dogecoin/random.h>
#include <dogecoin/sha2.h>

#ifdef _WIN32
#ifndef WINVER
#define WINVER 0x0600
#endif
#include <windows.h>
#endif

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
    char* entropyBits = dogecoin_char_vla(entropysize + 1);
    entropyBits[0] = '\0';
    dogecoin_mem_zero(entropyBits, entropysize + 1);  // Initialize entropyBits to all zeros

    char binaryByte[9] = "";

    /* OpenSSL */
    /* Gather entropy bytes locally unless optionally provided */
    unsigned char* local_entropy = dogecoin_uchar_vla(entBytes);
    if (entropy == NULL) {
        int rc = (int) dogecoin_random_bytes(local_entropy, entBytes, 0);
        if (rc != 1) {
            fprintf(stderr, "ERROR: Failed to generate random entropy\n");
            return -1;
        }
    }
    else {
        /* Convert optional entropy string to bytes */
        unsigned char* entropy_bytes = utils_hex_to_uint8(entropy);
        if (entropy_bytes == NULL) {
            fprintf(stderr, "ERROR: Failed to convert entropy string to bytes\n");
            dogecoin_free(entropyBits);
            dogecoin_free(local_entropy);
            return -1;
        }
        memcpy_safe(local_entropy, entropy_bytes, entBytes);
    }

    /* Concatenate string of bits from entropy bytes */
    for (int i = 0; i < entBytes; i++) {

        /* Convert valid byte to string of bits */
        sprintf(binaryByte, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(local_entropy[i]));
        binaryByte[8] = '\0';  // null-terminate the binary byte

        /* Concatentate the bits */
        if (strcat(entropyBits, binaryByte) == NULL) {
            fprintf(stderr, "ERROR: Failed to concatenate entropy\n");
            dogecoin_free(entropyBits);
            dogecoin_free(local_entropy);
            return -1;
        }
    }

    /*
     * ENT SHA256 checksum
     */
    static char checksum[SHA256_DIGEST_STRING_LENGTH];
    dogecoin_mem_zero(checksum, sizeof(checksum));
    checksum[0] = '\0';

    /* SHA256 of entropy bytes */
    unsigned char hash[SHA256_DIGEST_LENGTH];
    sha256_raw(local_entropy, entBytes, hash);

    /* done with local_entropy */
    dogecoin_free(local_entropy);

    /* Checksum from SHA256 */
    dogecoin_mem_zero(checksum, sizeof(checksum));  // Initialize checksum to all zeros
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
        dogecoin_free(entropyBits);
        return -1;
    }

    /* done with entropyBits */
    dogecoin_free(entropyBits);

    return 0;
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
        dogecoin_free(salt);
        return -1;
    }

    /* normalize the passphrase and salt */
    size_t norm_pass_len, norm_salt_len;
#ifdef _WIN32
    int pass_len = strlen(pass);
    int salt_len = strlen(salt);

    /* Convert passphrase and salt to wide characters */
    int pass_wlen = MultiByteToWideChar(CP_UTF8, 0, pass, pass_len, NULL, 0);
    int salt_wlen = MultiByteToWideChar(CP_UTF8, 0, salt, salt_len, NULL, 0);

    if (pass_wlen == 0) {
        fprintf(stderr, "Error converting passphrase to wide characters\n");
        dogecoin_free(salt);
        return -1;
    }
    if (salt_wlen == 0) {
        fprintf(stderr, "Error converting salt to wide characters\n");
        dogecoin_free(salt);
        return -1;
    }

    LPWSTR pass_w = malloc((pass_wlen) * sizeof(WCHAR));
    if (pass_w == NULL) {
        fprintf(stderr, "Error allocating memory for passphrase wide characters\n");
        dogecoin_free(salt);
        return -1;
    }
    LPWSTR salt_w = malloc((salt_wlen) * sizeof(WCHAR));
    if (salt_w == NULL) {
        fprintf(stderr, "Error allocating memory for salt wide characters\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        return -1;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, pass, pass_len, pass_w, pass_wlen) == 0) {
        fprintf(stderr, "Error converting passphrase to wide characters\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        return -1;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, salt, salt_len, salt_w, salt_wlen) == 0) {
        fprintf(stderr, "Error converting salt to wide characters\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        return -1;
    }

    norm_pass_len = NormalizeString(NormalizationKD, pass_w, pass_wlen, NULL, 0);
    norm_salt_len = NormalizeString(NormalizationKD, salt_w, salt_wlen, NULL, 0);
    if (norm_pass_len <= 0) {
        LPVOID message;
        DWORD error = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&message, 0, NULL);
        fprintf(stderr, "Error getting length of normalized passphrase: %s\n", message);
        LocalFree(message);
        fprintf(stderr, "Error getting length of normalized passphrase\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        return -1;
    }
    if (norm_salt_len <= 0) {
        fprintf(stderr, "Error getting length of normalized salt\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        return -1;
    }

    LPWSTR norm_pass = malloc((norm_pass_len) * sizeof(WCHAR));
    if (norm_pass == NULL) {
        fprintf(stderr, "Error allocating memory for normalized passphrase\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        return -1;
    }
    LPWSTR norm_salt = malloc((norm_salt_len) * sizeof(WCHAR));
    if (norm_salt == NULL) {
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        dogecoin_free(norm_pass);
        fprintf(stderr, "Error allocating memory for normalized salt\n");
        return -1;
    }

    norm_pass_len = NormalizeString(NormalizationKD, pass_w, pass_wlen, norm_pass, norm_pass_len);
    norm_salt_len = NormalizeString(NormalizationKD, salt_w, salt_wlen, norm_salt, norm_salt_len);

    if (norm_pass_len <= 0) {
        fprintf(stderr, "Error getting normalized passphrase\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        dogecoin_free(norm_pass);
        dogecoin_free(norm_salt);
        return -1;
    }
    if (norm_salt_len <= 0) {
        fprintf(stderr, "Error getting normalized salt\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        dogecoin_free(norm_pass);
        dogecoin_free(norm_salt);
        return -1;
    }

    /* Convert normalized passphrase and salt to multi-byte characters */
    int norm_pass_mb_len = WideCharToMultiByte(CP_UTF8, 0, norm_pass, norm_pass_len, NULL, 0, NULL, NULL);
    int norm_salt_mb_len = WideCharToMultiByte(CP_UTF8, 0, norm_salt, norm_salt_len, NULL, 0, NULL, NULL);

    if (norm_pass_len == 0) {
        fprintf(stderr, "Error converting normalized passphrase to multi-byte characters\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        dogecoin_free(norm_pass);
        dogecoin_free(norm_salt);
        return -1;
    }
    if (norm_salt_len == 0) {
        fprintf(stderr, "Error converting normalized seed to multi-byte characters\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        dogecoin_free(norm_pass);
        dogecoin_free(norm_salt);
        return -1;
    }

    char* norm_pass_mb = malloc(norm_pass_mb_len * sizeof(char));
    if (norm_pass_mb == NULL) {
        fprintf(stderr, "Error allocating memory for normalized passphrase multi-byte characters\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        dogecoin_free(norm_pass);
        dogecoin_free(norm_salt);
        return -1;
    }
    char* norm_salt_mb = malloc(norm_salt_mb_len * sizeof(char));
    if (norm_salt_mb == NULL) {
        fprintf(stderr, "Error allocating memory for normalized salt multi-byte characters\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        dogecoin_free(norm_pass);
        dogecoin_free(norm_salt);
        dogecoin_free(norm_pass_mb);
        return -1;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, norm_pass, norm_pass_len, norm_pass_mb, norm_pass_mb_len, NULL, NULL) == 0) {
        fprintf(stderr, "Error converting normalized passphrase to multi-byte characters\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        dogecoin_free(norm_pass);
        dogecoin_free(norm_salt);
        dogecoin_free(norm_pass_mb);
        dogecoin_free(norm_salt_mb);
        return -1;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, norm_salt, norm_salt_len, norm_salt_mb, norm_salt_mb_len, NULL, NULL) == 0) {
        fprintf(stderr, "Error converting normalized passphrase to multi-byte characters\n");
        dogecoin_free(salt);
        dogecoin_free(pass_w);
        dogecoin_free(salt_w);
        dogecoin_free(norm_pass);
        dogecoin_free(norm_salt);
        dogecoin_free(norm_pass_mb);
        dogecoin_free(norm_salt_mb);
        return -1;
    }

    dogecoin_free(pass_w);
    dogecoin_free(salt_w);

    /* pbkdf2 hmac sha512 */
    pbkdf2_hmac_sha512((const unsigned char*) norm_pass_mb, norm_pass_mb_len, (const unsigned char*) norm_salt_mb, norm_salt_mb_len, LANG_WORD_CNT, digest);

    dogecoin_free(norm_pass_mb);
    dogecoin_free(norm_salt_mb);

#else
    uint8_t *norm_pass;
    uint8_t *norm_salt;

    norm_pass = u8_normalize(UNINORM_NFKD, (const uint8_t *) pass, strlen(pass), NULL, &norm_pass_len);
    if (norm_pass == NULL) {
        fprintf(stderr, "Error normalizing passphrase\n");
        dogecoin_free(salt);
        return -1;
    }
    norm_salt = u8_normalize(UNINORM_NFKD, (const uint8_t *) salt, strlen(salt), NULL, &norm_salt_len);
    if (norm_salt == NULL) {
        fprintf(stderr, "Error normalizing salt\n");
        dogecoin_free(salt);
        dogecoin_free(norm_pass);
        return -1;
    }

    /* pbkdf2 hmac sha512 */
    pbkdf2_hmac_sha512((const unsigned char*) norm_pass, norm_pass_len, (const unsigned char*) norm_salt, norm_salt_len, LANG_WORD_CNT, digest);
#endif

    /* we're done with salt */
    dogecoin_free(salt);
    dogecoin_free(norm_pass);
    dogecoin_free(norm_salt);

    /* copy the digest into seed*/
    memcpy_safe(seed, digest, SHA512_DIGEST_LENGTH);

    return 0;
}

/*
 * This function reads the language file once and loads an array of words for
 * repeated use.
 */

int get_custom_words(const char *filepath, char* wordlist[]) {
    int i = 0;
    FILE * fp;
    char word[1024];

    fp = fopen(filepath, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: file read error\n");
        return -1;
    }

    while (fscanf(fp, "%s", word) == 1) {
        if (i >= LANG_WORD_CNT) {
            fprintf(stderr, "ERROR: too many words in file\n");
            return -1;
        }
        wordlist[i] = malloc(strlen(word) + 1);
        if (wordlist[i] == NULL) {
            fprintf(stderr, "ERROR: cannot allocate memory\n");
            return -1;
        }
        strcpy(wordlist[i], word);
        i++;
    }

    fclose(fp);

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
    char *segment = dogecoin_string_vla (segSize);
    strcpy(segment, "");

    char *csBits = dogecoin_string_vla (checksumBits);
    strcpy(csBits, "");

    /* Convert the checksum string to a byte */
    unsigned char *bytes = utils_hex_to_uint8(firstByte);
    if (bytes == NULL) {
        /* Invalid byte, return from the function */
        fprintf(stderr, "ERROR: Failed to convert first byte\n");
        dogecoin_free (segment);
        dogecoin_free (csBits);
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
            dogecoin_free (segment);
            dogecoin_free (csBits);
            break;
    }
    csBits[checksumBits - 1] = '\0';   // null-terminate the checksum string

    /* Concatenate the entropy and checksum bits onto the segment array,
     * ensuring that the segment array does not overflow.
     */

    strncat(segment, entropy, segSize - strlen(segment) - 1);
    strncat(segment, csBits, segSize - strlen(segment) - 1);

    dogecoin_free (csBits);

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
                dogecoin_free (segment);
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

    dogecoin_free (segment);

    /* Remove the trailing space from the mnemonic sentence */
    mnemonic[strlen(mnemonic) - strlen(space)] = '\0';

    /* Update the mnemonic_len output parameter to reflect the length of the generated mnemonic */
    *mnemonic_len = strlen(mnemonic);

    return 0;
}

/**
 * @brief This function generates a mnemonic for a given entropy size and language
 *
 * @param entropy_size The 128, 160, 192, 224, or 256 bits of entropy
 * @param language The ISO 639-2 code for the mnemonic language
 * @param space The character to seperate mnemonic words
 * @param entropy The entropy to generate the mnemonic (optional)
 * @param filepath The path to a custom word file (optional)
 * @param wordlist The language word list as an array
 * @param length The length of the generated mnemonic in bytes
 *
 * @return mnemonic code words
*/
int dogecoin_generate_mnemonic (const char* entropy_size, const char* language, const char* space, const char* entropy, const char* filepath, size_t* length, char* words)
{
    char *wordlist[LANG_WORD_CNT] = {0};

    /* validate input, optional entropy checked below */
    if (entropy_size != NULL) {

        /* load words into memory */
        if (language != NULL){
            get_words(language, wordlist);
        }
        /* load custom word file into memory */
	else if (filepath != NULL) {
            get_custom_words (filepath, (char **) wordlist);
        }
        /* handle input validation errors */
        else {
            fprintf(stderr, "ERROR: Failed to get language or custom words file\n");
            return -1;
        }

        /* Validate optional entropy */
        if (entropy != NULL) {
            /* Calculate expected entropy size in bits */
            size_t expected_entropy_size = strtol(entropy_size, NULL, 10) / 4;
            /* Verify size of the string equals the entropy_size specified */
            if (strlen(entropy) != expected_entropy_size) {
                fprintf(stderr, "ERROR: Length of optional entropy does not equal entropy size\n");
                return -1;
            }
        }

        /* convert string value for entropy size to base 10 and get mnemonic */
        if (get_mnemonic(strtol(entropy_size, NULL, 10), entropy, (const char **) wordlist, space, words, length) == -1) {
            fprintf(stderr, "ERROR: Failed to get mnemonic\n");

            /* Free memory for custom words */
            if (language == NULL) {
                for (int i = 0; i < LANG_WORD_CNT; i++) {
                    dogecoin_free(wordlist[i]);
                }
            }
            return -1;
        }

        /* Free memory for custom words */
        if (language == NULL) {
            for (int i = 0; i < LANG_WORD_CNT; i++) {
                dogecoin_free(wordlist[i]);
            }
        }
    }
    else {
        fprintf(stderr, "ERROR: Failed to get entropy size\n");
        return -1;
    }

    return 0;
}

/**
 * @brief This function derives the seed from the mnemonic
 * @param mnemonic The mnemonic code words
 * @param passphrase The passphrase (optional)
 * @param seed The 512-bit seed
 *
 * @return 0 (success), -1 (fail)
*/
int dogecoin_seed_from_mnemonic (const char* mnemonic, const char* passphrase, uint8_t seed[64])
{
    /* get seed if not null */
    if (seed != NULL) {

        /* set passphrase to empty string if null */
        if (passphrase == NULL) {
            passphrase = "";
        }

        /* get random binary seed */
        if (get_root_seed(mnemonic, passphrase, seed) == -1) {
            fprintf(stderr, "ERROR: Failed to get root seed\n");
            return -1;
        }

    }

    return 0;
}
