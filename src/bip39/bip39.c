/**
 * Copyright (c) 2022 edtubbs
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

#include <stdlib.h>

#include <dogecoin/bip39.h>

#include "bip39c.h"

/**
 * @brief This funciton generates a mnemonic for
 * a given entropy size and language
 *
 * @param entropy_size The 128, 160, 192, 224, or 256 bits of entropy
 * @param language The ISO 639-2 code for the mnemonic language
 * @param filepath The path to a custom word file (optional)
 * @param wordlist The language word list as an array
 * @param length The length of the generated mnemonic in bytes
 *
 * @return mnemonic code words
*/
const char* dogecoin_generate_mnemonic (const char* entropy_size, const char* language, const char* filepath, size_t* length)
{
    char *wordlist[LANG_WORD_CNT] = {0};
    static char words[] = "";
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
            fprintf(stderr, "ERROR: Failed to get language or language file\n");
            return NULL;
        }

        /* convert string value and get mnemonic */
        if (get_mnemonic(strtol(entropy_size, NULL, 10), (const char **) wordlist, words, length) == -1) {
            fprintf(stderr, "ERROR: Failed to get mnemonic\n");
            return NULL;
        }

        /* Free memory for custom words */
        if (language == NULL) {
            for (int i = 0; i < LANG_WORD_CNT; i++) {
                free(wordlist[i]);
            }
        }

    }
    else {
        fprintf(stderr, "ERROR: Failed to get entropy size\n");
        return NULL;
    }

    return words;
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
    if (seed != NULL) {

        /* set passphrase to empty string if null */
        if (passphrase == NULL) {
            passphrase = "";
        }

        /* get truly random binary seed */
        if (get_root_seed(mnemonic, passphrase, seed) == -1) {
            fprintf(stderr, "ERROR: Failed to get root seed\n");
            return -1;
        }

    }

    return 0;
}
