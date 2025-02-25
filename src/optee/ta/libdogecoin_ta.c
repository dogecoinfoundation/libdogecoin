/**
 * Copyright (c) 2024 edtubbs
 * Copyright (c) 2024 The Dogecoin Foundation
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

/*
 * Copyright (c) 2017, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <inttypes.h>
#include <libdogecoin.h>
#include <libdogecoin_ta.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#define MAX_AUTH_TOKEN_SIZE 64

// Function prototypes for stubs
void exit(int status);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
int fflush(void *stream);
char *fgets(char *str, int n, void *stream);
void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function);
void assert(int expression);
int *__errno_location(void);
size_t fread(void *ptr, size_t size, size_t nmemb, void *stream);
void hmac_sha256(const uint8_t* key, const size_t keylen, const uint8_t* msg, const size_t msglen, uint8_t* hmac);
char* strcat(char *dest, const char *src);
char* strncat(char *dest, const char *src, size_t n);
char* strlcat(char *dst, const char *src, size_t size);
long int strtol (const char* str, char** endptr, int base);
int fprintf(void *stream, const char *format, ...);
int fopen(const char *path, const char *mode);
int fclose(void *stream);

// Function prototypes for the TA
void set_rng(void (*ptr)(void *, uint32_t));

#define AUTH_TOKEN_LEN 6 // 6-digit TOTP
uint32_t get_totp(const char* shared_secret, uint64_t timestamp);

// Stubs for functions that are not supported by OP-TEE
void exit(int status)
{
    EMSG("exit called with status %d", status);
    TEE_Panic(status);
    while(1); // Add an infinite loop to ensure the function does not return
}

unsigned long long strtoull(const char *nptr, char **endptr, int base)
{
    const char *pch = nptr;
    unsigned long long idx = 0;

    while (*pch >= '0' && *pch <= '9') {
        idx = (idx * base) + (*pch - '0');
        pch++;
    }

    if (endptr) {
        *endptr = (char *)pch;
    }

    return idx;
}

int fflush(void *stream) {
    // OP-TEE does not support fflush. This is a stub.
    (void)stream; // Avoid unused parameter warning
    EMSG("fflush not supported");
    return 0;
}

void *stdin = NULL; // OP-TEE does not support stdin. This is a stub.

char *fgets(char *str, int n, void *stream) {
    // OP-TEE does not support fgets. This is a stub.
    (void)n; // Avoid unused parameter warning
    (void)stream; // Avoid unused parameter warning
    EMSG("fgets not supported");
    return str;
}

void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function)
{
    EMSG("Assertion failed: %s (%s: %s: %u)\n", assertion, file, function, line);
    TEE_Panic(0);
}

void assert(int expression)
{
    if (!expression) {
        EMSG("Assertion failed\n");
        TEE_Panic(0);
    }
}

int *__errno_location(void)
{
    static int errno_val = 0;
    EMSG("errno_location called");
    return &errno_val;
}

size_t fread(void *ptr, size_t size, size_t nmemb, void *stream)
{
    // OP-TEE does not support fread. This is a stub.
    (void)ptr; // Avoid unused parameter warning
    (void)size; // Avoid unused parameter warning
    (void)stream; // Avoid unused parameter warning
    EMSG("fread not supported");
    return nmemb;
}

char* strcat(char *dest, const char *src)
{
    // Call strlcat
    strlcat(dest, src, strlen(dest) + strlen(src) + 1);
    return dest;
}

char* strncat(char *dest, const char *src, size_t n)
{
    // Call strlcat
    strlcat(dest, src, strlen(dest) + n + 1);
    return dest;
}

long int strtol (const char* str, char** endptr, int base)
{
    // Call strtoul
    return (long int)strtoul(str, endptr, base);
}

int fprintf(void *stream, const char *format, ...)
{
    (void)stream; // ignore the stream parameter as EMSG doesn't use it

    va_list ap;
    va_start(ap, format);
    char buf[256]; // buffer for the formatted message
    vsnprintf(buf, sizeof(buf), format, ap);
    EMSG("%s", buf);
    va_end(ap);

    return 0; // EMSG doesn't return a value, so return 0
}

int fopen(const char *path, const char *mode)
{
    // OP-TEE does not support fopen. This is a stub.
    (void)path; // Avoid unused parameter warning
    (void)mode; // Avoid unused parameter warning
    EMSG("Cannot open file %s", path);
    EMSG("fopen not supported");
    return 0;
}

int fclose(void *stream)
{
    // OP-TEE does not support fclose. This is a stub.
    (void)stream; // Avoid unused parameter warning
    EMSG("fclose not supported");
    return 0;
}

// TOTP Time step in seconds
#define TOTP_TIME_STEP 30

// TOTP Shared secret size in bytes
#define TOTP_SECRET_SIZE 20

// Maximum size of managed credentials (shared secret, password)
#define MAX_MANAGED_CREDS_SIZE  1024

uint32_t get_totp(const char* shared_secret, uint64_t timestamp) {
    uint8_t hmac[SHA1_DIGEST_LENGTH];
    uint8_t time_bytes[8];
    uint32_t totp;

    // Convert the timestamp to a byte array
    for (int i = 7; i >= 0; i--) {
        time_bytes[i] = timestamp & 0xFF;
        timestamp >>= 8;
    }

    // Compute the HMAC-SHA1 of the time_bytes using the shared_secret
    hmac_sha1((const uint8_t*)utils_hex_to_uint8(shared_secret), TOTP_SECRET_SIZE, time_bytes, sizeof(time_bytes), hmac);

    // Dynamic truncation to get a 4-byte string (31 bits)
    int offset = hmac[SHA1_DIGEST_LENGTH - 1] & 0x0F;  // Corrected the index to 19
    totp = (hmac[offset] & 0x7F) << 24
         | (hmac[offset + 1] & 0xFF) << 16
         | (hmac[offset + 2] & 0xFF) << 8
         | (hmac[offset + 3] & 0xFF);

    // Return TOTP modulo 10^6
    return totp % 1000000;
}

/*
 * Generate and store a random seed in secure storage
 */
