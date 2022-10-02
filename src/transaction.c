/*

 The MIT License (MIT)

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

#include <dogecoin/base58.h>
#include <dogecoin/koinu.h>
#include <dogecoin/transaction.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

/**
 * @brief This function instantiates a new working transaction,
 * but does not add it to the hash table.
 * 
 * @return A pointer to the new working transaction. 
 */
working_transaction* new_transaction() {
    working_transaction* working_tx = (struct working_transaction*)dogecoin_calloc(1, sizeof *working_tx);
    working_tx->transaction = dogecoin_tx_new();
    working_tx->idx = HASH_COUNT(transactions) + 1;
    return working_tx;
}

/**
 * @brief This function takes a pointer to an existing working
 * transaction object and adds it to the hash table.
 * 
 * @param working_tx The pointer to the working transaction.
 * 
 * @return Nothing.
 */
void add_transaction(working_transaction *working_tx) {
    working_transaction *tx;
    HASH_FIND_INT(transactions, &working_tx->idx, tx);
    if (tx == NULL) {
        HASH_ADD_INT(transactions, idx, working_tx);
    } else {
        HASH_REPLACE_INT(transactions, idx, working_tx, tx);
    }
    dogecoin_free(tx);
}

/**
 * @brief This function takes an index and returns the working
 * transaction associated with that index in the hash table.
 * 
 * @param idx The index of the target working transaction.
 * 
 * @return The pointer to the working transaction associated with
 * the provided index.
 */
working_transaction* find_transaction(int idx) {
    working_transaction *working_tx;
    HASH_FIND_INT(transactions, &idx, working_tx);
    return working_tx;
}

/**
 * @brief This function removes the specified working transaction
 * from the hash table and frees the transactions in memory.
 * 
 * @param working_tx The pointer to the transaction to remove.
 * 
 * @return Nothing.
 */
void remove_transaction(working_transaction *working_tx) {
    HASH_DEL(transactions, working_tx); /* delete it (transactions advances to next) */
    dogecoin_tx_free(working_tx->transaction);
    dogecoin_free(working_tx);
}

/**
 * @brief This function removes all working transactions from
 * the hash table.
 * 
 * @return Nothing. 
 */
void remove_all() {
    struct working_transaction *current_tx;
    struct working_transaction *tmp;

    HASH_ITER(hh, transactions, current_tx, tmp) {
        remove_transaction(current_tx);
    }
}

/**
 * @brief This function prints the raw hex representation of
 * each working transaction in the hash table.
 * 
 * @return Nothing. 
 */
void print_transactions()
{
    struct working_transaction *s;

    for (s = transactions; s != NULL; s = (struct working_transaction*)(s->hh.next)) {
        printf("\nworking transaction id: %d\nraw transaction (hexadecimal): %s\n", s->idx, get_raw_transaction(s->idx));
    }
}

/**
 * @brief This function counts the number of working
 * transactions currently in the hash table.
 * 
 * @return Nothing. 
 */
void count_transactions() {
    int temp = HASH_COUNT(transactions);
    printf("there are %d transactions\n", temp);
}

/**
 * @brief This function takes two working transactions
 * and returns the difference of their indices to aid
 * in sorting transactions.
 * 
 * @param a The pointer to the first working transaction.
 * @param b The pointer to the second working transaction.
 * 
 * @return The integer difference between the indices of
 * the two provided transactions. 
 */
int by_id(const struct working_transaction *a, const struct working_transaction *b)
{
    return (a->idx - b->idx);
}

/**
 * @brief This function prints a prompt and parses the user's
 * response for a CLI tool.
 * 
 * @param prompt The prompt to display to the user.
 * 
 * @return The string containing user input. 
 */
const char *getl(const char *prompt)
{
    static char buf[100];
    char *p;
    printf("%s? ", prompt); fflush(stdout);
    p = fgets(buf, sizeof(buf), stdin);
    if (p == NULL || (p = strchr(buf, '\n')) == NULL) {
        puts("invalid input!");
        exit(EXIT_FAILURE);
    }
    *p = '\0';
    return buf;
}

/**
 * @brief This function prompts the user to enter a raw
 * transaction and parses it.
 * 
 * @param prompt_tx The prompt to display to the user.
 * 
 * @return The string containing user input.
 */
