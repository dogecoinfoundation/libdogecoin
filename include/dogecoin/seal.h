/**********************************************************************
 * Copyright (c) 2023 edtubbs                                         *
 * Copyright (c) 2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef __LIBDOGECOIN_SEAL_H__
#define __LIBDOGECOIN_SEAL_H__

#include <dogecoin/dogecoin.h>
#include <dogecoin/bip32.h>
#include <dogecoin/bip39.h>

LIBDOGECOIN_BEGIN_DECL

/*
 * Defines
 */


/*
 * Type definitions
 */


/* Seal a seed with the TPM */
LIBDOGECOIN_API int dogecoin_seal_seed (const SEED seed);
/* return 0 (success), -1 (fail) */

/* Unseal a seed with the TPM */
LIBDOGECOIN_API int dogecoin_unseal_seed (SEED seed);
/* return 0 (success), -1 (fail) */

LIBDOGECOIN_API dogecoin_bool dogecoin_hdnode_from_tpm(dogecoin_hdnode* out);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_SEAL_H__