static TEE_Result generate_and_store_seed(uint32_t param_types, TEE_Param params[4]) {
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT,
                        TEE_PARAM_TYPE_NONE,
                        TEE_PARAM_TYPE_NONE,
                        TEE_PARAM_TYPE_NONE);

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    TEE_ObjectHandle object;
    TEE_Result res;
    SEED seed;
    uint32_t obj_data_flag = TEE_DATA_FLAG_ACCESS_WRITE | TEE_DATA_FLAG_ACCESS_READ |
                             TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_OVERWRITE;

    // Generate a random seed
    TEE_GenerateRandom(seed, sizeof(seed));

    // Create a persistent object to store the seed
    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
                                     "seed_object", sizeof("seed_object"),
                                     obj_data_flag,
                                     TEE_HANDLE_NULL, NULL, 0, &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to create persistent seed object, res=0x%08x", res);
        return res;
    }

    // Write the seed into the persistent object
    res = TEE_WriteObjectData(object, seed, sizeof(seed));
    if (res != TEE_SUCCESS) {
        EMSG("Failed to write seed into persistent object, res=0x%08x", res);
        TEE_CloseAndDeletePersistentObject1(object);
        return res;
    }

    TEE_CloseObject(object);

    // Check if the provided buffer is large enough
    if (params[0].memref.size < sizeof(seed)) {
        params[0].memref.size = sizeof(seed);
        return TEE_ERROR_SHORT_BUFFER;
    }

    // Optionally, return the seed to the caller
    TEE_MemMove(params[0].memref.buffer, seed, sizeof(seed));
    params[0].memref.size = sizeof(seed);

    return TEE_SUCCESS;
}

static TEE_Result generate_and_store_master_key(uint32_t param_types, TEE_Param params[4]) {
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE);

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    TEE_ObjectHandle object;
    TEE_Result res;
    uint32_t obj_data_flag = TEE_DATA_FLAG_ACCESS_WRITE | TEE_DATA_FLAG_ACCESS_READ |
                             TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_OVERWRITE;

    // Initialize the random number generator
    set_rng(&TEE_GenerateRandom);

    dogecoin_ecc_start();

    char hd_master_privkey[HDKEYLEN];
    char p2pkh_master_pubkey[P2PKHLEN];
    if (!generateHDMasterPubKeypair(hd_master_privkey, p2pkh_master_pubkey, 0)) {
        dogecoin_ecc_stop();
        EMSG("Failed to generate master keypair");
        return TEE_ERROR_GENERIC;
    }

    dogecoin_ecc_stop();

    // Create a persistent object to store the serialized HDNode
    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE, "hd_master_privkey", sizeof("hd_master_privkey"),
                                     obj_data_flag, TEE_HANDLE_NULL, NULL, 0, &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to create persistent master key object, res=0x%08x", res);
        return res;
    }

    // Write the serialized HDNode into the persistent object
    res = TEE_WriteObjectData(object, hd_master_privkey, HDKEYLEN);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to write master key into persistent object, res=0x%08x", res);
        TEE_CloseAndDeletePersistentObject1(object);
        return res;
    }

    TEE_CloseObject(object);

    EMSG("Successfully wrote master key into persistent object");

    // Check if the provided buffer is large enough
    if (params[0].memref.size < HDKEYLEN) {
        params[0].memref.size = HDKEYLEN;
        return TEE_ERROR_SHORT_BUFFER;
    }

    // Optionally, return the master key to the caller
    TEE_MemMove(params[0].memref.buffer, hd_master_privkey, HDKEYLEN);
    params[0].memref.size = HDKEYLEN;

    return TEE_SUCCESS;
}

