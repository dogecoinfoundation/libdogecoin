/**
 * Copyright (c) 2023 edtubbs
 * Copyright (c) 2023 The Dogecoin Foundation
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
 * @brief This function derives the BIP 44 extended key from a master key.
 *
 * BIP 44 is a standard for deterministic wallet generation, which allows for the generation of
 * a hierarchy of keys starting from a single seed. The extended key is a specific key
 * in this hierarchy that is derived using BIP 44 constants and the input parameters.
 *
 * @param master_key The master key to derive from.
 * @param account The account number to use in the BIP 44 key derivation.
 * @param address_index The address index to use in the BIP 44 key derivation.
 * @param change_level The change level to use in the BIP 44 key derivation, either "external" or "internal".
 * @param custom_path A custom BIP 44 keypath to use for the derivation (optional).
 * @param keypath A string buffer to store the BIP 44 keypath used for the derivation.
 * @param bip44_key The BIP 44 extended key to be generated.
 * @return 0 if the key is derived successfully, -1 otherwise.
 */
int derive_bip44_extended_key(const dogecoin_hdnode *master_key, const uint32_t *account, const uint32_t* address_index, const CHANGE_LEVEL change_level, const KEY_PATH custom_path, const dogecoin_bool is_testnet, KEY_PATH keypath, dogecoin_hdnode *bip44_key)
{
    char addr_idx_str[UINT32_MAX_DIGITS + 1] = "";
    char acct_str[UINT32_MAX_DIGITS + 1] = "";

    /* Validate input parameters */
    if (master_key == NULL || keypath == NULL || bip44_key == NULL) {
        fprintf(stderr, "ERROR: invalid input arguments\n");
        return -1;
    }

    /* Convert the address index to a string */
    if (account != NULL) {
        snprintf(acct_str, UINT32_MAX_DIGITS + 1, "%u", (uint32_t) *account);
    }

    /* Convert the address index to a string */
    if (address_index != NULL) {
        snprintf(addr_idx_str, UINT32_MAX_DIGITS + 1, "%u", (uint32_t) *address_index);
    }

    /* Validate the change level input */
    if (change_level != NULL && strcmp(change_level, BIP44_CHANGE_EXTERNAL) != 0 && strcmp(change_level, BIP44_CHANGE_INTERNAL) != 0) {
        fprintf(stderr, "ERROR: invalid change level\n");
        return -1;
    }

    /* Check for custom key path */
    if (custom_path != NULL) {
        /* Construct the BIP 44 keypath using the custom path */
        snprintf(keypath, BIP44_KEY_PATH_MAX_LENGTH, "%s%s", custom_path, addr_idx_str);
    }
    else if (account == NULL) {
        /* Construct the BIP 44 keypath using the input parameters and BIP 44 constants */
        snprintf(keypath, BIP44_KEY_PATH_MAX_LENGTH, SLIP44_KEY_PATH "%s'", is_testnet ? BIP44_COIN_TYPE_TEST : BIP44_COIN_TYPE);
    }
    else if (change_level == NULL) {
        /* Construct the BIP 44 keypath using the input parameters and BIP 44 constants */
        snprintf(keypath, BIP44_KEY_PATH_MAX_LENGTH, SLIP44_KEY_PATH "%s'/%s'", is_testnet ? BIP44_COIN_TYPE_TEST : BIP44_COIN_TYPE, acct_str);
    }
    else if (address_index == NULL) {
        /* Construct the BIP 44 keypath using the input parameters and BIP 44 constants */
        snprintf(keypath, BIP44_KEY_PATH_MAX_LENGTH, SLIP44_KEY_PATH "%s'/%s'/%s", is_testnet ? BIP44_COIN_TYPE_TEST : BIP44_COIN_TYPE, acct_str, change_level);
    }
    else {
        /* Construct the BIP 44 keypath using the input parameters and BIP 44 constants */
        snprintf(keypath, BIP44_KEY_PATH_MAX_LENGTH, SLIP44_KEY_PATH "%s'/%s'/%s/%s", is_testnet ? BIP44_COIN_TYPE_TEST : BIP44_COIN_TYPE, acct_str, change_level, addr_idx_str);
    }

    /* Generate the BIP 44 extended key using the master key and keypath */
    if (!dogecoin_hd_generate_key(bip44_key, keypath, master_key->private_key, master_key->depth, master_key->chain_code, false)) {
        return -1;
    }
    debug_print("Account: %s\n", acct_str);
    debug_print("Derivation path: %s\n", keypath);
    return 0;
}

