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

#include <dogecoin/aes.h>
#include <dogecoin/base58.h>
#include <dogecoin/bip32.h>
#include <dogecoin/bip39.h>
#include <dogecoin/ecc.h>
#include <dogecoin/eckey.h>
#include <dogecoin/random.h>
#include <dogecoin/sha2.h>
#include <dogecoin/seal.h>
#include <dogecoin/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

#ifdef _MSC_VER
#include <win/winunistd.h>
#else
#include <unistd.h>
#endif

#if defined (_WIN64) && !defined(__MINGW64__) && defined(USE_TPM2)
#include <tbs.h>
#include <ncrypt.h>
#endif

#ifndef WINVER
#define WINVER 0x0600
#endif

/*
 * Defines
 */
#define RESP_RAND_OFFSET 12 // Offset to the random data in the TPM2_CC_GetRandom response
#define AES_KEY_SIZE 32
#define AES_IV_SIZE 16
#define SALT_SIZE 16
#define NAME_MAX_LEN 100
#define PASS_MAX_LEN 100
#define FILE_PATH_MAX_LEN 1000
#define PBKDF2_ITERATIONS 10000
#define ENCRYPTED_SEED_SIZE 64 // AES-256 CBC encrypted seed, no padding
#define ENCRYPTED_MNEMONIC_SIZE 768 // AES-256 CBC encrypted mnemonic, no padding

// Common format string for file numbering
#define FILE_NUM_FORMAT "%03d" // Used for Unix-like systems
#define FILE_NUM_FORMAT_W L"%03d" // Wide string version for Windows

// Base names for the items
#define BASE_NAME_MNEMONIC "dogecoin_mnemonic_"
#define BASE_NAME_MNEMONIC_W L"dogecoin_mnemonic_"
#define BASE_NAME_SEED "dogecoin_seed_"
#define BASE_NAME_SEED_W L"dogecoin_seed_"
#define BASE_NAME_MASTER "dogecoin_master_"
#define BASE_NAME_MASTER_W L"dogecoin_master_"

// Suffices for the items
#define SUFFIX_TPM "_tpm"
#define SUFFIX_TPM_W L"_tpm"
#define SUFFIX_SW "_sw"
#define SUFFIX_SW_W L"_sw"

// Directory path for storage
#define CRYPTO_DIR_PATH "./.store/"
#define CRYPTO_DIR_PATH_W L"store\\"

// TPM object names without encryption method suffix for Windows
#define MNEMONIC_TPM_OBJ_NAME_WIN BASE_NAME_MNEMONIC_W FILE_NUM_FORMAT_W
#define SEED_TPM_OBJ_NAME_WIN BASE_NAME_SEED_W FILE_NUM_FORMAT_W
#define MASTER_TPM_OBJ_NAME_WIN BASE_NAME_MASTER_W FILE_NUM_FORMAT_W

// Full file path and names for Windows (with TPM encryption method suffix)
#define MNEMONIC_TPM_FILE_NAME_WIN CRYPTO_DIR_PATH_W BASE_NAME_MNEMONIC_W FILE_NUM_FORMAT_W SUFFIX_TPM_W
#define SEED_TPM_FILE_NAME_WIN CRYPTO_DIR_PATH_W BASE_NAME_SEED_W FILE_NUM_FORMAT_W SUFFIX_TPM_W
#define MASTER_TPM_FILE_NAME_WIN CRYPTO_DIR_PATH_W BASE_NAME_MASTER_W FILE_NUM_FORMAT_W SUFFIX_TPM_W

// Full file path and names for Windows (with software encryption method suffix)
#define MNEMONIC_SW_FILE_NAME_WIN CRYPTO_DIR_PATH_W BASE_NAME_MNEMONIC_W FILE_NUM_FORMAT_W SUFFIX_SW_W
#define SEED_SW_FILE_NAME_WIN CRYPTO_DIR_PATH_W BASE_NAME_SEED_W FILE_NUM_FORMAT_W SUFFIX_SW_W
#define MASTER_SW_FILE_NAME_WIN CRYPTO_DIR_PATH_W BASE_NAME_MASTER_W FILE_NUM_FORMAT_W SUFFIX_SW_W

// Full file path and names for Unix-like systems (with software encryption method suffix)
#define MNEMONIC_SW_FILE_NAME CRYPTO_DIR_PATH BASE_NAME_MNEMONIC FILE_NUM_FORMAT SUFFIX_SW
#define SEED_SW_FILE_NAME CRYPTO_DIR_PATH BASE_NAME_SEED FILE_NUM_FORMAT SUFFIX_SW
#define MASTER_SW_FILE_NAME CRYPTO_DIR_PATH BASE_NAME_MASTER FILE_NUM_FORMAT SUFFIX_SW

/**
 * @brief Validates a file number
 *
 * Validates a file number to ensure it is within the valid range.
 *
 * @param[in] file_num The file number to validate
 * @return true if the file number is valid, false otherwise.
 */
dogecoin_bool fileValid (const int file_num)
{

    // Check if the file number is valid
    if (file_num < DEFAULT_FILE || file_num > TEST_FILE)
    {
        return false;
    }
    return true;

}