const char *get_raw_tx(const char *prompt_tx)
{
    static char buf_tx[1000*100];
    char *p_tx;
    printf("%s? ", prompt_tx); fflush(stdout);
    p_tx = fgets(buf_tx, sizeof(buf_tx), stdin);
    if (p_tx == NULL || (p_tx = strchr(buf_tx, '\n')) == NULL) {
        puts("invalid input!");
        exit(EXIT_FAILURE);
    }
    *p_tx = '\0';
    return buf_tx;
}

/**
 * @brief This function prompts the user to enter a private key
 * and parses it.
 * 
 * @param prompt_tx The prompt to display to the user.
 * 
 * @return The string containing user input.
 */
const char *get_private_key(const char *prompt_key)
{
    static char buf_key[100];
    char *p_key;
    printf("%s? ", prompt_key); fflush(stdout);
    p_key = fgets(buf_key, sizeof(buf_key), stdin);
    if (p_key == NULL || (p_key = strchr(buf_key, '\n')) == NULL) {
        puts("invalid input!");
        exit(EXIT_FAILURE);
    }
    *p_key = '\0';
    return buf_key;
}

/**
 * @brief This function creates a new transaction, places it in
 * the hash table, and returns the index of the new transaction,
 * starting from 1 and incrementing each subsequent call.
 * 
 * @return The index of the new transaction.
 */
int start_transaction() {
    working_transaction* working_tx = new_transaction();
    int index = working_tx->idx;
    add_transaction(working_tx);
    return index;
}

/**
 * @brief This function takes a transaction represented in raw
 * hex and serializes it into a transaction which is then saved
 * in the hashtable at the specified index.
 * 
 * @param txindex The index to save the transaction to.
 * @param hexadecimal_transaction The raw hex of the transaction to serialize and save.
 * 
 * @return 1 if the transaction was saved successfully, 0 otherwise. 
 */
int save_raw_transaction(int txindex, const char* hexadecimal_transaction) {
    debug_print("raw_hexadecimal_transaction: %s\n", hexadecimal_transaction);
    if (strlen(hexadecimal_transaction) > 1024*100) { //don't accept tx larger then 100kb
        printf("tx too large (max 100kb)\n");
        return false;
    }

    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    uint8_t* data_bin = dogecoin_malloc(strlen(hexadecimal_transaction));
    size_t outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(hexadecimal_transaction, data_bin, strlen(hexadecimal_transaction), &outlength);
    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex");
        return false;
    }
    // free byte array
    working_transaction* tx_raw = find_transaction(txindex);
    dogecoin_tx_copy(tx_raw->transaction, txtmp);
    dogecoin_tx_free(txtmp);
    dogecoin_free(data_bin);
    return true;
}

/**
 * @brief This function takes a transaction represented in raw
 * hex and adds it as an input to the specified working transaction.
 * 
 * @param txindex The index of the transaction to add the input to.
 * @param hex_utxo_txid The raw transaction hex of the input transaction.
 * @param vout The output index of the input transaction containing spendable funds.
 * 
 * @return 1 if the transaction input was added successfully, 0 otherwise. 
 */
int add_utxo(int txindex, char* hex_utxo_txid, int vout) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    size_t flag = tx->transaction->vin->len;

    // instantiate empty dogecoin_tx_in object to set previous output txid and output n:
    dogecoin_tx_in* tx_in = dogecoin_tx_in_new();

    // add prevout hash to tx_in->prevout.hash in prep of adding to tx->transaction-vin vector
    utils_uint256_sethex((char *)hex_utxo_txid, (uint8_t *)tx_in->prevout.hash);

    // set index of utxo we want to spend
    tx_in->prevout.n = vout;

    // add to working tx object
    vector_add(tx->transaction->vin, tx_in);

    // free tx_in struct since it has been added to our working tx
    // ensure the length of our working tx inputs length has incremented by 1
    // which will return true if successful:
    return flag + 1 == tx->transaction->vin->len;
}

/**
 * @brief This function constructs an output sending the specified
 * amount to the specified address and adds it to the transaction
 * with the specified index.
 * 
 * @param txindex The index of the transaction where the output will be added.
 * @param destinationaddress The address to send the funds to.
 * @param amount The amount of dogecoin to send.
 * 
 * @return 1 if the transaction input was added successfully, 0 otherwise.
 */