/** @brief This function derives the BIP 44 extended private key from a master private key.
 *
 * @param hd_privkey_master The master private key to derive from.
 * @param account The account number to use in the BIP 44 key derivation.
 * @param change_level The change level to use in the BIP 44 key derivation, either "external" or "internal".
 * @param address_index The address index to use in the BIP 44 key derivation.
 * @param path A custom BIP 44 keypath to use for the derivation (optional).
 * @param extkeyout A string buffer to store the BIP 44 extended private key.
 * @param keypath A string buffer to store the BIP 44 keypath used for the derivation.
 *
 * @return true if the key is derived successfully, false otherwise.
 */
dogecoin_bool deriveBIP44ExtendedKey(
    const char hd_privkey_master[HDKEYLEN],
    const uint32_t* account,
    const CHANGE_LEVEL change_level,
    const uint32_t* address_index,
    const KEY_PATH path,
    char extkeyout[HDKEYLEN],
    KEY_PATH keypath) {
    dogecoin_hdnode master_key;
    dogecoin_hdnode derived_key;
    dogecoin_bool is_testnet = isTestnetFromB58Prefix(hd_privkey_master);
    if (hd_privkey_master == NULL || extkeyout == NULL) {
        fprintf(stderr, "ERROR: invalid input arguments\n");
        return false;
    }
    if (!dogecoin_hdnode_deserialize(hd_privkey_master, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, &master_key)) {
        fprintf(stderr, "ERROR: invalid master key\n");
        return false;
    }
    if (derive_bip44_extended_key(&master_key, account, address_index, change_level, path, is_testnet, keypath, &derived_key) != 0) {
        fprintf(stderr, "ERROR: failed to derive BIP 44 extended key\n");
        return false;
    }
    dogecoin_hdnode_serialize_private(&derived_key, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, extkeyout, HDKEYLEN);
    return true;
}

/** @brief This function derives the BIP 44 extended public key from a master private key.
 *
 * @param hd_privkey_master The master private key to derive from.
 * @param account The account number to use in the BIP 44 key derivation.
 * @param change_level The change level to use in the BIP 44 key derivation, either "external" or "internal".
 * @param address_index The address index to use in the BIP 44 key derivation.
 * @param path A custom BIP 44 keypath to use for the derivation (optional).
 * @param extkeyout A string buffer to store the BIP 44 extended public key.
 * @param keypath A string buffer to store the BIP 44 keypath used for the derivation.
 *
 * @return true if the key is derived successfully, false otherwise.
 */
dogecoin_bool deriveBIP44ExtendedPublicKey(
    const char hd_privkey_master[HDKEYLEN],
    const uint32_t* account,
    const CHANGE_LEVEL change_level,
    const uint32_t* address_index,
    const KEY_PATH path,
    char extkeyout[HDKEYLEN],
    KEY_PATH keypath) {
    dogecoin_hdnode master_key;
    dogecoin_hdnode derived_key;
    dogecoin_bool is_testnet = isTestnetFromB58Prefix(hd_privkey_master);
    if (hd_privkey_master == NULL || extkeyout == NULL) {
        fprintf(stderr, "ERROR: invalid input arguments\n");
        return false;
    }
    if (!dogecoin_hdnode_deserialize(hd_privkey_master, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, &master_key)) {
        fprintf(stderr, "ERROR: invalid master key\n");
        return false;
    }
    if (derive_bip44_extended_key(&master_key, account, address_index, change_level, path, is_testnet, keypath, &derived_key) != 0) {
        fprintf(stderr, "ERROR: failed to derive BIP 44 extended public key\n");
        return false;
    }
    dogecoin_hdnode_serialize_public(&derived_key, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, extkeyout, HDKEYLEN);
    return true;
}
