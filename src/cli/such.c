/*

 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
 Copyright (c) 2023 bluezr
 Copyright (c) 2023 edtubbs
 Copyright (c) 2023 The Dogecoin Foundation

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
#ifndef _MSC_VER
#include <getopt.h>
#include <unistd.h>
#else
#include <win/wingetopt.h>
#include <win/winunistd.h>
#endif

#ifdef HAVE_CONFIG_H
#  include "libdogecoin-config.h"
#endif
#include <stdbool.h>
#include <stdio.h>   /* printf */
#include <stdlib.h>  /* atoi, malloc */
#include <string.h>  /* strcpy */

#include <dogecoin/uthash.h>

#include <dogecoin/address.h>
#include <dogecoin/bip32.h>
#include <dogecoin/bip39.h>
#include <dogecoin/bip44.h>
#include <dogecoin/cstr.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/ecc.h>
#include <dogecoin/eckey.h>
#include <dogecoin/koinu.h>
#include <dogecoin/seal.h>
#include <dogecoin/serialize.h>
#include <dogecoin/sign.h>
#include <dogecoin/tool.h>
#include <dogecoin/transaction.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>
#include <dogecoin/wow.h>

// ******************************** SUCH -C TRANSACTION MENU ********************************
#ifdef WITH_NET
#include <dogecoin/net.h>
void broadcasting_menu(int txindex, int is_testnet) {
    int running = 1;
    int selected = -1;
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;
    working_transaction* tx = find_transaction(txindex);
    char* raw_hexadecimal_tx = get_raw_transaction(txindex);
    int length = HASH_COUNT(transactions);
    while (running) {
        printf("length: %d\n", length);
        for (int i = 0; i <= length; i++) {
            printf("\n--------------------------------\n");
            printf("transaction to broadcast: %s\n", raw_hexadecimal_tx);
            selected == i ? printf("confirm:         [X]\n") : 0;

            if (selected == i) {
                printf("\n\n");
                printf("please confirm this is the transaction you want to send:\n");
                printf("1. yes\n");
                printf("2. no\n");
                switch (atoi(getl("\ncommand"))) {
                        case 1:
                            /* The above code is checking if the data is NULL, empty or larger than the maximum
                            size of a p2p message. */
                            if (raw_hexadecimal_tx == NULL || strlen(raw_hexadecimal_tx) == 0 || strlen(raw_hexadecimal_tx) > DOGECOIN_MAX_P2P_MSG_SIZE) {
                                printf("Transaction in invalid or to large.\n");
                                }
                            uint8_t* data_bin = dogecoin_malloc(strlen(raw_hexadecimal_tx) / 2 + 1);
                            size_t outlen = 0;
                            utils_hex_to_bin(raw_hexadecimal_tx, data_bin, strlen(raw_hexadecimal_tx), &outlen);

                            /* Deserializing the transaction and broadcasting it to the network. */
                            if (dogecoin_tx_deserialize(data_bin, outlen, tx->transaction, NULL)) {
                                broadcast_tx(chain, tx->transaction, 0, 10, 15, 0);
                                }
                            else {
                                printf("Transaction is invalid\n");
                                }
                            dogecoin_free(data_bin);
                            selected = -1; // set selected to number out of bounds for i
                            i = length; // reset loop to start
                            break;
                        case 2:
                            selected = -1; // set selected to number out of bounds for i
                            i = length; // reset loop to start
                            break;
                    }
                }
            // if on last iteration, jump into switch case pausing loop
            // execution so user has ability to reset loop index in order
            // to target desired input to edit. otherwise set loop index to
            // length thus finishing final iteration and set running to 0 to
            // escape encompassing while loop so we return to previous menu
            if (i == length) {
                printf("\n\n");
                printf("1. broadcast transaction\n");
                printf("2. main menu\n");
                switch (atoi(getl("\ncommand"))) {
                        case 1:
                            // tx_input submenu

                            selected = i;
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
    }
#endif

// keeping is_testnet for integration with validation functions
// can remove #pragma once that's completed
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void signing_menu(int txindex, int is_testnet) {
#pragma GCC diagnostic pop
    int running = 1;
    int input_to_sign;
    char* raw_hexadecimal_tx;
    char* script_pubkey;
    char* private_key_wif;
    while (running) {
        printf("\n 1. sign input (from current working transaction)\n");
        printf(" 2. sign input (raw hexadecimal transaction)\n");
        printf(" 3. print signed transaction\n");
        printf(" 4. go back\n\n");
        switch (atoi(getl("command"))) {
                case 1:
                    input_to_sign = atoi(getl("input to sign")); // 0
                    private_key_wif = (char*)get_private_key("private_key"); // ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy
                    script_pubkey = dogecoin_private_key_wif_to_pubkey_hash(private_key_wif);
                    // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                    raw_hexadecimal_tx = get_raw_transaction(txindex);
                    // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                    if (!sign_indexed_raw_transaction(txindex, input_to_sign, raw_hexadecimal_tx, script_pubkey, 1, private_key_wif)) {
                        printf("signing indexed raw transaction failed!\n");
                        }
                    else printf("transaction input successfully signed!\n");
                    break;
                case 2:
                    input_to_sign = atoi(getl("input to sign")); // 0
                    private_key_wif = (char*)get_private_key("private_key"); // ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy
                    script_pubkey = dogecoin_private_key_wif_to_pubkey_hash(private_key_wif);
                    raw_hexadecimal_tx = (char*)get_raw_tx("raw transaction");
                    // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                    debug_print("input_to_sign: %d\n", input_to_sign);
                    debug_print("raw_hexadecimal_transaction: %s\n", raw_hexadecimal_tx);
                    debug_print("script_pubkey: %s\n", script_pubkey);
                    debug_print("input_to_sign: %d\n", input_to_sign);
                    debug_print("private_key: %s\n", private_key_wif);
                    if (!sign_indexed_raw_transaction(txindex, input_to_sign, raw_hexadecimal_tx, script_pubkey, 1, private_key_wif)) {
                        printf("signing indexed raw transaction failed!\n");
                        }
                    else printf("transaction input successfully signed!\n");
                    break;
                case 3:
                    printf("raw_tx: %s\n", get_raw_transaction(txindex));
                    break;
                case 4:
                    running = 0;
                    break;
            }
        }
    }

void sub_menu(int txindex, int is_testnet) {
    int running = 1;
    int temp_vout_index;
    char* temp_hex_utxo_txid;
    const char* temp_ext_p2pkh;
    char* temp_amt;
    char* output_address;
    char* desired_fee;
    char* total_amount_for_verification;
    char* public_key;
    char* raw_hexadecimal_transaction;
    while (running) {
        printf("\n 1. add input\n");
        printf(" 2. add output\n");
        printf(" 3. finalize transaction\n");
        printf(" 4. sign transaction\n");
#ifdef WITH_NET
        printf(" 5. broadcast transaction\n");
#endif
        printf(" 8. print transaction\n");
        printf(" 9. main menu\n\n");
        switch (atoi(getl("command"))) {
                case 1:
                    printf("raw_tx: %s\n", get_raw_transaction(txindex));
                    temp_vout_index = atoi(getl("vout index")); // 1
                    temp_hex_utxo_txid = (char*)getl("txid"); // b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074 & 42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2
                    add_utxo(txindex, temp_hex_utxo_txid, temp_vout_index);
                    printf("raw_tx: %s\n", get_raw_transaction(txindex));
                    break;
                case 2:
                    temp_amt = (char*)getl("amount to send to destination address"); // 5
                    temp_ext_p2pkh = getl("destination address"); // nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde
                    printf("destination: %s\n", temp_ext_p2pkh);
                    printf("addout success: %d\n", add_output(txindex, (char*)temp_ext_p2pkh, temp_amt));
                    char* str = get_raw_transaction(txindex);
                    printf("raw_tx: %s\n", str);
                    break;
                case 3:
                    output_address = (char*)getl("re-enter destination address for verification"); // nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde
                    desired_fee = (char*)getl("desired fee"); // .00226
                    total_amount_for_verification = (char*)getl("total amount for verification"); // 12
                    public_key = (char*)getl("senders address");
                    // noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy
                    raw_hexadecimal_transaction = finalize_transaction(txindex, output_address, desired_fee, total_amount_for_verification, public_key);
                    printf("raw_tx: %s\n", raw_hexadecimal_transaction);
                    break;
                case 4:
                    signing_menu(txindex, is_testnet);
                    break;
#ifdef WITH_NET
                case 5:
                    broadcasting_menu(txindex, is_testnet);
                    break;
#endif
                case 8:
                    printf("raw_tx: %s\n", get_raw_transaction(txindex));
                    break;
                case 9:
                    running = 0;
                    break;
            }
        }
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void transaction_input_menu(int txindex, int is_testnet) {
#pragma GCC diagnostic pop
    int running_transaction_input_menu = 1;
    working_transaction* tx = find_transaction(txindex);
    while (running_transaction_input_menu) {
        int length = tx->transaction->vin->len;
        int selected = -1;
        char* hex_utxo_txid;
        int vout;
        char* raw_hexadecimal_tx;
        char* script_pubkey;
        int input_to_sign;
        char* private_key_wif;
        for (int i = 0; i < length; i++) {
            printf("\n--------------------------------\n");
            printf("input index:      %d\n", i);
            dogecoin_tx_in* tx_in = vector_idx(tx->transaction->vin, i);
            vout = tx_in->prevout.n;
            printf("prevout.n:        %d\n", vout);
            hex_utxo_txid = utils_uint8_to_hex(tx_in->prevout.hash, sizeof tx_in->prevout.hash);
            printf("txid:             %s\n", hex_utxo_txid);
            printf("script signature: %s\n", utils_uint8_to_hex((const uint8_t*)tx_in->script_sig->str, tx_in->script_sig->len));
            printf("tx_in->sequence:  %x\n", tx_in->sequence);
            selected == i ? printf("selected:         [X]\n") : 0;

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
                                        break;
                                    case 2:
                                        hex_utxo_txid = (char*)get_raw_tx("new txid");
                                        utils_uint256_sethex((char*)hex_utxo_txid, (uint8_t*)tx_in->prevout.hash);
                                        tx_in->prevout.n = vout;
                                        break;
                                    case 3:
                                        printf("\nediting script signature:\n\n");
                                        input_to_sign = i;
                                        private_key_wif = (char*)get_private_key("private_key"); // ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy
                                        script_pubkey = dogecoin_private_key_wif_to_pubkey_hash(private_key_wif);
                                        cstr_erase(tx_in->script_sig, 0, tx_in->script_sig->len);
                                        // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                                        raw_hexadecimal_tx = get_raw_transaction(txindex);
                                        printf("raw_hexadecimal_transaction: %s\n", raw_hexadecimal_tx);
                                        // 76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac
                                        if (!sign_indexed_raw_transaction(txindex, input_to_sign, raw_hexadecimal_tx, script_pubkey, 1, private_key_wif)) {
                                            printf("signing indexed raw transaction failed!\n");
                                            }
                                        else printf("transaction input successfully signed!\n");
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
                            running_transaction_input_menu = 0;
                            break;
                    }
                }
            }
        }
    }

void transaction_output_menu(int txindex, int is_testnet) {
    int running_transaction_output_menu = 1;
    while (running_transaction_output_menu) {
        char* destinationaddress;
        char* coin_amount[21];
        dogecoin_mem_zero(coin_amount, 21);
        uint64_t koinu_amount;
        uint64_t tx_out_total = 0;
        const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;
        working_transaction* tx = find_transaction(txindex);
        int length = tx->transaction->vout->len;
        int selected = -1;
        printf("length: %d\n", length);
        for (int i = 0; i < length; i++) {
            dogecoin_tx_out* tx_out = vector_idx(tx->transaction->vout, i);
            tx_out_total += tx_out->value;
            printf("\n--------------------------------\n");
            printf("output index:       %d\n", i);
            printf("script public key:  %s\n", utils_uint8_to_hex((const uint8_t*)tx_out->script_pubkey->str, tx_out->script_pubkey->len));
            koinu_to_coins_str(tx_out->value, (char*)coin_amount);
            printf("amount:             %s\n", (char*)coin_amount);
            // selected should only equal anything other than -1 upon setting
            // loop index in conditional targetting last iteration:
            selected == i ? printf("selected:           [X]\n") : 0;
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
                                        destinationaddress = (char*)getl("new destination address");
                                        if (!verifyP2pkhAddress(destinationaddress, strlen(destinationaddress))) {
                                            printf("\ninvalid destination address!\n");
                                            break;
                                            }
                                        else {
                                            koinu_amount = coins_to_koinu_str((char*)coin_amount);
                                            vector_remove_idx(tx->transaction->vout, i);
                                            dogecoin_tx_add_address_out(tx->transaction, chain, koinu_amount, destinationaddress);
                                            }
                                        break;
                                    case 2:
                                        memcpy_safe(coin_amount, (char*)getl("new amount"), 21);
                                        if (strspn((char*)coin_amount, "0123456789") == strlen((char*)coin_amount)) {
                                            koinu_amount = coins_to_koinu_str((char*)coin_amount);
                                            tx_out->value = koinu_amount;
                                            }
                                        else {
                                            printf("\namount is not a number!\n");
                                            }
                                        break;
                                }
                            tx_out_total = 0;
                            i = i - i - 1; // reset loop to start
                            break;
                        case 2:
                            selected = -1; // set selected to number out of bounds for i
                            tx_out_total = 0;
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
                char* subtotal[21];
                dogecoin_mem_zero(subtotal, 21);
                koinu_to_coins_str(tx_out_total, (char*)subtotal);
                printf("subtotal - desired fee: %s\n", (char*)subtotal);
                printf("\n");
                printf("1. select output to edit\n");
                printf("2. main menu\n");
                switch (atoi(getl("\ncommand"))) {
                        case 1:
                            // tx_input submenu
                            selected = atoi(getl("vout index"));
                            tx_out_total = 0;
                            i = i - i - 1;
                            break;
                        case 2:
                            i = length;
                            running_transaction_output_menu = 0;
                            break;
                    }
                }
            }
        }
    }