int add_output(int txindex, char* destinationaddress, char* amount) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);
    // guard against null pointer exceptions
    if (tx == NULL) {
        return false;
    }
    // determine intended network by checking address prefix:
    const dogecoin_chainparams* chain = (destinationaddress[0] == 'D') ? &dogecoin_chainparams_main : &dogecoin_chainparams_test;

    uint64_t koinu = coins_to_koinu_str(amount);
    // calculate total minus fees
    // pass in transaction obect, network paramters, amount of dogecoin to send to address and finally p2pkh address:
    return dogecoin_tx_add_address_out(tx->transaction, chain, (int64_t)koinu, destinationaddress);
}

/**
 * @brief This function is for internal use and constructs an extra
 * output which returns the change back to the sender so that all of
 * the funds from inputs are spent in the current transaction.  
 * 
 * @param txindex The transaction which needs the output for returning change.
 * @param public_key The address of the sender for returning the change.
 * @param subtractedfee The amount to set aside for the mining fee.
 * @param amount The remaining funds after outputs have been subtracted from the inputs.
 * 
 * @return 1 if the additional output was created successfully, 0 otherwise.
 */
static int make_change(int txindex, char* public_key, uint64_t subtractedfee, uint64_t amount) {
    if (amount==subtractedfee) return false; // utxos already fully spent, no change needed
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    // determine intended network by checking address prefix:
    const dogecoin_chainparams* chain = (public_key[0] == 'D') ? &dogecoin_chainparams_main : &dogecoin_chainparams_test;

    // calculate total minus fees
    uint64_t total_change_back = amount - subtractedfee;

    return dogecoin_tx_add_address_out(tx->transaction, chain, total_change_back, public_key);
}

/**
 * @brief This function 'closes the inputs' by returning change to the recipient
 * after the total amount and desired fee is confirmed.
 * 
 * @param txindex The index of the working transaction to finalize.
 * @param destinationaddress The address where the funds are being sent.
 * @param subtractedfee The amount to set aside as a fee to the miner.
 * @param out_dogeamount_for_verification An echo of the total amount to send.
 * @param changeaddress The address of the sender to receive the change.
 * 
 * @return The hex of the finalized transaction.
 */
char* finalize_transaction(int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* changeaddress) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    // determine intended network by checking address prefix:
    int is_testnet = chain_from_b58_prefix_bool(destinationaddress);

    uint64_t subtractedfee_koinu = coins_to_koinu_str(subtractedfee);
    uint64_t out_koinu_for_verification = coins_to_koinu_str(out_dogeamount_for_verification);

    // calculate total minus desired fees
    uint64_t total = (uint64_t)out_koinu_for_verification - (uint64_t)subtractedfee_koinu, tx_out_total = 0;

    int i, p2pkh_count = 0, length = (int)tx->transaction->vout->len;

    // iterate through transaction output values while adding each one to tx_out_total:
    for (i = 0; i < length; i++) {
        dogecoin_tx_out* tx_out_tmp = vector_idx(tx->transaction->vout, i);
        tx_out_total += tx_out_tmp->value;
        char p2pkh[36]; //mlumin: this was originally 17, caused problems if < 25.  p2pkh len is 24-36.
        //MLUMIN:MSVC
        dogecoin_mem_zero(p2pkh, sizeof(p2pkh));
        p2pkh_count = dogecoin_script_hash_to_p2pkh(tx_out_tmp, (char *)p2pkh, is_testnet);
        if (i == length - 1 && changeaddress) {
            // manually make change and send back to our public key address
            if (make_change(txindex, changeaddress, subtractedfee_koinu, out_koinu_for_verification - tx_out_total)) {
                p2pkh_count += 1;
                tx_out_tmp = vector_idx(tx->transaction->vout, tx->transaction->vout->len - 1);
                tx_out_total += tx_out_tmp->value;
            }
            break;
        }
    }

    if (p2pkh_count < 1) {
        printf("p2pkh address not found from any output script hash!\n");
        return false;
    }

    // pass in transaction obect, network paramters, amount of dogecoin to send to address and finally p2pkh address:
    return tx_out_total == total ? get_raw_transaction(txindex) : false;
}

