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

#include <dogecoin/crypto/base58.h>
#include <dogecoin/tx.h>
#include <dogecoin/transaction.h>
#include <dogecoin/utils.h>

// instantiates a new transaction
working_transaction* new_transaction() {
    working_transaction* working_tx = (struct working_transaction*)dogecoin_calloc(1, sizeof *working_tx);
    working_tx->transaction = dogecoin_tx_new();
    working_tx->idx = HASH_COUNT(transactions) + 1;
    return working_tx;
}

void add_transaction(working_transaction *working_tx) {
    working_transaction *tx;
    HASH_FIND_INT(transactions, &working_tx->idx, tx);
    if (tx == NULL) {
        HASH_ADD_INT(transactions, idx, working_tx);
    } else {
        HASH_REPLACE_INT(transactions, idx, tx, working_tx);
    }
    dogecoin_free(tx);
}

working_transaction* find_transaction(int idx) {
    working_transaction *working_tx;
    HASH_FIND_INT(transactions, &idx, working_tx);
    return working_tx;
}

void remove_transaction(working_transaction *working_tx) {
    HASH_DEL(transactions, working_tx);
    dogecoin_free(working_tx);
}

void remove_all() {
    struct working_transaction *current_tx;
    struct working_transaction *tmp;

    HASH_ITER(hh, transactions, current_tx, tmp) {
        HASH_DEL(transactions, current_tx); /* delete it (transactions advances to next) */
        dogecoin_tx_free(current_tx->transaction);
        free(current_tx);
    }
}

void print_transactions()
{
    struct working_transaction *s;

    for (s = transactions; s != NULL; s = (struct working_transaction*)(s->hh.next)) {
        printf("transaction id %d: hash \n", s->idx);
    }
}

void count_transactions() {
    int temp = HASH_COUNT(transactions);
    printf("there are %d transactions\n", temp);
}

int by_id(const struct working_transaction *a, const struct working_transaction *b)
{
    return (a->idx - b->idx);
}

const char *getl(const char *prompt)
{
    static char buf[1024];
    char *p;
    printf("%s? ", prompt); fflush(stdout);
    p = fgets(buf, sizeof(buf), stdin);
    if (p == NULL || (p = strchr(buf, '\n')) == NULL) {
        puts("Invalid input!");
        exit(EXIT_FAILURE);
    }
    *p = '\0';
    return buf;
}

// #returns an index of a transaction to build in memory.  (1, 2, etc) ..
int start_transaction() {
    working_transaction* working_tx = new_transaction();
    int index = working_tx->idx;
    add_transaction(working_tx);
    return index;
}

// #returns 1 if success.
int add_utxo(int txindex, char* hex_utxo_txid, int vout) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    int flag = tx->transaction->vin->len;

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
    return flag + 1 == (int)tx->transaction->vin->len;
}

int add_output(int txindex, char* destinationaddress, uint64_t amount) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);
    // guard against null pointer exceptions
    if (tx == NULL) {
        printf("add_output failed! %d\n", tx);
        return false;
    }
    // determine intended network by checking address prefix:
    const dogecoin_chainparams* chain = (destinationaddress[0] == 'D') ? &dogecoin_chainparams_main : &dogecoin_chainparams_test;

    amount = coins_to_koinu(amount);
    printf("chainparams: %s\n", chain->chainname);
    printf("amount: %d\n", amount);
    printf("tx->transactions: %d\n", tx->transaction->vout->len);
    printf("tx->transactions: %d\n", tx->idx);
    printf("tx->add: %s\n", destinationaddress);
    // calculate total minus fees
    // pass in transaction obect, network paramters, amount of dogecoin to send to address and finally p2pkh address:
    int res = dogecoin_tx_add_address_out(tx->transaction, chain, (int64_t)amount, destinationaddress);
    if (!res) { 
        printf("dogecoin_tx_add_address failed!\n");
        printf("tx->transactions: %d\n", tx->transaction->vout->len);
        printf("tx->transactions: %d\n", tx->idx);
        return false;
    } else return res;
}