static TEE_Result generate_and_store_mnemonic(uint32_t param_types, TEE_Param params[4]) {
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_MEMREF_INPUT);

    if (param_types != exp_param_types) {
        EMSG("Bad parameter types: 0x%08x", param_types);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    TEE_ObjectHandle object;
    TEE_Result res;
    uint32_t obj_data_flag = TEE_DATA_FLAG_ACCESS_WRITE | TEE_DATA_FLAG_ACCESS_READ |
                             TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_OVERWRITE;

    MNEMONIC mnemonic = {0};
    SEED seed = {0};
    char managed_creds[MAX_MANAGED_CREDS_SIZE] = {0};
    ENTROPY_SIZE entropy_size = {0};

    EMSG("Starting mnemonic generation");

    // Initialize the random number generator
    set_rng(&TEE_GenerateRandom);

    dogecoin_ecc_start();

    if (params[2].memref.size > 0) {
        // Copy the mnemonic input from params
        TEE_MemMove(mnemonic, params[2].memref.buffer, params[2].memref.size);
        if (dogecoin_seed_from_mnemonic((const char*)mnemonic, NULL, seed) != 0) {
            EMSG("Failed to generate seed from mnemonic");
            dogecoin_ecc_stop();
            return TEE_ERROR_GENERIC;
        }
    } else {
        if (params[3].memref.size > 0) {
            // Copy the entropy size from params
            TEE_MemMove(entropy_size, params[3].memref.buffer, params[3].memref.size);

            // Generate the BIP-39 mnemonic
            int mnemonicResult = generateRandomEnglishMnemonic(entropy_size, mnemonic);
            if (mnemonicResult == -1) {
                EMSG("Failed to generate mnemonic");
                dogecoin_ecc_stop();
                return TEE_ERROR_GENERIC;
            }
        } else {
            int mnemonicResult = generateRandomEnglishMnemonic("256", mnemonic);
            if (mnemonicResult == -1) {
                EMSG("Failed to generate mnemonic");
                dogecoin_ecc_stop();
                return TEE_ERROR_GENERIC;
            }
        }
    }

    if (params[1].memref.size > 0 && params[1].memref.size <= MAX_MANAGED_CREDS_SIZE) {
        // Copy the managed credentials from params
        TEE_MemMove(managed_creds, params[1].memref.buffer, params[1].memref.size);
    } else {
        EMSG("Managed credentials not provided or invalid");
        dogecoin_ecc_stop();
        return TEE_ERROR_BAD_PARAMETERS;
    }

    // Concatenate the mnemonic and managed credentials
    char mnemonic_and_creds[MAX_MNEMONIC_SIZE + MAX_MANAGED_CREDS_SIZE] = {0};
    snprintf(mnemonic_and_creds, sizeof(mnemonic_and_creds), "%s,%s", mnemonic, managed_creds);

    dogecoin_ecc_stop();

    // Create a persistent object to store the mnemonic and managed credentials
    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE, "mnemonic", sizeof("mnemonic"),
                                     obj_data_flag, TEE_HANDLE_NULL, NULL, 0, &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to create persistent mnemonic object, res=0x%08x", res);
        return res;
    }

    // Write the mnemonic and managed credentials into the persistent object
    res = TEE_WriteObjectData(object, mnemonic_and_creds, strlen(mnemonic_and_creds));
    if (res != TEE_SUCCESS) {
        TEE_CloseAndDeletePersistentObject1(object);
        EMSG("Failed to write mnemonic and managed credentials into persistent object, res=0x%08x", res);
        return res;
    }

    // Hash and log the mnemonic and managed credentials
    uint8_t loghash[SHA256_DIGEST_LENGTH];

    // Compute and log the hash of the mnemonic and managed credentials
    sha256_raw((const uint8_t*) mnemonic_and_creds, sizeof(mnemonic_and_creds), loghash);
    EMSG("%s", utils_uint8_to_hex((const uint8_t*) loghash, SHA256_DIGEST_LENGTH));

    TEE_CloseObject(object);

    // Check if the provided buffer is large enough
    if (params[0].memref.size < strlen(mnemonic)) {
        params[0].memref.size = strlen(mnemonic);
        EMSG("Provided buffer is too small for mnemonic");
        return TEE_ERROR_SHORT_BUFFER;
    }

    // Return the mnemonic to the caller
    TEE_MemMove(params[0].memref.buffer, mnemonic, strlen(mnemonic));
    params[0].memref.size = strlen(mnemonic);

    return TEE_SUCCESS;
}

static TEE_Result generate_extended_public_key(uint32_t param_types, TEE_Param params[4]) {
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_VALUE_INPUT, TEE_PARAM_TYPE_MEMREF_INPUT);

    if (param_types != exp_param_types) {
        EMSG("Bad parameter types: 0x%08x", param_types);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    const char* key_path = (const char*)params[1].memref.buffer;
    uint32_t auth_token = params[2].value.a;
    const char* password = (const char*)params[3].memref.buffer;  // Optional password
    size_t password_size = password ? strlen(password) + 1 : 0;

    TEE_ObjectHandle object;
    TEE_Result res;
    char pubkey[HDKEYLEN];
    uint32_t read_bytes;

    char persistent_data[MAX_MNEMONIC_SIZE + MAX_MANAGED_CREDS_SIZE];

    EMSG("Generating extended public key");

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, "mnemonic", sizeof("mnemonic"),
                                   TEE_DATA_FLAG_ACCESS_READ, &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to open persistent mnemonic object, res=0x%08x", res);
        return res;
    }

    res = TEE_ReadObjectData(object, persistent_data, sizeof(persistent_data), &read_bytes);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to read mnemonic and managed credentials from persistent object, res=0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    TEE_CloseObject(object);

    // Split the persistent data into mnemonic and managed credentials
    char* saveptr;
    char* mnemonic = strtok_r(persistent_data, ",", &saveptr);
    char* shared_secret = strtok_r(NULL, ",", &saveptr);
    char* stored_password = strtok_r(NULL, ",", &saveptr);

    // Verify that the password is part of the managed credentials, if supplied
    if ((password_size > 0 && password && stored_password && strcmp(password, stored_password)) ||
        (password_size == 0 && stored_password && strcmp(stored_password, "none") != 0) ||
        (password_size > 0 && !password && stored_password && strcmp(stored_password, "none") != 0)) {
        EMSG("Password verification failed");
        return TEE_ERROR_SECURITY;
    }

    // Verify TOTP using the managed credentials
    TEE_Time current_time;
    TEE_GetREETime(&current_time);
    uint32_t totp = get_totp(shared_secret, current_time.seconds / TOTP_TIME_STEP);

    char totp_str[AUTH_TOKEN_LEN + 1];
    snprintf(totp_str, sizeof(totp_str), "%06u", totp);

    char auth_token_str[AUTH_TOKEN_LEN + 1];
    snprintf(auth_token_str, sizeof(auth_token_str), "%06u", auth_token);

    if (strcmp(totp_str, auth_token_str) != 0) {
        EMSG("TOTP verification failed");
        return TEE_ERROR_SECURITY;
    }

    SEED seed;
    dogecoin_seed_from_mnemonic((const char*)mnemonic, password, seed);

    set_rng(&TEE_GenerateRandom);

    dogecoin_ecc_start();

    char master_key[HDKEYLEN];
    char key_path_out[KEYPATHMAXLEN];
    getHDRootKeyFromSeed(seed, sizeof(seed), false, master_key);

    deriveBIP44ExtendedPublicKey(master_key, NULL, NULL, NULL, key_path, pubkey, key_path_out);

    dogecoin_ecc_stop();

    if (params[0].memref.size < strlen(pubkey) + 1) {
        params[0].memref.size = strlen(pubkey) + 1;
        EMSG("Buffer too small for extended public key");
        return TEE_ERROR_SHORT_BUFFER;
    }

    TEE_MemMove(params[0].memref.buffer, pubkey, strlen(pubkey) + 1);
    params[0].memref.size = strlen(pubkey) + 1;

    EMSG("Extended public key generated successfully");

    return TEE_SUCCESS;
}

