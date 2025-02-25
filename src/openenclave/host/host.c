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

#include <openenclave/host.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <ykpers-1/ykpers.h>
#include <ykpers-1/ykdef.h>

// Include the untrusted libdogecoin header that is generated
#include "libdogecoin_u.h"

// Include the libdogecoin header
#include "libdogecoin.h"

#define TIME_STEP 30
#define NUM_ADDRESSES 1
#define TOTP_SECRET_HEX_SIZE 41 // hex, 40 characters + null

// Base names for the items
#define BASE_NAME_MNEMONIC "dogecoin_mnemonic_"
#define BASE_NAME_SEED "dogecoin_seed_"
#define BASE_NAME_MASTER "dogecoin_master_"

// Suffices for the items
#define SUFFIX_TEE "_tee"

// Directory path for storage
#define CRYPTO_DIR_PATH "./.store/"

// Full file path and names for Unix-like systems (with TEE encryption method suffix)
#define MNEMONIC_TEE_FILE_NAME CRYPTO_DIR_PATH BASE_NAME_MNEMONIC SUFFIX_TEE
#define SEED_TEE_FILE_NAME CRYPTO_DIR_PATH BASE_NAME_SEED SUFFIX_TEE
#define MASTER_TEE_FILE_NAME CRYPTO_DIR_PATH BASE_NAME_MASTER SUFFIX_TEE

bool check_simulate_opt(int* argc, char* argv[])
{
    for (int i = 0; i < *argc; i++)
    {
        if (strcmp(argv[i], "--simulate") == 0)
        {
            fprintf(stdout, "Running in simulation mode\n");
            memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char*));
            (*argc)--;
            return true;
        }
    }
    return false;
}

// This is the function that the enclave will call back into to print a message.
void host_libdogecoin()
{
    fprintf(stdout, "Enclave called into host to print: Libdogecoin!\n");
}

static struct option long_options[] =
{
    {"command", required_argument, NULL, 'c'},
    {"account_int", required_argument, NULL, 'o'},
    {"input_index", required_argument, NULL, 'i'},
    {"change_level", required_argument, NULL, 'l'},
    {"message", required_argument, NULL, 'm'},
    {"transaction", required_argument, NULL, 't'},
    {"mnemonic_input", required_argument, NULL, 'n'},
    {"shared_secret", required_argument, NULL, 's'},
    {"entropy_size", required_argument, NULL, 'e'},
    {NULL, 0, NULL, 0}
};

static void print_usage()
{
    printf("Usage: host -c <cmd> (-o|-account_int <account_int>) (-i|-input_index <input index>) (-l|-change_level <change level>) \
(-m|-message <message>) (-t|-transaction <transaction>) (-n|-mnemonic_input <mnemonic input>) (-s|-shared_secret <shared secret>) \
(-e|-entropy_size <entropy size>)\n");
    printf("Available commands:\n");
    printf("  generate_mnemonic (optional -n <mnemonic_input> -s <shared_secret> -e <entropy_size>)\n");
    printf("  generate_extended_public_key (requires -o <account_int> -i <input_index> -l <change_level>\n");
    printf("  generate_address (requires -o <account_int> -i <input_index> -l <change_level>)\n");
    printf("  sign_message (requires -o <account_int> -i <input_index> -l <change_level> -m <message>)\n");
    printf("  sign_transaction (requires -o <account_int> -i <input_index> -l <change_level> -t <transaction>)\n");
}

void write_encrypted_file(const char* filename, const uint8_t* data, size_t data_size)
{
    FILE* file = fopen(filename, "wb");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file %s for writing\n", filename);
        return;
    }
    size_t written = fwrite(data, 1, data_size, file);
    if (written != data_size)
    {
        fprintf(stderr, "Failed to write data to file %s\n", filename);
    }
    fclose(file);
}

void read_encrypted_file(const char* filename, uint8_t* data, size_t* data_size)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Failed to open file %s for reading\n", filename);
        *data_size = 0;
        return;
    }
    *data_size = fread(data, 1, *data_size, file);
    if (*data_size == 0)
    {
        fprintf(stderr, "Failed to read data from file %s\n", filename);
    }
    fclose(file);
}

