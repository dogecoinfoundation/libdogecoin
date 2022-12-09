/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David L. Whitehurst
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
#include "print_util.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include <unicode/unorm2.h>

#include <wchar.h>
#include <wctype.h>

#include <bip39/index.h>
/*
 * Global variables
 */

char *words[LANG_WORD_CNT];

/* program usage statement */
static char const usage[] = "\
Usage: " PACKAGE_NAME " [-e] <bit value> [-l] <language code>\n\
              [-k] \"mnemonic\" [-p <passphrase>]\n\
 Options:\n\
  -e    specify the entropy to use\n\
            128\n\
            160\n\
            192\n\
            224\n\
            256\n\
  -l    specify the language code for the mnemonic\n\
            eng   English\n\
            spa   Spanish\n\
            fra   French\n\
            ita   Italian\n\
            kor   Korean\n\
            jpn   Japanese\n\
            tc    Traditional Chinese\n\
            sc    Simplified Chinese\n\
";

/*
 * The main function uses the GNU-added getopt function to std=c99 to provide options
 * to 1) create a varying length mnemonic and 2) a root seed or key for the creation
 * of an HD-wallet per BIP-39.
 */
/*
int main(int argc, char **argv)
{
    char *evalue, *kvalue, *lvalue, *pvalue = NULL;

    int c;

    if (argc == 1) {
        fprintf(stderr, usage);
        exit(EXIT_FAILURE);
    }

    while ((c = getopt(argc, argv, "e: l: k: p:")) != -1) {

        switch (c) {

            case 'e': // entropy set
                evalue = optarg;
                break;

            case 'l': // language
                lvalue = optarg;
                break;

            case 'k': // root seed key derived from mnemonic
                evalue = NULL;
                lvalue = NULL;
                kvalue = optarg;
                break;

            case 'p': // optional passphrase
                pvalue = optarg;
                break;

            case '?':
                if (optopt == 'e' || optopt == 'l' || optopt == 'k' || optopt == 'p')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                return 1;

            default:
                exit(EXIT_FAILURE);
        } // end switch


    } // end while

    if (evalue != NULL && lvalue != NULL) {
//        / * load word file into memory * /
        get_words(lvalue);

//        / * convert string value to long * /
        long entropyBits = strtol(evalue, NULL, 10);

//        / * actual program call * /
        get_mnemonic(entropyBits);

    } else if (kvalue != NULL) {

//        / * set passphrase to empty string if null * /
        if (pvalue == NULL) {
            pvalue = "";
        }

//        / * get truly random binary seed * /
        get_root_seed(kvalue, pvalue);

    } else {
            fprintf(stderr, "Both entropy (-e) and language (-l) options are required.\n");
            exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
*/
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

int get_mnemonic(int entropysize, char *mnemonic, size_t* mnemonic_len) {

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

    unsigned char entropy[entBytes];
    char entropyBits[entropysize + 1];
    entropyBits[0] = '\0';

    char binaryByte[9];

    /* OpenSSL */
    int rc = RAND_bytes(entropy, sizeof(entropy));
    if (rc != 1) {
        fprintf(stderr, "ERROR: Failed to generate random entropy\n");
        return -1;
    }

    for (size_t i = 0; i < sizeof(entropy); i++) {
        char buffer[3];
        memcpy(buffer, &entropy[i], 2);
        buffer[2] = '\0';

        unsigned char *byte = hexstr_to_char(buffer);
        if (byte == NULL) {
          fprintf(stderr, "ERROR: Failed to convert hexadecimal string to character\n");
            return -1;
        }

        sprintf(binaryByte, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(*byte));
        binaryByte[8] = '\0';
        if (strcat(entropyBits, binaryByte) == NULL) {
          fprintf(stderr, "ERROR: Failed to concatenate entropy\n");
        return -1;
        }

    }

    /*
     * ENT SHA256 checksum
     */

    static char checksum[65];
    char entropyStr[sizeof(entropy) * 2 + 1];

    /* me and OpenSSL */
    sha256(entropyStr, checksum);

    char hexStr[3];
    memcpy(hexStr, &checksum[0], 2);
    hexStr[2] = '\0';

    /*
     * CS (Checksum portion) to add to entropy
     */

    int ret = produce_mnemonic_sentence(csAdd * 33 + 1, csAdd + 1, hexStr, entropyBits, mnemonic, mnemonic_len);
    if (ret != 0) {
        fprintf(stderr, "ERROR: Failed to generate mnemonic sentence\n");
        return -1;
    }

    return 0;
}