static TEE_Result generate_address(uint32_t param_types, TEE_Param params[4]) {
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_VALUE_INPUT, TEE_PARAM_TYPE_MEMREF_INPUT);

    if (param_types != exp_param_types) {
        EMSG("Bad parameter types: 0x%08x", param_types);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    const char* key_path = (const char*)params[1].memref.buffer;
    uint32_t auth_token = params[2].value.a;
    const char* password = (const char*)params[3].memref.buffer;  // Optional password
    size_t password_size = password ? strlen(password) + 1 : 0;

    TEE_ObjectHandle object;
    TEE_Result res;
    char address[P2PKHLEN];
    uint32_t read_bytes;

    char persistent_data[MAX_MNEMONIC_SIZE + MAX_MANAGED_CREDS_SIZE];

    EMSG("Generating address");

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, "mnemonic", sizeof("mnemonic"),
                                   TEE_DATA_FLAG_ACCESS_READ, &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to open persistent mnemonic object, res=0x%08x", res);
        return res;
    }

    res = TEE_ReadObjectData(object, persistent_data, sizeof(persistent_data), &read_bytes);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to read mnemonic and managed credentials from persistent object, res=0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    TEE_CloseObject(object);

    // Split the persistent data into mnemonic and managed credentials
    char* saveptr;
    char* mnemonic = strtok_r(persistent_data, ",", &saveptr);
    char* shared_secret = strtok_r(NULL, ",", &saveptr);
    char* stored_password = strtok_r(NULL, ",", &saveptr);

    // Verify that the password is part of the managed credentials, if supplied
    if ((password_size > 0 && password && stored_password && strcmp(password, stored_password)) ||
        (password_size == 0 && stored_password && strcmp(stored_password, "none") != 0) ||
        (password_size > 0 && !password && stored_password && strcmp(stored_password, "none") != 0)) {
        EMSG("Password verification failed");
        return TEE_ERROR_SECURITY;
    }

    // Verify TOTP using the managed credentials
    TEE_Time current_time;
    TEE_GetREETime(&current_time);
    uint32_t totp = get_totp(shared_secret, current_time.seconds / TOTP_TIME_STEP);

    char totp_str[AUTH_TOKEN_LEN + 1];
    snprintf(totp_str, sizeof(totp_str), "%06u", totp);

    char auth_token_str[AUTH_TOKEN_LEN + 1];
    snprintf(auth_token_str, sizeof(auth_token_str), "%06u", auth_token);

    if (strcmp(totp_str, auth_token_str) != 0) {
        EMSG("TOTP verification failed");
        return TEE_ERROR_SECURITY;
    }

    SEED seed;
    dogecoin_seed_from_mnemonic((const char*)mnemonic, password, seed);

    set_rng(&TEE_GenerateRandom);

    dogecoin_ecc_start();

    char master_key[HDKEYLEN];
    getHDRootKeyFromSeed(seed, sizeof(seed), false, master_key);

    getDerivedHDAddressByPath(master_key, key_path, address, false);


    dogecoin_ecc_stop();

    if (params[0].memref.size < strlen(address) + 1) {
        params[0].memref.size = strlen(address) + 1;
        EMSG("Buffer too small for address");
        return TEE_ERROR_SHORT_BUFFER;
    }

    TEE_MemMove(params[0].memref.buffer, address, strlen(address) + 1);
    params[0].memref.size = strlen(address) + 1;

    EMSG("Address generated successfully");

    return TEE_SUCCESS;
}

