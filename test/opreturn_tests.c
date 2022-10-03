/**********************************************************************
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "utest.h"

#include <dogecoin/address.h>
#include <dogecoin/buffer.h>
#include <dogecoin/key.h>
#include <dogecoin/transaction.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>
#include <dogecoin/rmd160.h>

void test_op_return() {
    // intended message included in OP_RETURN:
    char* msg = "RADIODOGETX1BYMICHI&BLUEZRMADEWLIBDOGECOINTX@3.5MHZ&BCASTVIASTARLINK";

    // expected intended message in hexadecimal format prepended with length of script:
    char* expected_hexmsg = "44524144494F444F474554583142594D4943484926424C55455A524D414445574C4942444F4745434F494E545840332E354D485A264243415354564941535441524C494E4B";

    // init new working transaction
    working_transaction* tx = new_transaction();

    // add to memory
    add_transaction(tx);
    
    char* msg_hex[69];

    // convert message text to hexadecimal:
    text_to_hex(msg, (char*)msg_hex);

    size_t length = (strlen((char*)msg_hex) / 2) + 1; // 69
    
    char decimal[32];
    sprintf(decimal, "%x", length - 1);
    prepend((char*)msg_hex, decimal);

    u_assert_int_eq(memcmp(msg_hex, expected_hexmsg, strlen(expected_hexmsg)), 0);

    size_t outlen;
    uint8_t* script_data=dogecoin_uint8_vla(strlen((char*)msg_hex));
    utils_hex_to_bin((char*)msg_hex, script_data, strlen((char*)msg_hex), &outlen);

    /* creating a new transaction output. */
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();
    
    /* creating a new string of size 1024. */
    tx_out->script_pubkey = cstr_new_sz(1024);
    
    /* this code is building a scriptPubKey for the output. */
    cstr_resize(tx_out->script_pubkey, 0); //clear script
    dogecoin_script_append_op(tx_out->script_pubkey, OP_RETURN);
    tx_out->value = 0;
    cstr_append_buf(tx_out->script_pubkey, script_data, outlen);
    free(script_data);
    /* adding a new output to the transaction. */
    vector_add(tx->transaction->vout, tx_out);
    remove_all();
}