void edit_menu(int txindex, int is_testnet) {
    int running_edit_menu = 1;
    while (running_edit_menu) {
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
                    running_edit_menu = 0;
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
    struct working_transaction* s;
    int temp, txindex;
    wow();

    // load existing testnet transaction into memory for demonstration purposes.
    save_raw_transaction(start_transaction(), "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000");
    while (running) {
        printf("\nsuch transaction: \n\n");
        printf(" 1. add transaction\n");
        printf(" 2. edit transaction by id\n");
        printf(" 3. find transaction\n");
        printf(" 4. sign transaction\n");
        printf(" 5. delete transaction\n");
        printf(" 6. delete all transactions\n");
        printf(" 7. print transactions\n");
        printf(" 8. import raw transaction (memory)\n");
#ifdef WITH_NET
        printf(" 9. broadcast transaction\n");
        printf(" 10. change network (current: %s)\n", is_testnet ? "testnet" : "mainnet");
        printf(" 11. quit\n");
#else
        printf(" 9. change network (current: %s)\n", is_testnet ? "testnet" : "mainnet");
        printf(" 10. quit\n");
#endif
        switch (atoi(getl("\ncommand"))) {
                case 1:
                    sub_menu(start_transaction(), is_testnet);
                    break;
                case 2:
                    temp = atoi(getl("ID of transaction to edit"));
                    s = find_transaction(temp);
                    if (s) {
                        edit_menu(temp, is_testnet);
                        }
                    else {
                        printf("\nno transaction found with that id. please try again!\n");
                        }
                    break;
                case 3:
                    s = find_transaction(atoi(getl("ID to find")));
                    s ? printf("transaction: %s\n", get_raw_transaction(s->idx)) : printf("\nno transaction found with that id. please try again!\n");
                    break;
                case 4:
                    temp = atoi(getl("ID of transaction to sign"));
                    s = find_transaction(temp);
                    if (s) {
                        signing_menu(temp, is_testnet);
                        }
                    else {
                        printf("\nno transaction found with that id. please try again!\n");
                        }
                    break;
                case 5:
                    s = find_transaction(atoi(getl("ID to delete")));
                    if (s) {
                        remove_transaction(s);
                        }
                    else {
                        printf("\nno transaction found with that id. please try again!\n");
                        }
                    break;
                case 6:
                    remove_all();
                    break;
                case 7:
                    count_transactions();
                    print_transactions();
                    break;
                case 8:
                    txindex = start_transaction();
                    int res = save_raw_transaction(txindex, get_raw_tx("raw transaction"));
                    if (!res) {
                        printf("error saving transaction!\n");
                        clear_transaction(txindex);
                        }
                    else {
                        printf("successfully saved raw transaction to memory for the session!\n");
                        printf("working transaction id is: %d\n", txindex);
                        }
                    break;
#ifdef WITH_NET
                case 9:
                    temp = atoi(getl("ID of transaction to edit"));
                    s = find_transaction(temp);
                    if (s) {
                        broadcasting_menu(temp, is_testnet);
                        }
                    else {
                        printf("\nno transaction found with that id. please try again!\n");
                        }
                    break;
                case 10:
                    is_testnet = chainparams_menu(is_testnet);
                    break;
                case 11:
                    running = 0;
                    break;
#else
                case 9:
                    is_testnet = chainparams_menu(is_testnet);
                    break;
                case 10:
                    running = 0;
                    break;
#endif
            }
        }
    remove_all();
    }

// ******************************** END TRANSACTION MENU ********************************

// ******************************** CLI INTERFACE ********************************
static struct option long_options[] =
    {
        {"privkey", required_argument, NULL, 'p'},
        {"pubkey", required_argument, NULL, 'k'},
        {"derived_path", required_argument, NULL, 'm'},
        {"entropy", required_argument, NULL, 'e'},
        {"mnemonic", required_argument, NULL, 'n'},
        {"pass_phrase", required_argument, NULL, 'a'},
        {"account_int", required_argument, NULL, 'o'},
        {"change_level", required_argument, NULL, 'g'},
        {"address_index", required_argument, NULL, 'i'},
        {"tpm", required_argument, NULL, 'y'},
        {"command", required_argument, NULL, 'c'},
        {"silent", no_argument, NULL, 'b'},
        {"overwrite", no_argument, NULL, 'w'},
        {"testnet", no_argument, NULL, 't'},
        {"regtest", no_argument, NULL, 'r'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0} };

static void print_version()
    {
    printf("Version: %s %s\n", PACKAGE_NAME, PACKAGE_VERSION);
    }

static void print_usage()
    {
    print_version();
    printf("Usage: such -c <cmd> (-m|-derived_path <bip_derived_path>) (-k|-pubkey <publickey>) (-p|-privkey <privatekey>) \
(-e|-entropy <hex_entropy>) (-n|-mnemonic <seed_phrase>) (-a|-pass_phrase <pass_phrase>) (-y|-tpm <slot 0-999>) (-w[--overwrite]) (-b[--silent]) \
(-t[--testnet]) (-r[--regtest])\n");
    printf("Available commands:\n");
    printf("generate_public_key (requires -p <wif>),\n");
    printf("p2pkh (requires -k <public key hex>),\n");
    printf("generate_private_key,\n");
    printf("bip32_extended_master_key (-y <slot> and -w (overwrite), both optional),\n");
    printf("generate_mnemonic (-e <hex_entropy> or -y <slot>, -w (overwrite) and -b (silent), all optional),\n");
    printf("list_mnemonics_in_tpm,\n");
    printf("list_objects_in_tpm,\n");
    printf("erase_mnemonic_from_tpm (-y <slot>),\n");
    printf("export_mnemonic_from_tpm (-y <slot>),\n");
    printf("mnemonic_to_addresses (requires -n <seed_phrase> or -y <slot>, -o <account_int>, -g <change_level>, -i <address_index> and -a <pass_phrase>, all optional),\n");
    printf("print_keys (requires -p <private key hex>),\n");
    printf("derive_child_keys (requires -m <custom path> -p <private key>),\n");
    printf("sign (-x <raw hex tx> -s <script pubkey> -i <input index> -h <sighash type> -p <private key>),\n");
    printf("comp2der (-s <compact signature>),\n");
    printf("bip32maintotest (-p <extended hd master key>),\n");
    printf("signmessage (-x '<message>' -p <private key>),\n");
    printf("verifymessage (-x '<message>' -s <signature (base64 encoded)> -k <address>),\n");
    printf("transaction\n");
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

int main(int argc, char* argv[])
    {
    int long_index = 0;
    int opt = 0;
    char* pkey = 0;
    char* pubkey = 0;
    char* cmd = 0;
    char* derived_path = 0;
    int account = BIP44_FIRST_ACCOUNT_NODE;   /* default account (BIP44_FIRST_ACCOUNT_NODE) */
    char* change_level = BIP44_CHANGE_EXTERNAL;    /* default external (BIP44_CHANGE_EXTERNAL) */
    char* mnemonic_in = 0;
    char* pass = 0;
    char* entropy = 0;
    MNEMONIC mnemonic = {0};
    dogecoin_bool tpm = false;
    dogecoin_bool overwrite = false;
    dogecoin_bool silent = false;
    int slot = NO_SLOT;

    char* txhex = 0;
    char* scripthex = 0;
    int inputindex = 0;
    int sighashtype = 1;
    dogecoin_mem_zero(&pkey, sizeof(pkey));
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;

    /* get arguments */
    while ((opt = getopt_long_only(argc, argv, "h:i:s:x:p:k:m:o:g:e:n:a:y:c:trvbw", long_options, &long_index)) != -1) {
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
                case 'o':
                    account = (int)strtol(optarg, (char**)NULL, 10);
                    break;
                case 'g':
                    change_level = optarg;
                    break;
                case 'e':
                    entropy = optarg;
                    break;
                case 'n':
                    mnemonic_in = optarg;
                    break;
                case 'a':
                    pass = optarg;
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
                case 'w':
                    overwrite = true;
                    break;
                case 'b':
                    silent = true;
                    break;
                case 'y':
                    tpm = true;
                    slot = (int)strtol(optarg, (char**)NULL, 10);
                    break;
                case 'x':
                    txhex = optarg;
                    break;
                case 's':
                    scripthex = optarg;
                    break;
                case 'i':
                    inputindex = (int)strtol(optarg, (char**)NULL, 10);
                    break;
                case 'h':
                    sighashtype = (int)strtol(optarg, (char**)NULL, 10);
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

        char pubkey_hex[128];
        size_t sizeout = sizeof(pubkey_hex);

        if (!pkey)
            return showError(pkey_error);
        if (!pubkey_from_privatekey(chain, pkey, pubkey_hex, &sizeout))
            return showError("attempt to generate pubkey from privatekey failed");

        /* erase previous private key */
        dogecoin_mem_zero(pkey, strlen(pkey));

        /* generate public key hex from private key hex */
        printf("public key hex: %s\n", pubkey_hex);

        /* give out p2pkh address */
        char* address_p2pkh = dogecoin_char_vla(sizeout);
        addresses_from_pubkey(chain, pubkey_hex, address_p2pkh);
        printf("p2pkh address: %s\n", address_p2pkh);

        /* clean memory */
        dogecoin_mem_zero(pubkey_hex, strlen(pubkey_hex));
        dogecoin_mem_zero(address_p2pkh, strlen(address_p2pkh));
        free(address_p2pkh);
        /* Creating a new address from a public key. */
        }
    else if (strcmp(cmd, "p2pkh") == 0) {
        char address_p2pkh[128];
        if (!pubkey)
            return showError("Missing public key (use -k)");
        if (!addresses_from_pubkey(chain, pubkey, address_p2pkh))
            return showError("Operation failed, invalid pubkey");
        printf("p2pkh address: %s\n", address_p2pkh);

        dogecoin_mem_zero(pubkey, strlen(pubkey));
        dogecoin_mem_zero(address_p2pkh, strlen(address_p2pkh));
        /* Generating a new private key and printing it out. */
        }
    else if (strcmp(cmd, "generate_private_key") == 0) {
        char newprivkey_wif[128];
        char newprivkey_hex[128];

        /* generate a new private key */
        gen_privatekey(chain, newprivkey_wif, sizeof(newprivkey_wif), newprivkey_hex);
        printf("private key wif: %s\n", newprivkey_wif);
        printf("private key hex: %s\n", newprivkey_hex);
        dogecoin_mem_zero(newprivkey_wif, strlen(newprivkey_wif));
        dogecoin_mem_zero(newprivkey_hex, strlen(newprivkey_hex));
        /* Generating a new master key. */
        }
    else if (strcmp(cmd, "bip32_extended_master_key") == 0) {
        char masterkey[128];

        /* if tpm is enabled, use it to generate a new master key */
        if (tpm) {

            /* if overwrite is enabled, ask for confirmation */
            if (overwrite) {
                printf("Overwrite? Y/N\n");

                /* get user input */
                char c = getchar();
                if (c != 'Y' && c != 'y') {

                    /* if not confirmed, abort */
                    printf("aborted\n");
                    dogecoin_ecc_stop();
                    return 1;
                    }
            }

            dogecoin_hdnode node;
            if (!dogecoin_generate_hdnode_in_tpm(&node, slot, overwrite)) {
                printf("bip32_extended_master_key (-y <slot> and -w (overwrite), both optional),\n");
                return showError("failed to generate master key in TPM");
            }

            /* serialize the master key */
            dogecoin_hdnode_serialize_private (&node, chain, masterkey, sizeof(masterkey));
            }

        /* otherwise, generate a new master key from entropy */
        else {
            /* generate a new hd master key */
            hd_gen_master(chain, masterkey, sizeof(masterkey));
            }
        printf("bip32 extended master key: %s\n", masterkey);
        dogecoin_mem_zero(masterkey, strlen(masterkey));
        }
    else if (strcmp(cmd, "print_keys") == 0) {
        if (!pkey)
            return showError("no extended key (-p)");
        if (!hd_print_node(chain, pkey))
            return showError("invalid extended key\n");
        }
    else if (strcmp(cmd, "derive_child_keys") == 0) {
        if (!pkey)
            return showError("no extended key (-p)");
        if (!derived_path)
            return showError("no derivation path (-m)");
        char newextkey[128];

        //check if we derive a range of keys
        unsigned int maxlen = 1024;
        int posanum = -1;
        int posbnum = -1;
        int end = -1;
        uint64_t from = 0;
        uint64_t to = 0;

        static char digits[] = "0123456789";
        unsigned int i;
        for (i = 0; i < strlen(derived_path); i++) {
            if (i > maxlen) {
                break;
                }
            if (posanum > -1 && posbnum == -1) {
                if (derived_path[i] == '-') {
                    if (i - posanum >= 9) {
                        break;
                        }
                    posbnum = i + 1;
                    char buf[9] = { 0 };
                    memcpy_safe(buf, &derived_path[posanum], i - posanum);
                    from = strtoull(buf, NULL, 10);
                    }
                else if (!strchr(digits, derived_path[i])) {
                    posanum = -1;
                    break;
                    }
                }
            else if (posanum > -1 && posbnum > -1) {
                if (derived_path[i] == ']' || derived_path[i] == ')') {
                    if (i - posbnum >= 9) {
                        break;
                        }
                    char buf[9] = { 0 };
                    memcpy_safe(buf, &derived_path[posbnum], i - posbnum);
                    to = strtoull(buf, NULL, 10);
                    end = i + 1;
                    break;
                    }
                else if (!strchr(digits, derived_path[i])) {
                    // posbnum = -1; // value stored is never read
                    break;
                    }
                }
            if (derived_path[i] == '[' || derived_path[i] == '(') {
                posanum = i + 1;
                }
            }

        if (end > -1 && from <= to) {
            for (i = from; i <= to; i++) {
                char* keypathnew = dogecoin_char_vla(strlen(derived_path) + 16);
                memcpy_safe(keypathnew, derived_path, posanum - 1);
                char index[11] = { 0 };
                sprintf(index, "%lld", (long long)i);
                memcpy_safe(keypathnew + posanum - 1, index, strlen(index));
                memcpy_safe(keypathnew + posanum - 1 + strlen(index), &derived_path[end], strlen(derived_path) - end);

                if (!hd_derive(chain, pkey, keypathnew, newextkey, sizeof(newextkey)))
                    {
                    free(keypathnew);
                    return showError("Deriving child key failed\n");
                    }
                else
                    {
                    free(keypathnew);
                    hd_print_node(chain, newextkey);
                    }
                }
            }
        else {
            if (!hd_derive(chain, pkey, derived_path, newextkey, sizeof(newextkey)))
                return showError("Deriving child key failed\n");
            else
                hd_print_node(chain, newextkey);
            }
        }
    else if (strcmp(cmd, "sign") == 0) {
        // ./such -c sign -x <raw hex tx> -s <script pubkey> -i <input index> -h <sighash type> -p <private key>
        if (!txhex || !scripthex) {
            return showError("Missing tx-hex or script-hex (use -x, -s)\n");
            }

        if (strlen(txhex) > 1024 * 100) { //don't accept tx larger then 100kb
            return showError("tx too large (max 100kb)\n");
            }

        //deserialize transaction
        dogecoin_tx* tx = dogecoin_tx_new();
        uint8_t* data_bin = dogecoin_malloc(strlen(txhex) / 2 + 1);
        size_t outlen = 0;
        utils_hex_to_bin(txhex, data_bin, strlen(txhex), &outlen);
        if (!dogecoin_tx_deserialize(data_bin, outlen, tx, NULL)) {
            dogecoin_free(data_bin);
            dogecoin_tx_free(tx);
            return showError("Invalid tx hex");
            }

        dogecoin_free(data_bin);

        if ((size_t)inputindex >= tx->vin->len) {
            dogecoin_tx_free(tx);
            return showError("Inputindex out of range");
            }

        uint8_t* script_data = dogecoin_uint8_vla(strlen(scripthex) / 2 + 1);
        utils_hex_to_bin(scripthex, script_data, strlen(scripthex), &outlen);
        cstring* script = cstr_new_buf(script_data, outlen);
        free(script_data);

        uint256 sighash;
        dogecoin_mem_zero(sighash, sizeof(sighash));
        dogecoin_tx_sighash(tx, script, inputindex, sighashtype, sighash);

        char* hex = utils_uint8_to_hex(sighash, 32);
        utils_reverse_hex(hex, 64);

        enum dogecoin_tx_out_type type = dogecoin_script_classify(script, NULL);
        printf("script: %s\n", scripthex);
        printf("script-type: %s\n", dogecoin_tx_out_type_to_str(type));
        printf("inputindex: %d\n", inputindex);
        printf("sighashtype: %d\n", sighashtype);
        printf("hash: %s\n", hex);

        // sign
        dogecoin_bool sign = false;
        dogecoin_key key;
        dogecoin_privkey_init(&key);
        if (dogecoin_privkey_decode_wif(pkey, chain, &key)) {
            sign = true;
            }
        else {
            if (pkey) {
                if (strlen(pkey) > 50) {
                    dogecoin_tx_free(tx);
                    cstr_free(script, true);
                    return showError("Invalid wif privkey\n");
                    }
                }
            else {
                printf("No private key provided, signing will not happen\n");
                }
            }
        if (sign) {
            uint8_t sigcompact[64] = { 0 };
            size_t sigderlen = 74 + 1; //&hashtype
            uint8_t sigder_plus_hashtype[75] = { 0 };
            enum dogecoin_tx_sign_result res = dogecoin_tx_sign_input(tx, script, &key, inputindex, sighashtype, sigcompact, sigder_plus_hashtype, &sigderlen);
            cstr_free(script, true);

            if (res != DOGECOIN_SIGN_OK) {
                printf("!!!Sign error:%s\n", dogecoin_tx_sign_result_to_str(res));
                }

            char sigcompacthex[64 * 2 + 1] = { 0 };
            utils_bin_to_hex((unsigned char*)sigcompact, 64, sigcompacthex);

            char sigderhex[74 * 2 + 2 + 1]; //74 der, 2 hashtype, 1 nullbyte
            dogecoin_mem_zero(sigderhex, sizeof(sigderhex));
            utils_bin_to_hex((unsigned char*)sigder_plus_hashtype, sigderlen, sigderhex);

            printf("\nSignature created:\n");
            printf("signature compact: %s\n", sigcompacthex);
            printf("signature DER (+hashtype): %s\n", sigderhex);

            cstring* signed_tx = cstr_new_sz(1024);
            dogecoin_tx_serialize(signed_tx, tx);

            char* signed_tx_hex = dogecoin_char_vla(signed_tx->len * 2 + 1);
            utils_bin_to_hex((unsigned char*)signed_tx->str, signed_tx->len, signed_tx_hex);
            printf("signed TX: %s\n", signed_tx_hex);
            cstr_free(signed_tx, true);
            free(signed_tx_hex);
            }
        dogecoin_tx_free(tx);
        }
    else if (strcmp(cmd, "comp2der") == 0) {
        // ./such -c comp2der -s <compact signature>
        if (!scripthex || strlen(scripthex) != 128) {
            return showError("Missing signature or invalid length (use hex, 128 chars == 64 bytes)\n");
            }

        size_t outlen = 0;
        uint8_t sig_comp[65];
        printf("%s\n", scripthex);
        utils_hex_to_bin(scripthex, sig_comp, 128, &outlen);

        unsigned char sigder[74];
        size_t sigderlen = sizeof(sigder);

        dogecoin_ecc_compact_to_der_normalized(sig_comp, sigder, &sigderlen);
        char* hexbuf = dogecoin_char_vla(sigderlen * 2 + 1);
        utils_bin_to_hex(sigder, sigderlen, hexbuf);
        printf("DER: %s\n", hexbuf);
        free(hexbuf);
        }
    else if (strcmp(cmd, "bip32maintotest") == 0) { /* Creating a bip32 master key from a private key. */
        dogecoin_hdnode node;
        if (!dogecoin_hdnode_deserialize(pkey, chain, &node)) {
            return showError("dogecoin_hd_deserialize failed!\n");
            }
        char masterkeyhex[200];
        int strsize = 200;
        dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_test, masterkeyhex, strsize);
        printf("xpriv: %s\n", masterkeyhex);
        dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_test, masterkeyhex, strsize);
        printf("xpub: %s\n", masterkeyhex);
        }
    else if (strcmp(cmd, "generate_mnemonic") == 0) { /* Creating a bip32 master key from a mnemonic. */

        /* if tpm is enabled, generate mnemonic with tpm */
        if (tpm) {

            /* if overwrite is enabled, ask for confirmation */
            if (overwrite) {
                printf("Overwrite? Y/N\n");

                /* get user input */
                char c = getchar();
                if (c != 'Y' && c != 'y') {

                    /* if not confirmed, abort */
                    printf("aborted\n");
                    dogecoin_ecc_stop();
                    return 1;
                    }
            }

            /* generate mnemonic with tpm */
            if (generateRandomEnglishMnemonicTPM (mnemonic, slot, overwrite) == false) {
                printf("generate_mnemonic -y <slot>, -w (overwrite), -b (silent),\n");
                return showError("failed to generate mnemonic in TPM");
                }
            }

        /* else generate mnemonic with ecc */
        else if (generateEnglishMnemonic(entropy, "256", mnemonic) == -1) {
            printf("generate_mnemonic (-e <hex_entropy>, optional),\n");
            return showError("failed to generate mnemonic\n");
            }

        /* if not silent, display mnemonic */
        if (!silent) {
            printf("%s\n", mnemonic);
            }
        }
    else if (strcmp(cmd, "list_mnemonics_in_tpm") == 0) {

        /* list mnemonics in tpm */
        wchar_t *names[MAX_SLOTS] = {0};
        size_t count = 0;

        if (dogecoin_list_mnemonics_in_tpm(names, &count) == false) {
            return showError("failed to list mnemonics in tpm\n");
            }

        /* display mnemonics and slot numbers */
        printf("object            slot\n");

        for (int i = 0; i < count; i++) {
            /* display mnemonic and slot number */
            printf("%ls\n", names[i]);
            }

        /* free memory */
        for (int i = 0; i < count; i++) {
            dogecoin_free(names[i]);
            }
        }
    else if (strcmp(cmd, "list_objects_in_tpm") == 0) {

        /* list objects in tpm */
        wchar_t *names[MAX_SLOTS] = {0};
        size_t count = 0;

        if (dogecoin_list_objects_in_tpm(names, &count) == false) {
            return showError("failed to list objects in tpm\n");
            }

        /* display objects names */
        for (int i = 0; i < count; i++) {
            wprintf(L"%ls\n", names[i]);
            }

        /* free memory */
        for (int i = 0; i < count; i++) {
            dogecoin_free(names[i]);
            }
        }
    else if (strcmp(cmd, "erase_mnemonic_from_tpm") == 0) {

        /* if tpm is enabled, erase mnemonic from tpm */
        if (tpm) {
            printf("Erase? Y/N\n");

            /* get user input */
            char c = getchar();
            if (c == 'Y' || c == 'y') {

                /* erase mnemonic from tpm */
                if (dogecoin_erase_mnemonic_from_tpm(slot) == false) {
                    printf("erase_mnemonic_from_tpm (-y <slot>),\n");
                    return showError("failed to erase mnemonic from tpm\n");
                    }
                }
            else {

                /* if not confirmed, abort */
                printf("aborted\n");
                dogecoin_ecc_stop();
                return 1;
                }
            }
        /* else display usage */
        else {
            return showError("erase_mnemonic_from_tpm (-y <slot>)\n");
            }
        }
    else if (strcmp(cmd, "export_mnemonic_from_tpm") == 0) {

        /* if tpm is enabled, export mnemonic from tpm */
        if (tpm) {
            printf("Export mnemonic? Y/N\n");

            /* get user input */
            char c = getchar();
            if (c != 'Y' && c != 'y') {

                /* if not confirmed, abort */
                printf("aborted\n");
                dogecoin_ecc_stop();
                return 1;
                }

            /* export mnemonic from tpm */
            if (exportEnglishMnemonicTPM(slot, mnemonic) == false) {
                printf("export_mnemonic_from_tpm (-y <slot>),\n");
                return showError("failed to export mnemonic from tpm\n");
                }

            /* display mnemonic */
            printf("%s\n", mnemonic);
            }

        /* else display usage */
        else {
            return showError("export_mnemonic_from_tpm (-y <slot>)\n");
            }
        }
    else if (strcmp(cmd, "mnemonic_to_addresses") == 0) { /* Creating wif addresses from a mnemonic via slip44. */

        char hd_pubkey_address[53];

        /* if tpm is enabled, get mnemonic from tpm */
        if (tpm) {
            printf("Export mnemonic for addresses? Y/N\n");

            /* get user input */
            char c = getchar();
            if (c != 'Y' && c != 'y') {

                /* if not confirmed, abort */
                printf("aborted\n");
                dogecoin_ecc_stop();
                return 1;
                }

            /* get mnemonic from tpm */
            if (exportEnglishMnemonicTPM(slot, mnemonic) == false) {
                printf("mnemonic_to_addresses (requires -y <slot>, -o <account_int>, -g <change_level>, -i <address_index> and -a <pass_phrase>, all optional),\n");
                return showError("failed to export mnemonic from tpm\n");
                }
            }

        /* else display usage */
        else {
            return showError("mnemonic_to_addresses (requires -n <seed_phrase> or -y <slot>, -o <account_int>, -g <change_level>, -i <address_index> and -a <pass_phrase> (all optional))\n");
            }

        /* generate wif address for slip44 account, index, and change_level, from bip39 mnemonic and password (optional) */
        if (inputindex == 0) {

            /* Generate all addresses for the account. */
            for (int i = 0; i < 20; i++) {
                if (getDerivedHDAddressFromMnemonic(account, i, change_level, tpm ? mnemonic : mnemonic_in, pass, hd_pubkey_address, (chain == &dogecoin_chainparams_test)) == -1) {
                    return showError("failed to generate wif address from mnemonic\n");
                    }
                printf("Address %d: %s\n", i, hd_pubkey_address);
                }
            }
        else {

            /* Generate a single address for the account. */
            if (getDerivedHDAddressFromMnemonic(account, inputindex, change_level, tpm ? mnemonic : mnemonic_in, pass, hd_pubkey_address, (chain == &dogecoin_chainparams_test)) == -1) {
                printf("mnemonic_to_addresses (requires -n <seed_phrase> or -y <slot>, -o <account_int>, -g <change_level>, -i <address_index> and -a <pass_phrase>, all optional),\n");
                return showError("failed to generate wif address from mnemonic\n");
                }

            printf("Address %d: %s\n", inputindex, hd_pubkey_address);
            }
        }
    else if (strcmp(cmd, "signmessage") == 0) {
        // ./such -c signmessage -x "<message>" -p <private key>
        if (!txhex) {
            return showError("Missing message (use -x)\n");
            }

        if (strlen(txhex) > 1024 * 100) { //don't accept tx larger then 100kb
            return showError("tx too large (max 100kb)\n");
            }

        eckey* key = new_eckey_from_privkey(pkey);
        char* sig = sign_message(key->private_key_wif, txhex);
        printf("message: %s\n", txhex);
        printf("content: %s\n", sig);
        printf("address: %s\n", key->address);
        dogecoin_free(key);
        dogecoin_free(sig);
        }
    else if (strcmp(cmd, "verifymessage") == 0) {
        // ./such -c verifymessage -x "<message>" -s <signature> -k <address>
        if (!txhex || !scripthex || !pubkey) {
            return showError("Missing message or signature or address (use -x, -s, -k)\n");
            }

        if (strlen(txhex) > 1024 * 100) { //don't accept tx larger then 100kb
            return showError("tx too large (max 100kb)\n");
            }

        if (verify_message(scripthex, txhex, pubkey)) {
            printf("Message is verified!\n");
        } else {
            printf("Message is not valid!\n");
        }
        }
    else if (strcmp(cmd, "transaction") == 0) {
        main_menu();
        }
    else {
        print_usage();
        return showError("Unknown command\n");
    }

    dogecoin_ecc_stop();

    return 0;
    }
