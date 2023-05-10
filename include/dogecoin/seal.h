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

LIBDOGECOIN_API

/*
 * Defines
 */

/* no slot specified */
#define NO_SLOT -1

/* default slot to use for TPM storage */
#define DEFAULT_SLOT 0

/* number of slots (per object type) to use for TPM storage */
#define MAX_SLOTS 1000

/* define test slot */
#define TEST_SLOT 999

/* format string for mnemonic object names */
/* e.g. mnemonic 000, mnemonic 001, mnemonic 002, etc. */
#define MNEMONIC_OBJECT_NAME_FORMAT L"dogecoin mnemonic %03d"

/* format string for seed object names */
/* e.g. seed 000, seed 001, seed 002, etc. */
#define SEED_OBJECT_NAME_FORMAT L"dogecoin seed %03d"

/* format string for HD node object names */
/* e.g. master 000, master 001, master 002, etc. */
#define HDNODE_OBJECT_NAME_FORMAT L"dogecoin master %03d"

/* Seal a seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_seal_seed (const SEED seed);
/* return 0 (success), -1 (fail) */

/* Unseal a seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_unseal_seed (SEED seed);
/* return 0 (success), -1 (fail) */

/* Seal a mnemonic with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_seal_mnemonic (const MNEMONIC mnemonic, const PASSPHRASE passphrase);

/* Unseal a mnemonic with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_unseal_mnemonic (MNEMONIC mnemonic, PASSPHRASE passphrase);

/* Seal a BIP32 seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_seal_seed_with_tpm (const SEED seed);

/* Unseal a BIP32 seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_unseal_seed_with_tpm (SEED seed);

/* Seal a BIP32 HD node object with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_seal_hdnode_with_tpm (const dogecoin_hdnode* hdnode, const PASSPHRASE passphrase);

/* Unseal a BIP32 HD node object with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_unseal_hdnode_with_tpm (const PASSPHRASE passphrase, dogecoin_hdnode* hdnode);

/* List all objects in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_list_objects_in_tpm(wchar_t* names[], size_t* count);

/* List all the mnemonic objects in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_list_mnemonics_in_tpm(wchar_t* names[], size_t* count);

/* Generate a BIP39 mnemonic in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_mnemonic_in_tpm(MNEMONIC mnemonic, const int slot, const dogecoin_bool overwrite, const char* lang, const char* space, const char* words);

/* Export a BIP39 mnemonic from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_export_mnemonic_from_tpm(const int slot, MNEMONIC mnemonic, const char* lang, const char* space, const char* words);

/* Erase a BIP39 mnemonic from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_erase_mnemonic_from_tpm(const int slot);

/* Generate a BIP32 seed in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_seed_in_tpm(SEED seed, const int slot, const dogecoin_bool overwrite);

/* Export a BIP32 seed from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_export_seed_from_tpm(const int slot, SEED seed);

/* Erase a BIP32 seed from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_erase_seed_from_tpm(const int slot);

/* Generate a BIP32 HD node object in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_hdnode_in_tpm(dogecoin_hdnode* out, const int slot, const dogecoin_bool overwrite);

/* Export a BIP32 HD node object from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_export_hdnode_from_tpm(const int slot, dogecoin_hdnode* out);

/* Erase an hdnode from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_erase_hdnode_from_tpm(const int slot);

/* Generate a 256-bit random english mnemonic in the TPM */
LIBDOGECOIN_API int generateRandomEnglishMnemonicTPM(MNEMONIC mnemonic, const int slot, const dogecoin_bool overwrite);

/* Export an english mnemonic from the TPM */
LIBDOGECOIN_API int exportEnglishMnemonicTPM(const int slot, MNEMONIC mnemonic);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_SEAL_H__
