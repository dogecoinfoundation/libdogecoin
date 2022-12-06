#include <stdlib.h>

#include <dogecoin/bip39.h>

#include "bip39c.h"

/**
 * @brief This funciton generates a mnemonic for
 * a given entropy size and language
 *
 * @param entropy_size The 128, 160, 192, 224, or 256 bits of entropy
 * @param language The ISO 639-2 code for the mnemonic language
 *
 * @return mnemonic code words
*/
const char* dogecoin_generate_mnemonic (const char* entropy_size, const char* language, size_t* length)
{
    static char words[] = "";

    if (entropy_size != NULL && language != NULL) {
        /* load word file into memory */
        get_words(language);

        /* convert string value to long */
        long entropyBits = strtol(entropy_size, NULL, 10);

        /* actual program call */
        get_mnemonic(entropyBits, words, length);

    }

    return words;
}

/**
 * @brief This function derives the seed from the mnemonic
 * @param mnemonic The mnemonic code words
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