/**
 * @brief Encrypts a seed using the TPM
 *
 * Encrypts a seed using the TPM and stores the encrypted seed in a file.
 *
 * @param[in] seed The seed to encrypt
 * @param[in] size The size of the seed
 * @param[in] file_num The file number to encrypt the seed for
 * @param[in] overwrite Whether or not to overwrite an existing seed
 * @return true if the seed was encrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_encrypt_seed_with_tpm(const SEED seed, const size_t size, const int file_num, const dogecoin_bool overwrite)
{
#if defined (_WIN64) && !defined(__MINGW64__) && defined(USE_TPM2)

    // Validate the input parameters
    if (seed == NULL)
    {
        fprintf(stderr, "ERROR: Invalid seed\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Format the name of the encrypted seed file
    wchar_t filename[FILE_PATH_MAX_LEN] = {0};
    swprintf(filename, sizeof(filename), SEED_TPM_FILE_NAME_WIN, file_num);

    // Check if the file already exists and if not, prompt for overwriting
    if (!overwrite && _waccess(filename, F_OK) != -1)
    {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }

    // Declare variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hEncryptionKey;
    DWORD cbResult;
    PBYTE pbOutput = NULL;
    DWORD cbOutput = 0;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Format the name of the encrypted seed object
    wchar_t name[NAME_MAX_LEN] = {0};
    swprintf(name, sizeof(name), SEED_TPM_OBJ_NAME_WIN, file_num);

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Create a new persistent encryption key
    status = NCryptCreatePersistedKey(hProvider, &hEncryptionKey, NCRYPT_RSA_ALGORITHM, name, 0, overwrite ? NCRYPT_OVERWRITE_KEY_FLAG : dwFlags);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to create new persistent encryption key (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

#ifndef TEST_PASSWD
    // Set the UI policy to force high protection (PIN dialog)
    NCRYPT_UI_POLICY uiPolicy;
    memset(&uiPolicy, 0, sizeof(NCRYPT_UI_POLICY));
    uiPolicy.dwVersion = 1;
    uiPolicy.dwFlags = NCRYPT_UI_FORCE_HIGH_PROTECTION_FLAG;
    uiPolicy.pszDescription = L"BIP32 seed for dogecoin wallet";
    status = NCryptSetProperty(hEncryptionKey, NCRYPT_UI_POLICY_PROPERTY, (PBYTE)&uiPolicy, sizeof(NCRYPT_UI_POLICY), 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to set UI policy for encryption key (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }
#endif

    // Generate a new encryption key in the TPM storage provider
    status = NCryptFinalizeKey(hEncryptionKey, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to generate new encryption key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Open the existing encryption key in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hEncryptionKey, name, 0, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open existing encryption key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Encrypt the seed using the encryption key
    status = NCryptEncrypt(hEncryptionKey, (PBYTE)seed, (DWORD)size, NULL, NULL, 0, &cbResult, NCRYPT_PAD_PKCS1_FLAG);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to encrypt the seed (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Allocate memory for the encrypted seed
    pbOutput = (PBYTE)malloc(cbResult);
    if (!pbOutput)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for encrypted data\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Encrypt the seed using the encryption key
    status = NCryptEncrypt(hEncryptionKey, (PBYTE)seed, (DWORD)size, NULL, pbOutput, cbResult, &cbOutput, NCRYPT_PAD_PKCS1_FLAG);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to encrypt the seed (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Create the directory for storing the encrypted seed if it doesn't exist
    if (_wmkdir(CRYPTO_DIR_PATH_W) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create directory\n");
        return false;
    }

    // Successfully encrypted the seed
    // Create a file with the encrypted seed
    // Open the file for binary write, "wb+" to overwrite if exists
    FILE* fp = _wfopen(filename, overwrite ? L"wb+" : L"wb");
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for writing\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Write the encrypted seed to the file
    size_t bytesWritten = fwrite(pbOutput, 1, cbOutput, fp);
    if (bytesWritten != cbOutput)
    {
        fprintf(stderr, "ERROR: Failed to write encrypted seed to file\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Close the file
    fclose(fp);

    // Free the encryption key handle and close the TPM storage provider
    NCryptFreeObject(hEncryptionKey);
    NCryptFreeObject(hProvider);

    return true;

#else
    (void) seed;
    (void) size;
    (void) file_num;
    (void) overwrite;
    return false;
#endif
}

/**
 * @brief Decrypt a BIP32 seed with the TPM
 *
 * Decrypt a BIP32 seed previously encrypted with a TPM2 persistent encryption key.
 *
 * @param seed Decrypted seed will be stored here
 * @param file_num The file number for the encrypted seed
 * @return Returns true if the seed is decrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_seed_with_tpm(SEED seed, const int file_num)
{
#if defined (_WIN64) && !defined(__MINGW64__) && defined(USE_TPM2)

    // Validate the input parameters
    if (seed == NULL)
    {
        fprintf(stderr, "ERROR: Invalid seed\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Declare variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hEncryptionKey;
    DWORD cbResult;
    PBYTE pbOutput = NULL;
    DWORD cbOutput = 0;

    // Format the name of the encrypted seed object
    wchar_t name[NAME_MAX_LEN] = {0};
    swprintf(name, sizeof(name), SEED_TPM_OBJ_NAME_WIN, file_num);

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Open the existing encryption key in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hEncryptionKey, name, 0, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open existing encryption key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Read the encrypted seed from the file
    wchar_t filename[FILE_PATH_MAX_LEN] = {0};
    swprintf(filename, sizeof(filename), SEED_TPM_FILE_NAME_WIN, file_num);
    FILE* fp = _wfopen(filename, L"rb");
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for reading\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Get the size of the encrypted seed
    fseek(fp, 0, SEEK_END);
    size_t fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate memory for the encrypted seed
    pbOutput = (PBYTE) malloc(fileSize);
    if (!pbOutput)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for reading file\n");
        fclose(fp);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Read the encrypted seed from the file
    DWORD bytesRead = (DWORD)fread(pbOutput, 1, fileSize, fp);
    fclose(fp);

    // Validate the number of bytes read
    if (bytesRead != fileSize)
    {
        fprintf(stderr, "ERROR: Failed to read file\n");
        dogecoin_free(pbOutput);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Decrypt the encrypted data
    status = NCryptDecrypt(hEncryptionKey, pbOutput, bytesRead, NULL, (PBYTE)seed, (DWORD)MAX_SEED_SIZE, &cbResult, NCRYPT_PAD_PKCS1_FLAG);
    if (status != ERROR_SUCCESS)
    {
        // Failed to decrypt the encrypted data
        fprintf(stderr, "ERROR: Failed to decrypt the encrypted data (0x%08x)\n", status);
        dogecoin_free(pbOutput);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);

        return false;
    }

    // Free the output buffer, encryption key handle, and close the TPM storage provider
    dogecoin_free(pbOutput);
    NCryptFreeObject(hEncryptionKey);
    NCryptFreeObject(hProvider);

    return true;
#else
    (void) seed;
    (void) file_num;
    return false;
#endif
}

/**
 * @brief Encrypt a BIP32 seed with software
 *
 * Encrypt a BIP32 seed with software and store the encrypted seed in a file.
 *
 * @param seed The seed to encrypt
 * @param size The size of the seed
 * @param file_num The file number to encrypt the seed for
 * @param overwrite Whether or not to overwrite an existing seed
 * @param test_password The password to use for testing
 * @return Returns true if the seed is encrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_encrypt_seed_with_sw(const SEED seed, const size_t size, const int file_num, const dogecoin_bool overwrite, const char* test_password)
{

    // Validate the input parameters
    if (seed == NULL)
    {
        fprintf(stderr, "ERROR: Invalid seed\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // File operations
#ifdef _WIN32
    if (_wmkdir(CRYPTO_DIR_PATH_W) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create directory\n");
        return false;
    }
    wchar_t fullpath[FILE_PATH_MAX_LEN] = {0};
    swprintf(fullpath, sizeof(fullpath), SEED_SW_FILE_NAME_WIN, file_num);
    if (!overwrite && _waccess(fullpath, F_OK) != -1)
    {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }
    FILE* fp = _wfopen(fullpath, overwrite ? L"wb+" : L"wb");
#else
    if (mkdir(CRYPTO_DIR_PATH, 0777) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create directory\n");
        return false;
    }
    char fullpath[FILE_PATH_MAX_LEN] = {0};
    snprintf(fullpath, sizeof(fullpath), SEED_SW_FILE_NAME, file_num);
    if (!overwrite && access(fullpath, F_OK) != -1)
    {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }
    FILE* fp = fopen(fullpath, overwrite ? "wb+" : "wb");
#endif
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for writing.\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    if (test_password)
    {
       strcpy(password, test_password);
    }
    else
#else
    (void) test_password;
#endif
    password = getpass("Enter password for seed encryption: \n");
    if (password == NULL)
    {
        fprintf(stderr, "ERROR: Failed to read password.\n");
        return false;
    }
    if (strlen(password) == 0)
    {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Confirm the password
    char* confirm_password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    if (test_password)
    {
       strcpy(confirm_password, test_password);
    }
    else
#endif
    confirm_password = getpass("Confirm password: \n");
    if (confirm_password == NULL)
    {
        fprintf(stderr, "ERROR: Failed to read password.\n");
        dogecoin_free(password);
        return false;
    }
    if (strcmp(password, confirm_password) != 0)
    {
        fprintf(stderr, "ERROR: Passwords do not match.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_mem_zero(confirm_password, strlen(confirm_password));
        dogecoin_free(password);
        dogecoin_free(confirm_password);
        return false;
    }
    // Clear the confirm password
    dogecoin_mem_zero(confirm_password, strlen(confirm_password));
    dogecoin_free(confirm_password);

    // Generate two random salts
    uint8_t salt_encryption[SALT_SIZE], salt_verification[SALT_SIZE];
    if (!dogecoin_random_bytes(salt_encryption, SALT_SIZE, 1) ||
        !dogecoin_random_bytes(salt_verification, SALT_SIZE, 1))
    {
        fprintf(stderr, "ERROR: Failed to generate random bytes.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the encryption key from the password and salt using PBKDF2
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_encryption, SALT_SIZE, PBKDF2_ITERATIONS, encryption_key, AES_KEY_SIZE);

    // Derive a separate key for verification
    uint8_t verification_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_verification, SALT_SIZE, PBKDF2_ITERATIONS, verification_key, AES_KEY_SIZE);

    // Hash the verification key
    uint8_t verification_key_hash[SHA512_DIGEST_LENGTH];
    sha512_raw(verification_key, AES_KEY_SIZE, verification_key_hash);

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Generate a random IV for AES encryption
    uint8_t iv[AES_IV_SIZE];
    if (!dogecoin_random_bytes(iv, sizeof(iv), 1))
    {
        fprintf(stderr, "ERROR: Failed to generate random bytes.\n");
        return false;
    }

    // Encrypt the seed using AES
    size_t encrypted_size = size;
    dogecoin_bool padding_used = false;
    uint8_t* encrypted_seed = malloc(encrypted_size);
    if (!encrypted_seed)
    {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        return false;
    }

    size_t encrypted_actual_size = aes256_cbc_encrypt(encryption_key, iv, seed, size, padding_used, encrypted_seed);
    if (encrypted_actual_size == 0)
    {
        fprintf(stderr, "ERROR: AES encryption failed.\n");
        dogecoin_free(encrypted_seed);
        return false;
    }

    // Write the IV, salt, verification key hash, and encrypted seed to the file
    fwrite(iv, 1, sizeof(iv), fp);
    fwrite(salt_encryption, 1, SALT_SIZE, fp);
    fwrite(salt_verification, 1, SALT_SIZE, fp);
    fwrite(verification_key_hash, 1, sizeof(verification_key_hash), fp);
    fwrite(encrypted_seed, 1, encrypted_size, fp);

    // Close the file and free memory
    fclose(fp);
    dogecoin_free(encrypted_seed);

    return true;
}

/**
 * @brief Decrypt a BIP32 seed with software
 *
 * Decrypt a BIP32 seed previously encrypted with software.
 *
 * @param seed Decrypted seed will be stored here
 * @param file_num The file number for the encrypted seed
 * @param test_password The password to use for testing
 * @return Returns true if the seed is decrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_seed_with_sw(SEED seed, const int file_num, const char* test_password)
{

    // Validate the input parameters
    if (seed == NULL)
    {
        fprintf(stderr, "ERROR: Invalid seed\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    if (test_password)
    {
       strcpy(password, test_password);
    }
    else
#else
    (void) test_password;
#endif
    password = getpass("Enter password for seed decryption: \n");
    if (password == NULL)
    {
        fprintf(stderr, "ERROR: Failed to read password.\n");
        return false;
    }
    if (strlen(password) == 0)
    {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Open the file for reading
#ifdef _WIN32
    wchar_t fullpath[FILE_PATH_MAX_LEN] = {0};
    swprintf(fullpath, sizeof(fullpath), SEED_SW_FILE_NAME_WIN, file_num);
    FILE* fp = _wfopen(fullpath, L"rb");
#else
    char fullpath[FILE_PATH_MAX_LEN] = {0};
    snprintf(fullpath, sizeof(fullpath), SEED_SW_FILE_NAME, file_num);
    FILE* fp = fopen(fullpath, "rb");
#endif
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for reading.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the IV from the file
    uint8_t iv[AES_IV_SIZE];
    if (fread(iv, 1, sizeof(iv), fp) != sizeof(iv))
    {
        fprintf(stderr, "ERROR: Failed to read IV from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the encryption and verification salts from the file
    uint8_t salt_encryption[SALT_SIZE], salt_verification[SALT_SIZE];
    if (fread(salt_encryption, 1, SALT_SIZE, fp) != SALT_SIZE ||
        fread(salt_verification, 1, SALT_SIZE, fp) != SALT_SIZE)
    {
        fprintf(stderr, "ERROR: Failed to read salts from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the verification key hash from the file
    uint8_t stored_verification_key_hash[SHA512_DIGEST_LENGTH];
    if (fread(stored_verification_key_hash, 1, sizeof(stored_verification_key_hash), fp) != sizeof(stored_verification_key_hash))
    {
        fprintf(stderr, "ERROR: Failed to read verification key hash from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the verification key from the password and verification salt using PBKDF2
    uint8_t derived_verification_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_verification, SALT_SIZE, PBKDF2_ITERATIONS, derived_verification_key, AES_KEY_SIZE);

    // Hash the derived verification key
    uint8_t derived_verification_key_hash[SHA512_DIGEST_LENGTH];
    sha512_raw(derived_verification_key, AES_KEY_SIZE, derived_verification_key_hash);

    // Compare the derived verification key hash with the stored one
    if (memcmp(stored_verification_key_hash, derived_verification_key_hash, SHA512_DIGEST_LENGTH) != 0)
    {
        fprintf(stderr, "ERROR: Incorrect password.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the encryption key from the password and encryption salt using PBKDF2
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_encryption, SALT_SIZE, PBKDF2_ITERATIONS, encryption_key, AES_KEY_SIZE);

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Read the encrypted seed from the file
    size_t encrypted_size = ENCRYPTED_SEED_SIZE;
    uint8_t* encrypted_seed = malloc(encrypted_size);
    if (!encrypted_seed)
    {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(fp);
        return false;
    }

    if (fread(encrypted_seed, 1, encrypted_size, fp) != encrypted_size)
    {
        fprintf(stderr, "ERROR: Failed to read encrypted seed from file.\n");
        fclose(fp);
        dogecoin_free(encrypted_seed);
        return false;
    }

    fclose(fp);

    // Decrypt the seed using AES
    dogecoin_bool padding_used = false;
    size_t decrypted_actual_size = aes256_cbc_decrypt(encryption_key, iv, encrypted_seed, encrypted_size, padding_used, seed);
    dogecoin_free(encrypted_seed);

    if (decrypted_actual_size == 0)
    {
        fprintf(stderr, "ERROR: AES decryption failed.\n");
        dogecoin_free(encrypted_seed);
        return false;
    }

    return true;
}

/**
 * @brief Generate a HD node object with the TPM
 *
 * Generate a HD node object with the TPM
 *
 * @param out The HD node object to generate
 * @param file_num The file number of the encrypted mnemonic
 * @param overwrite Whether or not to overwrite the existing HD node object
 * @return Returns true if the keypair and chain_code are generated successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_hdnode_encrypt_with_tpm(dogecoin_hdnode* out, const int file_num, dogecoin_bool overwrite)
{
#if defined (_WIN64) && !defined(__MINGW64__) && defined(USE_TPM2)

    // Validate the input parameters
    if (out == NULL)
    {
        fprintf(stderr, "ERROR: Invalid HD node\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Format the name of the encrypted HD node file
    wchar_t filename[FILE_PATH_MAX_LEN] = {0};
    swprintf(filename, sizeof(filename), MASTER_TPM_FILE_NAME_WIN, file_num);

    // Check if the file already exists and if not, prompt for overwriting
        if (!overwrite && _waccess(filename, F_OK) != -1)
    {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }

    // Initialize variables
    dogecoin_mem_zero(out, sizeof(dogecoin_hdnode));
    out->depth = 0;
    out->fingerprint = 0x00000000;
    out->child_num = 0;

    // Generate a new master key
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hEncryptionKey;
    DWORD cbResult;
    PBYTE pbResult = NULL;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Format the name of the HD node
    wchar_t name[NAME_MAX_LEN] = {0};
    swprintf(name, sizeof(name), MASTER_TPM_OBJ_NAME_WIN, file_num);

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Create a new persistent encryption key
    status = NCryptCreatePersistedKey(hProvider, &hEncryptionKey, NCRYPT_RSA_ALGORITHM, name, 0, overwrite ? NCRYPT_OVERWRITE_KEY_FLAG : dwFlags);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to create new persistent encryption key (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

#ifndef TEST_PASSWD
    // Set the UI policy to force high protection (PIN dialog)
    NCRYPT_UI_POLICY uiPolicy;
    memset(&uiPolicy, 0, sizeof(NCRYPT_UI_POLICY));
    uiPolicy.dwVersion = 1;
    uiPolicy.dwFlags = NCRYPT_UI_PROTECT_KEY_FLAG;
    uiPolicy.pszDescription = L"BIP32 master key for dogecoin wallet";
    status = NCryptSetProperty(hEncryptionKey, NCRYPT_UI_POLICY_PROPERTY, (PBYTE)&uiPolicy, sizeof(NCRYPT_UI_POLICY), 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to set UI policy for encryption key (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }
#endif

    // Generate a new encryption key in the TPM storage provider
    status = NCryptFinalizeKey(hEncryptionKey, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to generate new encryption key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Open the existing encryption key in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hEncryptionKey, name, 0, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open existing encryption key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Create TBS context (TPM2)
    TBS_HCONTEXT hContext = 0;
    TBS_CONTEXT_PARAMS2 params;
    params.version = TBS_CONTEXT_VERSION_TWO;
    TBS_RESULT hr = Tbsi_Context_Create((PCTBS_CONTEXT_PARAMS)&params, &hContext);
    if (hr != TBS_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to create TBS context (0x%08x)\n", hr);
        return false;
    }

    // Send TPM2_CC_GetRandom command
    const BYTE cmd_random[] = {
        0x80, 0x01,             // tag: TPM_ST_SESSIONS
        0x00, 0x00, 0x00, 0x0C, // commandSize: size of the entire command byte array
        0x00, 0x00, 0x01, 0x7B, // commandCode: TPM2_CC_GetRandom
        0x00, 0x20              // parameter: 32 bytes
    };
    BYTE resp_random[TBS_IN_OUT_BUF_SIZE_MAX] = { 0 };
    UINT32 resp_randomSize = TBS_IN_OUT_BUF_SIZE_MAX;
    hr = Tbsip_Submit_Command(hContext, TBS_COMMAND_LOCALITY_ZERO, TBS_COMMAND_PRIORITY_NORMAL, cmd_random, sizeof(cmd_random), resp_random, &resp_randomSize);
    if (hr != TBS_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to send TPM2_CC_GetRandom command (0x%08x)\n", hr);

        // Close TBS context
        hr = Tbsip_Context_Close(hContext);
        if (hr != TBS_SUCCESS)
        {
            fprintf(stderr, "ERROR: Failed to close TBS context (0x%08x)\n", hr);
        }
        return false;
    }

    // Close TBS context
    hr = Tbsip_Context_Close(hContext);
    if (hr != TBS_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to close TBS context (0x%08x)\n", hr);
        return false;
    }

    // Derive the HD node from the seed
    dogecoin_hdnode_from_seed((uint8_t*)&resp_random[RESP_RAND_OFFSET], 32, out);

    // Encrypt the HD node with the encryption key
    status = NCryptEncrypt(hEncryptionKey, (PBYTE)out, (DWORD)sizeof(dogecoin_hdnode), NULL, NULL, 0, &cbResult, NCRYPT_PAD_PKCS1_FLAG);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to encrypt the HD node with the encryption key (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Allocate memory for the encrypted HD node
    pbResult = (PBYTE)malloc(cbResult);
    if (pbResult == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for the encrypted HD node\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Encrypt the HD node with the encryption key
    status = NCryptEncrypt(hEncryptionKey, (PBYTE)out, (DWORD)sizeof(dogecoin_hdnode), NULL, pbResult, cbResult, &cbResult, NCRYPT_PAD_PKCS1_FLAG);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to encrypt the HD node with the encryption key (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        dogecoin_free(pbResult);
        return false;
    }

    // Create the directory for storing the encrypted key if it doesn't exist
    if (_wmkdir(CRYPTO_DIR_PATH_W) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create directory\n");
        return false;
    }

    // Successfully encrypted the HD node with the encryption key
    // Create a file with the encrypted HD node
    // Open the file for binary write, "wb+" to overwrite if exists
    FILE* fp = _wfopen(filename, overwrite ? L"wb+" : L"wb");
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for writing\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        dogecoin_free(pbResult);
        return false;
    }

    // Write the encrypted HD node to the file
    size_t bytesWritten = fwrite(pbResult, 1, cbResult, fp);
    if (bytesWritten != cbResult)
    {
        fprintf(stderr, "ERROR: Failed to write encrypted hdnode to file\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        dogecoin_free(pbResult);
        fclose(fp);
        return false;
    }

    // Close the file
    fclose(fp);

    // Free the memory for the encrypted HD node
    dogecoin_free(pbResult);

    // Free the encryption key and provider
    NCryptFreeObject(hEncryptionKey);
    NCryptFreeObject(hProvider);

    return true;

#else
    (void) out;
    (void) file_num;
    (void) overwrite;
    return false;
#endif
}

/**
 * @brief Decrypt a HD node with the TPM
 *
 * Decrypt a HD node previously encrypted with a TPM2 persistent encryption key.
 *
 * @param out The decrypted HD node will be stored here
 * @param file_num The file number for the encrypted HD node
 * @return Returns true if the HD node is decrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_hdnode_with_tpm(dogecoin_hdnode* out, const int file_num)
{
#if defined (_WIN64) && !defined(__MINGW64__) && defined(USE_TPM2)

    // Validate the input parameters
    if (out == NULL)
    {
        fprintf(stderr, "ERROR: Invalid HD node\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Declare variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hEncryptionKey;
    DWORD cbResult;
    PBYTE pbOutput = NULL;
    DWORD cbOutput = 0;

    // Format the name of the encrypted HD node object
    wchar_t name[NAME_MAX_LEN] = {0};
    swprintf(name, sizeof(name), MASTER_TPM_OBJ_NAME_WIN, file_num);

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Open the existing encryption key in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hEncryptionKey, name, 0, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open existing encryption key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Read the encrypted HD node from the file
    wchar_t filename[FILE_PATH_MAX_LEN] = {0};
    swprintf(filename, sizeof(filename), MASTER_TPM_FILE_NAME_WIN, file_num);
    FILE* fp = _wfopen(filename, L"rb");
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for reading\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Get the size of the encrypted HD node
    fseek(fp, 0, SEEK_END);
    size_t fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate memory for the encrypted HD node
    pbOutput = (PBYTE) malloc(fileSize);
    if (!pbOutput)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for reading file\n");
        fclose(fp);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Read the encrypted HD node from the file
    DWORD bytesRead = (DWORD)fread(pbOutput, 1, fileSize, fp);
    fclose(fp);

    // Validate the number of bytes read
    if (bytesRead != fileSize)
    {
        fprintf(stderr, "ERROR: Failed to read file\n");
        dogecoin_free(pbOutput);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Decrypt the encrypted data
    status = NCryptDecrypt(hEncryptionKey, pbOutput, bytesRead, NULL, (PBYTE)out, (DWORD)cbResult, &cbResult, NCRYPT_PAD_PKCS1_FLAG);
    if (status != ERROR_SUCCESS)
    {
        // Failed to decrypt the encrypted data
        fprintf(stderr, "ERROR: Failed to decrypt the encrypted data (0x%08x)\n", status);
        dogecoin_free(pbOutput);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Free memory and close handles
    dogecoin_free(pbOutput);
    NCryptFreeObject(hEncryptionKey);
    NCryptFreeObject(hProvider);

    return true;

#else
    (void) out;
    (void) file_num;
    return false;
#endif
}

/**
 * @brief Generate a HD node object with software encryption
 *
 * Generate a HD node object with software encryption and store it in a file.
 *
 * @param out The HD node object to generate
 * @param file_num The file number of the encrypted mnemonic
 * @param overwrite Whether or not to overwrite the existing HD node object
 * @param test_password The password to use for testing
 * @return Returns true if the HD node is generated and encrypted successfully, false otherwise.
 */
