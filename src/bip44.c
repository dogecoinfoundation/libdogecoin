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

#include <dogecoin/bip44.h>
#include <dogecoin/bip32.h>
#include <dogecoin/utils.h>

#define UINT32_MAX_DIGITS 10  /* The maximum number of digits needed to represent a 32-bit unsigned integer in decimal format */

/**
 * @brief This function derives the BIP 44 extended private key from a master private key.
 *
 * BIP 44 is a standard for deterministic wallet generation, which allows for the generation of
 * a hierarchy of keys starting from a single seed. The extended private key is a specific key
 * in this hierarchy that is derived using BIP 44 constants and the input parameters.
 *
 * @param master_key The master private key to derive from.
 * @param account The account number to use in the BIP 44 key derivation.
 * @param address_index The address index to use in the BIP 44 key derivation.
 * @param change_level The change level to use in the BIP 44 key derivation, either "external" or "internal".
 * @param hard_addr A flag indicating whether or not to use a hardened address in the BIP 44 key derivation.
 * @param keypath A string buffer to store the BIP 44 keypath used for the derivation.
 * @param bip44_key The BIP 44 extended private key to be generated.
 * @return 0 if the key is derived successfully, -1 otherwise.
 */
int derive_bip44_extended_private_key(const dogecoin_hdnode *master_key, const uint32_t account, const uint32_t* address_index, const char* change_level, const int hard_addr, char* keypath, dogecoin_hdnode *bip44_key)
{
    char addr_idx_str[UINT32_MAX_DIGITS + 1] = "";
    char hard_addr_char = '\0';

    /* Validate input parameters */
    if (master_key == NULL || change_level == NULL || keypath == NULL || bip44_key == NULL) {
        return -1;
    }

    /* Convert the address index to a string and set the harden character if necessary */
    if (address_index != NULL) {
        snprintf(addr_idx_str, UINT32_MAX_DIGITS + 1, "/%u", (uint32_t) *address_index);
        if (hard_addr == BIP44_HARDENED_ADDRESS) {
            hard_addr_char = '\'';
        }
    }

    /* Validate the change level input */
    if (strcmp(change_level, BIP44_CHANGE_EXTERNAL) != 0 && strcmp(change_level, BIP44_CHANGE_INTERNAL) != 0) {
        return -1;
    }

    /* Construct the BIP 44 keypath using the input parameters and BIP 44 constants */
    snprintf(keypath, BIP44_KEY_PATH_MAX_LENGTH, "m/" BIP44_PURPOSE "'/" BIP44_COIN_TYPE "'/%u'/%s%s%c", account, change_level, addr_idx_str, hard_addr_char);

    /* Generate the BIP 44 extended private key using the master key and keypath */
    if (!dogecoin_hd_generate_key(bip44_key, keypath, master_key->private_key, master_key->chain_code, false)) {
        return -1;
    }
    debug_print("Account: %u\n", account);
    debug_print("Derivation path: %s\n", keypath);
    return 0;
}

/**
 * @brief This function derives the BIP 44 extended public key from a master public key.
 *
 * BIP 44 is a standard for deterministic wallet generation, which allows for the generation of
 * a hierarchy of keys starting from a single seed. The extended public key is a specific key
 * in this hierarchy that is derived using BIP 44 constants and the input parameters.
 *
 * @param master_key The master public key to derive from.
 * @param account The account number to use in the BIP 44 key derivation.
 * @param address_index The address index to use in the BIP 44 key derivation.
 * @param change_level The change level to use in the BIP 44 key derivation, either "external" or "internal".
 * @param hard_addr A flag indicating whether or not to use a hardened address in the BIP 44 key derivation.
 * @param keypath A string buffer to store the BIP 44 keypath used for the derivation.
 * @param bip44_key The BIP 44 extended public key to be generated.
 * @return 0 if the key is derived successfully, -1 otherwise.
 */
int derive_bip44_extended_public_key(const dogecoin_hdnode *master_key, const uint32_t account, const uint32_t* address_index, const char* change_level, const int hard_addr, char* keypath, dogecoin_hdnode *bip44_key)
{
    char addr_idx_str[UINT32_MAX_DIGITS + 1] = "";
    char hard_addr_char = '\0';

    /* Validate input parameters */
    if (master_key == NULL || change_level == NULL || keypath == NULL || bip44_key == NULL) {
        return -1;
    }

    /* Convert the address index to a string and set the harden character if necessary */
    if (address_index != NULL) {
        snprintf(addr_idx_str, UINT32_MAX_DIGITS + 1, "/%u", (uint32_t) *address_index);
        if (hard_addr == BIP44_HARDENED_ADDRESS) {
            hard_addr_char = '\'';
        }
    }

    /* Validate the change level input */
    if (strcmp(change_level, BIP44_CHANGE_EXTERNAL) != 0 && strcmp(change_level, BIP44_CHANGE_INTERNAL) != 0) {
        return -1;
    }

    /* Construct the BIP 44 keypath using the input parameters and BIP 44 constants */
    snprintf(keypath, BIP44_KEY_PATH_MAX_LENGTH, "m/" BIP44_PURPOSE "'/" BIP44_COIN_TYPE "'/%u'/%s/%s%c", account, change_level, addr_idx_str, hard_addr_char);

    /* Generate the BIP 44 extended public key using the master key and keypath */
    if (!dogecoin_hd_generate_key(bip44_key, keypath, master_key->public_key, master_key->chain_code, true)) {
        return -1;
    }
    debug_print("Account: %u\n", account);
    debug_print("Derivation path: %s\n", keypath);
    return 0;
}