char *nfkd(const char *input) {
    UErrorCode status = U_ZERO_ERROR;
    UNormalizer2 *nfkd = unorm2_getNFKDInstance(&status);
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

    char *normalized_utf8 = calloc(normalized_length * 4 + 1, sizeof(int8_t));
    if (normalized_utf8 == NULL) {
        fprintf(stderr, "Error allocating memory for normalized UTF-8\n");
        free(normalized_u);
        return NULL;
    }

    u_strToUTF8(normalized_utf8, normalized_length * 4 + 1, NULL, normalized_u, normalized_length, &status);
    if (U_FAILURE(status)) {
        fprintf(stderr, "Error converting normalized UChar to UTF-8: %s\n", u_errorName(status));
        free(normalized_u);
        free(normalized_utf8);
        return NULL;
    }

    free(normalized_u);

    return normalized_utf8;
}


/*
 * This function implements the second part of the BIP-39 algorithm.
 */

int get_root_seed(const char *pass, const char *passphrase, uint8_t seed[64]) {

        /* initialize variables */
        char HexResult[128];
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

        char *normalized = nfkd(pass);
        char *norm_salt = nfkd(salt);

        /* openssl function */
        int ret = PKCS5_PBKDF2_HMAC((const char*) normalized, strlen(normalized), (const unsigned char*) norm_salt, strlen((const char *) norm_salt), 2048, EVP_sha512(), 64, digest);
        if (ret == 0) {
            fprintf(stderr, "ERROR: PKCS5_PBKDF2_HMAC failed\n");
            free(salt);
            free(normalized);
            free(norm_salt);
            return -1;
        }

        /* we're done with salt */
        free(salt);
        free(normalized);
        free(norm_salt);

        for (size_t i = 0; i < sizeof(digest); i++)
            sprintf(HexResult + (i * 2), "%02x", (unsigned int) digest[i]);

        printf("%s\n", HexResult);

        /* copy the digest into seed*/
        memcpy(seed, digest, 64);

        return 0;
}


/*
 * This function reads the language file once and loads an array of words for
 * repeated use.
 */

void get_custom_words(const char *filepath) {

    char *source = NULL;

    FILE *fp = fopen(filepath, "r");

    if (fp != NULL) {

        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {

            /* Get the size of the file. */
            long bufsize = ftell(fp);

            if (bufsize == -1) {
                fprintf(stderr,
                        "ERROR: File size?\n");
            }

            /* Allocate our buffer to that size. */
            source = malloc(sizeof(char) * (bufsize + 1));

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) {
                fprintf(stderr,
                        "ERROR: File seek beginning of file.\n");
            }

            /* Read the entire file into memory. */
            size_t newLen;
            newLen = fread(source, sizeof(char), (size_t) bufsize, fp);
            if ( ferror( fp ) != 0 ) {
                fprintf(stderr,
                        "ERROR: File read.\n");
            } else {
                source[newLen++] = '\0'; /* Just to be safe. */
            }
        }
        fclose(fp);
    } else {
        fprintf(stderr, "Custom words file does not exist.\n");
        exit(EXIT_FAILURE);
    }

    char * word;
    word = strtok (source,"\n");
    int i = 0;
    while (word != NULL)
    {
        words[i] = malloc(strlen(word) + 1 );
        strcpy(words[i], word);
        i++;
        word = strtok (NULL, "\n");
    }

    free(source);
}

