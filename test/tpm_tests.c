/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2023 edtubbs                                         *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "utest.h"

#include <dogecoin/sha2.h>
#include <dogecoin/seal.h>
#include <dogecoin/utils.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <tbs.h>
#include <ncrypt.h>
#ifndef WINVER
#define WINVER 0x0600
#endif

void test_tpm()
{
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
        0x00, 0x40              // parameter: 32 bytes
    };
    BYTE resp_random[TBS_IN_OUT_BUF_SIZE_MAX] = { 0 };
    UINT32 resp_randomSize =  TBS_IN_OUT_BUF_SIZE_MAX;
    hr = Tbsip_Submit_Command(hContext, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL, cmd_random, sizeof(cmd_random), resp_random, &resp_randomSize);
    u_assert_uint32_eq (hr, TBS_SUCCESS);
    char* rand_hex;
    rand_hex = utils_uint8_to_hex(&resp_random[12], 0x40);
    debug_print ("TPM2_CC_GetRandom response: %s\n", rand_hex);


    SEED seed = {0};
    sha512_raw(&resp_random[12], 32, seed);
    dogecoin_seal_seed (seed);

    printf("BIP32 seed sealed inside TPM.\n");

    SEED unseed = {0};
    dogecoin_unseal_seed (unseed);

    printf("BIP32 seed unsealed inside TPM.\n");

    dogecoin_hdnode node, node2;
    const wchar_t* master_name = L"test master";
    const wchar_t* seed_name = L"test seed";
    const wchar_t* mnemonic_name = L"test mnemonic";

    // Generate a random HD node with the TPM2
    dogecoin_bool ret = dogecoin_generate_hdnode_in_tpm (&node, master_name, true);
    
    // Generate a random HD node with the TPM2
    ret = dogecoin_generate_hdnode_in_tpm (&node2, master_name, true);

    // Export the HD node from the TPM2
    ret = dogecoin_export_hdnode_from_tpm (master_name, &node2);

    // Compare node and node2
    //u_assert_mem_eq (&node, &node2, sizeof (dogecoin_hdnode));

    // Erase the HD node from the TPM2
    ret = dogecoin_erase_hdnode_from_tpm(master_name);

    // Export the HD node from the TPM2
    ret = dogecoin_export_hdnode_from_tpm (master_name, &node2);

    // Generate a random seed with the TPM2
    ret = dogecoin_generate_seed_in_tpm (&seed, seed_name, true);

    // Export the seed from the TPM2
    ret = dogecoin_export_seed_from_tpm (seed_name, &seed);

    // Erase the seed from the TPM2
    ret = dogecoin_erase_seed_from_tpm(seed_name);

    // Export the seed from the TPM2
    ret = dogecoin_export_seed_from_tpm (seed_name, &seed);

    // Generate a mnemonic with the TPM2
    MNEMONIC mnemonic = {0};

    ret = dogecoin_generate_mnemonic_in_tpm (&mnemonic, mnemonic_name, true);

    ret = dogecoin_hdnode_from_tpm (&node2);


/*
    // Seal the random with the TPM2
    const BYTE cmd_seal[] = {
        0x80, 0x01,             // tag: TPM_ST_SESSIONS
        0x00, 0x00, 0x00, 0x36, // commandSize: size of the entire command byte array
        0x00, 0x00, 0x01, 0x91, // commandCode: TPM2_CC_CreateLoaded
        0x40, 0x00, 0x00, 0x01, // parentHandle: Handle of a transient storage key, a persistent storage key, TPM_RH_ENDORSEMENT, TPM_RH_OWNER, TPM_RH_PLATFORM+{PP}, or TPM_RH_NULL
        0x00, 0x00, 0x00, 0x20, // inSensitiveSize: size of the sensitive data
        0x00, 0x00,             // inSensitive: userAuth: empty buffer (size 0)
        // sensitiveDataValue: <data from TPM2_GetRandom response>
        // Replace the following 32 bytes with the random data obtained from TPM2_GetRandom response
        resp_random[12], resp_random[13], resp_random[14], resp_random[15], resp_random[16], resp_random[17], resp_random[18], resp_random[19], 
        resp_random[20], resp_random[21], resp_random[22], resp_random[23], resp_random[24], resp_random[25], resp_random[26], resp_random[27],
        resp_random[28], resp_random[29], resp_random[30], resp_random[31], resp_random[32], resp_random[33], resp_random[34], resp_random[35],
        resp_random[36], resp_random[37], resp_random[38], resp_random[39], resp_random[40], resp_random[41], resp_random[42], resp_random[43],
        0x00, 0x00,             // inPublic: size of the public template (0 bytes)
    };

    BYTE resp_seal[TBS_IN_OUT_BUF_SIZE_MAX] = { 0 };
    UINT32 resp_sealSize = TBS_IN_OUT_BUF_SIZE_MAX;
    hr = Tbsip_Submit_Command(hContext, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL, cmd_seal, sizeof(cmd_seal), resp_seal, &resp_sealSize);
    u_assert_uint32_eq(hr, TBS_SUCCESS);
    debug_print("TPM2_CreatePrimary response: %02X %02X %02X %02X\n", resp_seal[0], resp_seal[1], resp_seal[2], resp_seal[3]);

    // Parse the TPM2_CreatePrimary response to get the handle of the created primary key
    UINT32 handle = 0;
    UINT32 offset = 10; // Skip tag (2 bytes) and commandSize (4 bytes)
    memcpy(&handle, resp_seal + offset, sizeof(UINT32));
    handle = ntohl(handle); // Convert handle from network byte order to host byte order
    */

