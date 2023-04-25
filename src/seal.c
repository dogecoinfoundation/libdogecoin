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

#include <dogecoin/bip32.h>
#include <dogecoin/bip39.h>
#include <dogecoin/ecc.h>
#include <windows.h>
#include <bcrypt.h>
#include <ncrypt.h>


/**
 * @brief This function seals the seed with Trusted Platform Module (TPM)
 *
 * Seal the seed with TPM
 *
 * @param seed The seed to seal within the TPM.
 * @return 0 if the key is derived successfully, -1 otherwise.
 */
/* Seal a seed with the TPM */
int dogecoin_seal_seed(const BYTE* seed)
{
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hKey;
    SECURITY_STATUS status;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys
    DWORD cbKey = 64; // Change this to the actual length of the key in bytes
    NCRYPT_KEY_BLOB_HEADER *pbCipherBlob;
    pbCipherBlob = (NCRYPT_KEY_BLOB_HEADER *) malloc(cbKey);
    pbCipherBlob->dwMagic = NCRYPT_CIPHER_KEY_BLOB_MAGIC;
    pbCipherBlob->cbSize += sizeof(NCRYPT_KEY_BLOB_HEADER);
    pbCipherBlob->cbAlgName = 0;
    pbCipherBlob->cbKeyData = seed;

    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open storage provider (0x%08x)\n", status);
        return -1;
    }

    status = NCryptImportKey(hProvider, NULL, NCRYPT_CIPHER_KEY_BLOB, NULL, &hKey, (PBYTE) pbCipherBlob, sizeof(pbCipherBlob), 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to import key (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return -1;
    }

    // Finalize the key and close the provider
    status = NCryptFinalizeKey(hKey, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to finalize key (0x%08x)\n", status);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return -1;
    }

    NCryptFreeObject(hKey);
    NCryptFreeObject(hProvider);
    return 0;
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
// Function to unseal a Bip32 seed from the TPM
int dogecoin_unseal_seed(BYTE* seed)
{
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
        return -1;
    }

    // Open the key in the TPM storage provider
    status = NCryptOpenKey(hProv, &hKey, NULL, 0, dwFlags);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to open key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProv);
        return -1;
    }

    // Export the key as a BLOB containing the Bip32 seed
    status = NCryptExportKey(hKey, NULL, NCRYPT_CIPHER_KEY_BLOB, NULL, seed, dwBlobLen, &dwBlobLen, 0);
    if (status != ERROR_SUCCESS)
    {
        printf("Error: Failed to export key from TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProv);
        return -1;
    }

    // Close the key and the provider
    NCryptFreeObject(hKey);
    NCryptFreeObject(hProv);

    return 0;
}




/**
 * @brief This function generates a private and public
 * keypair along with chain_code for a hierarchical 
 * deterministic wallet. This is derived from a seed 
 * which usually consists of 12 random mnemonic words.
 * 
 * @param out The master node which stores the generated data.
 * 
 * @return Returns true if the keypair and chain_code are generated successfully, false otherwise.
 */
dogecoin_bool dogecoin_hdnode_from_tpm(dogecoin_hdnode* out)
{
    // Initialize variables
    dogecoin_mem_zero(out, sizeof(dogecoin_hdnode));
    out->depth = 0;
    out->fingerprint = 0x00000000;
    out->child_num = 0;

    uint8_t I[DOGECOIN_ECKEY_PKEY_LENGTH + DOGECOIN_BIP32_CHAINCODE_SIZE];
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
    BYTE seed[64] = { 0xFF };

    NCRYPT_KEY_BLOB_HEADER header;
    header.dwMagic = NCRYPT_CIPHER_KEY_BLOB_MAGIC;
    header.cbAlgName = 0;
    header.cbKeyData = &seed;
    header.cbSize = sizeof(NCRYPT_KEY_BLOB_HEADER);

    status = NCryptCreatePersistedKey(hProvider, &hKey, NCRYPT_ECDSA_P256_ALGORITHM, L"dogecoin seed", 0, dwFlags);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hProvider);
        return false;
    }

    // Make the key exportable