void get_words(const char *lang) {
    int i = 0;
    for (; i < 2048; i++) {
      if (strcmp(lang,"spa") == 0) {
          words[i]=(char*)wordlist_spa[i];
      } else if (strcmp(lang,"eng") == 0) {
          words[i]=(char*)wordlist_eng[i];
      } else if (strcmp(lang,"jpn") == 0) {
          words[i]=(char*)wordlist_jpn[i];
      } else if (strcmp(lang,"ita") == 0) {
          words[i]=(char*)wordlist_ita[i];
      } else if (strcmp(lang,"fra") == 0) {
          words[i]=(char*)wordlist_fra[i];
      } else if (strcmp(lang,"kor") == 0) {
          words[i]=(char*)wordlist_kor[i];
      } else if (strcmp(lang,"sc") == 0) {
          words[i]=(char*)wordlist_sc[i];
      } else if (strcmp(lang,"tc") == 0) {
          words[i]=(char*)wordlist_tc[i];
      } else if (strcmp(lang,"cze") == 0) {
          words[i]=(char*)wordlist_cze[i];
      } else if (strcmp(lang,"por") == 0) {
          words[i]=(char*)wordlist_por[i];
      } else {
          fprintf(stderr, "Language or language file does not exist.\n");
          exit(EXIT_FAILURE);
      }
    }
}

/*
 * This function prints the mnemonic sentence of size based on the segment
 * size and number of checksum bits appended to the entropy bits.
 */

int produce_mnemonic_sentence(int segSize, int checksumBits, char *firstByte, char entropy[], char *mnemonic, size_t *mnemonic_len) {

    if (segSize <= 0 || checksumBits <= 0) {
        fprintf(stderr, "Error: invalid input arguments\n");
        return -1;
    }

    char segment[segSize];
    memset(segment, 0, segSize * sizeof(char));

    char csBits[checksumBits];
    memset(csBits, 0, checksumBits * sizeof(char));

    unsigned char *bytes;
    bytes = hexstr_to_char(firstByte);
    if (bytes == NULL) {
        fprintf(stderr, "ERROR: Failed to convert hexadecimal string to character\n");
        return -1;
    }

    switch(checksumBits) {
        case 5:
            sprintf(csBits, BYTE_TO_FIRST_FOUR_BINARY_PATTERN, BYTE_TO_FIRST_FOUR_BINARY(*bytes));
            break;
        case 6:
            sprintf(csBits, BYTE_TO_FIRST_FIVE_BINARY_PATTERN, BYTE_TO_FIRST_FIVE_BINARY(*bytes));
            break;
        case 7:
            sprintf(csBits, BYTE_TO_FIRST_SIX_BINARY_PATTERN, BYTE_TO_FIRST_SIX_BINARY(*bytes));
            break;
        case 8:
            sprintf(csBits, BYTE_TO_FIRST_SEVEN_BINARY_PATTERN, BYTE_TO_FIRST_SEVEN_BINARY(*bytes));
            break;
        case 9:
            sprintf(csBits, BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(*bytes));
            break;
        default:
            return -1;
            break;
    }

    csBits[checksumBits - 1] = '\0';

    /* Concatenate the entropy and checksum bits onto the segment array,
     * ensuring that the segment array does not overflow.
     */
    strncat(segment, entropy, segSize - strlen(segment) - 1);
    strncat(segment, csBits, segSize - strlen(segment) - 1);
    segment[segSize - 1] = '\0';

    char elevenBits[12] = {""};

    int elevenBitIndex = 0;

    for (int i = 0; i < segSize; i++) {

        if (elevenBitIndex == 11) {
            elevenBits[11] = '\0';
            /* Convert the 11-bit binary chunk to a decimal value */
            long real = strtol(elevenBits, NULL, 2);

            if (strcat(mnemonic, words[real]) == NULL) {
              fprintf(stderr, "ERROR: Failed to concatenate entropy\n");
            return -1;
            }

            if (strcat(mnemonic, " ") == NULL) {
              fprintf(stderr, "ERROR: Failed to concatenate entropy\n");
            return -1;
            }

            printf("%s", words[real]);
            printf(" ");
            elevenBitIndex = 0;
        }

        elevenBits[elevenBitIndex] = segment[i];
        elevenBitIndex++;
    }

    printf("\n");

    /* Remove the trailing space from the mnemonic sentence */
    mnemonic[strlen(mnemonic) - 1] = '\0';

    /* Update the mnemonic_len output parameter to reflect the length of the generated mnemonic */
    *mnemonic_len = strlen(mnemonic);

    return 0;
}
