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

#include <dogecoin/base58.h>
#include <dogecoin/bip32.h>
#include <dogecoin/bip39.h>
#include <dogecoin/ecc.h>
#include <dogecoin/eckey.h>
#include <dogecoin/seal.h>
#include <dogecoin/utils.h>

#ifdef _WIN32
#include <windows.h>
#include <tbs.h>
#include <bcrypt.h>
#include <ncrypt.h>
#endif

#ifndef WINVER
#define WINVER 0x0600
#endif


/**
 * @brief This function seals the seed with Trusted Platform Module (TPM)
 *
 * Seal the seed with TPM
 *
 * @param seed The seed to seal within the TPM.
 * @return 0 if the key is derived successfully, -1 otherwise.
 */
/* Seal a seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_seal_seed(const SEED seed)
{
#ifdef _WIN32

    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hKey;
    SECURITY_STATUS status;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys
    DWORD cbKey = 64; // Change this to the actual length of the key in bytes
    PCP_KEY_BLOB_WIN8* header = malloc(sizeof(PCP_KEY_BLOB_WIN8) + 120 + 96 + 32);
    header->magic = BCRYPT_PCP_KEY_MAGIC;
    header->cbHeader = sizeof(PCP_KEY_BLOB_WIN8);
    header->pcpType = PCPTYPE_TPM20;
    header->flags = 31;
    header->cbPublic = 120;
    header->cbPrivate = 96;
    header->cbMigrationPublic = 0;
    header->cbMigrationPrivate = 0;
    header->cbPolicyDigestList = 0;
    header->cbPCRBinding = 0;
    header->cbPCRDigest = 0;
    header->cbEncryptedSecret = 0;
    header->cbTpm12HostageBlob = 32;
    header->pcrAlgId = 0;

    BYTE* public = malloc(header->cbPublic);
    ECDSAPublicKeyHeader* publicKey = (ECDSAPublicKeyHeader*) public;

    CryptStringToBinary("AHYAIwALAAQEcgAgnf/L82w4OuaZ+5ho3G3LidcVOIS+KAOSLBJBWL+tIq4AEAAQAAMAEAAg4rReSD8nHN/8xZrsUv13gU6p6vnlO7+RrLr8MnaZMjcAIGOEwnhfZwKk571j3CzQyM/4WyiOo/o7qOVU4VXgLC/h", 0, CRYPT_STRING_BASE64, public, &header->cbPublic, 0, 0);

    BYTE* private = malloc(header->cbPrivate);

    CryptStringToBinary("AF4AIC+cUaX14ATiUtsqWe0sudbyMCFoQu3UeVbnqshg3O0nABDK+JhRfdoZX+CftWv9FaSNP/OXrapdn15aK9NgFnpxl06jimKauIafXvJFAtfD4IYBQ1cRpaML4pkn", 0, CRYPT_STRING_BASE64, private, &header->cbPrivate, 0, 0);

    // Copy the public key into the blob
    memcpy((BYTE*) header + header->cbHeader, public, header->cbPublic);

    // Copy the private key into the blob
    memcpy((BYTE*) header + header->cbHeader + header->cbPublic, private, header->cbPrivate);

    // Copy the seed into the blob
    memcpy((BYTE*) header + header->cbHeader + header->cbPublic + header->cbPrivate, (BYTE*) &header->cbTpm12HostageBlob, 2);
    memcpy((BYTE*) header + header->cbHeader + header->cbPublic + header->cbPrivate + 2, seed, header->cbTpm12HostageBlob);

//    memcpy((BYTE*)header + header->cbHeader + header->cbPublic, seed, 64);
    DWORD blobSize = sizeof(PCP_KEY_BLOB_WIN8) + header->cbPublic + header->cbPrivate + header->cbTpm12HostageBlob;

    // Open the TPM provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM provider (0x%08x)\n", status);
        return false;
    }

    // Create a persisted key object with the specified key name and algorithm identifier
    status = NCryptCreatePersistedKey(hProvider, &hKey, BCRYPT_ECDSA_P256_ALGORITHM, L"dogecoin seal", 0, NCRYPT_OVERWRITE_KEY_FLAG);
    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hProvider);
        return false;
    }
/*
    status = NCryptFinalizeKey(hKey, 0);
    if (status != ERROR_SUCCESS) {
        printf("Error: Failed to finalize key import (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the private key
    DWORD keyBlobLength;
    status = NCryptExportKey(hKey, NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, NULL, 0, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    PCP_KEY_BLOB_WIN8* keyBlob = (BYTE*) malloc(keyBlobLength);

    if (!keyBlob) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    status = NCryptExportKey(hKey, NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, keyBlob, keyBlobLength, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        free(keyBlob);
        return false;
    }
    */