static TEE_Result sign_message_with_private_key(uint32_t param_types, TEE_Param params[4]) {
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_VALUE_INPUT);

    if (param_types != exp_param_types) {
        EMSG("Bad parameter types: 0x%08x", param_types);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    char *saveptr;
    char *key_path = strtok_r(params[2].memref.buffer, ",", &saveptr);
    size_t password_size = params[3].value.b;
    char *password = password_size > 0 ? strtok_r(NULL, ",", &saveptr) : NULL;
    char *message = (char*)params[0].memref.buffer;
    uint32_t auth_token = params[3].value.a;

    TEE_ObjectHandle object;
    TEE_Result res;
    char *sig = NULL;
    uint32_t read_bytes;

    char persistent_data[MAX_MNEMONIC_SIZE + MAX_MANAGED_CREDS_SIZE];

    EMSG("Signing message");

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, "mnemonic", sizeof("mnemonic"),
                                   TEE_DATA_FLAG_ACCESS_READ, &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to open persistent mnemonic object, res=0x%08x", res);
        return res;
    }

    res = TEE_ReadObjectData(object, persistent_data, sizeof(persistent_data), &read_bytes);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to read mnemonic and managed credentials from persistent object, res=0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    TEE_CloseObject(object);

    // Split the persistent data into mnemonic and managed credentials
    char* saveptr2;
    char* mnemonic = strtok_r(persistent_data, ",", &saveptr2);
    char* shared_secret = strtok_r(NULL, ",", &saveptr2);
    char* stored_password = strtok_r(NULL, ",", &saveptr2);

    // Verify that the password is part of the managed credentials, if supplied
    if ((password_size > 0 && password && stored_password && strcmp(password, stored_password)) ||
        (password_size == 0 && stored_password && strcmp(stored_password, "none") != 0) ||
        (password_size > 0 && !password && stored_password && strcmp(stored_password, "none") != 0)) {
        EMSG("Password verification failed");
        return TEE_ERROR_SECURITY;
    }

    // Verify TOTP using the managed credentials
    TEE_Time current_time;
    TEE_GetREETime(&current_time);
    uint32_t totp = get_totp(shared_secret, current_time.seconds / TOTP_TIME_STEP);

    char totp_str[AUTH_TOKEN_LEN + 1];
    snprintf(totp_str, sizeof(totp_str), "%06u", totp);

    char auth_token_str[AUTH_TOKEN_LEN + 1];
    snprintf(auth_token_str, sizeof(auth_token_str), "%06u", auth_token);

    if (strcmp(totp_str, auth_token_str) != 0) {
        EMSG("TOTP verification failed");
        return TEE_ERROR_SECURITY;
    }

    SEED seed;
    dogecoin_seed_from_mnemonic((const char*)mnemonic, password, seed);

    set_rng(&TEE_GenerateRandom);

    dogecoin_ecc_start();

    char master_key[HDKEYLEN];
    getHDRootKeyFromSeed(seed, sizeof(seed), false, master_key);

    char outaddress[P2PKHLEN];
    char privkeywif[PRIVKEYWIFLEN];
    size_t wiflen = PRIVKEYWIFLEN;
    const dogecoin_chainparams* chain = chain_from_b58_prefix(master_key);

    dogecoin_hdnode* hdnode = getHDNodeAndExtKeyByPath(master_key, key_path, outaddress, true);
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, chain, privkeywif, &wiflen);

    sig = sign_message(privkeywif, message);
    if (!sig) {
        EMSG("Failed to sign message with private key");
        dogecoin_ecc_stop();
        return TEE_ERROR_GENERIC;
    }

    if (params[1].memref.size < strlen(sig) + 1) {
        params[1].memref.size = strlen(sig) + 1;
        dogecoin_free(sig);
        dogecoin_ecc_stop();
        return TEE_ERROR_SHORT_BUFFER;
    }

    TEE_MemMove(params[1].memref.buffer, sig, strlen(sig) + 1);
    params[1].memref.size = strlen(sig) + 1;

    dogecoin_free(sig);
    dogecoin_hdnode_free(hdnode);
    dogecoin_ecc_stop();

    EMSG("Message signed successfully");

    return TEE_SUCCESS;
}

