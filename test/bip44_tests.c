/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 edtubbs                                         *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "utest.h"

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
    char mnemonic[256] = "";
    size_t length;
    char keypath[BIP44_KEY_PATH_MAX_LENGTH + 1] = "";

    /* generate mnemonic(s) */
    #define MAX_MNEMONIC_LENGTH 1024
    char *words = NULL;
    char *entropy = "00000000000000000000000000000000";

    /* allocate space for mnemonics */
    words = malloc(sizeof(char) * MAX_MNEMONIC_LENGTH);
    if (words == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for mnemonic\n");
    }
    memset(words, '\0', MAX_MNEMONIC_LENGTH);

    printf ("\nTests with known entropy values\n");
    dogecoin_generate_mnemonic ("128", "eng", " ", entropy, NULL, &length, words);
    u_assert_mem_eq(words, "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about", length);


    // Convert mnemonic to seed
//    dogecoin_seed_from_mnemonic (words, "", seed);
    uint8_t* seed = malloc(64);
    memset(seed, 0, 64);
    seed = utils_hex_to_uint8("000102030405060708090a0b0c0d0e0f");
    char* seed_hex;
    seed_hex = utils_uint8_to_hex(seed, 16);
    printf ("%s\n", seed_hex);

    // Generate the root key from the seed
    result = dogecoin_hdnode_from_seed(seed, 16, &node);
    u_assert_int_eq(result, true);

    // Print the root key
    char root_key_str[112];
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, root_key_str, sizeof(root_key_str));
    printf("BIP32 root pub key: %s\n", root_key_str);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, root_key_str, sizeof(root_key_str));
    printf("BIP32 root prv key: %s\n", root_key_str);

    // Derive the BIP 44 extended key
    result = derive_bip44_extended_private_key(&node, BIP44_FIRST_ACCOUNT_NODE, NULL, BIP44_CHANGE_EXTERNAL, 0, keypath, &bip44_key);
    u_assert_int_eq(result, 0);

    // Print the BIP 44 extended private key
    char bip44_private_key[112];
    dogecoin_hdnode_serialize_private(&bip44_key, &dogecoin_chainparams_main, bip44_private_key, sizeof(bip44_private_key));
    printf("BIP 44 extended private key: %s\n", bip44_private_key);

    char str[112];
//    result = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node);
//    u_assert_int_eq(result, true);
//    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));

//    u_assert_str_eq(str, "DQKnfKgsqVDxXjcCUKSs8Xz7bDe2SNcyof");

    // Print the BIP 44 extended public key
    char bip44_public_key[112];
    dogecoin_hdnode_serialize_public(&bip44_key, &dogecoin_chainparams_main, bip44_public_key, sizeof(bip44_public_key));
    printf("BIP 44 extended public key: %s\n", bip44_public_key);

    printf("Derived Addresses\n");

    for (uint32_t index = BIP44_FIRST_ACCOUNT_NODE; index < BIP44_ADDRESS_GAP_LIMIT; index++) {
        // Derive the addresses
        result = derive_bip44_extended_private_key(&node, BIP44_FIRST_ACCOUNT_NODE, &index, BIP44_CHANGE_EXTERNAL, BIP44_HARDENED_ADDRESS, keypath, &bip44_key);
        u_assert_int_eq(result, 0);

        // Print the private key
        dogecoin_hdnode_serialize_private(&bip44_key, &dogecoin_chainparams_main, bip44_private_key, sizeof(bip44_private_key));
        printf("private key: %s\n", utils_uint8_to_hex(bip44_key.private_key, sizeof(bip44_key.private_key)));
        printf("private key (serialized): %s\n", bip44_private_key);

        // Print the public key
        dogecoin_hdnode_serialize_public(&bip44_key, &dogecoin_chainparams_main, bip44_public_key, sizeof(bip44_public_key));
        printf("public key: %s\n", utils_uint8_to_hex(bip44_key.public_key, sizeof(bip44_key.public_key)));
        printf("public key (serialized): %s\n", bip44_public_key);

        dogecoin_hdnode_get_p2pkh_address(&bip44_key, &dogecoin_chainparams_main, str, sizeof(str));
        printf("Address: %s\n", str);
    }

    free (words);
}





