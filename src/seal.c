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
#define TEST_PASS "12345678"
#define PASS_MAX_LEN 100
#define PBKDF2_ITERATIONS 10000
#define ENCRYPTED_SEED_SIZE 64 // AES-256 CBC encrypted seed, no padding
#define ENCRYPTED_MNEMONIC_SIZE 768 // AES-256 CBC encrypted  mnemonic, no padding

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
 * @brief Gets a password from the user
 *
 * Gets a password from the user without echoing the input to the console.
 *
 * @param[in] prompt The prompt to display to the user
 * @return The password entered by the user
 */
#ifndef TEST_PASSWD
char *getpass(const char *prompt) {
    char buffer[PASS_MAX_LEN] = {0};  // Initialize to zero

#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode, count;

    if (!GetConsoleMode(hStdin, &mode) || !SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT))
        return NULL;

    printf("%s", prompt);
    fflush(stdout);

    if (!ReadConsole(hStdin, buffer, sizeof(buffer) - 1, &count, NULL))
        return NULL;  // -1 to ensure null-termination

    if (!SetConsoleMode(hStdin, mode))
        return NULL;

    buffer[count] = '\0';  // Ensure null-termination

#else
    struct termios old, new;
    ssize_t nread;

    if (tcgetattr(STDIN_FILENO, &old) != 0)
        return NULL;

    new = old;
    new.c_lflag &= ~ECHO;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new) != 0)
        return NULL;

    printf("%s", prompt);
    fflush(stdout);

    if (!fgets(buffer, sizeof(buffer), stdin))
        return NULL;

    nread = strlen(buffer);
    if (nread > 0 && buffer[nread-1] == '\n')
        buffer[nread-1] = '\0';  // Remove newline character

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &old) != 0)
        return NULL;