int make_change(int txindex, char* public_key_hex, float subtractedfee, uint64_t amount) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    size_t sizeout = 33; // public hexadecimal keys are 66 characters long (divided by 2 for byte size)

    // generate pubkey for signing tx
    dogecoin_pubkey pubkeytx;

    // initalize public key object
    dogecoin_pubkey_init(&pubkeytx);
    pubkeytx.compressed = true;

    // convert our public key hex to byte array:
    uint8_t* pubkeydat = utils_hex_to_uint8(public_key_hex);

    // copy byte array pubkeydat to dogecoin_pubkey.pubkey:
    memcpy(&pubkeytx.pubkey, pubkeydat, sizeout);

    // calculate total minus fees
    uint64_t total_change_back = (uint64_t)amount - (uint64_t)subtractedfee;

    return dogecoin_tx_add_p2pkh_out(tx->transaction, total_change_back, &pubkeytx);
}

// 'closes the inputs', specifies the recipient, specifies the amnt-to-subtract-as-fee, and returns the raw tx..
// out_dogeamount == just an echoback of the total amount specified in the addutxos for verification
char* finalize_transaction(int txindex, char* destinationaddress, float subtractedfee, uint64_t out_dogeamount_for_verification, char* public_key_hex) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    // determine intended network by checking address prefix:
    int is_testnet = chain_from_b58_prefix_bool(destinationaddress);

    subtractedfee = coins_to_koinu(subtractedfee);
    out_dogeamount_for_verification = coins_to_koinu(out_dogeamount_for_verification);

    // calculate total minus desired fees
    uint64_t total = (uint64_t)out_dogeamount_for_verification - (uint64_t)subtractedfee; // - subtractedfee;

    int i, p2pkh_count = 0;
    uint64_t tx_out_total = 0;

    // iterate through transaction output values while adding each one to tx_out_total:
    for (i = 0; i < (int)tx->transaction->vout->len; i++) {
        dogecoin_tx_out* tx_out_tmp = vector_idx(tx->transaction->vout, i);
        tx_out_total += tx_out_tmp->value;
        size_t len = 17;
        char* p2pkh[len];
        dogecoin_mem_zero(p2pkh, sizeof(p2pkh));
        p2pkh_count = dogecoin_script_hash_to_p2pkh(tx_out_tmp, (char *)p2pkh, is_testnet);
        if (i == (int)tx->transaction->vout->len - 1 && public_key_hex) {
            // manually make change and send back to our public key address
            p2pkh_count += make_change(txindex, public_key_hex, subtractedfee, out_dogeamount_for_verification - tx_out_total);
            tx_out_tmp = vector_idx(tx->transaction->vout, tx->transaction->vout->len - 1);
            tx_out_total += tx_out_tmp->value;
            break;
        }
    }

    if (p2pkh_count != 2) {
        printf("p2pkh address not found from any output script hash!\n");
        return false;
    }

    // pass in transaction obect, network paramters, amount of dogecoin to send to address and finally p2pkh address:
    return tx_out_total == total ? get_raw_transaction(txindex) : false;
}

// returns 0 if not closed, returns rawtx again if closed/created.
char* get_raw_transaction(int txindex) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    // new allocated cstring to store hexadeicmal buffer string:
    cstring* serialized_transaction = cstr_new_sz(1024);

    // serialize transaction object to new cstring and ignore witness:
    dogecoin_tx_serialize(serialized_transaction, tx->transaction, false);

    char* hexadecimal_buffer = utils_uint8_to_hex((unsigned char*)serialized_transaction->str, serialized_transaction->len);

    cstr_free(serialized_transaction, true);

    return hexadecimal_buffer;
}