static TEE_Result sign_transaction_with_private_key(uint32_t param_types, TEE_Param params[4]) {
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_VALUE_INPUT);

    if (param_types != exp_param_types) {
        EMSG("Bad parameter types: 0x%08x", param_types);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    char *saveptr;
    char *key_path = strtok_r(params[2].memref.buffer, ",", &saveptr);
    size_t password_size = params[3].value.b;
    char *password = password_size > 0 ? strtok_r(NULL, ",", &saveptr) : NULL;
    char *raw_tx = (char*)params[0].memref.buffer;
    uint32_t auth_token = params[3].value.a;

    TEE_ObjectHandle object;
    TEE_Result res;
    char *myscriptpubkey = NULL;
    char *signed_tx = NULL;
    uint32_t read_bytes;

    char persistent_data[MAX_MNEMONIC_SIZE + MAX_MANAGED_CREDS_SIZE];

    EMSG("Signing transaction");

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, "mnemonic", sizeof("mnemonic"),
                                   TEE_DATA_FLAG_ACCESS_READ, &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to open persistent mnemonic object, res=0x%08x", res);
        return res;
    }

    res = TEE_ReadObjectData(object, persistent_data, sizeof(persistent_data), &read_bytes);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to read mnemonic and managed credentials from persistent object, res=0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    TEE_CloseObject(object);

    // Split the persistent data into mnemonic and managed credentials
    char* saveptr2;
    char* mnemonic = strtok_r(persistent_data, ",", &saveptr2);
    char* shared_secret = strtok_r(NULL, ",", &saveptr2);
    char* stored_password = strtok_r(NULL, ",", &saveptr2);

    // Verify that the password is part of the managed credentials, if supplied
    if ((password_size > 0 && password && stored_password && strcmp(password, stored_password)) ||
        (password_size == 0 && stored_password && strcmp(stored_password, "none") != 0) ||
        (password_size > 0 && !password && stored_password && strcmp(stored_password, "none") != 0)) {
        EMSG("Password verification failed");
        return TEE_ERROR_SECURITY;
    }

    // Verify TOTP using the managed credentials
    TEE_Time current_time;
    TEE_GetREETime(&current_time);
    uint32_t totp = get_totp(shared_secret, current_time.seconds / TOTP_TIME_STEP);

    char totp_str[AUTH_TOKEN_LEN + 1];
    snprintf(totp_str, sizeof(totp_str), "%06u", totp);

    char auth_token_str[AUTH_TOKEN_LEN + 1];
    snprintf(auth_token_str, sizeof(auth_token_str), "%06u", auth_token);

    if (strcmp(totp_str, auth_token_str) != 0) {
        EMSG("TOTP verification failed");
        return TEE_ERROR_SECURITY;
    }

    SEED seed;
    dogecoin_seed_from_mnemonic((const char*)mnemonic, password, seed);

    set_rng(&TEE_GenerateRandom);

    dogecoin_ecc_start();

    char master_key[HDKEYLEN];
    getHDRootKeyFromSeed(seed, sizeof(seed), false, master_key);

    char outaddress[P2PKHLEN];
    char privkeywif[PRIVKEYWIFLEN];
    size_t wiflen = PRIVKEYWIFLEN;
    const dogecoin_chainparams* chain = chain_from_b58_prefix(master_key);
    dogecoin_hdnode* hdnode = getHDNodeAndExtKeyByPath(master_key, key_path, outaddress, true);
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, chain, privkeywif, &wiflen);

    int idx = store_raw_transaction(raw_tx);
    if (idx < 0) {
        EMSG("Failed to store raw transaction");
        dogecoin_ecc_stop();
        return TEE_ERROR_GENERIC;
    }

    myscriptpubkey = dogecoin_private_key_wif_to_pubkey_hash(privkeywif);
    if (!sign_transaction(idx, myscriptpubkey, privkeywif)) {
        EMSG("Failed to sign transaction");
        dogecoin_free(myscriptpubkey);
        dogecoin_ecc_stop();
        return TEE_ERROR_GENERIC;
    }

    signed_tx = get_raw_transaction(idx);
    if (!signed_tx) {
        EMSG("Failed to get signed transaction");
        dogecoin_free(myscriptpubkey);
        dogecoin_ecc_stop();
        return TEE_ERROR_GENERIC;
    }

    if (params[1].memref.size < strlen(signed_tx) + 1) {
        params[1].memref.size = strlen(signed_tx) + 1;
        dogecoin_free(signed_tx);
        dogecoin_free(myscriptpubkey);
        dogecoin_ecc_stop();
        return TEE_ERROR_SHORT_BUFFER;
    }

    TEE_MemMove(params[1].memref.buffer, signed_tx, strlen(signed_tx) + 1);
    params[1].memref.size = strlen(signed_tx) + 1;

    dogecoin_free(myscriptpubkey);
    dogecoin_hdnode_free(hdnode);
    dogecoin_ecc_stop();

    EMSG("Transaction signed successfully");

    return TEE_SUCCESS;
}

static TEE_Result delegate_key(uint32_t param_types, TEE_Param params[4]) {
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_VALUE_INPUT, TEE_PARAM_TYPE_MEMREF_INPUT);

    if (param_types != exp_param_types) {
        EMSG("Bad parameter types: 0x%08x", param_types);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    TEE_Result res;
    TEE_ObjectHandle object;
    uint32_t obj_data_flag = TEE_DATA_FLAG_ACCESS_WRITE | TEE_DATA_FLAG_ACCESS_READ |
                             TEE_DATA_FLAG_ACCESS_WRITE_META | TEE_DATA_FLAG_OVERWRITE;

    char *delegate_key = params[0].memref.buffer;
    const char *account = (const char *)params[1].memref.buffer;
    uint32_t auth_token = params[2].value.a;
    char *delegate_creds = params[3].memref.buffer;

    // Open the mnemonic persistent object
    char persistent_data[MAX_MNEMONIC_SIZE + MAX_MANAGED_CREDS_SIZE];
    uint32_t read_bytes;

    EMSG("Delegating key");

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, "mnemonic", sizeof("mnemonic"),
                                   TEE_DATA_FLAG_ACCESS_READ, &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to open persistent mnemonic object, res=0x%08x", res);
        return res;
    }

    res = TEE_ReadObjectData(object, persistent_data, sizeof(persistent_data), &read_bytes);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to read mnemonic and managed credentials from persistent object, res=0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    TEE_CloseObject(object);

    // Split the persistent data into shared_secret, stored_password, and flags
    char* saveptr;
    char* mnemonic = strtok_r(persistent_data, ",", &saveptr);
    char* shared_secret = strtok_r(NULL, ",", &saveptr);
    char* stored_password = strtok_r(NULL, ",", &saveptr);
    char* flags = strtok_r(NULL, ",", &saveptr);

    // Split the delegate credentials into delegate_password and password
    char* saveptr2;
    char* delegate_password = strtok_r(delegate_creds, ",", &saveptr2);
    char* password = strtok_r(NULL, ",", &saveptr2);

    // Verify flag for delegate key creation
    if (!flags || strcmp(flags, "delegate") != 0) {
        EMSG("Delegate key creation flag not set");
        return TEE_ERROR_SECURITY;
    }

    // Verify that the password is part of the managed credentials, if supplied
    if (strcmp(password, stored_password) != 0) {
        EMSG("Password verification failed");
        return TEE_ERROR_SECURITY;
    }

    // Verify delegate password was provided
    if (strcmp(delegate_password, "none") == 0) {
        EMSG("Delegate password not provided");
        return TEE_ERROR_BAD_PARAMETERS;
    }

    // Verify TOTP using the managed credentials
    TEE_Time current_time;
    TEE_GetREETime(&current_time);
    uint32_t totp = get_totp(shared_secret, current_time.seconds / TOTP_TIME_STEP);

    char totp_str[AUTH_TOKEN_LEN + 1];
    snprintf(totp_str, sizeof(totp_str), "%06u", totp);

    char auth_token_str[AUTH_TOKEN_LEN + 1];
    snprintf(auth_token_str, sizeof(auth_token_str), "%06u", auth_token);

    if (strcmp(totp_str, auth_token_str) != 0) {
        EMSG("TOTP verification failed");
        return TEE_ERROR_SECURITY;
    }

    // Store the delegate credentials and mnemonic with the account number as the ID
    char delegate_object_data[MAX_MNEMONIC_SIZE + MAX_MANAGED_CREDS_SIZE];
    snprintf(delegate_object_data, sizeof(delegate_object_data), "%s,%s", mnemonic, delegate_creds);

    char delegate_object_id[64];
    snprintf(delegate_object_id, sizeof(delegate_object_id), "delegate_%s", account);

    res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE, delegate_object_id, strlen(delegate_object_id),
                                     obj_data_flag, TEE_HANDLE_NULL, delegate_object_data, strlen(delegate_object_data), &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to create persistent delegate object, res=0x%08x", res);
        return res;
    }

    TEE_CloseObject(object);

    // Derive the private child key
    char key_path[BIP44_KEY_PATH_MAX_LENGTH + 1] = "";
    SEED seed;
    dogecoin_seed_from_mnemonic((const char*)mnemonic, delegate_password, seed);

    set_rng(&TEE_GenerateRandom);

    dogecoin_ecc_start();

    char master_key[HDKEYLEN];
    char privkey[HDKEYLEN];
    getHDRootKeyFromSeed(seed, sizeof(seed), false, master_key);

    deriveBIP44ExtendedKey(master_key, NULL, NULL, NULL, account, privkey, key_path);
    strncpy(delegate_key, privkey, HDKEYLEN);

    dogecoin_ecc_stop();

    EMSG("Delegate key generated and stored successfully");

    return TEE_SUCCESS;
}

