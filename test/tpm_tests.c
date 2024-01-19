/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2023 edtubbs                                         *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <test/utest.h>
#include <dogecoin/address.h>
#include <dogecoin/random.h>
#include <dogecoin/sha2.h>
#include <dogecoin/seal.h>
#include <dogecoin/utils.h>
#include <stdlib.h>

#if defined (_WIN64) && !defined(__MINGW64__)
#include <windows.h>
#include <tbs.h>
#include <ncrypt.h>
#endif

#ifndef WINVER
#define WINVER 0x0600
#endif

void test_tpm()
{

    // Generate a random number
    uint8_t random[32] = {0};
    dogecoin_random_bytes(random, sizeof(random), 1);

    // Define a random seed and a decrypted seed
    SEED seed = {0};
    SEED decrypted_seed = {0};
    sha512_raw(&random[0], 32, seed);

    // Define a test password
#ifdef TEST_PASSWD
    char* test_password = PASSWD_STR;
#else
    char* test_password = NULL;
#endif

    // Encrypt a random seed with software
    u_assert_true (dogecoin_encrypt_seed_with_sw (seed, sizeof(SEED), TEST_FILE, true, test_password));
    debug_print ("Seed: %s\n", utils_uint8_to_hex (seed, sizeof (SEED)));

    // Decrypt the seed with software
    u_assert_true (dogecoin_decrypt_seed_with_sw (decrypted_seed, TEST_FILE, test_password));
    debug_print ("Decrypted seed: %s\n", utils_uint8_to_hex (decrypted_seed, sizeof (SEED)));

    // Compare the seed and the decrypted seed
    u_assert_mem_eq (seed, decrypted_seed, sizeof (SEED));

    // Define a random HD node and a decrypted HD node
    dogecoin_hdnode node, decrypted_node;

    // Generate a random HD node with software
    u_assert_true (dogecoin_generate_hdnode_encrypt_with_sw (&node, TEST_FILE, true, test_password));
    debug_print ("HD node: %s\n", utils_uint8_to_hex ((uint8_t *) &node, sizeof (dogecoin_hdnode)));

    // Decrypt the HD node with software
    u_assert_true (dogecoin_decrypt_hdnode_with_sw (&decrypted_node, TEST_FILE, test_password));
    debug_print ("Decrypted HD node: %s\n", utils_uint8_to_hex ((uint8_t *) &decrypted_node, sizeof (dogecoin_hdnode)));

    // Compare the HD node and the decrypted HD node
    u_assert_mem_eq (&node, &decrypted_node, sizeof (dogecoin_hdnode));

    // Generate a mnemonic with software
    MNEMONIC mnemonic = {0};
    MNEMONIC decrypted_mnemonic = {0};

    // Generate a random mnemonic with software
    u_assert_true (dogecoin_generate_mnemonic_encrypt_with_sw(mnemonic, TEST_FILE, true, "eng", " ", NULL, test_password));
    debug_print("Mnemonic: %s\n", mnemonic);

    // Decrypt the mnemonic with software
    u_assert_true (dogecoin_decrypt_mnemonic_with_sw(decrypted_mnemonic, TEST_FILE, test_password));
    debug_print("Decrypted mnemonic: %s\n", decrypted_mnemonic);

    // Compare the mnemonic and the decrypted mnemonic
    u_assert_mem_eq (mnemonic, decrypted_mnemonic, sizeof (MNEMONIC));

#if defined (_WIN64) && !defined(__MINGW64__) && defined(USE_TPM2)

    // Create TBS context (TPM2)
    TBS_HCONTEXT hContext = 0;
    TBS_CONTEXT_PARAMS2 params;
    params.version = TBS_CONTEXT_VERSION_TWO;
    TBS_RESULT hr = Tbsi_Context_Create((PCTBS_CONTEXT_PARAMS)&params, &hContext);
    u_assert_uint32_eq (hr, TBS_SUCCESS);

    // Get TPM device information
    TPM_DEVICE_INFO info;
    hr = Tbsi_GetDeviceInfo (sizeof (info), &info);
    u_assert_uint32_eq (hr, TBS_SUCCESS);

    // Verify TPM2
    u_assert_uint32_eq (info.tpmVersion, TPM_VERSION_20);

    // Send TPM2_CC_GetRandom command
    const BYTE cmd_random[] = {
        0x80, 0x01,             // tag: TPM_ST_SESSIONS
        0x00, 0x00, 0x00, 0x0C, // commandSize: size of the entire command byte array
        0x00, 0x00, 0x01, 0x7B, // commandCode: TPM2_CC_GetRandom
        0x00, 0x20              // parameter: 32 bytes
    };
    BYTE resp_random[TBS_IN_OUT_BUF_SIZE_MAX] = { 0 };
    UINT32 resp_randomSize =  TBS_IN_OUT_BUF_SIZE_MAX;
    hr = Tbsip_Submit_Command(hContext, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL, cmd_random, sizeof(cmd_random), resp_random, &resp_randomSize);
    u_assert_uint32_eq (hr, TBS_SUCCESS);
    char* rand_hex;
    rand_hex = utils_uint8_to_hex(&resp_random[12], 0x20);
    debug_print ("TPM2_CC_GetRandom response: %s\n", rand_hex);

    // Generate a random HD node with the TPM2
    u_assert_true (dogecoin_generate_hdnode_encrypt_with_tpm (&node, TEST_FILE, true));
    debug_print ("HD node: %s\n", utils_uint8_to_hex ((uint8_t *) &node, sizeof (dogecoin_hdnode)));

    // Decrypt the HD node with the TPM2
    u_assert_true (dogecoin_decrypt_hdnode_with_tpm (&decrypted_node, TEST_FILE));
    debug_print ("Decrypted HD node: %s\n", utils_uint8_to_hex ((uint8_t *) &decrypted_node, sizeof (dogecoin_hdnode)));

    // Compare the HD node and the decrypted HD node
    u_assert_mem_eq (&node, &decrypted_node, sizeof (dogecoin_hdnode));
    debug_print ("HD node and decrypted HD node are equal\n");

    // Define a random seed and a decrypted seed
    sha512_raw(&resp_random[12], 32, seed);

    // Generate a random seed with the TPM2
    u_assert_true (dogecoin_encrypt_seed_with_tpm (seed, sizeof(SEED), TEST_FILE, true));
    debug_print ("Seed: %s\n", utils_uint8_to_hex (seed, sizeof (SEED)));

    // Decrypt the seed with the TPM2
    u_assert_true (dogecoin_decrypt_seed_with_tpm (decrypted_seed, TEST_FILE));
    debug_print ("Decrypted seed: %s\n", utils_uint8_to_hex (decrypted_seed, sizeof (SEED)));

    // Compare the seed and the decrypted seed
    u_assert_mem_eq (seed, decrypted_seed, sizeof (SEED));
    debug_print ("Seed and decrypted seed are equal\n");

    // Generate a random mnemonic with the TPM2
    u_assert_true (dogecoin_generate_mnemonic_encrypt_with_tpm(mnemonic, TEST_FILE, true, "eng", " ", NULL));
    debug_print("Mnemonic: %s\n", mnemonic);

    // Decrypt the mnemonic with the TPM2
    u_assert_true (dogecoin_decrypt_mnemonic_with_tpm(decrypted_mnemonic, TEST_FILE));
    debug_print("Decrypted mnemonic: %s\n", decrypted_mnemonic);

    // Compare the mnemonic and the decrypted mnemonic
    u_assert_mem_eq (mnemonic, decrypted_mnemonic, sizeof (MNEMONIC));
    debug_print("Mnemonic and decrypted mnemonic are equal\n");

    // test generateRandomEnglishMnemonicTPM
    u_assert_true (generateRandomEnglishMnemonicTPM(mnemonic, TEST_FILE, true));
    debug_print("Mnemonic: %s\n", mnemonic);

    // test getDerivedHDAddressFromEncryptedSeed
    char derived_address[P2PKHLEN];
    u_assert_true (getDerivedHDAddressFromEncryptedSeed(0, 0, BIP44_CHANGE_EXTERNAL, derived_address, false, TEST_FILE) == 0);
    debug_print("Derived address: %s\n", derived_address);

    // test getDerivedHDAddressFromEncryptedMnemonic
    u_assert_true (getDerivedHDAddressFromEncryptedMnemonic(0, 0, BIP44_CHANGE_EXTERNAL, NULL, derived_address, false, TEST_FILE) == 0);
    debug_print("Derived address: %s\n", derived_address);

    // test getDerivedHDAddressFromEncryptedHDNode
    u_assert_true (getDerivedHDAddressFromEncryptedHDNode(0, 0, BIP44_CHANGE_EXTERNAL, derived_address, false, TEST_FILE) == 0);
    debug_print("Derived address: %s\n", derived_address);

#endif

}