/**
 * @brief This function takes an index of a working transaction and returns
 * the hex representation of it.
 * 
 * @param txindex The index of the working transaction.
 * 
 * @return The hex representation of the transaction.
 */
char* get_raw_transaction(int txindex) {
    // find working transaction by index and pass to function local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    // new allocated cstring to store hexadeicmal buffer string:
    cstring* serialized_transaction = cstr_new_sz(1024);

    // serialize transaction object to new cstring:
    dogecoin_tx_serialize(serialized_transaction, tx->transaction);

    char* hexadecimal_buffer = utils_uint8_to_hex((unsigned char*)serialized_transaction->str, serialized_transaction->len);

    cstr_free(serialized_transaction, true);

    return hexadecimal_buffer;
}

/**
 * @brief This function removes the specified working transaction
 * from the hash table.
 * 
 * @param txindex The index of the working transaction to remove.
 * 
 * @return Nothing.
 */
void clear_transaction(int txindex) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);
    // remove from hashmap
    remove_transaction(tx);
}

/**
 * @brief This function signs the specified input of a working transaction,
 * according to the signing parameters specified.
 * 
 * @param inputindex The index of the current transaction input to sign.
 * @param incomingrawtx The hex representation of the transaction to sign.
 * @param scripthex The hex representation of the public key script.
 * @param sighashtype The type of signature hash to perform.
 * @param privkey The private key used to sign the transaction input.
 * 
 * @return 1 if the raw transaction was signed successfully, 0 otherwise.
 */
int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, char* privkey) {
    if(!incomingrawtx || !scripthex) return false;

    if (strlen(incomingrawtx) > 1024*100) { //don't accept tx larger then 100kb
        printf("tx too large (max 100kb)\n");
        return false;
    }

    const dogecoin_chainparams* chain = (privkey[0] == 'c') ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    uint8_t* data_bin = dogecoin_malloc(strlen(incomingrawtx) / 2);
    size_t outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(incomingrawtx, data_bin, strlen(incomingrawtx), &outlength);

    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex\n");
        return false;
    }
    // free byte array
    dogecoin_free(data_bin);

    // if utxo input doesn't exist abort attempt to sign message
    if ((size_t)inputindex >= txtmp->vin->len) {
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("input index out of range");
        return false;
    }

    // initialize byte array with length equal to account for byte size 
    uint8_t* script_data = dogecoin_uint8_vla(strlen(scripthex));
    // convert hex string to byte array
    utils_hex_to_bin(scripthex, script_data, strlen(scripthex), &outlength);
    cstring* script = cstr_new_buf(script_data, outlength);

    uint256 sighash;
    dogecoin_mem_zero(sighash, sizeof(sighash));
    free(script_data);

    dogecoin_tx_sighash(txtmp, script, inputindex, sighashtype, sighash);

    char *hex = utils_uint8_to_hex(sighash, 32);
    utils_reverse_hex(hex, 64);

    debug_print("script: %s\n", scripthex);
    debug_print("script-type: %s\n", dogecoin_tx_out_type_to_str(dogecoin_script_classify(script, NULL)));
    debug_print("inputindex: %d\n", inputindex);
    debug_print("sighashtype: %d\n", sighashtype);
    debug_print("hash: %s\n", hex);
    // sign
    dogecoin_bool sign = false;
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    if (dogecoin_privkey_decode_wif(privkey, chain, &key)) {
        sign = true;
    } else {
        if (privkey) {
            if (strlen(privkey) > 50) {
                dogecoin_tx_free(txtmp);
                cstr_free(script, true);
                return false;
            }
        } else {
            return false;
        }
    }
    if (sign) {
        uint8_t sigcompact[64] = {0};
        size_t sigderlen = 74 + 1; //&hashtype
        uint8_t sigder_plus_hashtype[75] = {0};
        enum dogecoin_tx_sign_result res = dogecoin_tx_sign_input(txtmp, script, &key, inputindex, sighashtype, sigcompact, sigder_plus_hashtype, &sigderlen);
        cstr_free(script, true);

        if (res != DOGECOIN_SIGN_OK) return false;

        char sigcompacthex[64*2+1] = {0};
        utils_bin_to_hex((unsigned char *)sigcompact, 64, sigcompacthex);

        char sigderhex[74*2+2+1]; //74 der, 2 hashtype, 1 nullbyte
        dogecoin_mem_zero(sigderhex, sizeof(sigderhex));
        utils_bin_to_hex((unsigned char *)sigder_plus_hashtype, sigderlen, sigderhex);

        printf("\nsignature created:\nsignature compact: %s\n", sigcompacthex);
        printf("signature DER (+hashtype): %s\n", sigderhex);

        cstring* signed_tx = cstr_new_sz(1024);
        dogecoin_tx_serialize(signed_tx, txtmp);

        char* signed_tx_hex = dogecoin_char_vla(signed_tx->len * 2 + 1);
        utils_bin_to_hex((unsigned char *)signed_tx->str, signed_tx->len, signed_tx_hex);
        memcpy(incomingrawtx, signed_tx_hex, strlen(signed_tx_hex));
        printf("signed TX: %s\n", incomingrawtx);
        cstr_free(signed_tx, true);
        dogecoin_tx_free(txtmp);
        free(signed_tx_hex);
    }
    return true;
}