/*
    // Generate secp256r1 key using the seed as private key
    BCRYPT_ALG_HANDLE hAlg;
    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0);
    status = BCryptGenerateKeyPair(hAlg, &hKey, 256, 0);
    status = BCryptSetProperty(hKey, BCRYPT_ECDSA_PRIVATE_P256_MAGIC, seed, 32, 0);

    // Export public and private keys to BCRYPT_OPAQUE_KEY_BLOB
    DWORD cbBlob;
    status = BCryptExportKey(hKey, NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, 0, &cbBlob, 0);
    PBYTE pbBlob = (PBYTE)malloc(cbBlob);
    status = BCryptExportKey(hKey, NULL, BCRYPT_OPAQUE_KEY_BLOB, pbBlob, cbBlob, &cbBlob, 0);
*/
    // Import the private key from the seed into the persisted key object
    status = NCryptSetProperty(
        hKey,
        BCRYPT_OPAQUE_KEY_BLOB,
        (PBYTE) header,
        blobSize,
        NCRYPT_PERSIST_FLAG
    );

    if (status != ERROR_SUCCESS) {
        printf("Error: Failed to set imported key BLOB property (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    status = NCryptFinalizeKey(hKey, 0);
    if (status != ERROR_SUCCESS) {
        printf("Error: Failed to finalize key import (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the private key
    DWORD keyBlobLength;
    status = NCryptExportKey(hKey, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, NULL, 0, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    PCP_KEY_BLOB_WIN8* keyBlob = (PCP_KEY_BLOB_WIN8*) malloc(keyBlobLength);

    if (!keyBlob) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    status = NCryptExportKey(hKey, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, (PBYTE) keyBlob, keyBlobLength, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        free(keyBlob);
        return false;
    }
/*
    // Import the private key from the dogecoin_hdnode into the persisted key object
    BYTE privateKeyBlob[48];
    PBYTE* pPrivateKeyBlob = privateKeyBlob;
    memcpy(pPrivateKeyBlob, seed, 32);
    pPrivateKeyBlob += 32;
    DWORD privateKeyBlobLength = sizeof(privateKeyBlob);
    status = NCryptImportKey(hProvider, NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL,
                            &hKey, pPrivateKeyBlob, privateKeyBlobLength, 0);
    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Import the public key from the dogecoin_hdnode into the persisted key object
    BYTE publicKeyBlob[88];
    BYTE* pPublicKeyBlob = publicKeyBlob;
    memcpy(pPublicKeyBlob, seed, 64);
    pPublicKeyBlob += 64;
    memcpy(pPublicKeyBlob, seed, 16);
    pPublicKeyBlob += 16;
    DWORD publicKeyBlobLength = sizeof(publicKeyBlob);
    status = NCryptImportKey(hProvider, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL,
                            &hKey, publicKeyBlob, publicKeyBlobLength, 0) != ERROR_SUCCESS;
    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }
*/

/*
    // Allocate memory for the key blob
    pbBlob = (PBYTE)malloc(cbHeaderSize + cbPublicBlob + cbPrivateBlob + cbName);
    if (pbBlob == NULL)
    {
        printf("Error: Failed to allocate memory for key blob\n");
        NCryptFreeObject(hProvider);
        return false;
    }

    // Populate the blob header
    pBlobHeader = (NCRYPT_TPM_LOADABLE_KEY_BLOB_HEADER*)pbBlob;
    pBlobHeader->magic = NCRYPT_TPM_LOADABLE_KEY_BLOB_MAGIC;
    pBlobHeader->cbHeader = cbHeaderSize;
    pBlobHeader->cbPublic = cbPublicBlob;
    pBlobHeader->cbPrivate = cbPrivateBlob;
    pBlobHeader->cbName = cbName;

    // Copy the public key into the blob
    memcpy(pbBlob + cbHeaderSize, seed, cbPublicBlob);

    // Copy the private key into the blob
    memcpy(pbBlob + cbHeaderSize + cbPublicBlob, seed, cbPrivateBlob);

    // Copy the key name into the blob
   memcpy(pbBlob + cbHeaderSize + cbPublicBlob + cbPrivateBlob, "dogecoin", cbName);

    // Import the key into TPM
    status = NCryptImportKey(hProvider,
                            NULL,
                            NCRYPT_TPM_LOADABLE_KEY_BLOB,
                            NULL,
                            &hKey,
                            pbBlob,
                            cbHeaderSize + cbPublicBlob + cbPrivateBlob + cbName,
                            dwFlags);

    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to import key (0x%08x)\n", status);
        free(pbBlob);
        NCryptFreeObject(hProvider);
        return false;
    }
*/

/*
    PBYTE exportPolicy = (PBYTE) NCRYPT_ALLOW_EXPORT_FLAG;
    status = NCryptSetProperty(hKey, NCRYPT_EXPORT_POLICY_PROPERTY, (PBYTE)&exportPolicy, sizeof(DWORD), 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to set property (0x%08x)\n", status);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    status = NCryptFinalizeKey(hKey, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to finalize key (0x%08x)\n", status);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }
*/

    NCryptFreeObject(hKey);
    NCryptFreeObject(hProvider);
    free(keyBlob);

    return true;
#else
    return false;
#endif

}

/**
 * @brief Unseal a seed with the TPM
 *
 * Unseal the seed with the TPM
 *
 * @param seed The seed sealed within the TPM
 * @return 0 if the seed is unsealed successfully, -1 otherwise.
 */
/* Unseal a seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_unseal_seed(SEED seed)
{
#ifdef _WIN32

    NCRYPT_PROV_HANDLE hProv;
    NCRYPT_KEY_HANDLE hKey;
    SECURITY_STATUS status;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys
    DWORD dwBlobLen = 64; // Size of the Bip32 seed in bytes

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProv, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Open the key in the TPM storage provider
    status = NCryptOpenKey(hProv, &hKey, NULL, 0, dwFlags);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProv);
        return false;
    }

    // Export the key as a BLOB containing the Bip32 seed
    status = NCryptExportKey(hKey, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, seed, dwBlobLen, &dwBlobLen, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to export key from TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProv);
        return false;
    }

    // Close the key and the provider
    NCryptFreeObject(hKey);
    NCryptFreeObject(hProv);

    return true;
#else
    return false;
#endif

}

/* Seal a mnemonic with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_seal_mnemonic (const MNEMONIC mnemonic, const PASSPHRASE passphrase)
{
    return true;
}

/* Unseal a mnemonic with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_unseal_mnemonic (MNEMONIC mnemonic, PASSPHRASE passphrase)
{
    return true;
}

/* Seal a BIP32 seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_seal_seed_with_tpm (const SEED seed)
{
    return true;
}

/* Unseal a BIP32 seed with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_unseal_seed_with_tpm (SEED seed)
{
    return true;
}

/* Seal a BIP32 HD node object with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_seal_hdnode_with_tpm (const dogecoin_hdnode* hdnode, const PASSPHRASE passphrase)
{
    return true;
}

/* Unseal a BIP32 HD node object with the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_unseal_hdnode_with_tpm (const PASSPHRASE passphrase, dogecoin_hdnode* hdnode)
{
    return true;
}

/* Generate a BIP39 mnemonic in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_mnemonic_in_tpm(MNEMONIC mnemonic, const wchar_t* name, const dogecoin_bool overwrite)
{
#ifdef _WIN32

    // Declare ncrypt variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hEntropy;
    DWORD dwFlags = NCRYPT_OVERWRITE_KEY_FLAG; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // If the overwrite flag is set, delete the existing entropy
    if (overwrite)
    {
        // Create a new entropy in the TPM storage provider
        status = NCryptCreatePersistedKey(hProvider, &hEntropy, BCRYPT_ECDSA_P256_ALGORITHM, name, 0, dwFlags);

        if (status != ERROR_SUCCESS)
        {
            printf("Error: Failed to create entropy in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hProvider);
            return false;
        }

        // Generate a new entropy in the TPM storage provider
        status = NCryptFinalizeKey(hEntropy, 0);

        if (status != ERROR_SUCCESS)
        {
            printf("Error: Failed to finalize entropy in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hEntropy);
            NCryptFreeObject(hProvider);
            return false;
        }
    }
    else
    {
        // Open the existing entropy in the TPM storage provider
        status = NCryptOpenKey(hProvider, &hEntropy, name, 0, dwFlags);

        if (status != ERROR_SUCCESS)
        {
            printf("Error: Failed to open entropy in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hProvider);
            return false;
        }
    }

    // Export the entropy from the TPM storage provider to allocate memory for the entropy BLOB
    DWORD entropyBlobLength = 0;
    status = NCryptExportKey(hEntropy, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, NULL, 0, &entropyBlobLength, 0);
    //status = NCryptExportKey(hEntropy, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, NULL, 0, &entropyBlobLength, 0);

    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to export entropy from TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hEntropy);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Allocate memory for the entropy BLOB
    PCP_KEY_BLOB_WIN8* entropyBlob = (PCP_KEY_BLOB_WIN8*) malloc(entropyBlobLength);

    if (entropyBlob == NULL)
    {
        printf("Error: Failed to allocate memory for entropy BLOB\n");
        NCryptFreeObject(hEntropy);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the entropy from the TPM storage provider
    status = NCryptExportKey(hEntropy, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, (PBYTE) entropyBlob, entropyBlobLength, &entropyBlobLength, 0);
    //status = NCryptExportKey(hEntropy, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, NULL, 0, &entropyBlobLength, 0);

    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to export entropy from TPM storage provider (0x%08x)\n", status);
        free(entropyBlob);
        NCryptFreeObject(hEntropy);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Free the entropy and provider objects and zero out the entropy buffer
    NCryptFreeObject(hEntropy);
    NCryptFreeObject(hProvider);

    // Declare the mnemonic generation variables
    const char* lang = "eng";     /* default english (eng) */
    const char* space = " ";      /* default utf8 ( ) */
    const char* size = "256";     /* default 256 bits of entropy */
    const char* words = 0;        /* default no custom words (NULL) */
    char* entropy_out = 0;
    size_t mnemonic_size = 0;

    // Allocate memory for the entropy out
    entropy_out = malloc(sizeof(char) * MAX_ENTROPY_STRING_SIZE);
    if (entropy_out == NULL) {

        fprintf(stderr, "ERROR: Failed to allocate memory for mnemonic\n");
        return -1;
    }
    memset(entropy_out, '\0', MAX_ENTROPY_STRING_SIZE);

    // The entropy generated by the TPM is a secure pseudorandom
    // number and therefore suitable for use as a entropy
    // Convert the entropy to a hex string
    utils_bin_to_hex ((BYTE*) entropyBlob + entropyBlob->cbHeader + entropyBlob->cbPublic + sizeof(ECDSAPrivateKeyHeader), MAX_ENTROPY_BITS / 8, entropy_out);

    // Generate the mnemonic from the entropy
    if (dogecoin_generate_mnemonic (size, lang, space, entropy_out, words, NULL, &mnemonic_size, mnemonic) == -1) {

        fprintf(stderr, "ERROR: Failed to generate mnemonic\n");
        dogecoin_mem_zero(entropy_out, MAX_ENTROPY_STRING_SIZE);
        dogecoin_mem_zero(entropyBlob, entropyBlobLength);
        dogecoin_free(entropyBlob);
        dogecoin_free(entropy_out);
        return -1;
    }

    // Zero out the entropy buffer and free the entropy BLOB
    dogecoin_mem_zero(entropy_out, MAX_ENTROPY_STRING_SIZE);
    dogecoin_mem_zero(entropyBlob, entropyBlobLength);
    dogecoin_free(entropyBlob);
    dogecoin_free(entropy_out);

    return true;
#else
    return false;
#endif

}

/* Export a BIP39 mnemonic from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_export_mnemonic_from_tpm(const wchar_t* name, MNEMONIC mnemonic)
{
    return true;
}

/* Generate a BIP32 seed in the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_seed_in_tpm(SEED seed, const wchar_t* name, const dogecoin_bool overwrite)
{
#ifdef _WIN32

    // Generate a new master seed
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hSeed;
    DWORD dwFlags = NCRYPT_OVERWRITE_KEY_FLAG; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // If the overwrite flag is set, delete the existing seed
    if (overwrite)
    {
        // Create a new seed in the TPM storage provider
        status = NCryptCreatePersistedKey(hProvider, &hSeed, BCRYPT_ECDSA_P256_ALGORITHM, name, 0, dwFlags);

        if (status != ERROR_SUCCESS)
        {
            printf("Error: Failed to create seed in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hProvider);
            return false;
        }

        // Generate a new seed in the TPM storage provider
        status = NCryptFinalizeKey(hSeed, 0);

        if (status != ERROR_SUCCESS)
        {
            printf("Error: Failed to finalize seed in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hSeed);
            NCryptFreeObject(hProvider);
            return false;
        }
    }
    else
    {
        // Open the existing seed in the TPM storage provider
        status = NCryptOpenKey(hProvider, &hSeed, name, 0, dwFlags);

        if (status != ERROR_SUCCESS)
        {
            printf("Error: Failed to open seed in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hProvider);
            return false;
        }
    }

    // Export the seed from the TPM storage provider to allocate memory for the seed BLOB
    DWORD seedBlobLength = 0;
    status = NCryptExportKey(hSeed, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, NULL, 0, &seedBlobLength, 0);
    //status = NCryptExportKey(hSeed, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, NULL, 0, &seedBlobLength, 0);

    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to export seed from TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hSeed);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Allocate memory for the seed BLOB
    BYTE* seedBlob = (BYTE*)malloc(seedBlobLength);

    if (seedBlob == NULL)
    {
        printf("Error: Failed to allocate memory for seed BLOB\n");
        NCryptFreeObject(hSeed);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the seed from the TPM storage provider
    status = NCryptExportKey(hSeed, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, seedBlob, seedBlobLength, &seedBlobLength, 0);
    //status = NCryptExportKey(hSeed, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, NULL, 0, &seedBlobLength, 0);

    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to export seed from TPM storage provider (0x%08x)\n", status);
        free(seedBlob);
        NCryptFreeObject(hSeed);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Copy the seed from the BLOB into the seed buffer
    // The seed generated by the TPM is a secure pseudorandom
    // number and therefore suitable for use as a seed
    PCP_KEY_BLOB_WIN8* header = (PCP_KEY_BLOB_WIN8*) seedBlob;
    memcpy_safe(seed, (BYTE*) header + header->cbHeader + header->cbPublic + sizeof(ECDSAPrivateKeyHeader), DOGECOIN_ECKEY_PKEY_LENGTH);

    // Free the seed and provider objects and zero out the seed buffer
    NCryptFreeObject(hSeed);
    NCryptFreeObject(hProvider);
    dogecoin_mem_zero(seedBlob, seedBlobLength);
    free(seedBlob);

    return true;
#else
    return false;
#endif

}

/* Export a BIP32 seed from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_export_seed_from_tpm(const wchar_t* name, SEED seed)
{
#ifdef _WIN32

    // Export the master key
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hSeed;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Open the seed in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hSeed, name, 0, dwFlags);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open seed in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the seed from the TPM storage provider
    DWORD seedBlobLength;

    // Export the seed from the TPM storage provider to get the size of the seed BLOB
    status = NCryptExportKey(hSeed, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, NULL, 0, &seedBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hSeed);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Allocate memory for the seed BLOB
    PCP_KEY_BLOB_WIN8* seedBlob = (PCP_KEY_BLOB_WIN8*) malloc(seedBlobLength);

    if (!seedBlob) {
        NCryptFreeObject(hSeed);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the seed from the TPM storage provider
    status = NCryptExportKey(hSeed, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, (PBYTE) seedBlob, seedBlobLength, &seedBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hSeed);
        NCryptFreeObject(hProvider);
        free(seedBlob);
        return false;
    }

    // Copy the seed from the BLOB into the seed buffer
    // The seed generated by the TPM is a secure pseudorandom
    // number and therefore suitable for use as a seed
    PCP_KEY_BLOB_WIN8* header = (PCP_KEY_BLOB_WIN8*) seedBlob;
    memcpy_safe(seed, (BYTE*) header + header->cbHeader + header->cbPublic + sizeof(ECDSAPrivateKeyHeader), DOGECOIN_ECKEY_PKEY_LENGTH);

    // Free the seed and provider objects and zero out the seed buffer
    NCryptFreeObject(hSeed);
    NCryptFreeObject(hProvider);
    dogecoin_mem_zero(seedBlob, seedBlobLength);
    free(seedBlob);

    return true;
#else
    return false;
#endif

}

/* Erase a BIP32 seed from the TPM */
LIBDOGECOIN_API dogecoin_bool dogecoin_erase_seed_from_tpm(const wchar_t* name)
{
#ifdef _WIN32

    // Initialize variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hSeed;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Open the seed in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hSeed, name, 0, dwFlags);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open seed in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Delete the seed
    status = NCryptDeleteKey(hSeed, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hSeed);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Free the key and provider objects
    NCryptFreeObject(hSeed);
    NCryptFreeObject(hProvider);

    return true;
#else
    return false;
#endif

}

/**
 * @brief Generate a HD node object in the TPM
 *
 * Generate a HD node object in the TPM
 *
 * @param out The HD node object to generate
 * @param overwrite Whether or not to overwrite the existing HD node object
 * @return Returns true if the keypair and chain_code are generated successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_hdnode_in_tpm(dogecoin_hdnode* out, const wchar_t* name, dogecoin_bool overwrite)
{
#ifdef _WIN32

    // Initialize variables
    dogecoin_mem_zero(out, sizeof(dogecoin_hdnode));
    out->depth = 0;
    out->fingerprint = 0x00000000;
    out->child_num = 0;

    // Generate a new master key
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hKey;
    DWORD dwFlags = NCRYPT_OVERWRITE_KEY_FLAG; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // If the overwrite flag is set, delete the existing key
    if (overwrite)
    {
        // Create a new key in the TPM storage provider
        status = NCryptCreatePersistedKey(hProvider, &hKey, BCRYPT_ECDSA_P256_ALGORITHM, L"dogecoin master", 0, dwFlags);

        if (status != ERROR_SUCCESS)
        {
            printf("Error: Failed to create key in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hProvider);
            return false;
        }

        // Generate a new key pair in the TPM storage provider
        status = NCryptFinalizeKey(hKey, 0);

        if (status != ERROR_SUCCESS)
        {
            printf("Error: Failed to finalize key in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hKey);
            NCryptFreeObject(hProvider);
            return false;
        }
    }
    else
    {
        // Open the existing key in the TPM storage provider
        status = NCryptOpenKey(hProvider, &hKey, L"dogecoin master", 0, dwFlags);

        if (status != ERROR_SUCCESS)
        {
            printf("Error: Failed to open key in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hProvider);
            return false;
        }
    }

    // Export the key from the TPM storage provider to allocate memory for the key BLOB
    DWORD keyBlobLength = 0;
    status = NCryptExportKey(hKey, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, NULL, 0, &keyBlobLength, 0);
    //status = NCryptExportKey(hKey, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, NULL, 0, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to export key from TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Allocate memory for the key BLOB
    BYTE* keyBlob = (BYTE*)malloc(keyBlobLength);

    if (keyBlob == NULL)
    {
        printf("Error: Failed to allocate memory for key BLOB\n");
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the key from the TPM storage provider
    status = NCryptExportKey(hKey, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, keyBlob, keyBlobLength, &keyBlobLength, 0);
    //status = NCryptExportKey(hKey, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, NULL, 0, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to export key from TPM storage provider (0x%08x)\n", status);
        free(keyBlob);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Copy the key from the BLOB into the seed buffer
    // The key generated by the TPM is a secure pseudorandom
    // number and therefore suitable for use as a seed
    SEED seed = { 0 };
    PCP_KEY_BLOB_WIN8* header = (PCP_KEY_BLOB_WIN8*) keyBlob;
    memcpy_safe(seed, (BYTE*) header + header->cbHeader + header->cbPublic + sizeof(ECDSAPrivateKeyHeader), DOGECOIN_ECKEY_PKEY_LENGTH);

    // Derive the HD node from the seed
    dogecoin_hdnode_from_seed (seed, DOGECOIN_ECKEY_PKEY_LENGTH, out);

    // Free the key and provider objects and zero out the key buffer
    NCryptFreeObject(hKey);
    NCryptFreeObject(hProvider);
    dogecoin_mem_zero(seed, sizeof(seed));
    dogecoin_mem_zero(keyBlob, keyBlobLength);
    free(keyBlob);

    return true;
#else
    return false;
#endif

}

/**
 * @brief Export a HD node object from the TPM
 *
 * Export a HD node object from the TPM
 *
 * @param out The HD node object to export
 * @return Returns true if the keypair and chain_code are exported successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_export_hdnode_from_tpm(const wchar_t* name, dogecoin_hdnode* out)
{
#ifdef _WIN32

    // Initialize variables
    dogecoin_mem_zero(out, sizeof(dogecoin_hdnode));
    out->depth = 0;
    out->fingerprint = 0x00000000;
    out->child_num = 0;

    // Export the master key
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hKey;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Open the key in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hKey, L"dogecoin master", 0, dwFlags);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the key from the TPM storage provider
    DWORD keyBlobLength;

    // Export the key from the TPM storage provider to get the size of the key BLOB
    status = NCryptExportKey(hKey, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, NULL, 0, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Allocate memory for the key BLOB
    PCP_KEY_BLOB_WIN8* keyBlob = (PCP_KEY_BLOB_WIN8*) malloc(keyBlobLength);

    if (!keyBlob) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the key from the TPM storage provider
    status = NCryptExportKey(hKey, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_OPAQUE_KEY_BLOB, NULL, (PBYTE) keyBlob, keyBlobLength, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        free(keyBlob);
        return false;
    }

    // Copy the key from the BLOB into the seed buffer
    // The key generated by the TPM is a secure pseudorandom
    // number and therefore suitable for use as a seed
    SEED seed = { 0 };
    PCP_KEY_BLOB_WIN8* header = (PCP_KEY_BLOB_WIN8*) keyBlob;
    memcpy_safe(seed, (BYTE*) header + header->cbHeader + header->cbPublic + sizeof(ECDSAPrivateKeyHeader), DOGECOIN_ECKEY_PKEY_LENGTH);

    // Derive the HD node from the seed
    dogecoin_hdnode_from_seed (seed, DOGECOIN_ECKEY_PKEY_LENGTH, out);

    // Free the key and provider objects and zero out the key buffer
    NCryptFreeObject(hKey);
    NCryptFreeObject(hProvider);
    dogecoin_mem_zero(seed, sizeof(seed));
    dogecoin_mem_zero(keyBlob, keyBlobLength);
    free(keyBlob);

    return true;
#else
    return false;
#endif

}

/**
 * @brief Erase the master key from the TPM
 *
 * Erase the master key from the TPM
 * 
 * @return Returns true if the master key was erased successfully
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_erase_hdnode_from_tpm(const wchar_t name)
{
#ifdef _WIN32
    // Initialize variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hKey;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Open the key in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hKey, L"dogecoin master", 0, dwFlags);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Delete the key
    status = NCryptDeleteKey(hKey, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Free the key and provider objects
    NCryptFreeObject(hKey);
    NCryptFreeObject(hProvider);

    return true;
#else
    return false;
#endif

}

/**
 * @brief This function generates a private and public
 * keypair along with chain_code for a hierarchical
 * deterministic wallet. This is derived from a seed
 * which is generated by the TPM.
 *
 * @param out The master node which stores the generated data.
 *
 * @return Returns true if the keypair and chain_code are generated successfully, false otherwise.
 */
dogecoin_bool dogecoin_hdnode_from_tpm(dogecoin_hdnode* out)
{
#ifdef _WIN32

    // Initialize variables
    dogecoin_mem_zero(out, sizeof(dogecoin_hdnode));
    out->depth = 0;
    out->fingerprint = 0x00000000;
    out->child_num = 0;

    BCryptBufferDesc params;

    DWORD dwFlags = NCRYPT_OVERWRITE_KEY_FLAG; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Open a storage provider
    NCRYPT_PROV_HANDLE hProvider;
    SECURITY_STATUS status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);

    if (status != ERROR_SUCCESS) {
        return false;
    }

    // Create a key object
    NCRYPT_KEY_HANDLE hKey;
    SEED seed = { 0 };

    PCP_KEY_BLOB_WIN8* header = malloc(sizeof(PCP_KEY_BLOB_WIN8) + 120 + 96);
    header->magic = BCRYPT_PCP_KEY_MAGIC;
    header->cbHeader = sizeof(PCP_KEY_BLOB_WIN8);
    header->pcpType = PCPTYPE_TPM20;
    header->flags = 0;
    header->cbPublic = 120;
    header->cbPrivate = 96;
    header->cbMigrationPublic = 0;
    header->cbMigrationPrivate = 0;
    header->cbPolicyDigestList = 0;
    header->cbPCRBinding = 0;
    header->cbPCRDigest = 0;
    header->cbEncryptedSecret = 0;
    header->cbTpm12HostageBlob = 0;
    header->pcrAlgId = 0;

    // Import the test public key
    BYTE* public = malloc(header->cbPublic);
    CryptStringToBinary("AHYAIwALAAQEcgAgnf/L82w4OuaZ+5ho3G3LidcVOIS+KAOSLBJBWL+tIq4AEAAQAAMAEAAg4rReSD8nHN/8xZrsUv13gU6p6vnlO7+RrLr8MnaZMjcAIGOEwnhfZwKk571j3CzQyM/4WyiOo/o7qOVU4VXgLC/h", 0, CRYPT_STRING_BASE64, public, &header->cbPublic, 0, 0);

    // Import the test private key
    BYTE* private = malloc(header->cbPrivate);
    CryptStringToBinary("AF4AIC+cUaX14ATiUtsqWe0sudbyMCFoQu3UeVbnqshg3O0nABDK+JhRfdoZX+CftWv9FaSNP/OXrapdn15aK9NgFnpxl06jimKauIafXvJFAtfD4IYBQ1cRpaML4pkn", 0, CRYPT_STRING_BASE64, private, &header->cbPrivate, 0, 0);

    // Copy the public key into the blob
    memcpy((BYTE*) header + header->cbHeader, public, header->cbPublic);

    // Copy the private key into the blob
    memcpy((BYTE*) header + header->cbHeader + header->cbPublic, private, header->cbPrivate);

    // Calculate the size of the blob
    DWORD blobSize = sizeof(PCP_KEY_BLOB_WIN8) + header->cbPublic + header->cbPrivate;

    // Create the seed key
    status = NCryptCreatePersistedKey(hProvider, &hKey, BCRYPT_ECDSA_P256_ALGORITHM, L"dogecoin seed", 0, dwFlags);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hProvider);
        return false;
    }

    BCRYPT_ALG_HANDLE hAlg;
    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    NCryptAlgorithmName* algList = NULL;
    DWORD numAlgs = 0;

    status = NCryptEnumAlgorithms(hProvider, NCRYPT_SIGNATURE_OPERATION, &numAlgs, &algList, 0);
    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    BCRYPT_PROVIDER_NAME* provList = NULL;
    DWORD numProv = 0;

    status = BCryptEnumAlgorithms(BCRYPT_SIGNATURE_OPERATION, &numProv, &provList, 0);
    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hProvider);
        return false;
    }

    printf("Supported ECC curves:\n");
    for (DWORD i = 0; i < numAlgs; i++) {
        printf("  %S\n", algList[i].pszName);
    }

    // Set export policy to allow export of private key
    DWORD dwExportPolicy = NCRYPT_ALLOW_EXPORT_FLAG | NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG;
    status = NCryptSetProperty(hKey, NCRYPT_EXPORT_POLICY_PROPERTY, (PBYTE)&dwExportPolicy, sizeof(dwExportPolicy), 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to set export policy (0x%08x)\n", status);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Import the test blob into the TPM
    status = NCryptSetProperty(
        hKey,
        BCRYPT_OPAQUE_KEY_BLOB,
        (PBYTE) header,
        blobSize,
        NCRYPT_PERSIST_FLAG
    );

    if (status != ERROR_SUCCESS) {
        printf("Error: Failed to set imported key BLOB property (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Finalize the key
    status = NCryptFinalizeKey(hKey, 0);

    if (status != ERROR_SUCCESS) {
        printf("Error: Failed to finalize key import (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Copy the private key from the PCP_KEY_BLOB_WIN8 structure into the seed buffer
    // This works because the private key is randomly generated and the seed is the same size
    memcpy_safe(seed, (BYTE*) header + header->cbHeader + header->cbPublic + sizeof(ECDSAPrivateKeyHeader), DOGECOIN_ECKEY_PKEY_LENGTH);

    // Derive the master key from the seed
    dogecoin_hdnode_from_seed (seed, DOGECOIN_ECKEY_PKEY_LENGTH, out);

    // Free the key and provider objects and zero out the prive key buffer
    NCryptFreeObject(hKey);
    NCryptFreeObject(hProvider);

    return true;
}

dogecoin_bool dogecoin_hdnode_from_tpm_ecc (dogecoin_hdnode *out)
{
    // Create a new key in the TPM storage provider with BCRYPT_ECDSA_P256_ALGORITHM
    DWORD dwFlags = NCRYPT_OVERWRITE_KEY_FLAG; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys
    NCRYPT_PROV_HANDLE hProv;
    NCRYPT_KEY_HANDLE hKey;
    SECURITY_STATUS status;
    DWORD dwBlobLen = sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(BCRYPT_ECCPRIVATE_BLOB);

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProv, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Create a new key in the TPM storage provider with BCRYPT_ECDSA_P256_ALGORITHM
    status = NCryptCreatePersistedKey(hProv, &hKey, BCRYPT_ECDSA_P256_ALGORITHM, L"dogecoin_master", 0, dwFlags);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to create key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProv);
        return false;
    }

    // Finalize the key and close the provider
    status = NCryptFinalizeKey(hKey, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to finalize key (0x%08x)\n", status);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProv);
        return false;
    }

    // Export the private key as a BLOB
    BYTE* keyBlob = (BYTE*)malloc(dwBlobLen);
    if (!keyBlob) {
        printf("Error: Failed to allocate memory for key blob\n");
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProv);
        return false;
    }

    // Zero the memory of the keyBlob buffer to avoid reading uninitialized data
    memset(keyBlob, 0, dwBlobLen);

    // Cast the buffer to BCRYPT_ECCKEY_BLOB*
    BCRYPT_ECCKEY_BLOB* ecckeyBlob = (BCRYPT_ECCKEY_BLOB*)keyBlob;
    ecckeyBlob->dwMagic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC; // Set the magic value for BCRYPT_ECCKEY_BLOB
    ecckeyBlob->cbKey = 32; // Set the key size to 32 bytes (P-256 curve)

    status = NCryptExportKey(hKey, (NCRYPT_KEY_HANDLE) NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, keyBlob + sizeof(BCRYPT_ECCKEY_BLOB), dwBlobLen - sizeof(BCRYPT_ECCKEY_BLOB), &dwBlobLen, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to export private key from TPM storage provider (0x%08x)\n", status);
        free(keyBlob);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProv);
        return false;
    }

    memcpy_safe(out->private_key, keyBlob + sizeof(BCRYPT_ECCKEY_BLOB), DOGECOIN_ECKEY_PKEY_LENGTH);
    if (!dogecoin_ecc_verify_privatekey(out->private_key)) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProv);
        return false;
    }

    // Fill in the public key fields of the output HD node
    memcpy_safe(out->chain_code, keyBlob + DOGECOIN_ECKEY_PKEY_LENGTH, DOGECOIN_BIP32_CHAINCODE_SIZE);
    dogecoin_hdnode_fill_public_key(out);

    NCryptFreeObject(hKey);
    NCryptFreeObject(hProv);
    return true;
#else
    return false;
#endif

}


