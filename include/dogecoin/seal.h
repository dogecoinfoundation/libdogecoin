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

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#include <ncrypt.h>
#endif

LIBDOGECOIN_BEGIN_DECL

/*
 * Defines
 */
#define BCRYPT_PCP_KEY_MAGIC 'MPCP' // Platform Crypto Provider Magic

#define PCPTYPE_TPM20 (0x00000002)

/*
 * Type definitions
 */
typedef struct PCP_KEY_BLOB_WIN8
{
    DWORD magic; // BCRYPT_PCP_KEY_MAGIC
    DWORD cbHeader; // Size of the header structure
    DWORD pcpType; // TPM type
    DWORD flags; // PCP_KEY_FLAGS_WIN8 Key flags
    ULONG cbPublic; // Size of Public key
    ULONG cbPrivate; // Size of Private key blob
    ULONG cbMigrationPublic; // Size of Public migration authorization object
    ULONG cbMigrationPrivate; // Size of Private migration authorization object
    ULONG cbPolicyDigestList; // Size of List of policy digest branches
    ULONG cbPCRBinding; // Size of PCR binding mask
    ULONG cbPCRDigest; // Size of PCR binding digest
    ULONG cbEncryptedSecret; // Size of hostage import symmetric key
    ULONG cbTpm12HostageBlob; // Size of hostage import private key
    ULONG pcrAlgId;
} PCP_KEY_BLOB_WIN8, *PPCP_KEY_BLOB_WIN8;

typedef struct {
    USHORT blobSize;
    USHORT keySize;
} ECDSAPublicKeyHeader;

typedef struct {
    USHORT blobSize;
    USHORT keySize;
} ECDSAPrivateKeyHeader;

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
LIBDOGECOIN_API dogecoin_bool dogecoin_unseal_hdnode_with_tpm (dogecoin_hdnode* hdnode, const PASSPHRASE passphrase);

/* Generate a BIP39 mnemonic in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_mnemonic_in_tpm(MNEMONIC mnemonic, const wchar_t* name, const dogecoin_bool overwrite);

/* Export a BIP39 mnemonic from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_export_mnemonic_from_tpm(MNEMONIC mnemonic, const wchar_t* name);

/* Generate a BIP32 seed in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_seed_in_tpm(SEED seed, const wchar_t* name, const dogecoin_bool overwrite);

/* Export a BIP32 seed from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_export_seed_from_tpm(const wchar_t* name, SEED seed);

/* Erase a BIP32 seed from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_erase_seed_from_tpm(const wchar_t* name);

/* Generate a BIP32 HD node object in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_hdnode_in_tpm(dogecoin_hdnode* out, const wchar_t* name, const dogecoin_bool overwrite);

/* Export a BIP32 HD node object from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_export_hdnode_from_tpm(dogecoin_hdnode* out, const wchar_t* name);

/* Erase an object from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_erase_object_from_tpm(const wchar_t* name);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_SEAL_H__
