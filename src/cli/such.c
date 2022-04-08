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

void signing_menu(int txindex, int is_testnet) {
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

void sub_menu(int txindex, int is_testnet) {
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
                    signing_menu(id, is_testnet);
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

void transaction_input_menu(int txindex, int is_testnet) {
    int id = txindex;
    int running = 1;
    working_transaction* tx = find_transaction(txindex);
        while (running) {
            int length = tx->transaction->vin->len;
            int selected = -1;
            char* hex_utxo_txid;
            int vout;
            char* raw_hexadecimal_tx;
            char* script_pubkey;
            int signature_hash_type;
            float input_amount;
            int input_to_sign;
            char* private_key_wif;
            for (int i = 0; i <= length; i++) {
                printf("\n--------------------------------\n");
                printf("input index:      %d\n", i);
                dogecoin_tx_in* tx_in = vector_idx(tx->transaction->vin, i);
                vout = tx_in->prevout.n;
                printf("prevout.n:        %d\n", vout);
                hex_utxo_txid = utils_uint8_to_hex(tx_in->prevout.hash, sizeof tx_in->prevout.hash);
                printf("txid:             %s\n", hex_utxo_txid);
                printf("script signature: %s\n", utils_uint8_to_hex((const uint8_t *)tx_in->script_sig->str, tx_in->script_sig->len));
                printf("tx_in->sequence:  %x\n", tx_in->sequence);
                selected == i ? printf("selected:         [X]\n") : "";

                if (selected == i) {
                    printf("\n\n");
                    printf("1. select field to edit\n");
                    printf("2. finish editing\n");
                    switch (atoi(getl("\ncommand"))) {
                        case 1:
                            // tx_input submenu
                            printf("1. prevout.n\n");
                            printf("2. txid\n");
                            printf("3. script signature\n");
                            switch (atoi(getl("field to edit"))) {
                                case 1:
                                    printf("prevout.n\n");
                                    vout = atoi(getl("new input index"));
                                    tx_in->prevout.n = vout;
                                    vector_add(tx->transaction->vin, tx_in);
                                    break;
                                case 2:
                                    hex_utxo_txid = get_raw_tx("new txid");
                                    utils_uint256_sethex((char *)hex_utxo_txid, (uint8_t *)tx_in->prevout.hash);
                                    tx_in->prevout.n = vout;
                                    break;
                                case 3:
                                    printf("\nediting script signature:\n\n");
                                    input_amount = atoi(getl("input amount")); // 2 & 10
                                    input_to_sign = i;
                                    private_key_wif = (char*)get_private_key("private_key"); // ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy
                                    script_pubkey = dogecoin_private_key_wif_to_script_hash(private_key_wif, 1);
                                    cstr_erase(tx_in->script_sig, 0, tx_in->script_sig->len);
                                    // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                                    raw_hexadecimal_tx = get_raw_transaction(txindex);
                                    printf("raw_hexadecimal_transaction: %s\n", raw_hexadecimal_tx);
                                    // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                                    if (!sign_indexed_raw_transaction(txindex, input_to_sign, raw_hexadecimal_tx, script_pubkey, 1, input_amount, private_key_wif)) {
                                        printf("error saving transaction!\n");
                                    }
                                    dogecoin_free(script_pubkey);
                                    break;
                            }
                            i = i - i - 1; // reset loop to start
                            break;
                        case 2:
                            selected = -1; // set selected to number out of bounds for i
                            i = i - i - 1; // reset loop to start
                            break;
                    }
                }
                // if on last iteration, jump into switch case pausing loop
                // execution so user has ability to reset loop index in order
                // to target desired input to edit. otherwise set loop index to 
                // length thus finishing final iteration and set running to 0 to
                // escape encompassing while loop so we return to previous menu
                if (i == length - 1) {
                    printf("\n\n");
                    printf("1. select input to edit\n");
                    printf("2. main menu\n");
                    switch (atoi(getl("\ncommand"))) {
                        case 1:
                            // tx_input submenu
                            selected = atoi(getl("vin index"));
                            i = i - i - 1;
                            break;
                        case 2:
                            i = length;
                            running = 0;
                            break;
                    }
                }
            }
        }
    // TODO: clean up garbage
}

void transaction_output_menu(int txindex, int is_testnet) {
    int id = txindex;
    int running = 1;
    char* script_pubkey;
    char* destinationaddress;
    double amount;
    uint64_t tx_out_total = 0;
    dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;
    working_transaction* tx = find_transaction(txindex);
        while (running) {
            int length = tx->transaction->vout->len;
            int selected = -1;
            for (int i = 0; i <= length; i++) {
                dogecoin_tx_out* tx_out = vector_idx(tx->transaction->vout, i);
                tx_out_total += tx_out->value;
                
                printf("\n--------------------------------\n");
                printf("output index:       %d\n", i);
                printf("script public key:  %s\n", utils_uint8_to_hex((const uint8_t*)tx_out->script_pubkey->str, tx_out->script_pubkey->len));
                amount = koinu_to_coins(tx_out->value);
                printf("amount:             %f\n", amount);
                // selected should only equal anything other than -1 upon setting
                // loop index in conditional targetting last iteration:
                selected == i ? printf("selected:           [X]\n") : "";
                if (selected == i) {
                    printf("\n\n");
                    printf("1. select field to edit\n");
                    printf("2. finish editing\n");
                    switch (atoi(getl("\ncommand"))) {
                        case 1:
                            // tx_input submenu
                            printf("1. script public key\n");
                            printf("2. amount\n");
                            switch (atoi(getl("field to edit"))) {
                                case 1:
                                    destinationaddress = getl("new destination address");
                                    script_pubkey = dogecoin_p2pkh_to_script_hash(destinationaddress);
                                    printf("script pubkey: %s\n", script_pubkey);
                                    printf("script public key:  %s\n", utils_uint8_to_hex((const uint8_t*)tx_out->script_pubkey->str, tx_out->script_pubkey->len));
                                    amount = coins_to_koinu(amount);
                                    vector_remove_idx(tx->transaction->vout, i);
                                    dogecoin_tx_add_address_out(tx->transaction, chain, (int64_t)amount, destinationaddress);
                                    tx_out = vector_idx(tx->transaction->vout, i);
                                    break;
                                case 2:
                                    amount = atof(getl("new amount"));
                                    printf("amount: %f\n", amount);
                                    amount = coins_to_koinu(amount);
                                    tx_out->value = amount;
                                    printf("script public key:  %s\n", utils_uint8_to_hex((const uint8_t*)tx_out->script_pubkey->str, tx_out->script_pubkey->len));
                                    break;
                            }
                            tx_out_total = 0;
                            i = i - i - 1; // reset loop to start
                            break;
                        case 2:
                            selected = -1; // set selected to number out of bounds for i
                            i = i - i - 1; // reset loop to start
                            break;
                    }
                }
                // if on last iteration, jump into switch case pausing loop
                // execution so user has ability to reset loop index in order
                // to target desired input to edit. otherwise set loop index to 
                // length thus finishing final iteration and set running to 0 to
                // escape encompassing while loop so we return to previous menu
                if (i == length - 1) {
                    printf("\n\n");
                    // printf("subtotal - desired fee: %f\n", koinu_to_coins(tx_out_total)); // TODO: reset to 0, is currently appending to previous calculated total
                    printf("\n");
                    printf("1. select output to edit\n");
                    printf("2. main menu\n");
                    switch (atoi(getl("\ncommand"))) {
                        case 1:
                            // tx_input submenu
                            selected = atoi(getl("vout index"));
                            i = i - i - 1;
                            break;
                        case 2:
                            i = length;
                            running = 0;
                            break;
                    }
                }
            }
        }
    // TODO: clean up garbage
}

void edit_menu(int txindex, int is_testnet) {
    int id = txindex;
    int running = 1;
        while (running) {
            printf("\n");
            printf("1. edit input\n");
            printf("2. edit output\n");
            printf("3. main menu\n");
            switch (atoi(getl("\ncommand"))) {
                case 1:
                    transaction_input_menu(txindex, is_testnet);
                    break;
                case 2:
                    transaction_output_menu(txindex, is_testnet);
                    break;
                case 3:
                    running = 0;
                    break;
            }
        }
}

int chainparams_menu(int is_testnet) {
    printf("\n1. mainnet\n");
    printf("2. testnet\n\n");
    switch (atoi(getl("command"))) {
        case 1:
            is_testnet = false;
            break;
        case 2:
            is_testnet = true;
            break;
    }
    return is_testnet;
}

int is_testnet = true;

void main_menu() {
    int running = 1;
    struct working_transaction *s;
    int temp;
    print_header("src/cli/wow.txt");

    // load existing testnet transaction into memory for demonstration purposes.
    save_raw_transaction(start_transaction(), "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4010000006b48304502210090bddac300243d16dca5e38ab6c80d5848e0d710d77702223bacd6682654f6fe02201b5c2e8b1143d8a807d604dc18068b4278facce561c302b0c66a4f2a5a4aa66f0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142010000006a47304402200e19c2a66846109aaae4d29376040fc4f7af1a519156fe8da543dc6f03bb50a102203a27495aba9eead2f154e44c25b52ccbbedef084f0caf1deedaca87efd77e4e70121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000");
        while (running) {
            printf("\nsuch transaction: \n\n");
            printf(" 1. add transaction\n");
            printf(" 2. edit transaction by id\n");
            printf(" 3. find transaction\n");
            printf(" 4. delete transaction\n");
            printf(" 5. delete all transactions\n");
            printf(" 6. sort items by id\n");
            printf(" 7. print transactions\n");
            printf(" 8. count transactions\n");
            printf(" 9. change network (current: %s)\n", is_testnet ? "testnet" : "mainnet");
            printf(" 10. quit\n");
            switch (atoi(getl("\ncommand"))) {
                case 1:
                    sub_menu(start_transaction(), is_testnet);
                    break;
                case 2:
                    temp = atoi(getl("ID of transaction to edit"));
                    s = find_transaction(temp);
                    if (s) {
                        edit_menu(temp, is_testnet);
                    } else {
                        printf("\nno transaction found with that id. please try again!\n");
                    }
                    break;
                case 3:
                    s = find_transaction(atoi(getl("ID to find")));
                    printf(s ? "transaction: %s\n", get_raw_transaction(s->idx) : "\nno transaction found with that id. please try again!\n");
                    break;
                case 4:
                    s = find_transaction(atoi(getl("ID to delete")));
                    if (s) {
                        remove_transaction(s);
                    } else {
                        printf("\nno transaction found with that id. please try again!\n");
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
                    is_testnet = chainparams_menu(is_testnet);
                    break;
                case 10:
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