void set_totp_secret(YK_KEY *yk, const char *secret) {
    YKP_CONFIG *cfg = ykp_alloc();
    YK_STATUS *st = ykds_alloc();

    if (!cfg || !st) {
        fprintf(stderr, "Failed to allocate YubiKey structures\n");
        ykp_free_config(cfg);
        ykds_free(st);
        return;
    }

    if (!yk_get_status(yk, st)) {
        fprintf(stderr, "Failed to get YubiKey status: %s\n", yk_strerror(yk_errno));
        ykp_free_config(cfg);
        ykds_free(st);
        return;
    }

    ykp_configure_version(cfg, st);

    // Set flags for HMAC challenge-response
    struct config_st *core_config = (struct config_st *) ykp_core_config(cfg);
    core_config->tktFlags |= TKTFLAG_CHAL_RESP;
    core_config->cfgFlags |= CFGFLAG_CHAL_HMAC;
    core_config->cfgFlags |= CFGFLAG_HMAC_LT64;
    core_config->cfgFlags &= ~CFGFLAG_CHAL_BTN_TRIG; // Disable button press
    core_config->extFlags |= EXTFLAG_SERIAL_API_VISIBLE;

    if (!ykp_configure_command(cfg, SLOT_CONFIG)) {
        fprintf(stderr, "Internal error: couldn't configure command\n");
        ykp_free_config(cfg);
        ykds_free(st);
        return;
    }

    printf("Configuring shared secret...\n");

    // Ensure the secret size is correct
    if (sizeof(secret) > TOTP_SECRET_HEX_SIZE) {
        fprintf(stderr, "Secret too long\n");
        ykp_free_config(cfg);
        ykds_free(st);
        return;
    }

    if (ykp_HMAC_key_from_hex(cfg, secret) != 0) {
        fprintf(stderr, "Internal error: couldn't configure key\n");
        ykp_free_config(cfg);
        ykds_free(st);
        return;
    }

    // Write to YubiKey
    printf("Writing configuration to YubiKey...\n");
    if (!yk_write_command(yk, ykp_core_config(cfg), ykp_command(cfg), NULL)) {
        fprintf(stderr, "Failed to write command: %s\n", yk_strerror(yk_errno));
        ykp_free_config(cfg);
        ykds_free(st);
        return;
    }

    printf("Shared secret set successfully\n");

    ykp_free_config(cfg);
    ykds_free(st);
}

uint32_t get_totp_from_yubikey(YK_KEY *yk) {
    unsigned int t_counter = (unsigned int)time(NULL) / 30;
    unsigned char challenge[8];
    for (int i = 7; i >= 0; i--) {
        challenge[i] = t_counter & 0xFF;
        t_counter >>= 8;
    }

    if (!yk) {
        fprintf(stderr, "Failed to get yubikey\n");
        return 0;
    }

    unsigned char response[SHA1_MAX_BLOCK_SIZE];
    if (!yk_challenge_response(yk, SLOT_CHAL_HMAC1, true, sizeof(challenge), challenge, sizeof(response), response)) {
        fprintf(stderr, "Failed to generate TOTP code\n");
       return 0;
    }

    unsigned int offset = response[19] & 0xf;
    unsigned int bin_code = (response[offset] & 0x7f) << 24 |
                            (response[offset + 1] & 0xff) << 16 |
                            (response[offset + 2] & 0xff) << 8 |
                            (response[offset + 3] & 0xff);

    return bin_code % 1000000;
}