/*	DWORD dwExportPolicy = NCRYPT_ALLOW_EXPORT_FLAG;
    status = NCryptSetProperty(hKey, NCRYPT_EXPORT_POLICY_PROPERTY, (PBYTE)&dwExportPolicy, sizeof(dwExportPolicy), NCRYPT_PERSIST_FLAG);
    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hProvider);
        return false;
    }
*/

    // Import the Bip32 seed into the TPM
/*    status = NCryptImportKey(hProvider, NULL, NCRYPT_OPAQUETRANSPORT_BLOB, NULL, &hKey, (PBYTE) seed, 64, 0);

	if (status != ERROR_SUCCESS) {
        printf("Error: Failed to import key (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return -1;
    }
*/

    // Generate a key pair
    status = NCryptFinalizeKey(hKey, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Export the private key
    DWORD keyBlobLength;
    status = NCryptExportKey(hKey, NULL, NCRYPT_CIPHER_KEY_BLOB, NULL, NULL, 0, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    NCRYPT_KEY_BLOB_HEADER* keyBlob = (BYTE*) malloc(keyBlobLength);

    if (!keyBlob) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    status = NCryptExportKey(hKey, NULL, BCRYPT_ECCFULLPUBLIC_BLOB, NULL, keyBlob, keyBlobLength, &keyBlobLength, 0);

    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        free(keyBlob);
        return false;
    }

    // Prepare the KDF salt parameter as a BCryptBuffer array
    BCryptBuffer salt_param = { KDF_HASH_ALGORITHM, sizeof("Bitcoin seed") - 1, (PBYTE)"Bitcoin seed" };

    params.cBuffers = 1;
    params.pBuffers = &salt_param;
    params.ulVersion = BCRYPTBUFFER_VERSION;
/*	NCryptBuffer buffer[] = {
		{cbLabel, KDF_LABEL, pbLabel},
		{cbContext, KDF_CONTEXT, pbContext},
		{sizeof(NCRYPT_SHA256_ALGORITHM), KDF_HASH_ALGORITHM, NCRYPT_SHA256_ALGORITHM},
	};
	NCryptBufferDesc bufferDesc = {NCRYPTBUFFER_VERSION, ARRAYSIZE(buffer), buffer};
*/
    DWORD pcbResult;

    // Derive a new key from the persisted key using the KDF salt and a null pointer for the secret agreement handle
    status = NCryptKeyDerivation(hKey, &params, I, DOGECOIN_ECKEY_PKEY_LENGTH + DOGECOIN_BIP32_CHAINCODE_SIZE, &pcbResult, 0);
    if (status != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }


    // Copy the private key into the hdnode
    memcpy_safe(out->private_key, I, DOGECOIN_ECKEY_PKEY_LENGTH);

    if (!dogecoin_ecc_verify_privatekey(out->private_key)) {
        dogecoin_mem_zero(I, sizeof(I));
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Copy the chain code into the hdnode
    memcpy_safe(out->chain_code, I + DOGECOIN_ECKEY_PKEY_LENGTH, DOGECOIN_BIP32_CHAINCODE_SIZE);

    // Fill in the public key for the hdnode
    dogecoin_hdnode_fill_public_key(out);

    // Free the key and provider objects and zero out I
    NCryptFreeObject(hKey);
    NCryptFreeObject(hProvider);
    dogecoin_mem_zero(I, sizeof(I));
    
    return true;
/*
    DWORD dwFlags = NCRYPT_OVERWRITE_KEY_FLAG | NCRYPT_ALLOW_EXPORT_FLAG; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys
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

    status = NCryptExportKey(hKey, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, keyBlob + sizeof(BCRYPT_ECCKEY_BLOB), dwBlobLen - sizeof(BCRYPT_ECCKEY_BLOB), &dwBlobLen, 0);
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
    return true;*/

}