// clears a tx in memory. (overwrites)
void clear_transaction(int txindex) {
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction(txindex);

    // guard against null pointer exceptions
    if (!tx->transaction) return;

    // free transaction input if they exist
    if (tx->transaction->vin) vector_free(tx->transaction->vin, true);

    // free transaction outputs if they exist
    if (tx->transaction->vout) vector_free(tx->transaction->vout, true);
    
    // free dogecoin_tx
    dogecoin_tx_free(tx->transaction);

    // remote from hashmap
    remove_transaction(tx);
}

// sign a given inputted transaction with a given private key, and return a hex signed transaction.
// we may want to add such things to 'advanced' section:
// locktime, possibilities for multiple outputs, data, sequence.
int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, int amount, char* privkey) {
    if(!incomingrawtx && !scripthex) return false;

    if (strlen(incomingrawtx) > 1024*100) { //don't accept tx larger then 100kb
        printf("tx too large (max 100kb)\n");
        return false;
    }

    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    uint8_t* data_bin = dogecoin_malloc(strlen(incomingrawtx) / 2);
    int outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(incomingrawtx, data_bin, strlen(incomingrawtx), &outlength);
    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL, true)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex");
        return false;
    }
    // free byte array
    dogecoin_free(data_bin);

    // if utxo input doesn't exist abort attempt to sign message
    if ((size_t)inputindex >= txtmp->vin->len) {
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("inputindex out of range");
        return false;
    }

    // initialize byte array with length equal to account for byte size 
    uint8_t script_data[strlen(scripthex) / 2];
    // convert hex string to byte array
    utils_hex_to_bin(scripthex, script_data, strlen(scripthex), &outlength);
    cstring* script = cstr_new_buf(script_data, outlength);

    uint256 sighash;
    dogecoin_mem_zero(sighash, sizeof(sighash));
    
    amount = coins_to_koinu(amount);

    dogecoin_tx_sighash(txtmp, script, inputindex, sighashtype, amount, SIGVERSION_BASE, sighash);

    char *hex = utils_uint8_to_hex(sighash, 32);
    utils_reverse_hex(hex, 64);

    enum dogecoin_tx_out_type type = dogecoin_script_classify(script, NULL);
    debug_print("script: %s\n", scripthex);
    debug_print("script-type: %s\n", dogecoin_tx_out_type_to_str(type));
    debug_print("inputindex: %d\n", inputindex);
    debug_print("sighashtype: %d\n", sighashtype);
    debug_print("hash: %s\n", hex);
    // sign
    dogecoin_bool sign = false;
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    if (dogecoin_privkey_decode_wif(privkey, &dogecoin_chainparams_test, &key)) {
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
        int sigderlen = 74+1; //&hashtype
        uint8_t sigder_plus_hashtype[75] = {0};
        enum dogecoin_tx_sign_result res = dogecoin_tx_sign_input(txtmp, script, amount, &key, inputindex, sighashtype, sigcompact, sigder_plus_hashtype, &sigderlen);
        cstr_free(script, true);

        if (res != DOGECOIN_SIGN_OK) return false;

        char sigcompacthex[64*2+1] = {0};
        utils_bin_to_hex((unsigned char *)sigcompact, 64, sigcompacthex);

        char sigderhex[74*2+2+1]; //74 der, 2 hashtype, 1 nullbyte
        dogecoin_mem_zero(sigderhex, sizeof(sigderhex));
        utils_bin_to_hex((unsigned char *)sigder_plus_hashtype, sigderlen, sigderhex);

        debug_print("\nSignature created:\nsignature compact: %s\n", sigcompacthex);
        debug_print("signature DER (+hashtype): %s\n", sigderhex);

        cstring* signed_tx = cstr_new_sz(1024);
        dogecoin_tx_serialize(signed_tx, txtmp, false);

        char signed_tx_hex[signed_tx->len*2+1];
        utils_bin_to_hex((unsigned char *)signed_tx->str, signed_tx->len, signed_tx_hex);
        memcpy(incomingrawtx, signed_tx_hex, sizeof(signed_tx_hex));
        debug_print("signed TX: %s\n", incomingrawtx);
        cstr_free(signed_tx, true);
        dogecoin_tx_free(txtmp);
    }
    return true;
}