static TEE_Result export_delegate_key(uint32_t param_types, TEE_Param params[4]) {
    const uint32_t exp_param_types =
        TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_MEMREF_INPUT);

    if (param_types != exp_param_types) {
        EMSG("Bad parameter types: 0x%08x", param_types);
        return TEE_ERROR_BAD_PARAMETERS;
    }

    TEE_Result res;
    TEE_ObjectHandle object;
    uint32_t read_bytes;
    const char *key_path = (const char *)params[1].memref.buffer;
    const char *password = (const char *)params[3].memref.buffer;  // Optional password
    char *exported_key = params[0].memref.buffer;

    // Open the delegate persistent object
    char delegate_object_id[64];
    snprintf(delegate_object_id, sizeof(delegate_object_id), "delegate_%s", key_path);

    EMSG("Exporting delegate key");

    res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE, delegate_object_id, strlen(delegate_object_id),
                                   TEE_DATA_FLAG_ACCESS_READ, &object);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to open persistent delegate object, res=0x%08x", res);
        return res;
    }

    char delegate_object_data[MAX_MNEMONIC_SIZE + MAX_MANAGED_CREDS_SIZE];
    res = TEE_ReadObjectData(object, delegate_object_data, sizeof(delegate_object_data), &read_bytes);
    if (res != TEE_SUCCESS) {
        EMSG("Failed to read delegate credentials and mnemonic from persistent object, res=0x%08x", res);
        TEE_CloseObject(object);
        return res;
    }

    TEE_CloseObject(object);

    // Split the delegate object data into mnemonic and stored delegate credentials
    char* saveptr;
    char* mnemonic = strtok_r(delegate_object_data, ",", &saveptr);
    char* stored_delegate_password = strtok_r(NULL, ",", &saveptr);

    // Verify that the password is the same as the stored delegate password
    if ((password && stored_delegate_password && strcmp(password, stored_delegate_password)) ||
        (!password && stored_delegate_password) ||
        (password && !stored_delegate_password)) {
        EMSG("Password verification failed");
        return TEE_ERROR_SECURITY;
    }

    SEED seed;
    dogecoin_seed_from_mnemonic((const char*)mnemonic, password, seed);

    set_rng(&TEE_GenerateRandom);

    dogecoin_ecc_start();

    char master_key[HDKEYLEN];
    char privkey[HDKEYLEN];
    char key_path_out[KEYPATHMAXLEN];
    getHDRootKeyFromSeed(seed, sizeof(seed), false, master_key);

    deriveBIP44ExtendedKey(master_key, NULL, NULL, NULL, key_path, privkey, key_path_out);
    strncpy(exported_key, privkey, HDKEYLEN);

    dogecoin_ecc_stop();

    EMSG("Delegate key exported successfully");

    return TEE_SUCCESS;
}

static TEE_Result delete_object(uint32_t param_types, TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	TEE_ObjectHandle object;
	TEE_Result res;
	char *obj_id;
	size_t obj_id_sz;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	obj_id_sz = params[0].memref.size;
	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	/*
	 * Check object exists and delete it
	 */
	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_ACCESS_WRITE_META, /* we must be allowed to delete it */
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open persistent object, res=0x%08x", res);
		TEE_Free(obj_id);
		return res;
	}

	TEE_CloseAndDeletePersistentObject1(object);
	TEE_Free(obj_id);

	return res;
}

