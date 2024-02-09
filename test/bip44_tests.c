/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 edtubbs                                         *
 * Copyright (c) 2022 michilumin                                      *
 * Copyright (c) 2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <test/utest.h>

#include <stdio.h>
#include <dogecoin/bip32.h>
#include <dogecoin/bip39.h>
#include <dogecoin/bip44.h>
#include <dogecoin/utils.h>

void test_bip44()
{
    /* Initialize variables */
    int result;
    dogecoin_hdnode node;
    dogecoin_hdnode bip44_key;
    dogecoin_hdnode bip44_change_key;
    dogecoin_hdnode bip44_address_key;
    uint32_t account = BIP44_FIRST_ACCOUNT_NODE;
    size_t size;
    char keypath[BIP44_KEY_PATH_MAX_LENGTH + 1] = "";

    /* generate mnemonic(s) */
    char *words = NULL;
    char *entropy = "00000000000000000000000000000000";
    char* entropy_out = NULL;

    /* allocate space for mnemonics */
    words = malloc(sizeof(char) * MAX_MNEMONIC_STRING_SIZE);
    if (words == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for mnemonic\n");
    }
    memset(words, '\0', MAX_MNEMONIC_STRING_SIZE);

    /* allocate space for entropy */
    entropy_out = malloc(sizeof(char) * MAX_ENTROPY_STRING_SIZE);
    if (words == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for mnemonic\n");
    }
    memset(words, '\0', MAX_ENTROPY_STRING_SIZE);

    debug_print ("%s", "\nTests with known entropy values\n");
    dogecoin_generate_mnemonic ("128", "eng", " ", entropy, NULL, entropy_out, &size, words);
    u_assert_mem_eq(words, "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about", size);


    // Convert mnemonic to seed
    uint8_t* seed = malloc(MAX_SEED_SIZE);
    memset(seed, 0, MAX_SEED_SIZE);
    uint8_t* test_seed = utils_hex_to_uint8("5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc19a5ac40b389cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4");
    dogecoin_seed_from_mnemonic (words, "", seed);
    u_assert_mem_eq(seed, test_seed, MAX_SEED_SIZE);


    char* seed_hex;
    seed_hex = utils_uint8_to_hex(seed, MAX_SEED_SIZE);
    debug_print ("%s\n", seed_hex);

    // Generate the root key from the seed
    result = dogecoin_hdnode_from_seed(seed, MAX_SEED_SIZE, &node);
    u_assert_int_eq(result, true);

    debug_print("%s", "\n\nTESTNET\n\n");

    // Print the root key (TESTNET)
    char root_key_str[HDKEYLEN];
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_test, root_key_str, sizeof(root_key_str));
    debug_print("BIP32 root pub key: %s\n", root_key_str);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_test, root_key_str, sizeof(root_key_str));
    debug_print("BIP32 root prv key: %s\n", root_key_str);

    // Derive the BIP 44 extended key at the account level
    result = derive_bip44_extended_key(&node, &account, NULL, NULL, NULL, true, keypath, &bip44_key);
    u_assert_int_eq(result, 0);

    // Print the BIP 44 extended key at the account level
    char bip44_account_private_key[HDKEYLEN];
    dogecoin_hdnode_serialize_private(&bip44_key, &dogecoin_chainparams_test, bip44_account_private_key, sizeof(bip44_account_private_key));
    debug_print("Account extended key: %s\n", bip44_account_private_key);

    // Print the BIP 44 extended public key at the account level
    char bip44_account_public_key[HDKEYLEN];
    dogecoin_hdnode_serialize_public(&bip44_key, &dogecoin_chainparams_test, bip44_account_public_key, sizeof(bip44_account_public_key));
    debug_print("Account extended public key: %s\n", bip44_account_public_key);

    char* change_level = BIP44_CHANGE_EXTERNAL;
    for (int i = 0; i <= 1; i++) {
        if (i==1)
            change_level = BIP44_CHANGE_INTERNAL;

        // Derive the BIP 44 extended key at the change level
        result = derive_bip44_extended_key(&node, &account, NULL, change_level, NULL, true, keypath, &bip44_change_key);
        u_assert_int_eq(result, 0);

        // Print the BIP 44 extended public key at the change level
        char bip44_change_public_key[HDKEYLEN];
        dogecoin_hdnode_serialize_public(&bip44_change_key, &dogecoin_chainparams_test, bip44_change_public_key, sizeof(bip44_change_public_key));
        debug_print("Change level extended public key %s\n", bip44_change_public_key);

        debug_print("%s", "Derived Addresses\n");

        char str[HDKEYLEN];

        for (uint32_t index = BIP44_FIRST_ADDRESS_INDEX; index < BIP44_ADDRESS_GAP_LIMIT; index++) {
            // Derive the addresses
            result = derive_bip44_extended_key(&node, &account, &index, change_level, NULL, true, keypath, &bip44_address_key);
            u_assert_int_eq(result, 0);

            // Print the public key
            char bip44_change_public_key[HDKEYLEN];
            dogecoin_hdnode_serialize_public(&bip44_address_key, &dogecoin_chainparams_test, bip44_change_public_key, sizeof(bip44_change_public_key));
            debug_print("public key: %s\n", utils_uint8_to_hex(bip44_address_key.public_key, sizeof(bip44_address_key.public_key)));
            debug_print("public key (serialized): %s\n", bip44_change_public_key);

            dogecoin_hdnode_get_p2pkh_address(&bip44_address_key, &dogecoin_chainparams_test, str, sizeof(str));
            debug_print("Address: %s\n", str);
        }
    }

    debug_print("%s", "\n\nMAINNET\n\n");

    // Print the root key (MAINNET)
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, root_key_str, sizeof(root_key_str));
    debug_print("BIP32 root pub key: %s\n", root_key_str);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, root_key_str, sizeof(root_key_str));
    debug_print("BIP32 root prv key: %s\n", root_key_str);

    // Derive the BIP 44 extended key at the account level
    result = derive_bip44_extended_key(&node, &account, NULL, NULL, NULL, false, keypath, &bip44_key);
    u_assert_int_eq(result, 0);

    // Print the BIP 44 extended key at the account level
    dogecoin_hdnode_serialize_private(&bip44_key, &dogecoin_chainparams_main, bip44_account_private_key, sizeof(bip44_account_private_key));
    debug_print("Account extended key: %s\n", bip44_account_private_key);

    // Print the BIP 44 extended public key at the account level
    dogecoin_hdnode_serialize_public(&bip44_key, &dogecoin_chainparams_main, bip44_account_public_key, sizeof(bip44_account_public_key));
    debug_print("Account extended public key: %s\n", bip44_account_public_key);

    change_level = BIP44_CHANGE_EXTERNAL;
    for (int i = 0; i <= 1; i++) {
        if (i==1)
            change_level = BIP44_CHANGE_INTERNAL;

        // Derive the BIP 44 extended key at the change level
        result = derive_bip44_extended_key(&node, &account, NULL, change_level, NULL, false, keypath, &bip44_change_key);
        u_assert_int_eq(result, 0);

        // Print the BIP 44 extended public key at the change level
        char bip44_change_public_key[HDKEYLEN];
        dogecoin_hdnode_serialize_public(&bip44_change_key, &dogecoin_chainparams_main, bip44_change_public_key, sizeof(bip44_change_public_key));
        debug_print("Change level extended public key %s\n", bip44_change_public_key);

        debug_print("%s", "Derived Addresses\n");

        char str[HDKEYLEN];

        for (uint32_t index = BIP44_FIRST_ACCOUNT_NODE; index < BIP44_ADDRESS_GAP_LIMIT; index++) {
            // Derive the addresses
            result = derive_bip44_extended_key(&node, &account, &index, change_level, NULL, false, keypath, &bip44_address_key);
            u_assert_int_eq(result, 0);

            // Print the public key
            char bip44_address_public_key[HDKEYLEN];
            dogecoin_hdnode_serialize_public(&bip44_address_key, &dogecoin_chainparams_main, bip44_address_public_key, sizeof(bip44_address_public_key));
            debug_print("public key: %s\n", utils_uint8_to_hex(bip44_address_key.public_key, sizeof(bip44_address_key.public_key)));
            debug_print("public key (serialized): %s\n", bip44_address_public_key);

            dogecoin_hdnode_get_p2pkh_address(&bip44_address_key, &dogecoin_chainparams_main, str, sizeof(str));
            debug_print("Address: %s\n", str);
        }
    }

    /* test deriveBIP44ExtendedKey */
    char masterkey[HDKEYLEN];
    char accountkey[HDKEYLEN];
    char accountPubkey[HDKEYLEN];
    char bip32key[HDKEYLEN];
    char changepubkey[HDKEYLEN];

    u_assert_true(getHDRootKeyFromSeed(test_seed, MAX_SEED_SIZE, true, masterkey));
    u_assert_true(deriveBIP44ExtendedKey(masterkey, NULL, NULL, NULL, NULL, accountkey, keypath));
    u_assert_true(deriveBIP44ExtendedKey(masterkey, &account, BIP44_CHANGE_EXTERNAL, NULL, NULL, bip32key, keypath));
    u_assert_str_eq(bip32key,
                    "tprv8hi9XJvkuKfu6oRGUsAnPAnQNUKcEjwrLbS2w2hTSPKrFj5YYS3Ax7UDDrZZHd4PSnPLW5whNxAXTW5bBrSNiSD1LUeg9n4j5sdGRJsZZwP");

    debug_print("deriveBIP44ExtendedKey: %s\n", accountkey);
    debug_print("deriveBIP44ExtendedKey: %s\n", bip32key);

    /* test deriveBIP44ExtendedPublicKey */
    u_assert_true(deriveBIP44ExtendedKey(masterkey, NULL, NULL, NULL, NULL, accountkey, keypath));
    u_assert_true(deriveBIP44ExtendedKey(masterkey, &account, BIP44_CHANGE_EXTERNAL, NULL, NULL, bip32key, keypath));
    u_assert_true(deriveBIP44ExtendedPublicKey(masterkey, &account, NULL, NULL, NULL, accountPubkey, keypath));
    u_assert_true(deriveBIP44ExtendedPublicKey(masterkey, &account, BIP44_CHANGE_EXTERNAL, NULL, NULL, changepubkey, keypath));
    u_assert_str_eq(changepubkey,
                    "tpubDEQBfiy13hMZzGT4NWqNnaSWwVqYQ58kuu2pDYjkrf8F6DLKAprm8c65Pyh7PrzodXHtJuEXFu5yf6JbvYaL8rz7v28zapwbuzZzr7z4UvR");

    debug_print("deriveBIP44ExtendedKey: %s\n", accountPubkey);
    debug_print("deriveBIP44ExtendedKey: %s\n", changepubkey);

    free (entropy_out);
    free (words);
    free (seed);
}