int main(int argc, char* argv[])
{
    oe_result_t result;
    int ret = 1;
    oe_enclave_t* enclave = NULL;

    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;

    // Parse the enclave image path and --simulate flag first
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s enclave_image_path [ --simulate ] -c <cmd> [options]\n", argv[0]);
        return ret;
    }

    const char* enclave_path = argv[1];
    int remaining_argc = argc - 1;
    char** remaining_argv = &argv[1];

    if (check_simulate_opt(&remaining_argc, remaining_argv))
    {
        flags |= OE_ENCLAVE_FLAG_SIMULATE;
    }

    // Create the enclave
    result = oe_create_libdogecoin_enclave(enclave_path, OE_ENCLAVE_TYPE_AUTO, flags, NULL, 0, &enclave);
    if (result != OE_OK)
    {
        fprintf(stderr, "oe_create_libdogecoin_enclave(): result=%u (%s)\n", result, oe_result_str(result));
        return ret;
    }

    // Variables for CLI options
    int opt = 0;
    int long_index = 0;
    char* cmd = 0;
    uint32_t* account = NULL;
    uint32_t* input_index = NULL;
    const char* change_level = NULL;
    char* message = "This is a test message";
    char* transaction = NULL;
    char* shared_secret = NULL;
    char* mnemonic_in = NULL;
    char* entropy_size = NULL;

    // Allocate memory for encrypted blobs
    uint8_t* encrypted_blob = malloc(MAX_ENCRYPTED_BLOB_SIZE);
    size_t blob_size = MAX_ENCRYPTED_BLOB_SIZE;

    // Parse remaining CLI options
    while ((opt = getopt_long_only(remaining_argc, remaining_argv, "c:o:i:l:m:t:n:s:e:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'c':
                cmd = optarg;
                break;
            case 'o':
                account = (uint32_t*)malloc(sizeof(uint32_t));
                *account = (uint32_t)strtol(optarg, NULL, 10);
                break;
            case 'i':
                input_index = (uint32_t*)malloc(sizeof(uint32_t));
                *input_index = (uint32_t)strtol(optarg, NULL, 10);
                break;
            case 'l':
                change_level = optarg;
                break;
            case 'm':
                message = optarg;
                break;
            case 't':
                transaction = optarg;
                break;
            case 'n':
                mnemonic_in = optarg;
                break;
            case 's':
                shared_secret = optarg;
                break;
            case 'e':
                entropy_size = optarg;
                break;
            default:
                print_usage();
                exit(EXIT_SUCCESS);
        }
    }

    if (!cmd)
    {
        print_usage();
        exit(EXIT_SUCCESS);
    }

    if (!yk_init()) {
        fprintf(stderr, "Failed to initialize YubiKey\n");
    }

    YK_KEY *yk = yk_open_first_key();
    if (!yk) {
        fprintf(stderr, "Failed to open YubiKey\n");
    }

    if (strcmp(cmd, "run_example") == 0)
    {
        printf("- Run the example\n");
        result = enclave_libdogecoin_run_example(enclave);
        if (result != OE_OK)
        {
            fprintf(stderr, "Failed to run the example: result=%u (%s)\n", result, oe_result_str(result));
        }
    }
    else if (strcmp(cmd, "generate_master_key") == 0)
    {
        printf("- Generate a master key\n");
        result = enclave_libdogecoin_generate_master_key(enclave, &encrypted_blob, &blob_size);
        if (result != OE_OK)
        {
            fprintf(stderr, "Failed to generate a master key: result=%u (%s)\n", result, oe_result_str(result));
        }
        else
        {
            write_encrypted_file(MASTER_TEE_FILE_NAME, encrypted_blob, blob_size);
        }
    }
    else if (strcmp(cmd, "generate_mnemonic") == 0)
    {
        printf("- Generate and encrypt a mnemonic\n");
        if (!shared_secret) {
            shared_secret = getpass("Enter shared secret (hex, 40 characters): ");
            if (!shared_secret) {
                fprintf(stderr, "Failed to read shared secret\n");
                goto exit;
            }
        }

        // Check if there is an existing configuration in slot 1
        YK_STATUS *status = ykds_alloc();
        if (yk && !yk_get_status(yk, status)) {
            fprintf(stderr, "Failed to get YubiKey status\n");
        }

        // Check if slot 1 has a configuration
        if (yk && (ykds_touch_level(status) & CONFIG1_VALID) == CONFIG1_VALID) {
            char response;
            printf("Slot 1 already has a configuration. Do you want to overwrite it? (y/N): ");
            scanf(" %c", &response);
            if (response != 'y' && response != 'Y') {
                printf("Aborted by user\n");
                ykds_free(status);
                goto exit;
            }
        }

        ykds_free(status);

        if (yk) {
            set_totp_secret(yk, shared_secret);
        }

        MNEMONIC mnemonic = {0};
        result = enclave_libdogecoin_generate_mnemonic(enclave, &encrypted_blob, &blob_size, mnemonic, shared_secret, mnemonic_in, entropy_size);
        if (result != OE_OK)
        {
            fprintf(stderr, "Failed to generate and encrypt a mnemonic: result=%u (%s)\n", result, oe_result_str(result));
        }
        else
        {
            printf("Generated Mnemonic: %s\n", mnemonic);
            if (mkdir(CRYPTO_DIR_PATH, 0777) == -1 && errno != EEXIST)
            {
                fprintf(stderr, "ERROR: Failed to create directory\n");
                return false;
            }
            write_encrypted_file(MNEMONIC_TEE_FILE_NAME, encrypted_blob, blob_size);
        }
        dogecoin_mem_zero(mnemonic, strlen(mnemonic));
    }
    else if (strcmp(cmd, "generate_extended_public_key") == 0)
    {
        printf("- Generate a public key\n");
        uint8_t pubkeyhex[128];
        uint32_t auth_token = get_totp_from_yubikey(yk);
        printf("Auth token: %u\n", auth_token);
        read_encrypted_file(MNEMONIC_TEE_FILE_NAME, encrypted_blob, &blob_size);
        if (blob_size == 0)
        {
            fprintf(stderr, "Failed to read encrypted mnemonic from file\n");
            ret = 0;
            goto exit;
        }
        // verify account and change_level are set
        if (!account || !change_level)
        {
            fprintf(stderr, "Account and change level must be set\n");
            ret = 0;
            goto exit;
        }
        result = enclave_libdogecoin_generate_extended_public_key(enclave, encrypted_blob, blob_size, (char*)pubkeyhex, account, change_level, auth_token);
        if (strlen(pubkeyhex) == 0)
        {
            fprintf(stderr, "Failed to generate public key: result=%u (%s)\n", result, oe_result_str(result));
        }
        else
        {
            printf("Generated Public Key: %s\n", pubkeyhex);
        }
    }
    else if (strcmp(cmd, "generate_address") == 0)
    {
        printf("- Generate address\n");
        char addresses[P2PKHLEN * NUM_ADDRESSES];
        uint32_t auth_token = get_totp_from_yubikey(yk);
        printf("Auth token: %u\n", auth_token);
        read_encrypted_file(MNEMONIC_TEE_FILE_NAME, encrypted_blob, &blob_size);
        if (blob_size == 0)
        {
            fprintf(stderr, "Failed to read encrypted mnemonic from file\n");
            ret = 0;
            goto exit;
        }
        // verify account, input_index and change_level are set
        if (!account || !input_index || !change_level)
        {
            fprintf(stderr, "Account, input index and change level must be set\n");
            ret = 0;
            goto exit;
        }
        result = enclave_libdogecoin_generate_address(enclave, encrypted_blob, blob_size, addresses, *account, *input_index, change_level, NUM_ADDRESSES, auth_token);
        if (result != OE_OK)
        {
            fprintf(stderr, "Failed to generate addresses: result=%u (%s)\n", result, oe_result_str(result));
        }
        else
        {
            printf("Generated Address: %s\n", addresses);
        }
    }
    else if (strcmp(cmd, "sign_message") == 0)
    {
        printf("- Sign a message\n");
        uint8_t signature[2048] = {0};
        uint32_t auth_token = get_totp_from_yubikey(yk);
        printf("Auth token: %u\n", auth_token);
        read_encrypted_file(MNEMONIC_TEE_FILE_NAME, encrypted_blob, &blob_size);
        if (blob_size == 0)
        {
            fprintf(stderr, "Failed to read encrypted mnemonic from file\n");
            ret = 0;
            goto exit;
        }
        // verify account, input_index and change_level are set
        if (!account || !input_index || !change_level)
        {
            fprintf(stderr, "Account, input index and change level must be set\n");
            ret = 0;
            goto exit;
        }
        printf ("Signing message: %s\n", message);
        result = enclave_libdogecoin_sign_message(enclave, encrypted_blob, blob_size, message, (char*)signature, *account, *input_index, change_level, auth_token);
        if (result != OE_OK)
        {
            fprintf(stderr, "Failed to sign the message: result=%u (%s)\n", result, oe_result_str(result));
        }
        else
        {
            printf("Signature: %s\n", signature);
        }
    }
    else if (strcmp(cmd, "sign_transaction") == 0)
    {
        printf("- Sign a transaction\n");
        char raw_tx[1024];
        char signed_tx[4096];
        uint32_t auth_token = get_totp_from_yubikey(yk);
        printf("Auth token: %u\n", auth_token);
        read_encrypted_file(MNEMONIC_TEE_FILE_NAME, encrypted_blob, &blob_size);
        if (blob_size == 0)
        {
            fprintf(stderr, "Failed to read encrypted mnemonic from file\n");
            ret = 0;
            goto exit;
        }

        // verify account, input_index and change_level are set
        if (!account || !input_index || !change_level)
        {
            fprintf(stderr, "Account, input index and change level must be set\n");
            ret = 0;
            goto exit;
        }

        // Example transaction creation process
        char *external_p2pkh_addr = "nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde";
        char *hash_2_doge = "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074";
        char *hash_10_doge = "42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2";

        int idx = start_transaction();
        printf("Empty transaction created at index %d.\n", idx);

        if (add_utxo(idx, hash_2_doge, 1))
        {
            printf("Input of value 2 dogecoin added to the transaction.\n");
        }
        else
        {
            printf("Error occurred while adding input of value 2 dogecoin.\n");
            ret = 0;
            goto exit;
        }

        if (add_utxo(idx, hash_10_doge, 1))
        {
            printf("Input of value 10 dogecoin added to the transaction.\n");
        }
        else
        {
            printf("Error occurred while adding input of value 10 dogecoin.\n");
            ret = 0;
            goto exit;
        }

        if (add_output(idx, external_p2pkh_addr, "5.0"))
        {
            printf("Output of value 5 dogecoin added to the transaction.\n");
        }
        else
        {
            printf("Error occurred while adding output of value 5 dogecoin.\n");
            ret = 0;
            goto exit;
        }

        int idx2 = store_raw_transaction(finalize_transaction(idx, external_p2pkh_addr, "0.00226", "12", "D5AkTLEwB4eCNcFoZN9pj1TxgkhQiVzt3T"));
        if (idx2 > 0)
        {
            printf("Change returned to address %s and finalized unsigned transaction saved at index %d.\n", "D5AkTLEwB4eCNcFoZN9pj1TxgkhQiVzt3T", idx2);
        }
        else
        {
            printf("Error occurred while storing finalized unsigned transaction.\n");
            ret = 0;
            goto exit;
        }

        const char* raw_tx_hex = get_raw_transaction(idx);
        strncpy(raw_tx, raw_tx_hex, sizeof(raw_tx) - 1);
        raw_tx[sizeof(raw_tx) - 1] = '\0';
        printf("Raw transaction created: %s\n", raw_tx);
        printf("Raw transaction length: %zu\n", strlen(raw_tx));

        result = enclave_libdogecoin_sign_transaction(enclave, encrypted_blob, blob_size, transaction != NULL ? transaction : raw_tx, signed_tx, *account, *input_index, change_level, auth_token);
        if (result != OE_OK)
        {
            fprintf(stderr, "Failed to sign the transaction: result=%u (%s)\n", result, oe_result_str(result));
        }
        else
        {
            printf("Signed Transaction: %s\n", signed_tx);
        }
    }
    else
    {
        print_usage();
        ret = 0;
    }
exit:
    if (enclave)
    {
        oe_terminate_enclave(enclave);
    }
    if (mnemonic_in) {
        dogecoin_mem_zero(mnemonic_in, strlen(mnemonic_in));
    }
    if (shared_secret) {
        dogecoin_mem_zero(shared_secret, strlen(shared_secret));
    }
    if (account)
    {
        free(account);
    }
    if (input_index)
    {
        free(input_index);
    }
    if (yk)
    {
        yk_close_key(yk);
    }
    yk_release();

    return ret;
}