/*
    // Read the public portion of the created primary key
    const BYTE cmd_readpublic[] = {
        0x80, 0x01,                   // tag: TPM_ST_SESSIONS
        0x00, 0x00, 0x00, 0x01,       // commandCode: TPM2_ReadPublic
        0x00, 0x00, 0x00, 0x00,       // name: TPMI_DH_OBJECT: handle of the public area
        (BYTE)(handle >> 24),         // handle: byte 1 (MSB)
        (BYTE)(handle >> 16),         // handle: byte 2
        (BYTE)(handle >> 8),          // handle: byte 3
        (BYTE)handle,                 // handle: byte 4 (LSB)
        0x00, 0x00, 0x00, 0x00,       // outPublic: TPMT_PUBLIC: public area
        0x00, 0x00, 0x00, 0x00,       // outPublic: nameAlg: algorithm used for computing the Name of the object
        0x00, 0x00, 0x00, 0x00,       // outPublic: objectType: type of the object
        0x00, 0x00, 0x00, 0x00,       // outPublic: attributes: attributes of the object
        0x00, 0x00, 0x00, 0x00,       // outPublic: authPolicy: authorization policy for the object
        0x00, 0x00, 0x00, 0x00,       // outPublic: parameters: parameters of the object
        0x00, 0x00,                   // outPublic: unique: unique field
        0x00, 0x00, 0x00, 0x00,       // size of outPublic: TPMT_PUBLIC
        0x00, 0x00, 0x00, 0x00,       // name: TPM2B_NAME: the Name of the object
        0x00, 0x00,                   // size of name: TPM2B_NAME
        0x00, 0x00, 0x00, 0x00,       // qualifiedName: TPM2B_NAME: qualified Name of the object
        0x00, 0x00                    // size of qualifiedName: TPM2B_NAME
    };

    BYTE resp_readpublic[TBS_IN_OUT_BUF_SIZE_MAX] = { 0 };
    UINT32 resp_readpublicSize = TBS_IN_OUT_BUF_SIZE_MAX;
    hr = Tbsip_Submit_Command(hContext, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL, cmd_readpublic, sizeof(cmd_readpublic), resp_readpublic, &resp_readpublicSize);
    u_assert_uint32_eq(hr, TBS_SUCCESS);
    debug_print("TPM2_ReadPublic response: %02X %02X %02X %02X\n", resp_readpublic[0], resp_readpublic[1], resp_readpublic[2], resp_readpublic[3]);
*/

    // Unseal the random with the TPM2
    const BYTE inPublic[] = {
        0x00, 0x00, 0x00, 0x04, // inPublic: size of the TPM2B_PUBLIC structure
        0x00, 0x00, 0x00, 0x00, // inPublic: TPM2B_PUBLIC: size
        0x00, 0x00, 0x00, 0x00, // inPublic: TPM2B_PUBLIC: type
        0x00, 0x00, 0x00, 0x00  // inPublic: TPM2B_PUBLIC: nameAlg
    };

    const BYTE cmd_unseal[] = {
        0x80, 0x01,                         // tag: TPM_ST_SESSIONS
        0x00, 0x00, 0x00, 0x18,             // commandSize: size of the entire command byte array
        0x00, 0x00, 0x01, 0x5E,             // commandCode: TPM2_CC_Unseal
        0x00, 0x00, 0x00, 0x04,             // inPublic: size of the TPM2B_PUBLIC structure
        inPublic[0], inPublic[1], inPublic[2], inPublic[3], // inPublic: TPM2B_PUBLIC: size
        inPublic[4], inPublic[5], inPublic[6], inPublic[7], // inPublic: TPM2B_PUBLIC: type
        inPublic[8], inPublic[9], inPublic[10], inPublic[11] // inPublic: TPM2B_PUBLIC: nameAlg
    };

    BYTE resp_unseal[TBS_IN_OUT_BUF_SIZE_MAX] = { 0 };
    UINT32 resp_unsealSize =  TBS_IN_OUT_BUF_SIZE_MAX;
    hr = Tbsip_Submit_Command(hContext, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL, cmd_unseal, sizeof(cmd_unseal), resp_unseal, &resp_unsealSize);
    u_assert_uint32_eq (hr, TBS_SUCCESS);

    // Check unsealed random
    BYTE* unsealedData = &resp_unseal[10]; // Unsealed data starts from the 10th byte of the response buffer
    int unsealedDataSize = resp_unsealSize - 10; // Size of unsealed data is the remaining bytes in the response buffer

    u_assert_mem_eq (unsealedData, resp_random + 12, 0x20);

#endif

}