dogecoin_bool dogecoin_generate_hdnode_encrypt_with_sw(dogecoin_hdnode* out, const int file_num, dogecoin_bool overwrite, const char* test_password)
{

    // Validate the input parameters
    if (out == NULL)
    {
        fprintf(stderr, "ERROR: Invalid HD node\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // File operations
#ifdef _WIN32
    if (_wmkdir(CRYPTO_DIR_PATH_W) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create directory\n");
        return false;
    }
    wchar_t fullpath[FILE_PATH_MAX_LEN] = {0};
    swprintf(fullpath, sizeof(fullpath), MASTER_SW_FILE_NAME_WIN, file_num);
    if (!overwrite && _waccess(fullpath, F_OK) != -1)
    {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }
    FILE* fp = _wfopen(fullpath, overwrite ? L"wb+" : L"wb");
#else
    if (mkdir(CRYPTO_DIR_PATH, 0777) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create directory\n");
        return false;
    }
    char fullpath[FILE_PATH_MAX_LEN] = {0};
    snprintf(fullpath, sizeof(fullpath), MASTER_SW_FILE_NAME, file_num);
    if (!overwrite && access(fullpath, F_OK) != -1)
    {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }
    FILE* fp = fopen(fullpath, overwrite ? "wb+" : "wb");
#endif
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for writing.\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    if (test_password)
    {
       strcpy(password, test_password);
    }
    else
#else
    (void) test_password;
#endif
    password = getpass("Enter password for HD node encryption: \n");
    if (password == NULL)
    {
        fprintf(stderr, "ERROR: Failed to read password.\n");
        return false;
    }
    if (strlen(password) == 0)
    {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }


    // Confirm the password
    char* confirm_password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    if (test_password)
    {
       strcpy(confirm_password, test_password);
    }
    else
#endif
    confirm_password = getpass("Confirm password: \n");
    if (confirm_password == NULL)
    {
        fprintf(stderr, "ERROR: Failed to read password.\n");
        dogecoin_free(password);
        return false;
    }
    if (strcmp(password, confirm_password) != 0)
    {
        fprintf(stderr, "ERROR: Passwords do not match.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_mem_zero(confirm_password, strlen(confirm_password));
        dogecoin_free(password);
        dogecoin_free(confirm_password);
        return false;
    }
    dogecoin_mem_zero(confirm_password, strlen(confirm_password));
    dogecoin_free(confirm_password);

    // Generate two random salts
    uint8_t salt_encryption[SALT_SIZE], salt_verification[SALT_SIZE];
    if (!dogecoin_random_bytes(salt_encryption, SALT_SIZE, 1) ||
        !dogecoin_random_bytes(salt_verification, SALT_SIZE, 1))
    {
        fprintf(stderr, "ERROR: Failed to generate random bytes.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive encryption key
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_encryption, SALT_SIZE, PBKDF2_ITERATIONS, encryption_key, AES_KEY_SIZE);

    // Derive a separate key for verification
    uint8_t verification_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_verification, SALT_SIZE, PBKDF2_ITERATIONS, verification_key, AES_KEY_SIZE);

    // Hash the verification key
    uint8_t verification_key_hash[SHA512_DIGEST_LENGTH];
    sha512_raw(verification_key, AES_KEY_SIZE, verification_key_hash);

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Generate a random IV for AES encryption
    uint8_t iv[AES_IV_SIZE];
    if (!dogecoin_random_bytes(iv, sizeof(iv), 1))
    {
        fprintf(stderr, "ERROR: Failed to generate random bytes.\n");
        return false;
    }

    // Derive the HD node from the seed
    SEED seed = {0};
    if (!dogecoin_random_bytes(seed, sizeof(seed), 1))
    {
        fprintf(stderr, "ERROR: Failed to generate random bytes.\n");
        return false;
    }
    dogecoin_hdnode_from_seed(seed, sizeof(seed), out);

    // Encrypt the HD node with AES
    size_t encrypted_size = sizeof(dogecoin_hdnode);
    dogecoin_bool padding_used = false;
    uint8_t* encrypted_data = malloc(encrypted_size);
    if (!encrypted_data)
    {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        return false;
    }

    size_t encrypted_actual_size = aes256_cbc_encrypt(encryption_key, iv, (uint8_t*)out, encrypted_size, padding_used, encrypted_data);
    if (encrypted_actual_size == 0)
    {
        fprintf(stderr, "ERROR: AES encryption failed.\n");
        dogecoin_free(encrypted_data);
        return false;
    }

    // Write the IV, salts, verification key hash, and encrypted HD node to the file
    fwrite(iv, 1, sizeof(iv), fp);
    fwrite(salt_encryption, 1, SALT_SIZE, fp);
    fwrite(salt_verification, 1, SALT_SIZE, fp);
    fwrite(verification_key_hash, 1, sizeof(verification_key_hash), fp);
    fwrite(encrypted_data, 1, encrypted_actual_size, fp);

    // Close the file and free memory
    fclose(fp);
    dogecoin_free(encrypted_data);

    return true;
}

/**
 * @brief Decrypt a HD node with software decryption
 *
 * Decrypt a HD node previously encrypted with software encryption.
 *
 * @param out The decrypted HD node will be stored here
 * @param file_num The file number for the encrypted HD node
 * @param test_password The password to use for testing
 * @return Returns true if the HD node is decrypted successfully, false otherwise.
 */
dogecoin_bool dogecoin_decrypt_hdnode_with_sw(dogecoin_hdnode* out, const int file_num, const char* test_password)
{

    // Validate the input parameters
    if (out == NULL)
    {
        fprintf(stderr, "ERROR: Invalid HD node\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    if (test_password)
    {
       strcpy(password, test_password);
    }
    else
#else
    (void) test_password;
#endif
    password = getpass("Enter password for HD node decryption: \n");
    if (password == NULL)
    {
        fprintf(stderr, "ERROR: Failed to read password.\n");
        return false;
    }
    if (strlen(password) == 0)
    {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Open the file for reading
#ifdef _WIN32
    wchar_t fullpath[FILE_PATH_MAX_LEN] = {0};
    swprintf(fullpath, sizeof(fullpath), MASTER_SW_FILE_NAME_WIN, file_num);
    FILE* fp = _wfopen(fullpath, L"rb");
#else
    char fullpath[FILE_PATH_MAX_LEN] = {0};
    snprintf(fullpath, sizeof(fullpath), MASTER_SW_FILE_NAME, file_num);
    FILE* fp = fopen(fullpath, "rb");
#endif
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for reading.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the IV from the file
    uint8_t iv[AES_IV_SIZE];
    if (fread(iv, 1, sizeof(iv), fp) != sizeof(iv))
    {
        fprintf(stderr, "ERROR: Failed to read IV from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the encryption and verification salts from the file
    uint8_t salt_encryption[SALT_SIZE], salt_verification[SALT_SIZE];
    if (fread(salt_encryption, 1, SALT_SIZE, fp) != SALT_SIZE ||
        fread(salt_verification, 1, SALT_SIZE, fp) != SALT_SIZE)
    {
        fprintf(stderr, "ERROR: Failed to read salts from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the verification key hash from the file
    uint8_t stored_verification_key_hash[SHA512_DIGEST_LENGTH];
    if (fread(stored_verification_key_hash, 1, sizeof(stored_verification_key_hash), fp) != sizeof(stored_verification_key_hash))
    {
        fprintf(stderr, "ERROR: Failed to read verification key hash from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the verification key from the password and verification salt using PBKDF2
    uint8_t derived_verification_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_verification, SALT_SIZE, PBKDF2_ITERATIONS, derived_verification_key, AES_KEY_SIZE);

    // Hash the derived verification key
    uint8_t derived_verification_key_hash[SHA512_DIGEST_LENGTH];
    sha512_raw(derived_verification_key, AES_KEY_SIZE, derived_verification_key_hash);

    // Compare the derived verification key hash with the stored one
    if (memcmp(stored_verification_key_hash, derived_verification_key_hash, SHA512_DIGEST_LENGTH) != 0)
    {
        fprintf(stderr, "ERROR: Incorrect password.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the encryption key from the password and encryption salt using PBKDF2
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_encryption, SALT_SIZE, PBKDF2_ITERATIONS, encryption_key, AES_KEY_SIZE);

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Read the encrypted HD node from the file
    size_t encrypted_size = sizeof(dogecoin_hdnode);
    uint8_t* encrypted_data = malloc(encrypted_size);
    if (!encrypted_data)
    {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(fp);
        return false;
    }

    if (fread(encrypted_data, 1, encrypted_size, fp) != encrypted_size)
    {
        fprintf(stderr, "ERROR: Failed to read encrypted HD node from file.\n");
        fclose(fp);
        dogecoin_free(encrypted_data);
        return false;
    }

    fclose(fp);

    // Decrypt the HD node with software decryption (AES)
    dogecoin_bool padding_used = false;
    size_t decrypted_actual_size = aes256_cbc_decrypt(encryption_key, iv, encrypted_data, encrypted_size, padding_used, (uint8_t*)out);
    dogecoin_free(encrypted_data);

    if (decrypted_actual_size == 0)
    {
        fprintf(stderr, "ERROR: AES decryption failed.\n");
        return false;
    }

    return true;
}

/**
 * @brief Generate a mnemonic and encrypt it with the TPM
 *
 * Generate a mnemonic and encrypt it with a TPM2 persistent encryption key.
 *
 * @param mnemonic The generated mnemonic will be stored here
 * @param file_num The file number for the encrypted mnemonic
 * @param overwrite If true, overwrite the existing encrypted mnemonic
 * @param lang The language to use for the mnemonic
 * @param space The mnemonic space to use
 * @param words The mnemonic words to use
 * @return Returns true if the mnemonic is generated and encrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_mnemonic_encrypt_with_tpm(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite, const char* lang, const char* space, const char* words)
{
#if defined (_WIN64) && !defined(__MINGW64__) && defined(USE_TPM2)

    // Validate the input parameters
    if (mnemonic == NULL)
    {
        fprintf(stderr, "ERROR: Invalid mnemonic\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Format the name of the encrypted HD node file
    wchar_t filename[FILE_PATH_MAX_LEN] = {0};
    swprintf(filename, sizeof(filename), MNEMONIC_TPM_FILE_NAME_WIN, file_num);

    // Check if the file already exists and if not, prompt for overwriting
        if (!overwrite && _waccess(filename, F_OK) != -1)
    {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }

    // Declare variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hEncryptionKey;
    DWORD cbResult;
    PBYTE pbOutput = NULL;
    DWORD cbOutput = 0;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Format the name of the mnemonic
    wchar_t name[NAME_MAX_LEN] = {0};
    swprintf(name, sizeof(name), MNEMONIC_TPM_OBJ_NAME_WIN, file_num);

    // Create TBS context (TPM2)
    TBS_HCONTEXT hContext = 0;
    TBS_CONTEXT_PARAMS2 params;
    params.version = TBS_CONTEXT_VERSION_TWO;
    TBS_RESULT hr = Tbsi_Context_Create((PCTBS_CONTEXT_PARAMS)&params, &hContext);
    if (hr != TBS_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to create TBS context (0x%08x)\n", hr);
        return false;
    }

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
    if (hr != TBS_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to send TPM2_CC_GetRandom command (0x%08x)\n", hr);

        // Close TBS context
        hr = Tbsip_Context_Close(hContext);
        if (hr != TBS_SUCCESS)
        {
            fprintf(stderr, "ERROR: Failed to close TBS context (0x%08x)\n", hr);
        }
        return false;
    }

    // Close TBS context
    hr = Tbsip_Context_Close(hContext);
    if (hr != TBS_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to close TBS context (0x%08x)\n", hr);
        return false;
    }

    // Convert the random data to hex
    // TODO: This is a hack, we should be able to use the random data directly
    char* rand_hex = utils_uint8_to_hex(&resp_random[RESP_RAND_OFFSET], 32);

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Create a new persistent encryption key
    status = NCryptCreatePersistedKey(hProvider, &hEncryptionKey, NCRYPT_RSA_ALGORITHM, name, 0, overwrite ? NCRYPT_OVERWRITE_KEY_FLAG : dwFlags);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to create new persistent encryption key (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

#ifndef TEST_PASSWD
    // Set the UI policy to force high protection (PIN dialog)
    NCRYPT_UI_POLICY uiPolicy;
    memset(&uiPolicy, 0, sizeof(NCRYPT_UI_POLICY));
    uiPolicy.dwVersion = 1;
    uiPolicy.dwFlags = NCRYPT_UI_PROTECT_KEY_FLAG;
    uiPolicy.pszDescription = L"BIP39 seed phrase for dogecoin wallet";
    status = NCryptSetProperty(hEncryptionKey, NCRYPT_UI_POLICY_PROPERTY, (PBYTE)&uiPolicy, sizeof(NCRYPT_UI_POLICY), 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to set UI policy for encryption key (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }
#endif

    // Generate a new encryption key in the TPM storage provider
    status = NCryptFinalizeKey(hEncryptionKey, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to generate new encryption key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Open the existing encryption key in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hEncryptionKey, name, 0, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open existing encryption key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Generate the BIP-39 mnemonic from the random data
    size_t mnemonicSize = 0;
    int mnemonicResult = dogecoin_generate_mnemonic("256", lang, space, (const char*)rand_hex, words, NULL, &mnemonicSize, mnemonic);
    if (mnemonicResult == -1)
    {
        fprintf(stderr, "ERROR: Failed to generate mnemonic\n");
        NCryptFreeObject(hProvider);
        utils_clear_buffers();
        return false;
    }

    // Clear the random data
    utils_clear_buffers();

    // Encrypt the mnemonic using the encryption key
    status = NCryptEncrypt(hEncryptionKey, (PBYTE)mnemonic, (DWORD) mnemonicSize, NULL, NULL, 0, &cbResult, NCRYPT_PAD_PKCS1_FLAG);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to encrypt the mnemonic (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Allocate memory for the encrypted data
    pbOutput = (PBYTE) malloc(cbResult);
    if (!pbOutput)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for encrypted data\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Encrypt the mnemonic using the encryption key
    status = NCryptEncrypt(hEncryptionKey, (PBYTE)mnemonic, (DWORD) mnemonicSize, NULL, pbOutput, cbResult, &cbOutput, NCRYPT_PAD_PKCS1_FLAG);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to encrypt the mnemonic (0x%08x)\n", status);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Create the directory for storing the encrypted mnemonic if it doesn't exist
    if (_wmkdir(CRYPTO_DIR_PATH_W) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create directory\n");
        return false;
    }

    // Successfully encrypted the mnemonic
    // Create a file with the encrypted mnemonic
    // Open the file for binary write, "wb+" to overwrite if exists
    FILE* fp = _wfopen(filename, overwrite ? L"wb+" : L"wb");
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for writing\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Write the encrypted mnemonic to the file
    size_t bytesWritten = fwrite(pbOutput, 1, cbOutput, fp);
    if (bytesWritten != cbOutput)
    {
        fprintf(stderr, "ERROR: Failed to write encrypted mnemonic to file\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Close the file
    fclose(fp);

    // Free the memory for the encrypted data
    dogecoin_free(pbOutput);

    // Free the encryption key and provider
    NCryptFreeObject(hEncryptionKey);
    NCryptFreeObject(hProvider);

    return true;
#else
    (void) mnemonic;
    (void) file_num;
    (void) overwrite;
    (void) lang;
    (void) space;
    (void) words;
    return false;
#endif
}

/**
 * @brief Decrypts a BIP-39 mnemonic
 *
 * Decrypts a BIP-39 mnemonic using the TPM storage provider
 *
 * @param mnemonic The decrypted mnemonic will be stored here
 * @param file_num The file number of the encrypted mnemonic
 *
 * @return True if the mnemonic was successfully decrypted, false otherwise
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_mnemonic_with_tpm(MNEMONIC mnemonic, const int file_num)
{
#if defined (_WIN64) && !defined(__MINGW64__) && defined(USE_TPM2)

    // Validate the input parameters
    if (mnemonic == NULL)
    {
        fprintf(stderr, "ERROR: Invalid mnemonic\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Declare variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hEncryptionKey;
    DWORD cbResult;
    PBYTE pbOutput = NULL;

    // Format the name of the mnemonic
    wchar_t name[NAME_MAX_LEN] = {0};
    swprintf(name, sizeof(name), MNEMONIC_TPM_OBJ_NAME_WIN, file_num);

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Open the existing encryption key in the TPM storage provider
    status = NCryptOpenKey(hProvider, &hEncryptionKey, name, 0, 0);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open existing encryption key in TPM storage provider (0x%08x)\n", status);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Read the encrypted mnemonic from the file
    wchar_t filename[FILE_PATH_MAX_LEN] = {0};
    swprintf(filename, sizeof(filename), MNEMONIC_TPM_FILE_NAME_WIN, file_num);
    FILE* fp = _wfopen(filename, L"rb");
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for reading\n");
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Get the size of the file
    fseek(fp, 0, SEEK_END);
    size_t fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate memory for the encrypted data
    pbOutput = (PBYTE) malloc(fileSize);
    if (!pbOutput)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for reading file\n");
        fclose(fp);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Read the encrypted data from the file
    DWORD bytesRead = (DWORD)fread(pbOutput, 1, fileSize, fp);
    fclose(fp);

    // Check that the file was read successfully
    if (bytesRead != fileSize)
    {
        fprintf(stderr, "ERROR: Failed to read file\n");
        dogecoin_free(pbOutput);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Decrypt the encrypted data
    status = NCryptDecrypt(hEncryptionKey, pbOutput, bytesRead, NULL, (PBYTE)mnemonic, sizeof(MNEMONIC), &cbResult, NCRYPT_PAD_PKCS1_FLAG);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to decrypt the encrypted data (0x%08x)\n", status);
        dogecoin_free(pbOutput);
        NCryptFreeObject(hEncryptionKey);
        NCryptFreeObject(hProvider);
        return false;
    }

    // Free the output buffer, encryption key handle, and close the TPM storage provider
    dogecoin_free(pbOutput);
    NCryptFreeObject(hEncryptionKey);
    NCryptFreeObject(hProvider);

    return true;
#else
    (void) mnemonic;
    (void) file_num;
    return false;
#endif
}

/**
 * @brief Generate a mnemonic and encrypt it with software encryption
 *
 * Generate a mnemonic, prompt for a password, and encrypt it with software-based encryption.
 *
 * @param mnemonic The generated mnemonic will be stored here
 * @param file_num The file number for the encrypted mnemonic
 * @param overwrite If true, overwrite the existing encrypted mnemonic
 * @param lang The language to use for the mnemonic
 * @param space The mnemonic space to use
 * @param words The mnemonic words to use
 * @param test_password The password to use for testing
 * @return Returns true if the mnemonic is generated and encrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_mnemonic_encrypt_with_sw(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite, const char* lang, const char* space, const char* words, const char* test_password)
{

    // Validate the input parameters
    if (mnemonic == NULL)
    {
        fprintf(stderr, "ERROR: Invalid mnemonic\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // File operations
#ifdef _WIN32
    if (_wmkdir(CRYPTO_DIR_PATH_W) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create directory\n");
        return false;
    }
    wchar_t fullpath[FILE_PATH_MAX_LEN] = {0};
    swprintf(fullpath, sizeof(fullpath), MNEMONIC_SW_FILE_NAME_WIN, file_num);
    if (!overwrite && _waccess(fullpath, F_OK) != -1)
    {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }
    FILE* fp = _wfopen(fullpath, overwrite ? L"wb+" : L"wb");
#else
    if (mkdir(CRYPTO_DIR_PATH, 0777) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create directory\n");
        return false;
    }
    char fullpath[FILE_PATH_MAX_LEN] = {0};
    snprintf(fullpath, sizeof(fullpath), MNEMONIC_SW_FILE_NAME, file_num);
    if (!overwrite && access(fullpath, F_OK) != -1)
    {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }
    FILE* fp = fopen(fullpath, overwrite ? "wb+" : "wb");
#endif
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for writing.\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    if (test_password)
    {
       strcpy(password, test_password);
    }
    else
#else
    (void) test_password;
#endif
    password = getpass("Enter password for mnenonic encryption: \n");
    if (password == NULL)
    {
        fprintf(stderr, "ERROR: Failed to read password.\n");
        return false;
    }
    if (strlen(password) == 0)
    {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Confirm the password
    char* confirm_password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    if (test_password)
    {
       strcpy(confirm_password, test_password);
    }
    else
#endif
    confirm_password = getpass("Confirm password: \n");
    if (confirm_password == NULL)
    {
        fprintf(stderr, "ERROR: Failed to read password.\n");
        dogecoin_free(password);
        return false;
    }
    if (strcmp(password, confirm_password) != 0)
    {
        fprintf(stderr, "ERROR: Passwords do not match.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_mem_zero(confirm_password, strlen(confirm_password));
        dogecoin_free(password);
        dogecoin_free(confirm_password);
        return false;
    }
    dogecoin_mem_zero(confirm_password, strlen(confirm_password));
    dogecoin_free(confirm_password);

    // Generate two random salts
    uint8_t salt_encryption[SALT_SIZE], salt_verification[SALT_SIZE];
    if (!dogecoin_random_bytes(salt_encryption, SALT_SIZE, 1) ||
        !dogecoin_random_bytes(salt_verification, SALT_SIZE, 1))
    {
        fprintf(stderr, "ERROR: Failed to generate random bytes.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive encryption key
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_encryption, SALT_SIZE, PBKDF2_ITERATIONS, encryption_key, sizeof(encryption_key));

    // Derive verification key
    uint8_t verification_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_verification, SALT_SIZE, PBKDF2_ITERATIONS, verification_key, sizeof(verification_key));

    // Hash the verification key
    uint8_t verification_key_hash[SHA512_DIGEST_LENGTH];
    sha512_raw(verification_key, AES_KEY_SIZE, verification_key_hash);

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Generate the BIP-39 mnemonic
    size_t mnemonicSize = 0;
    int mnemonicResult = dogecoin_generate_mnemonic("256", lang, space, NULL, words, NULL, &mnemonicSize, mnemonic);
    if (mnemonicResult == -1)
    {
        fprintf(stderr, "ERROR: Failed to generate mnemonic\n");
        return false;
    }

    // Encrypt the mnemonic with AES
    uint8_t iv[AES_IV_SIZE];
    if (!dogecoin_random_bytes(iv, sizeof(iv), 1))
    {
        fprintf(stderr, "ERROR: Failed to generate random bytes.\n");
        return false;
    }

    size_t encrypted_size = ENCRYPTED_MNEMONIC_SIZE;
    dogecoin_bool padding_used = false;
    uint8_t* encrypted_data = malloc(encrypted_size);
    if (!encrypted_data)
    {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        return false;
    }
    memset(encrypted_data, 0, encrypted_size);

    size_t encrypted_actual_size = aes256_cbc_encrypt(encryption_key, iv, (uint8_t*)mnemonic, encrypted_size, padding_used, encrypted_data);
    if (encrypted_actual_size == 0)
    {
        fprintf(stderr, "ERROR: AES encryption failed.\n");
        dogecoin_free(encrypted_data);
        return false;
    }

    // Write the IV, salts, verification key hash, and encrypted mnemonic to the file
    fwrite(iv, 1, sizeof(iv), fp);
    fwrite(salt_encryption, 1, SALT_SIZE, fp);
    fwrite(salt_verification, 1, SALT_SIZE, fp);
    fwrite(verification_key_hash, 1, sizeof(verification_key_hash), fp);
    fwrite(encrypted_data, 1, encrypted_actual_size, fp);

    fclose(fp);
    dogecoin_free(encrypted_data);

    return true;
}

/**
 * @brief Decrypts a BIP-39 mnemonic with software decryption
 *
 * Decrypts a BIP-39 mnemonic previously encrypted with software-based encryption.
 *
 * @param mnemonic The decrypted mnemonic will be stored here
 * @param file_num The file number for the encrypted mnemonic
 * @param test_password The password to use for testing
 *
 * @return True if the mnemonic was successfully decrypted, false otherwise
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_mnemonic_with_sw(MNEMONIC mnemonic, const int file_num, const char* test_password)
{

    // Validate the input parameters
    if (mnemonic == NULL)
    {
        fprintf(stderr, "ERROR: Invalid mnemonic\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    if (test_password)
    {
       strcpy(password, test_password);
    }
    else
#else
    (void) test_password;
#endif
    password = getpass("Enter password for mnemonic decryption: \n");
    if (password == NULL)
    {
        fprintf(stderr, "ERROR: Failed to read password.\n");
        return false;
    }
    if (strlen(password) == 0)
    {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Open the file for reading
#ifdef _WIN32
    wchar_t fullpath[FILE_PATH_MAX_LEN] = {0};
    swprintf(fullpath, sizeof(fullpath), MNEMONIC_SW_FILE_NAME_WIN, file_num);
    FILE* fp = _wfopen(fullpath, L"rb");
#else
    char fullpath[FILE_PATH_MAX_LEN] = {0};
    snprintf(fullpath, sizeof(fullpath), MNEMONIC_SW_FILE_NAME, file_num);
    FILE* fp = fopen(fullpath, "rb");
#endif
    if (!fp)
    {
        fprintf(stderr, "ERROR: Failed to open file for reading.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the IV, encryption salt, and verification salt from the file
    uint8_t iv[AES_IV_SIZE], salt_encryption[SALT_SIZE], salt_verification[SALT_SIZE];
    if (fread(iv, 1, sizeof(iv), fp) != sizeof(iv) ||
        fread(salt_encryption, 1, SALT_SIZE, fp) != SALT_SIZE ||
        fread(salt_verification, 1, SALT_SIZE, fp) != SALT_SIZE)
    {
        fprintf(stderr, "ERROR: Failed to read data from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the verification key hash from the file
    uint8_t stored_verification_key_hash[SHA512_DIGEST_LENGTH];
    if (fread(stored_verification_key_hash, 1, sizeof(stored_verification_key_hash), fp) != sizeof(stored_verification_key_hash))
    {
        fprintf(stderr, "ERROR: Failed to read verification key hash from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the verification key from the password and verification salt using PBKDF2
    uint8_t derived_verification_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_verification, SALT_SIZE, PBKDF2_ITERATIONS, derived_verification_key, AES_KEY_SIZE);

    // Hash the derived verification key
    uint8_t derived_verification_key_hash[SHA512_DIGEST_LENGTH];
    sha512_raw(derived_verification_key, AES_KEY_SIZE, derived_verification_key_hash);

    // Compare the derived verification key hash with the stored one
    if (memcmp(stored_verification_key_hash, derived_verification_key_hash, SHA512_DIGEST_LENGTH) != 0)
    {
        fprintf(stderr, "ERROR: Incorrect password.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the encryption key from the password and encryption salt using PBKDF2
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt_encryption, SALT_SIZE, PBKDF2_ITERATIONS, encryption_key, AES_KEY_SIZE);

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Read the encrypted mnemonic from the file
    size_t encrypted_size = ENCRYPTED_MNEMONIC_SIZE;
    uint8_t* encrypted_data = malloc(encrypted_size);
    if (!encrypted_data)
    {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(fp);
        return false;
    }

    if (fread(encrypted_data, 1, encrypted_size, fp) != encrypted_size)
    {
        fprintf(stderr, "ERROR: Failed to read encrypted mnemonic from file.\n");
        fclose(fp);
        dogecoin_free(encrypted_data);
        return false;
    }

    fclose(fp);

    // Decrypt the mnemonic with AES
    dogecoin_bool padding_used = false;
    size_t decrypted_actual_size = aes256_cbc_decrypt(encryption_key, iv, encrypted_data, encrypted_size, padding_used, (uint8_t*)mnemonic);
    dogecoin_free(encrypted_data);

    if (decrypted_actual_size == 0)
    {
        fprintf(stderr, "ERROR: AES decryption failed.\n");
        return false;
    }

    return true;
}

/**
 * @brief List the encryption keys in the TPM storage provider
 *
 * Lists the encryption keys in the TPM storage provider
 *
 * @param names The names of the encryption keys will be stored here
 * @param count The number of encryption keys will be stored here
 *
 * @return True if the encryption keys were successfully listed, false otherwise
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_list_encryption_keys_in_tpm(wchar_t* names[], size_t* count)
{
#if defined (_WIN64) && !defined(__MINGW64__) && defined(USE_TPM2)

    // Declare ncrypt variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys
    PVOID ppEnumState = NULL;

    // Open the TPM storage provider
    status = NCryptOpenStorageProvider(&hProvider, MS_PLATFORM_CRYPTO_PROVIDER, dwFlags);
    if (status != ERROR_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to open TPM storage provider (0x%08x)\n", status);
        return false;
    }

    // Enumerate the keys in the TPM storage provider
    NCryptKeyName* keyList = NULL;

    while (true)
    {
        // Get the next key in the list
        status = NCryptEnumKeys(hProvider, NULL, &keyList, &ppEnumState, dwFlags);
        if (status == NTE_NO_MORE_ITEMS)
        {
            break;
        }
        else if (status != ERROR_SUCCESS)
        {
            fprintf(stderr, "ERROR: Failed to enumerate keys in TPM storage provider (0x%08x)\n", status);
            NCryptFreeObject(hProvider);
            return false;
        }

        // Allocate memory for the name
        names[*count] = malloc((wcslen(keyList->pszName) + 1) * sizeof(wchar_t));

        if (names[*count] == NULL)
        {
            fprintf(stderr, "ERROR: Failed to allocate memory for object name\n");
            NCryptFreeObject(hProvider);
            return false;
        }

        // Copy the name
        swprintf(names[*count], (wcslen(keyList->pszName) + 1) * sizeof(wchar_t), L"%ls", keyList->pszName);

        // Increment the count of keys
        (*count)++;
    }

    // Free the key list
    NCryptFreeBuffer(keyList);

    // Close the TPM storage provider
    NCryptFreeObject(hProvider);

    // Free the enumeration state
    NCryptFreeBuffer(ppEnumState);

    return true;
#else
    (void) names;
    (void) count;
    return false;
#endif

}

/**
 * @brief Generate a BIP39 english mnemonic with the TPM
 *
 * Generates a BIP39 english mnemonic with the TPM storage provider
 *
 * @param mnemonic The generated mnemonic will be stored here
 * @param file_num The file number of the encrypted mnemonic
 * @param overwrite If true, overwrite the existing mnemonic
 *
 * @return True if the mnemonic was successfully generated, false otherwise
 */
LIBDOGECOIN_API dogecoin_bool generateRandomEnglishMnemonicTPM(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite)
{

    // Generate an English mnemonic with the TPM
    return dogecoin_generate_mnemonic_encrypt_with_tpm(mnemonic, file_num, overwrite, "eng", " ", NULL);
}

/**
 * @brief Generate a BIP39 english mnemonic with software encryption
 *
 * Generates a BIP39 english mnemonic with software-based encryption
 *
 * @param mnemonic The generated mnemonic will be stored here
 * @param file_num The file number of the encrypted mnemonic
 * @param overwrite If true, overwrite the existing mnemonic
 *
 * @return True if the mnemonic was successfully generated, false otherwise
 */
LIBDOGECOIN_API dogecoin_bool generateRandomEnglishMnemonicSW(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite)
{

    // Generate an English mnemonic with software encryption
    return dogecoin_generate_mnemonic_encrypt_with_sw(mnemonic, file_num, overwrite, "eng", " ", NULL, NULL);
}
