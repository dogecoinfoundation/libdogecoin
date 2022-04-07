/*

 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
 Copyright (c) 2022 bluezr
 Copyright (c) 2022 The Dogecoin Foundation

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 
*/
#include <assert.h>
#include <getopt.h>
#include <src/libdogecoin-config.h>
#include <stdbool.h>
#include <stdio.h>   /* printf */
#include <stdlib.h>  /* atoi, malloc */
#include <string.h>  /* strcpy */
#include <unistd.h>
#include <contrib/uthash/uthash.h>

#include <dogecoin/bip32.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/crypto/ecc.h>
#include <dogecoin/serialize.h>
#include <dogecoin/tool.h>
#include <dogecoin/transaction.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

static struct option long_options[] =
    {
        {"privkey", required_argument, NULL, 'p'},
        {"pubkey", required_argument, NULL, 'k'},
        {"derived_path", required_argument, NULL, 'm'},
        {"command", required_argument, NULL, 'c'},
        {"testnet", no_argument, NULL, 't'},
        {"regtest", no_argument, NULL, 'r'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}};

static void print_version()
{
    printf("Version: %s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
}

static void print_usage()
{
    print_version();
    printf("Usage: such (-m|-derived_path <bip_derived_path>) (-k|-pubkey <publickey>) (-p|-privkey <privatekey>) (-t[--testnet]) (-r[--regtest]) -c <command>\n");
    printf("Available commands: generate_public_key (requires -p <wif>), p2pkh (requires -k <public key hex>), generate_private_key, bip32_extended_master_key, print_keys (requires -p <private key hex>), derive_child_keys (requires -m <custom path> -p <private key>) \n");
    printf("\nExamples: \n");
    printf("Generate a testnet private ec keypair wif/hex:\n");
    printf("> such -c generate_private_key\n\n");
    printf("> such -c generate_public_key -p QRYZwxVxBFKgKP4bWPEwWBJpN3C3cTN6fads8SgJTgaPTJhEWgLH\n\n");
}

static bool showError(const char* er)
{
    printf("Error: %s\n", er);
    dogecoin_ecc_stop();
    return 1;
}

void signing_menu(int txindex) {
    int id = txindex;
    int running = 1;
    int input_to_sign;
    char* raw_hexadecimal_tx;
    char* script_pubkey;
    int signature_hash_type;
    float input_amount;
    char* private_key_wif;
        while (running) {
            printf("\n 1. sign input (from current working transaction)\n");
            printf(" 2. sign input (raw hexadecimal transaction)\n");
            printf(" 3. save (raw hexadecimal transaction)\n");
            printf(" 8. print signed transaction\n");
            printf(" 9. go back\n\n");
            switch (atoi(getl("command"))) {
                case 1:
                    input_amount = atoi(getl("input amount")); // 2 & 10
                    input_to_sign = atoi(getl("input to sign")); // 0
                    private_key_wif = get_private_key("private_key"); // ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy
                    script_pubkey = dogecoin_private_key_wif_to_script_hash(private_key_wif, 1);
                    // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                    raw_hexadecimal_tx = get_raw_transaction(id);
                    printf("input_to_sign: %d\n", input_to_sign);
                    printf("raw_hexadecimal_transaction: %s\n", raw_hexadecimal_tx);
                    printf("script_pubkey: %s\n", script_pubkey);
                    printf("input_to_sign: %d\n", input_to_sign);
                    printf("private_key: %s\n", private_key_wif);
                    // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                    sign_raw_transaction(input_to_sign, raw_hexadecimal_tx, script_pubkey, 1, input_amount, private_key_wif);
                    if (!save_raw_transaction(id, raw_hexadecimal_tx)) {
                        printf("error saving transaction!\n");
                    }
                    printf("raw tx real: %s\n", get_raw_transaction(id));
                    break;
                case 2:
                    input_amount = atoi(getl("input amount")); // 2 & 10
                    input_to_sign = atoi(getl("input to sign")); // 0
                    private_key_wif = get_private_key("private_key"); // ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy
                    script_pubkey = dogecoin_private_key_wif_to_script_hash(private_key_wif, 1);
                    raw_hexadecimal_tx = get_raw_tx("raw transaction");
                    // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                    printf("input_to_sign: %d\n", input_to_sign);
                    printf("raw_hexadecimal_transaction: %s\n", raw_hexadecimal_tx);
                    printf("script_pubkey: %s\n", script_pubkey);
                    printf("input_to_sign: %d\n", input_to_sign);
                    printf("private_key: %s\n", private_key_wif);
                    sign_raw_transaction(input_to_sign, raw_hexadecimal_tx, script_pubkey, 1, input_amount, private_key_wif);
                    if (!save_raw_transaction(id, raw_hexadecimal_tx)) {
                        printf("error saving transaction!\n");
                    }
                    printf("raw tx: %s\n", get_raw_transaction(id));
                    break;
                case 3:
                    printf("id: %d\n", id);
                    id = save_raw_transaction(id, get_raw_tx("raw transaction"));
                    if (!id) {
                        printf("error saving transaction!\n");
                    }
                    printf("id: %d\n", id);
                    break;
                case 8:
                    printf("raw_tx: %s\n", get_raw_transaction(id));
                    break;
                case 9:
                    running = 0;
                    break;
            }
        }
}

void sub_menu(int txindex) {
    int id = txindex;
    int running = 1;
    int temp;
    int temp_vout_index;
    char* temp_hex_utxo_txid;
    const char* temp_ext_p2pkh;
    uint64_t temp_amt;
    char* output_address;
    float desired_fee;
    float total_amount_for_verification;
    char* public_key;
    int input_to_sign;
    char* raw_hexadecimal_transaction;
        while (running) {
            printf("\n 1. add input\n");
            printf(" 2. add output\n");
            printf(" 3. finalize transaction\n");
            printf(" 4. sign transaction\n");
            printf(" 8. print transaction\n");
            printf(" 9. main menu\n\n");
            switch (atoi(getl("command"))) {
                case 1:
                    printf("raw_tx: %s\n", get_raw_transaction(id));
                    temp_vout_index = atoi(getl("vout index")); // 1
                    temp_hex_utxo_txid = getl("txid"); // b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074 & 42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2
                    add_utxo(id, temp_hex_utxo_txid, temp_vout_index);
                    printf("raw_tx: %s\n", get_raw_transaction(id));
                    break;
                case 2:
                    temp_amt = atof(getl("amount to send to destination address")); // 5
                    temp_ext_p2pkh = getl("destination address"); // nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde
                    printf("destination: %s\n", temp_ext_p2pkh);
                    printf("addout success: %d\n", add_output(id, (char *)temp_ext_p2pkh, temp_amt));
                    char* str = get_raw_transaction(id);
                    printf("raw_tx: %s\n", str);
                    break;
                case 3:
                    output_address = getl("re-enter destination address for verification"); // nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde
                    desired_fee = atof(getl("desired fee")); // .00226
                    total_amount_for_verification = atof(getl("total amount for verification")); // 12
                    public_key = getl("senders address");
                    // noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy
                    raw_hexadecimal_transaction = finalize_transaction(id, output_address, desired_fee, total_amount_for_verification, public_key);
                    printf("raw_tx: %s\n", raw_hexadecimal_transaction);
                    break;
                case 4:
                    signing_menu(id);
                    break;
                case 8:
                    printf("raw_tx: %s\n", get_raw_transaction(id));
                    break;
                case 9:
                    running = 0;
                    break;
            }
        }
}

void main_menu() {
    int running = 1;
    struct working_transaction *s;
    int temp;
    print_header("src/cli/wow.txt");
        while (running) {
            printf("create transaction: \n");
            printf(" 1. add transaction\n");
            printf(" 2. edit transaction by id\n");
            printf(" 3. find transaction\n");
            printf(" 4. delete transaction\n");
            printf(" 5. delete all transactions\n");
            printf(" 6. sort items by id\n");
            printf(" 7. print transactions\n");
            printf(" 8. count transactions\n");
            printf(" 9. quit\n");
            switch (atoi(getl("command"))) {
                case 1:
                    sub_menu(start_transaction());
                    break;
                case 2:
                    temp = atoi(getl("ID"));
                    break;
                case 3:
                    s = find_transaction(atoi(getl("ID to find")));
                    printf("transaction: %s\n", s ? get_raw_transaction(s->idx) : "unknown");
                    break;
                case 4:
                    s = find_transaction(atoi(getl("ID to delete")));
                    if (s) {
                        remove_transaction(s);
                    } else {
                        printf("id unknown\n");
                    }
                    break;
                case 5:
                    remove_all();
                    break;
                case 6:
                    HASH_SORT(transactions, by_id);
                    break;
                case 7:
                    print_transactions();
                    break;
                case 8:
                    count_transactions();
                    break;
                case 9:
                    running = 0;
                    break;
            }
        }
    remove_all();
}

int main(int argc, char* argv[])
{
    int long_index = 0;
    int opt = 0;
    char* pkey = 0;
    char* pubkey = 0;
    char* cmd = 0;
    char* derived_path = 0;
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;

    /* get arguments */
    while ((opt = getopt_long_only(argc, argv, "p:k:m:c:trv", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'p':
            pkey = optarg;
            if (strlen(pkey) < 50)
                return showError("Private key must be WIF encoded");
            break;
        case 'c':
            cmd = optarg;
            break;
        case 'm':
            derived_path = optarg;
            break;
        case 'k':
            pubkey = optarg;
            break;
        case 't':
            chain = &dogecoin_chainparams_test;
            break;
        case 'r':
            chain = &dogecoin_chainparams_regtest;
            break;
        case 'v':
            print_version();
            exit(EXIT_SUCCESS);
            break;
        default:
            print_usage();
            exit(EXIT_FAILURE);
        }
    }

    if (!cmd) {
        /* exit if no command was provided */
        print_usage();
        exit(EXIT_FAILURE);
    }

    /* start ECC context */
    dogecoin_ecc_start();

    const char* pkey_error = "missing extended key (use -p)";

    if (strcmp(cmd, "generate_public_key") == 0) {
        /* output compressed hex pubkey from hex privkey */

        size_t sizeout = 128;
        char pubkey_hex[sizeout];
        if (!pkey)
            return showError(pkey_error);
        if (!pubkey_from_privatekey(chain, pkey, pubkey_hex, &sizeout))
            return showError("attempt to generate pubkey from privatekey failed");

        /* erase previous private key */
        memset(pkey, 0, strlen(pkey));

        /* generate public key hex from private key hex */
        printf("public key hex: %s\n", pubkey_hex);

        /* give out p2pkh address */
        char address_p2pkh[sizeout];
        char address_p2sh_p2wpkh[sizeout];
        char address_p2wpkh[sizeout];
        addresses_from_pubkey(chain, pubkey_hex, address_p2pkh, address_p2sh_p2wpkh, address_p2wpkh);
        printf("p2pkh address: %s\n", address_p2pkh);
        printf("p2sh-p2wpkh address: %s\n", address_p2sh_p2wpkh);

        /* clean memory */
        memset(pubkey_hex, 0, strlen(pubkey_hex));
        memset(address_p2pkh, 0, strlen(address_p2pkh));
        memset(address_p2sh_p2wpkh, 0, strlen(address_p2sh_p2wpkh));
    } else if (strcmp(cmd, "p2pkh") == 0) {
        /* get p2pkh address from pubkey */

        size_t sizeout = 128;
        char address_p2pkh[sizeout];
        char address_p2sh_p2wpkh[sizeout];
        char address_p2wpkh[sizeout];
        if (!pubkey)
            return showError("Missing public key (use -k)");
        if (!addresses_from_pubkey(chain, pubkey, address_p2pkh, address_p2sh_p2wpkh, address_p2wpkh))
            return showError("Operation failed, invalid pubkey");
        printf("p2pkh address: %s\n", address_p2pkh);
        printf("p2sh-p2wpkh address: %s\n", address_p2sh_p2wpkh);
        printf("p2wpkh (doge / bech32) address: %s\n", address_p2wpkh);

        memset(pubkey, 0, strlen(pubkey));
        memset(address_p2pkh, 0, strlen(address_p2pkh));
        memset(address_p2sh_p2wpkh, 0, strlen(address_p2sh_p2wpkh));
    } else if (strcmp(cmd, "generate_private_key") == 0) {
        size_t sizeout = 128;
        char newprivkey_wif[sizeout];
        char newprivkey_hex[sizeout];

        /* generate a new private key */
        gen_privatekey(chain, newprivkey_wif, sizeout, newprivkey_hex);
        printf("private key wif: %s\n", newprivkey_wif);
        printf("private key hex: %s\n", newprivkey_hex);
        memset(newprivkey_wif, 0, strlen(newprivkey_wif));
        memset(newprivkey_hex, 0, strlen(newprivkey_hex));
    } else if (strcmp(cmd, "bip32_extended_master_key") == 0) {
        size_t sizeout = 128;
        char masterkey[sizeout];

        /* generate a new hd master key */
        hd_gen_master(chain, masterkey, sizeout);
        printf("bip32 extended master key: %s\n", masterkey);
        memset(masterkey, 0, strlen(masterkey));
    } else if (strcmp(cmd, "print_keys") == 0) {
        if (!pkey)
            return showError("no extended key (-p)");
        if (!hd_print_node(chain, pkey))
            return showError("invalid extended key\n");
    } else if (strcmp(cmd, "derive_child_keys") == 0) {
        if (!pkey)
            return showError("no extended key (-p)");
        if (!derived_path)
            return showError("no derivation path (-m)");
        size_t sizeout = 128;
        char newextkey[sizeout];
        if (!hd_derive(chain, pkey, derived_path, newextkey, sizeout))
            return showError("deriving child key failed\n");
        else
            hd_print_node(chain, newextkey);
    } else if (strcmp(cmd, "create_transaction") == 0) {
        main_menu();
    }

    dogecoin_ecc_stop();

    return 0;
}