static TEE_Result create_raw_object(uint32_t param_types, TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	TEE_ObjectHandle object;
	TEE_Result res;
	char *obj_id;
	size_t obj_id_sz;
	char *data;
	size_t data_sz;
	uint32_t obj_data_flag;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	obj_id_sz = params[0].memref.size;
	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	data_sz = params[1].memref.size;
	data = TEE_Malloc(data_sz, 0);
	if (!data)
		return TEE_ERROR_OUT_OF_MEMORY;
	TEE_MemMove(data, params[1].memref.buffer, data_sz);

	/*
	 * Create object in secure storage and fill with data
	 */
	obj_data_flag = TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
			TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
			TEE_DATA_FLAG_ACCESS_WRITE_META |	/* we can later destroy or rename the object */
			TEE_DATA_FLAG_OVERWRITE;		/* destroy existing object of same ID */

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					obj_data_flag,
					TEE_HANDLE_NULL,
					NULL, 0,		/* we may not fill it right now */
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_CreatePersistentObject failed 0x%08x", res);
		TEE_Free(obj_id);
		TEE_Free(data);
		return res;
	}

	res = TEE_WriteObjectData(object, data, data_sz);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_WriteObjectData failed 0x%08x", res);
		TEE_CloseAndDeletePersistentObject1(object);
	} else {
		TEE_CloseObject(object);
	}
	TEE_Free(obj_id);
	TEE_Free(data);
	return res;
}

static TEE_Result read_raw_object(uint32_t param_types, TEE_Param params[4])
{
	const uint32_t exp_param_types =
		TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_MEMREF_OUTPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE);
	TEE_ObjectHandle object;
	TEE_ObjectInfo object_info;
	TEE_Result res;
	uint32_t read_bytes;
	char *obj_id;
	size_t obj_id_sz;
	char *data;
	size_t data_sz;

	/*
	 * Safely get the invocation parameters
	 */
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	obj_id_sz = params[0].memref.size;
	obj_id = TEE_Malloc(obj_id_sz, 0);
	if (!obj_id)
		return TEE_ERROR_OUT_OF_MEMORY;

	TEE_MemMove(obj_id, params[0].memref.buffer, obj_id_sz);

	data_sz = params[1].memref.size;
	data = TEE_Malloc(data_sz, 0);
	if (!data)
		return TEE_ERROR_OUT_OF_MEMORY;

	/*
	 * Check the object exist and can be dumped into output buffer
	 * then dump it.
	 */
	res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
					obj_id, obj_id_sz,
					TEE_DATA_FLAG_ACCESS_READ |
					TEE_DATA_FLAG_SHARE_READ,
					&object);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to open persistent object, res=0x%08x", res);
		TEE_Free(obj_id);
		TEE_Free(data);
		return res;
	}

	res = TEE_GetObjectInfo1(object, &object_info);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to create persistent object, res=0x%08x", res);
		goto exit;
	}

	if (object_info.dataSize > data_sz) {
		/*
		 * Provided buffer is too short.
		 * Return the expected size together with status "short buffer"
		 */
		params[1].memref.size = object_info.dataSize;
		res = TEE_ERROR_SHORT_BUFFER;
		goto exit;
	}

	res = TEE_ReadObjectData(object, data, object_info.dataSize,
				 &read_bytes);
	if (res == TEE_SUCCESS)
		TEE_MemMove(params[1].memref.buffer, data, read_bytes);
	if (res != TEE_SUCCESS || read_bytes != object_info.dataSize) {
		EMSG("TEE_ReadObjectData failed 0x%08x, read %" PRIu32 " over %u",
				res, read_bytes, object_info.dataSize);
		goto exit;
	}

	/* Return the number of byte effectively filled */
	params[1].memref.size = read_bytes;
exit:
	TEE_CloseObject(object);
	TEE_Free(obj_id);
	TEE_Free(data);
	return res;
}

TEE_Result TA_CreateEntryPoint(void)
{
	/* Nothing to do */
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	/* Nothing to do */
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t __unused param_types,
				    TEE_Param __unused params[4],
				    void __unused **session)
{
	/* Nothing to do */
	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __unused *session)
{
	/* Nothing to do */
}

TEE_Result TA_InvokeCommandEntryPoint(void __unused *session,
				      uint32_t command,
				      uint32_t param_types,
				      TEE_Param params[4])
{
	switch (command) {
    case TA_LIBDOGECOIN_CMD_WRITE_RAW:
        return create_raw_object(param_types, params);
    case TA_LIBDOGECOIN_CMD_READ_RAW:
        return read_raw_object(param_types, params);
    case TA_LIBDOGECOIN_CMD_DELETE:
        return delete_object(param_types, params);
    case TA_LIBDOGECOIN_CMD_GENERATE_SEED:
        return generate_and_store_seed(param_types, params);
    case TA_LIBDOGECOIN_CMD_GENERATE_MNEMONIC:
        return generate_and_store_mnemonic(param_types, params);
    case TA_LIBDOGECOIN_CMD_GENERATE_MASTERKEY:
        return generate_and_store_master_key(param_types, params);
    case TA_LIBDOGECOIN_CMD_GENERATE_EXTENDED_PUBLIC_KEY:
        return generate_extended_public_key(param_types, params);
    case TA_LIBDOGECOIN_CMD_GENERATE_ADDRESS:
        return generate_address(param_types, params);
    case TA_LIBDOGECOIN_CMD_SIGN_MESSAGE:
        return sign_message_with_private_key(param_types, params);
    case TA_LIBDOGECOIN_CMD_SIGN_TRANSACTION:
        return sign_transaction_with_private_key(param_types, params);
    case TA_LIBDOGECOIN_CMD_DELEGATE_KEY:
        return delegate_key(param_types, params);
    case TA_LIBDOGECOIN_CMD_EXPORT_DELEGATED_KEY:
        return export_delegate_key(param_types, params);
	default:
		EMSG("Command ID 0x%x is not supported", command);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}