#endif

    return strdup(buffer);
}
#endif

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

    // Declare variables
    SECURITY_STATUS status;
    NCRYPT_PROV_HANDLE hProvider;
    NCRYPT_KEY_HANDLE hEncryptionKey;
    DWORD cbResult;
    PBYTE pbOutput = NULL;
    DWORD cbOutput = 0;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Format the name of the encrypted seed object
    wchar_t* name = SEED_OBJECT_NAME_FORMAT;
    swprintf(name, (wcslen(name) + 1) * sizeof(wchar_t), SEED_OBJECT_NAME_FORMAT, file_num);

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

    // Successfully encrypted the seed
    // Create a file with the encrypted seed
    // Open the file for binary write, "wb+" to overwrite if exists
    FILE* fp = _wfopen(name, overwrite ? L"wb+" : L"wb");
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
    wchar_t* name = SEED_OBJECT_NAME_FORMAT;
    swprintf(name, (wcslen(name) + 1) * sizeof(wchar_t), SEED_OBJECT_NAME_FORMAT, file_num);

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
    FILE* fp = _wfopen(name, L"rb");
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
 * @return Returns true if the seed is encrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_encrypt_seed_with_sw(const SEED seed, const size_t size, const int file_num, const dogecoin_bool overwrite) {

    char filename[PASS_MAX_LEN];
    snprintf(filename, sizeof(filename), "dogecoin_seed_%03d_sw", file_num);

    // Check if the file already exists and if not, prompt for overwriting
    if (!overwrite && access(filename, F_OK) != -1) {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    strcpy(password, TEST_PASS);
#else
    password = getpass("Enter password for seed encryption: \n");
#endif
    if (strlen(password) == 0) {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Confirm the password
    char* confirm_password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    strcpy(confirm_password, TEST_PASS);
#else
    confirm_password = getpass("Confirm password: \n");
#endif
    if (strcmp(password, confirm_password) != 0) {
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

    // Generate a random salt for key derivation
    uint8_t salt[SALT_SIZE];
    dogecoin_random_bytes(salt, sizeof(salt), 1);

    // Derive the encryption key from the password and salt using PBKDF2
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt, sizeof(salt), PBKDF2_ITERATIONS, encryption_key, sizeof(encryption_key));

    // Generate a random IV for AES encryption
    uint8_t iv[AES_IV_SIZE];
    dogecoin_random_bytes(iv, sizeof(iv), 1);

    // Encrypt the seed using AES
    size_t encrypted_size = size;
    dogecoin_bool padding_used = false;
    uint8_t* encrypted_seed = malloc(encrypted_size);
    if (!encrypted_seed) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        return false;
    }

    aes256_cbc_encrypt(encryption_key, iv, seed, size, padding_used, encrypted_seed);

    // Write the IV and salt to the file
    FILE* fp = fopen(filename, "wb+");
    if (!fp) {
        fprintf(stderr, "ERROR: Failed to open file for writing.\n");
        dogecoin_free(encrypted_seed);
        return false;
    }

    // Hash the password
    uint8_t password_hash[SHA512_DIGEST_LENGTH];
    sha512_raw((const uint8_t*)password, strlen(password), password_hash);

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Write the password hash to the file
    fwrite(password_hash, 1, sizeof(password_hash), fp);

    fwrite(iv, 1, sizeof(iv), fp);
    fwrite(salt, 1, sizeof(salt), fp);

    // Write the encrypted seed to the file
    fwrite(encrypted_seed, 1, encrypted_size, fp);

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
 * @return Returns true if the seed is decrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_seed_with_sw(SEED seed, const int file_num) {

    char filename[PASS_MAX_LEN];
    snprintf(filename, sizeof(filename), "dogecoin_seed_%03d_sw", file_num);

    // Check if the file exists
    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "ERROR: File does not exist.\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    strcpy(password, TEST_PASS);
#else
    password = getpass("Enter password for seed decryption: \n");
#endif
    if (strlen(password) == 0) {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Open the file for reading
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: Failed to open file for reading.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the password hash from the file
    uint8_t password_hash[SHA512_DIGEST_LENGTH];
    size_t bytesRead = fread(password_hash, 1, sizeof(password_hash), fp);
    if (bytesRead != sizeof(password_hash)) {
        fprintf(stderr, "ERROR: Failed to read password hash from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Hash the password
    uint8_t password_hash2[SHA512_DIGEST_LENGTH];
    sha512_raw((const uint8_t*)password, strlen(password), password_hash2);

    // Compare the password hashes
    if (memcmp(password_hash, password_hash2, sizeof(password_hash)) != 0) {
        fprintf(stderr, "ERROR: Incorrect password.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the IV and salt from the file
    uint8_t iv[AES_IV_SIZE];
    uint8_t salt[SALT_SIZE];
    bytesRead = fread(iv, 1, sizeof(iv), fp);
    if (bytesRead != sizeof(iv)) {
        fprintf(stderr, "ERROR: Failed to read IV from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }
    bytesRead = fread(salt, 1, sizeof(salt), fp);
    if (bytesRead != sizeof(salt)) {
        fprintf(stderr, "ERROR: Failed to read salt from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the encryption key from the password and salt using PBKDF2
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt, sizeof(salt), PBKDF2_ITERATIONS, encryption_key, sizeof(encryption_key));

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Read the encrypted seed from the file
    size_t encrypted_size = ENCRYPTED_SEED_SIZE;
    uint8_t* encrypted_seed = malloc(encrypted_size);
    if (!encrypted_seed) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(fp);
        return false;
    }

    bytesRead = fread(encrypted_seed, 1, encrypted_size, fp);
    if (bytesRead != encrypted_size) {
        fprintf(stderr, "ERROR: Failed to read encrypted seed from file.\n");
        fclose(fp);
        dogecoin_free(encrypted_seed);
        return false;
    }

    fclose(fp);

    // Decrypt the seed using AES
    aes256_cbc_decrypt(encryption_key, iv, encrypted_seed, encrypted_size, false, seed);

    dogecoin_free(encrypted_seed);

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
    if (!fileValid(file_num)) {
        fprintf(stderr, "ERROR: Invalid file number\n");
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
    wchar_t* name = HDNODE_OBJECT_NAME_FORMAT;
    swprintf(name, (wcslen(name) + 1) * sizeof(wchar_t), HDNODE_OBJECT_NAME_FORMAT, file_num);

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

    // Successfully encrypted the HD node with the encryption key
    // Create a file with the encrypted HD node
    // Open the file for binary write, "wb+" to overwrite if exists
    FILE* fp = _wfopen(name, overwrite ? L"wb+" : L"wb");
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
    if (out == NULL) {
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
    wchar_t* name = HDNODE_OBJECT_NAME_FORMAT;
    swprintf(name, (wcslen(name) + 1) * sizeof(wchar_t), HDNODE_OBJECT_NAME_FORMAT, file_num);

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
    FILE* fp = _wfopen(name, L"rb");
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
 * @return Returns true if the HD node is generated and encrypted successfully, false otherwise.
 */
dogecoin_bool dogecoin_generate_hdnode_encrypt_with_sw(dogecoin_hdnode* out, const int file_num, dogecoin_bool overwrite) {

    if (out == NULL) {
        fprintf(stderr, "ERROR: Invalid HD node\n");
        return false;
    }

    char filename[PASS_MAX_LEN];
    snprintf(filename, sizeof(filename), "dogecoin_master_%03d_sw", file_num);

    // Check if the file already exists and prompt for overwriting
    if (!overwrite && access(filename, F_OK) != -1) {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    strcpy(password, TEST_PASS);
#else
    password = getpass("Enter password for HD node encryption: \n");
#endif
    if (strlen(password) == 0) {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Confirm the password
    char* confirm_password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    strcpy(confirm_password, TEST_PASS);
#else
    confirm_password = getpass("Confirm password: \n");
#endif

    if (strcmp(password, confirm_password) != 0) {
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

    // Generate a random salt for key derivation
    uint8_t salt[SALT_SIZE];
    dogecoin_random_bytes(salt, sizeof(salt), 1);

    // Derive the encryption key from the password and salt using PBKDF2
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt, sizeof(salt), PBKDF2_ITERATIONS, encryption_key, sizeof(encryption_key));

    // Generate a random seed
    SEED seed = {0};
    dogecoin_random_bytes(seed, sizeof(seed), 1);

    // Derive the HD node from the seed
    dogecoin_hdnode_from_seed(seed, sizeof(seed), out);

    // Encrypt the HD node with software encryption (AES)
    uint8_t iv[AES_IV_SIZE];
    dogecoin_random_bytes(iv, sizeof(iv), 1);

    size_t encrypted_size = sizeof(dogecoin_hdnode);
    dogecoin_bool padding_used = false;
    uint8_t* encrypted_data = malloc(encrypted_size);
    if (!encrypted_data) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        return false;
    }

    size_t encrypted_actual_size = aes256_cbc_encrypt(encryption_key, iv, (uint8_t*)out, encrypted_size, padding_used, encrypted_data);
    if (encrypted_actual_size == 0) {
        fprintf(stderr, "ERROR: AES encryption failed.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        dogecoin_free(encrypted_data);
        return false;
    }

    // Open the file for writing
    FILE* fp = fopen(filename, "wb+");
    if (!fp) {
        fprintf(stderr, "ERROR: Failed to open file for writing.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        dogecoin_free(encrypted_data);
        return false;
    }

    // Hash the password
    uint8_t password_hash[SHA512_DIGEST_LENGTH];
    sha512_raw((const uint8_t*)password, strlen(password), password_hash);

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Write the password hash to the file
    fwrite(password_hash, 1, sizeof(password_hash), fp);

    // Write the IV and salt to the file
    size_t bytes_written = fwrite(iv, 1, sizeof(iv), fp);
    if (bytes_written != sizeof(iv)) {
        fprintf(stderr, "ERROR: Failed to write IV to file.\n");
        dogecoin_free(encrypted_data);
        fclose(fp);
        return false;
    }

    bytes_written = fwrite(salt, 1, sizeof(salt), fp);
    if (bytes_written != sizeof(salt)) {
        fprintf(stderr, "ERROR: Failed to write salt to file.\n");
        dogecoin_free(encrypted_data);
        fclose(fp);
        return false;
    }

    // Write the encrypted HD node to the file
    bytes_written = fwrite(encrypted_data, 1, encrypted_actual_size, fp);
    if (bytes_written != encrypted_actual_size) {
        fprintf(stderr, "ERROR: Failed to write encrypted HD node to file.\n");
        dogecoin_free(encrypted_data);
        fclose(fp);
        return false;
    }

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
 * @return Returns true if the HD node is decrypted successfully, false otherwise.
 */
dogecoin_bool dogecoin_decrypt_hdnode_with_sw(dogecoin_hdnode* out, const int file_num) {

    if (out == NULL) {
        fprintf(stderr, "ERROR: Invalid HD node\n");
        return false;
    }

    char filename[PASS_MAX_LEN];
    snprintf(filename, sizeof(filename), "dogecoin_master_%03d_sw", file_num);

    // Check if the file exists
    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "ERROR: File does not exist.\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    strcpy(password, TEST_PASS);
#else
    password = getpass("Enter password for HD node decryption: \n");
#endif
    if (strlen(password) == 0) {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Read the IV and salt from the file
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: Failed to open file for reading.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the password hash from the file
    uint8_t password_hash[SHA512_DIGEST_LENGTH];
    size_t bytesRead = fread(password_hash, 1, sizeof(password_hash), fp);
    if (bytesRead != sizeof(password_hash)) {
        fprintf(stderr, "ERROR: Failed to read password hash from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Hash the password
    uint8_t password_hash2[SHA512_DIGEST_LENGTH];
    sha512_raw((const uint8_t*)password, strlen(password), password_hash2);

    // Compare the password hashes
    if (memcmp(password_hash, password_hash2, sizeof(password_hash)) != 0) {
        fprintf(stderr, "ERROR: Incorrect password.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    uint8_t iv[AES_IV_SIZE];
    size_t bytes_read = fread(iv, 1, sizeof(iv), fp);
    if (bytes_read != sizeof(iv)) {
        fprintf(stderr, "ERROR: Failed to read IV from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    uint8_t salt[SALT_SIZE];
    bytes_read = fread(salt, 1, sizeof(salt), fp);
    if (bytes_read != sizeof(salt)) {
        fprintf(stderr, "ERROR: Failed to read salt from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the encryption key from the password and salt using PBKDF2
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt, sizeof(salt), PBKDF2_ITERATIONS, encryption_key, sizeof(encryption_key));

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Read the encrypted HD node from the file
    size_t encrypted_size = sizeof(dogecoin_hdnode);
    uint8_t* encrypted_data = malloc(encrypted_size);
    if (!encrypted_data) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(fp);
        return false;
    }

    bytes_read = fread(encrypted_data, 1, encrypted_size, fp);
    fclose(fp);

    if (bytes_read != encrypted_size) {
        fprintf(stderr, "ERROR: Failed to read encrypted HD node from file.\n");
        dogecoin_free(encrypted_data);
        return false;
    }

    // Decrypt the HD node with software decryption (AES)
    dogecoin_bool padding_used = false;
    size_t decrypted_actual_size = aes256_cbc_decrypt(encryption_key, iv, encrypted_data, encrypted_size, padding_used, (uint8_t*)out);
    dogecoin_free(encrypted_data);

    if (decrypted_actual_size == 0) {
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
    if (mnemonic == NULL) {
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
    DWORD cbOutput = 0;
    DWORD dwFlags = 0; // Use NCRYPT_MACHINE_KEY_FLAG for machine-level keys or 0 for user-level keys

    // Format the name of the mnemonic
    wchar_t* name = MNEMONIC_OBJECT_NAME_FORMAT;
    swprintf(name, (wcslen(name) + 1) * sizeof(wchar_t), MNEMONIC_OBJECT_NAME_FORMAT, file_num);

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

    // Successfully encrypted the mnemonic
    // Create a file with the encrypted mnemonic
    // Open the file for binary write, "wb+" to overwrite if exists
    FILE* fp = _wfopen(name, overwrite ? L"wb+" : L"wb");
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
    if (mnemonic == NULL) {
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
    wchar_t* name = MNEMONIC_OBJECT_NAME_FORMAT;
    swprintf(name, (wcslen(name) + 1) * sizeof(wchar_t), MNEMONIC_OBJECT_NAME_FORMAT, file_num);

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
    FILE* fp = _wfopen(name, L"rb");
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
 * @return Returns true if the mnemonic is generated and encrypted successfully, false otherwise.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_generate_mnemonic_encrypt_with_sw(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite, const char* lang, const char* space, const char* words)
{

    // Validate the input parameters
    if (mnemonic == NULL) {
        fprintf(stderr, "ERROR: Invalid mnemonic\n");
        return false;
    }

    // Validate the file number
    if (!fileValid(file_num))
    {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    char filename[PASS_MAX_LEN];
    snprintf(filename, sizeof(filename), "dogecoin_mnemonic_%03d_sw", file_num);

    // Check if the file already exists and prompt for overwriting
    if (!overwrite && access(filename, F_OK) != -1) {
        fprintf(stderr, "ERROR: File already exists. Use overwrite flag to replace it.\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    strcpy(password, TEST_PASS);
#else
    password = getpass("Enter password for mnemonic encryption: \n");
#endif
    if (strlen(password) == 0) {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Confirm the password
    char* confirm_password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    strcpy(confirm_password, TEST_PASS);
#else
    confirm_password = getpass("Confirm password: \n");
#endif

    if (strcmp(password, confirm_password) != 0) {
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

    // Derive the encryption key from the password and a random salt using PBKDF2
    uint8_t salt[SALT_SIZE];
    dogecoin_random_bytes(salt, sizeof(salt), 1);

    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt, sizeof(salt), PBKDF2_ITERATIONS, encryption_key, sizeof(encryption_key));

    // Generate the BIP-39 mnemonic from the random data
    size_t mnemonicSize = 0;
    int mnemonicResult = dogecoin_generate_mnemonic("256", lang, space, NULL, words, NULL, &mnemonicSize, mnemonic);
    if (mnemonicResult == -1)
    {
        fprintf(stderr, "ERROR: Failed to generate mnemonic\n");
        return false;
    }

    // Encrypt the mnemonic with software encryption (AES)
    uint8_t iv[AES_IV_SIZE];
    dogecoin_random_bytes(iv, sizeof(iv), 1);

    size_t encrypted_size = ENCRYPTED_MNEMONIC_SIZE;
    dogecoin_bool padding_used = false;
    uint8_t* encrypted_data = malloc(encrypted_size);
    if (!encrypted_data) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        return false;
    }
    memset(encrypted_data, 0, encrypted_size);

    size_t encrypted_actual_size = aes256_cbc_encrypt(encryption_key, iv, (uint8_t*)mnemonic, encrypted_size, padding_used, encrypted_data);
    if (encrypted_actual_size == 0) {
        fprintf(stderr, "ERROR: AES encryption failed.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        dogecoin_free(encrypted_data);
        return false;
    }

    // Open the file for writing
    FILE* fp = fopen(filename, "wb+");
    if (!fp) {
        fprintf(stderr, "ERROR: Failed to open file for writing.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        dogecoin_free(encrypted_data);
        return false;
    }

    // Hash the password
    uint8_t password_hash[SHA512_DIGEST_LENGTH];
    sha512_raw((const uint8_t*)password, strlen(password), password_hash);

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Write the password hash to the file
    fwrite(password_hash, 1, sizeof(password_hash), fp);

    // Write the IV and salt to the file
    size_t bytes_written = fwrite(iv, 1, sizeof(iv), fp);
    if (bytes_written != sizeof(iv)) {
        fprintf(stderr, "ERROR: Failed to write IV to file.\n");
        dogecoin_free(encrypted_data);
        fclose(fp);
        return false;
    }

    bytes_written = fwrite(salt, 1, sizeof(salt), fp);
    if (bytes_written != sizeof(salt)) {
        fprintf(stderr, "ERROR: Failed to write salt to file.\n");
        dogecoin_free(encrypted_data);
        fclose(fp);
        return false;
    }

    // Write encrypted mnemonic to the file
    bytes_written = fwrite(encrypted_data, 1, encrypted_actual_size, fp);
    if (bytes_written != encrypted_actual_size) {
        fprintf(stderr, "ERROR: Failed to write encrypted mnemonic to file.\n");
        dogecoin_free(encrypted_data);
        fclose(fp);
        return false;
    }

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
 *
 * @return True if the mnemonic was successfully decrypted, false otherwise
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_decrypt_mnemonic_with_sw(MNEMONIC mnemonic, const int file_num)
{

    // Validate the input parameters
    if (mnemonic == NULL) {
        fprintf(stderr, "ERROR: Invalid mnemonic\n");
        return false;
    }

    char filename[PASS_MAX_LEN];
    snprintf(filename, sizeof(filename), "dogecoin_mnemonic_%03d_sw", file_num);

    // Check if the file exists
    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "ERROR: File does not exist.\n");
        return false;
    }

    // Prompt for the password
    char* password = malloc(PASS_MAX_LEN);
#ifdef TEST_PASSWD
    strcpy(password, TEST_PASS);
#else
    password = getpass("Enter password for mnemonic decryption: \n");
#endif
    if (strlen(password) == 0) {
        fprintf(stderr, "ERROR: Password cannot be empty.\n");
        dogecoin_free(password);
        return false;
    }

    // Read the IV and encrypted mnemonic from the file
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: Failed to open file for reading.\n");
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Read the password hash from the file
    uint8_t password_hash[SHA512_DIGEST_LENGTH];
    size_t bytesRead = fread(password_hash, 1, sizeof(password_hash), fp);
    if (bytesRead != sizeof(password_hash)) {
        fprintf(stderr, "ERROR: Failed to read password hash from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Hash the password
    uint8_t password_hash2[SHA512_DIGEST_LENGTH];
    sha512_raw((const uint8_t*)password, strlen(password), password_hash2);

    // Compare the password hashes
    if (memcmp(password_hash, password_hash2, sizeof(password_hash)) != 0) {
        fprintf(stderr, "ERROR: Incorrect password.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    uint8_t iv[AES_IV_SIZE];
    size_t bytes_read = fread(iv, 1, sizeof(iv), fp);
    if (bytes_read != sizeof(iv)) {
        fprintf(stderr, "ERROR: Failed to read IV from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    uint8_t salt[SALT_SIZE];
    bytes_read = fread(salt, 1, sizeof(salt), fp);
    if (bytes_read != sizeof(salt)) {
        fprintf(stderr, "ERROR: Failed to read salt from file.\n");
        fclose(fp);
        dogecoin_mem_zero(password, strlen(password));
        dogecoin_free(password);
        return false;
    }

    // Derive the encryption key from the password and salt using PBKDF2
    uint8_t encryption_key[AES_KEY_SIZE];
    pbkdf2_hmac_sha256((const uint8_t*)password, strlen(password), salt, sizeof(salt), PBKDF2_ITERATIONS, encryption_key, sizeof(encryption_key));

    // Clear the password
    dogecoin_mem_zero(password, strlen(password));
    dogecoin_free(password);

    // Read the encrypted mnemonic from the file
    size_t encrypted_size = ENCRYPTED_MNEMONIC_SIZE;
    uint8_t* encrypted_data = malloc(encrypted_size);
    if (!encrypted_data) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(fp);
        return false;
    }

    bytes_read = fread(encrypted_data, 1, encrypted_size, fp);
    fclose(fp);

    if (bytes_read != encrypted_size) {
        fprintf(stderr, "ERROR: Failed to read encrypted mnemonic from file.\n");
        dogecoin_free(encrypted_data);
        return false;
    }

    // Decrypt the mnemonic with software decryption (AES)
    dogecoin_bool padding_used = false;
    size_t decrypted_actual_size = aes256_cbc_decrypt(encryption_key, iv, encrypted_data, encrypted_size, padding_used, (uint8_t*)mnemonic);
    dogecoin_free(encrypted_data);

    if (decrypted_actual_size == 0) {
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
    // Validate the input parameters
    if (!fileValid(file_num)) {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

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
    // Validate the input parameters
    if (!fileValid(file_num)) {
        fprintf(stderr, "ERROR: Invalid file number\n");
        return false;
    }

    // Generate an English mnemonic with software encryption
    return dogecoin_generate_mnemonic_encrypt_with_sw(mnemonic, file_num, overwrite, "eng", " ", NULL);
}
