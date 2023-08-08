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

/* no file number specified */
#define NO_FILE -1

/* default file number to use for storage */
#define DEFAULT_FILE 0

/* number of files (per object type) to use for storage */
#define MAX_FILES 1000

/* define test file number */
#define TEST_FILE 999

/* format string for mnemonic object names */
/* e.g. mnemonic 000, mnemonic 001, mnemonic 002, etc. */
#define MNEMONIC_OBJECT_NAME_FORMAT L"dogecoin_mnemonic_%03d"

/* format string for seed object names */
/* e.g. seed 000, seed 001, seed 002, etc. */
#define SEED_OBJECT_NAME_FORMAT L"dogecoin_seed_%03d"

/* format string for HD node object names */
/* e.g. master 000, master 001, master 002, etc. */
#define HDNODE_OBJECT_NAME_FORMAT L"dogecoin_master_%03d"

/* Encrypt a BIP32 seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_encrypt_seed_with_tpm (const SEED seed, const size_t size, const int file_num, const dogecoin_bool overwrite);

/* Decrypt a BIP32 seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_seed_with_tpm (SEED seed, const int file_num);

/* Generate a BIP39 mnemonic and encrypt it with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_mnemonic_encrypt_with_tpm(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite, const char* lang, const char* space, const char* words);

/* Decrypt a BIP39 mnemonic with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_mnemonic_with_tpm(MNEMONIC mnemonic, const int file_num);

/* Generate a BIP32 HD node and encrypt it with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_hdnode_encrypt_with_tpm(dogecoin_hdnode* out, const int file_num, const dogecoin_bool overwrite);

/* Decrypt a BIP32 HD node object with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_hdnode_with_tpm(dogecoin_hdnode* out, const int file_num);

/* List all encryption keys in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_list_encryption_keys_in_tpm(wchar_t* names[], size_t* count);

/* Generate a 256-bit random english mnemonic with the TPM */
LIBDOGECOIN_API dogecoin_bool generateRandomEnglishMnemonicTPM(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_SEAL_H__
