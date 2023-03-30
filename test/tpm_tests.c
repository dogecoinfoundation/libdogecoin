/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2023 edtubbs                                         *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "utest.h"

#include <dogecoin/utils.h>
#include <stdlib.h>

#ifdef _WIN32
#ifndef WINVER
#define WINVER 0x0600
#endif
#include <windows.h>
#include <tbs.h>

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
    const BYTE cmd[] = {
        0x80, 0x01,             // tag: TPM_ST_SESSIONS
        0x00, 0x00, 0x00, 0x0C, // commandSize: size of the entire command byte array
        0x00, 0x00, 0x01, 0x7B, // commandCode: TPM2_CC_GetRandom
        0x00, 0x20              // parameter: 32 bytes
    };
    BYTE resp[TBS_IN_OUT_BUF_SIZE_MAX] = { 0 };
    UINT32 respSize =  TBS_IN_OUT_BUF_SIZE_MAX;
    hr = Tbsip_Submit_Command(hContext, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL, cmd, sizeof(cmd), resp, &respSize);
    u_assert_uint32_eq (hr, TBS_SUCCESS);
    char* rand_hex;
    rand_hex = utils_uint8_to_hex(&resp[12], 0x20);
    debug_print ("TPM2_CC_GetRandom response: %s\n", rand_hex);

#endif

}