/**
 * @brief This function is for internal use and saves the result of
 * sign_raw_transaction to a working transaction in the hash table.
 * 
 * @param txindex The index where the signed transaction will be saved.
 * @param inputindex The index of the current transaction input to sign.
 * @param incomingrawtx The hex representation of the transaction to sign.
 * @param scripthex The hex representation of the public key script.
 * @param sighashtype The type of signature hash to perform.
 * @param privkey The private key used to sign the transaction input.
 * 
 * @return 1 if the transaction was signed successfully, 0 otherwise.
 */
int sign_indexed_raw_transaction(int txindex, int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, char* privkey) {
    if (!txindex) return false;
    if (!sign_raw_transaction(inputindex, incomingrawtx, scripthex, sighashtype, privkey)) {
        printf("error signing raw transaction\n");
        return false;
    }
    if (!save_raw_transaction(txindex, incomingrawtx)) {
        printf("error saving transaction!\n");
        return false;
    }
    return true;
}

/**
 * @brief This function signs all of the inputs in the specified working
 * transaction using the provided script pubkey and private key.
 * 
 * @param txindex The index of the working transaction to sign.
 * @param script_pubkey The hex representation of the public key script.
 * @param privkey The private key used to sign the transaction input.
 * 
 * @return 1 if the transaction was signed successfully, 0 otherwise.
 */
int sign_transaction(int txindex, char* script_pubkey, char* privkey) {
    char* raw_hexadecimal_transaction = get_raw_transaction(txindex);
    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    uint8_t* data_bin = dogecoin_malloc(strlen(raw_hexadecimal_transaction) / 2);
    size_t outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(raw_hexadecimal_transaction, data_bin, strlen(raw_hexadecimal_transaction), &outlength);
    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex\n");
        return false;
    }
    // free byte array
    dogecoin_free(data_bin);
    size_t i = 0, len = txtmp->vin->len;
    for (; i < len; i++) {
        if (!sign_raw_transaction(i, raw_hexadecimal_transaction, script_pubkey, 1, privkey)) {
            printf("error signing raw transaction\n");
            return false;
        }
    }
    save_raw_transaction(txindex, raw_hexadecimal_transaction);
    dogecoin_tx_free(txtmp);
    return true;
}

/**
 * @brief This function stores a raw transaction to the next available
 * working transaction in the hash table.
 * 
 * @param incomingrawtx The hex of the raw transaction
 * 
 * @return The index of the new working transaction if stored successfully, 0 otherwise.
 */
int store_raw_transaction(char* incomingrawtx) {
    if (strlen(incomingrawtx) > 1024*100) { //don't accept tx larger then 100kb
        printf("tx too large (max 100kb)\n");
        return false;
    }

    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    int txindex = start_transaction();
    working_transaction* tx_raw = find_transaction(txindex);
    uint8_t* data_bin = dogecoin_malloc(strlen(incomingrawtx));
    size_t outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(incomingrawtx, data_bin, strlen(incomingrawtx), &outlength);
    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex");
        return false;
    }
    // free byte array
    dogecoin_free(data_bin);
    dogecoin_tx_copy(tx_raw->transaction, txtmp);
    dogecoin_tx_free(txtmp);
    return txindex;
}
