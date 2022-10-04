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
#include <dogecoin/koinu.h>
#include <dogecoin/transaction.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

void test_transaction()
{
    // internal keys
    char* private_key_wif = "ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy";
    char* public_key_hex = "031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075b";
    char* internal_p2pkh_address = "noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy";
    char* utxo_scriptpubkey = "76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac";

    // external keys
    char* external_p2pkh_address = "nbGfXLskPh7eM1iG5zz5EfDkkNTo9TRmde";

    // our raw hexadecimal transaction step by step
    const char* unsigned_single_utxo_hexadecimal_transaction = "0100000001746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffff0000000000";
    const char* unsigned_double_utxo_hexadecimal_transaction = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff0000000000";
    const char* unsigned_double_utxo_single_output_hexadecimal_transaction = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff010065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac00000000";
    const char* unsigned_hexadecimal_transaction = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b40100000000ffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000";
    const char* expected_single_input_signed_transaction = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4010000006b48304502210090bddac300243d16dca5e38ab6c80d5848e0d710d77702223bacd6682654f6fe02201b5c2e8b1143d8a807d604dc18068b4278facce561c302b0c66a4f2a5a4aa66f0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b11420100000000ffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000";
    const char* expected_signed_raw_hexadecimal_transaction = "0100000002746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4010000006b48304502210090bddac300243d16dca5e38ab6c80d5848e0d710d77702223bacd6682654f6fe02201b5c2e8b1143d8a807d604dc18068b4278facce561c302b0c66a4f2a5a4aa66f0121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffffe216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142010000006a47304402200e19c2a66846109aaae4d29376040fc4f7af1a519156fe8da543dc6f03bb50a102203a27495aba9eead2f154e44c25b52ccbbedef084f0caf1deedaca87efd77e4e70121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffff020065cd1d000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac30b4b529000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac00000000";
    const char* expected_single_utxo_signed_transaction = "0100000001e216461c60c629333ac6b40d29b5b0b6d0ce241aea5903cf4329fc65dc3b1142010000006b483045022100e22ad3aba33c15a6f24f68c059369c9d6d4e8bc9a76af5ef589e483fa0c14ce202206cfacacf81f97766a3451df6bd073482fbeba379d441120ce3d13ee4cf154ec10121031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075bffffffff019810993b000000001976a9144da2f8202789567d402f7f717c01d98837e4325488ac00000000";

    // we will begin by deserializing the 2 raw hexadecimal transactions from dogecoin core as assigned below (raw_hexadecimal_transaction_from_tx_worth_2_dogecoin, raw_hexadecimal_transaction_from_tx_worth_10_dogecoin) which contain the utxos we intended to spend. the assertions are interspersed within each corresponding JSON response from dogecoin core to make cross referencing the validity of our assertions containing the deserialized data easier. this is intended to provide proof that the data we used to create our first valid and accepted dogecoin testnet transaction with the variables in this test is legitimate:

    char* utxo_txid_from_tx_worth_2_dogecoin = "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074";
    const char* raw_hexadecimal_transaction_from_tx_worth_2_dogecoin = "0100000001e298a076ea26489c4ea60b34cb79a386a16aeef17cd646e9bdc3e4486b4abadf0100000068453042021e623cf9ebc2e2736343827c2dda22a85c41347d5fe17e4a1dfa57ebb3eb0e022075baa343944021a24a8a99c5a90b3af2fd47b92bd1e1fe0f7dc1a5cb95086df0012102ac1447c59fd7b96cee31e4a22ec051cf393d76bc3f275bcd5aa7580377d32e14feffffff02208d360b890000001976a914a4a942c99c94522a025b2b8cfd2edd149fb4995488ac00c2eb0b000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac96fe3700";
    int utxo_previous_output_index_from_tx_worth_2_dogecoin = 1;

    dogecoin_tx* tx_worth_2 = dogecoin_tx_new();

    uint8_t* data_bin_2 = dogecoin_malloc(strlen(raw_hexadecimal_transaction_from_tx_worth_2_dogecoin) / 2 + 1);
    size_t outlength_2 = 0;
    // convert raw_hexadecimal_transaction_from_tx_worth_2_dogecoin to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(raw_hexadecimal_transaction_from_tx_worth_2_dogecoin, data_bin_2, strlen(raw_hexadecimal_transaction_from_tx_worth_2_dogecoin), &outlength_2);

    if (!dogecoin_tx_deserialize(data_bin_2, outlength_2, tx_worth_2, NULL)) {
        // free dogecoin_tx
        printf("deserializing tx_worth_2 failed\n");
        dogecoin_tx_free(tx_worth_2);
    }
    // free byte array
    dogecoin_free(data_bin_2);

    // below is the JSON response from dogecoin cores RPC method `getrawtransaction` but by appending true to the end we receive the data in JSON format as oppose to raw hexadecimal format:

    // > dogecoin-cli getrawtransaction b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074 true
    // {
    //   "hex": "0100000001e298a076ea26489c4ea60b34cb79a386a16aeef17cd646e9bdc3e4486b4abadf0100000068453042021e623cf9ebc2e2736343827c2dda22a85c41347d5fe17e4a1dfa57ebb3eb0e022075baa343944021a24a8a99c5a90b3af2fd47b92bd1e1fe0f7dc1a5cb95086df0012102ac1447c59fd7b96cee31e4a22ec051cf393d76bc3f275bcd5aa7580377d32e14feffffff02208d360b890000001976a914a4a942c99c94522a025b2b8cfd2edd149fb4995488ac00c2eb0b000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac96fe3700",
    u_assert_str_eq("0100000001e298a076ea26489c4ea60b34cb79a386a16aeef17cd646e9bdc3e4486b4abadf0100000068453042021e623cf9ebc2e2736343827c2dda22a85c41347d5fe17e4a1dfa57ebb3eb0e022075baa343944021a24a8a99c5a90b3af2fd47b92bd1e1fe0f7dc1a5cb95086df0012102ac1447c59fd7b96cee31e4a22ec051cf393d76bc3f275bcd5aa7580377d32e14feffffff02208d360b890000001976a914a4a942c99c94522a025b2b8cfd2edd149fb4995488ac00c2eb0b000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac96fe3700", raw_hexadecimal_transaction_from_tx_worth_2_dogecoin);
    //   "txid": "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074",
        u_assert_str_eq("b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074", utxo_txid_from_tx_worth_2_dogecoin);
    //   "hash": "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074",
    //   "size": 223,
    //   "vsize": 223,
    //   "version": 1,
        u_assert_int_eq(1, tx_worth_2->version);
    //   "locktime": 3669654,
        u_assert_int_eq(3669654, tx_worth_2->locktime);
        dogecoin_tx_in* tx_in_2 = vector_idx(tx_worth_2->vin, 0);
    //   "vin": [
    //     {
            char* reversed_txid = utils_uint8_to_hex(tx_in_2->prevout.hash, sizeof(tx_in_2->prevout.hash));
            utils_reverse_hex(reversed_txid, 64);
    //       "txid": "dfba4a6b48e4c3bde946d67cf1ee6aa186a379cb340ba64e9c4826ea76a098e2",
            u_assert_str_eq("dfba4a6b48e4c3bde946d67cf1ee6aa186a379cb340ba64e9c4826ea76a098e2", reversed_txid);
    //       "vout": 1,
            u_assert_int_eq(1, tx_in_2->prevout.n);
    //       "scriptSig": {
    //         "asm": "3042021e623cf9ebc2e2736343827c2dda22a85c41347d5fe17e4a1dfa57ebb3eb0e022075baa343944021a24a8a99c5a90b3af2fd47b92bd1e1fe0f7dc1a5cb95086df0[ALL] 02ac1447c59fd7b96cee31e4a22ec051cf393d76bc3f275bcd5aa7580377d32e14",
    //         "hex": "453042021e623cf9ebc2e2736343827c2dda22a85c41347d5fe17e4a1dfa57ebb3eb0e022075baa343944021a24a8a99c5a90b3af2fd47b92bd1e1fe0f7dc1a5cb95086df0012102ac1447c59fd7b96cee31e4a22ec051cf393d76bc3f275bcd5aa7580377d32e14"
                u_assert_str_eq("453042021e623cf9ebc2e2736343827c2dda22a85c41347d5fe17e4a1dfa57ebb3eb0e022075baa343944021a24a8a99c5a90b3af2fd47b92bd1e1fe0f7dc1a5cb95086df0012102ac1447c59fd7b96cee31e4a22ec051cf393d76bc3f275bcd5aa7580377d32e14", utils_uint8_to_hex((const uint8_t *)tx_in_2->script_sig->str, tx_in_2->script_sig->len));
    //       },
    //       "sequence": 4294967294
            uint8_t* sequence = (uint8_t *)4294967294;
            u_assert_str_eq(utils_uint8_to_hex((const uint8_t *)&sequence, strlen((const char *)(const uint8_t *)&sequence)), utils_uint8_to_hex((const uint8_t *)&tx_in_2->sequence, strlen((const char *)(const uint8_t *)&tx_in_2->sequence)));
    //     }
    //   ],
    //   "vout": [
    //     {
    //       "value": 5885.98644000,
            dogecoin_tx_out* tx_out = vector_idx(tx_worth_2->vout, 0);
            char* dogecoin[21];
            dogecoin_mem_zero(dogecoin, 21);
            koinu_to_coins_str(tx_out->value, (char*)dogecoin);
            u_assert_str_eq((char*)dogecoin, "5885.98644000");
    //       "n": 0,
    //       "scriptPubKey": {
    //         "asm": "OP_DUP OP_HASH160 a4a942c99c94522a025b2b8cfd2edd149fb49954 OP_EQUALVERIFY OP_CHECKSIG",
    //         "hex": "76a914a4a942c99c94522a025b2b8cfd2edd149fb4995488ac",
                u_assert_str_eq("76a914a4a942c99c94522a025b2b8cfd2edd149fb4995488ac", utils_uint8_to_hex((const uint8_t *)tx_out->script_pubkey->str, tx_out->script_pubkey->len));
    //         "reqSigs": 1,
    //         "type": "pubkeyhash",
    //         "addresses": [
    //           "njCorBdd1TZxHzDGQgnRqA8UTLforArtQn"
    //         ]
    //       }
    //     },
    //     {
    //       "value": 2.00000000,
    //       "n": 1,
            tx_out = vector_idx(tx_worth_2->vout, 1);
            koinu_to_coins_str(tx_out->value, (char*)dogecoin);
            u_assert_str_eq((char*)dogecoin, "2.00000000");
    //       "scriptPubKey": {
    //         "asm": "OP_DUP OP_HASH160 d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c OP_EQUALVERIFY OP_CHECKSIG",
    //         "hex": "76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac",
                u_assert_str_eq("76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac", utxo_scriptpubkey);
                u_assert_str_eq("76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac", utils_uint8_to_hex((const uint8_t *)tx_out->script_pubkey->str, tx_out->script_pubkey->len));
    //         "reqSigs": 1,
    //         "type": "pubkeyhash",
    //         "addresses": [
    //           "noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy"
    //         ]
    //       }
    //     }
    //   ],
        dogecoin_tx_free(tx_worth_2);
    //   "blockhash": "69960ffcd0194ee7578c9ad49d89aef1eb2074bbbceb201344c386462d53344f",
    //   "confirmations": 25192,
    //   "time": 1647548015,
    //   "blocktime": 1647548015
    // }
    // ---------------------------------------------------------------- end 1st transaction data validation----------------------------------------------------------------


    char* utxo_txid_from_tx_worth_10_dogecoin = "42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2";
    const char* raw_hexadecimal_transaction_from_tx_worth_10_dogecoin = "01000000011b557be8ca232244085641b91d6a587ebaf227d7dd1db4c578b3a3878ac2c676010000006a4730440220739ee157e98f60eda768fb473168fb6b25878572e9aaa9d2593ef1217291558e02206d0da7f862571f6826d5cacea408445b934c1191cde77c46e146ad8b867250d70121024b67a792594a459d525d50dd4d4fb21a792c0241596d522ed627cabf0ed3d4abfeffffff02600c39fab91400001976a9141476c35e582eb198e1a28c455005a70c6869586888ac00ca9a3b000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac95fe3700";
    int utxo_previous_output_index_from_tx_worth_10_dogecoin = 1;

    dogecoin_tx* tx_worth_10 = dogecoin_tx_new();
    uint8_t* data_bin_10 = dogecoin_malloc(strlen(raw_hexadecimal_transaction_from_tx_worth_10_dogecoin));
    size_t outlength_10 = 0;

    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(raw_hexadecimal_transaction_from_tx_worth_10_dogecoin, data_bin_10, strlen(raw_hexadecimal_transaction_from_tx_worth_10_dogecoin), &outlength_10);
    if (!dogecoin_tx_deserialize(data_bin_10, outlength_10, tx_worth_10, NULL)) {
        // free dogecoin_tx
        dogecoin_tx_free(tx_worth_10);
    }
    // free byte array
    dogecoin_free(data_bin_10);

    // dogecoin-cli getrawtransaction 42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2 true
    // {
    //   "hex": "01000000011b557be8ca232244085641b91d6a587ebaf227d7dd1db4c578b3a3878ac2c676010000006a4730440220739ee157e98f60eda768fb473168fb6b25878572e9aaa9d2593ef1217291558e02206d0da7f862571f6826d5cacea408445b934c1191cde77c46e146ad8b867250d70121024b67a792594a459d525d50dd4d4fb21a792c0241596d522ed627cabf0ed3d4abfeffffff02600c39fab91400001976a9141476c35e582eb198e1a28c455005a70c6869586888ac00ca9a3b000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac95fe3700",
    u_assert_str_eq("01000000011b557be8ca232244085641b91d6a587ebaf227d7dd1db4c578b3a3878ac2c676010000006a4730440220739ee157e98f60eda768fb473168fb6b25878572e9aaa9d2593ef1217291558e02206d0da7f862571f6826d5cacea408445b934c1191cde77c46e146ad8b867250d70121024b67a792594a459d525d50dd4d4fb21a792c0241596d522ed627cabf0ed3d4abfeffffff02600c39fab91400001976a9141476c35e582eb198e1a28c455005a70c6869586888ac00ca9a3b000000001976a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac95fe3700", raw_hexadecimal_transaction_from_tx_worth_10_dogecoin);
    //   "txid": "42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2",
        u_assert_str_eq("42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2", utxo_txid_from_tx_worth_10_dogecoin);
    //   "hash": "42113bdc65fc2943cf0359ea1a24ced0b6b0b5290db4c63a3329c6601c4616e2",
    //   "size": 225,
    //   "vsize": 225,
    //   "version": 1,
        u_assert_int_eq(1, tx_worth_10->version);
    //   "locktime": 3669653,
        u_assert_int_eq(3669653, tx_worth_10->locktime);
    //   "vin": [
        dogecoin_tx_in* tx_in_10 = vector_idx(tx_worth_10->vin, 0);
    //     {    
            reversed_txid = utils_uint8_to_hex(tx_in_10->prevout.hash, sizeof(tx_in_10->prevout.hash));
            utils_reverse_hex(reversed_txid, 64);
    //       "txid": "76c6c28a87a3b378c5b41dddd727f2ba7e586a1db9415608442223cae87b551b",
            u_assert_str_eq("76c6c28a87a3b378c5b41dddd727f2ba7e586a1db9415608442223cae87b551b", reversed_txid);
    //       "vout": 1,
            u_assert_int_eq(1, tx_in_10->prevout.n);
    //       "scriptSig": {
    //         "asm": "30440220739ee157e98f60eda768fb473168fb6b25878572e9aaa9d2593ef1217291558e02206d0da7f862571f6826d5cacea408445b934c1191cde77c46e146ad8b867250d7[ALL] 024b67a792594a459d525d50dd4d4fb21a792c0241596d522ed627cabf0ed3d4ab",
    //         "hex": "4730440220739ee157e98f60eda768fb473168fb6b25878572e9aaa9d2593ef1217291558e02206d0da7f862571f6826d5cacea408445b934c1191cde77c46e146ad8b867250d70121024b67a792594a459d525d50dd4d4fb21a792c0241596d522ed627cabf0ed3d4ab"
                u_assert_str_eq("4730440220739ee157e98f60eda768fb473168fb6b25878572e9aaa9d2593ef1217291558e02206d0da7f862571f6826d5cacea408445b934c1191cde77c46e146ad8b867250d70121024b67a792594a459d525d50dd4d4fb21a792c0241596d522ed627cabf0ed3d4ab", utils_uint8_to_hex((const uint8_t *)tx_in_10->script_sig->str, tx_in_10->script_sig->len));
    //       },
    //       "sequence": 4294967294
            sequence = (uint8_t *)4294967294;
            u_assert_str_eq(utils_uint8_to_hex((const uint8_t *)&sequence, strlen((const char *)(const uint8_t *)&sequence)), utils_uint8_to_hex((const uint8_t *)&tx_in_10->sequence, strlen((const char *)(const uint8_t *)&tx_in_10->sequence)));
    //     }
    //   ],
    //   "vout": [
    //     {
    //       "value": 227889.99548000,
    //       "n": 0,
            dogecoin_tx_out* tx_out_10 = vector_idx(tx_worth_10->vout, 0);
            koinu_to_coins_str(tx_out_10->value, (char*)dogecoin);
            u_assert_str_eq((char*)dogecoin, "227889.99548000");
    //       "scriptPubKey": {
    //         "asm": "OP_DUP OP_HASH160 1476c35e582eb198e1a28c455005a70c68695868 OP_EQUALVERIFY OP_CHECKSIG",
    //         "hex": "76a9141476c35e582eb198e1a28c455005a70c6869586888ac",
                u_assert_str_eq("76a9141476c35e582eb198e1a28c455005a70c6869586888ac", utils_uint8_to_hex((const uint8_t *)tx_out_10->script_pubkey->str, tx_out_10->script_pubkey->len));
    //         "reqSigs": 1,
    //         "type": "pubkeyhash",
    //         "addresses": [
    //           "nW4N3v84cSn1eeH5mVTDeqzqrNGvTXNUb7"
    //         ]
    //       }
    //     },
    //     {
    //       "value": 10.00000000,
    //       "n": 1,
            tx_out_10 = vector_idx(tx_worth_10->vout, 1);
            koinu_to_coins_str(tx_out_10->value, (char*)dogecoin);
            u_assert_str_eq((char*)dogecoin, "10.00000000");
    //       "scriptPubKey": {
    //         "asm": "OP_DUP OP_HASH160 d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c OP_EQUALVERIFY OP_CHECKSIG",
    //         "hex": "76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac",
                u_assert_str_eq("76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac", utils_uint8_to_hex((const uint8_t *)tx_out_10->script_pubkey->str, tx_out_10->script_pubkey->len));
    //         "reqSigs": 1,
    //         "type": "pubkeyhash",
    //         "addresses": [
    //           "noxKJyGPugPRN4wqvrwsrtYXuQCk7yQEsy"
    //         ]
    //       }
    //     }
    //   ],
        dogecoin_tx_free(tx_worth_10);
    //   "blockhash": "69960ffcd0194ee7578c9ad49d89aef1eb2074bbbceb201344c386462d53344f",
    //   "confirmations": 25358,
    //   "time": 1647548015,
    //   "blocktime": 1647548015
    // }
    // ---------------------------------------------------------------- end 2nd transaction data validation----------------------------------------------------------------


    // -------------------------------- address validation --------------------------------

    // prove internal p2pkh was derived from private key in wif format which
    // validates libdogecoins addressing functionality as both were generated by dogecoin core:
    u_assert_int_eq(verifyPrivPubKeypair(private_key_wif, internal_p2pkh_address, true), 1);

    // prove internal p2pkh address was derived from public key hex:

    char p2pkh_pubkey_internal[35];
    dogecoin_pubkey pubkeytx;
    dogecoin_pubkey_init(&pubkeytx);
    pubkeytx.compressed = true;

    // convert our public key hex to byte array:
    uint8_t* pubkeydat = utils_hex_to_uint8(public_key_hex);

    // copy byte array pubkeydat to dogecoin_pubkey.pubkey:
    memcpy(pubkeytx.pubkey, pubkeydat, strlen(public_key_hex) / 2);

    // derive p2pkh address from new injected dogecoin_pubkey with known hexadecimal public key:
    dogecoin_pubkey_getaddr_p2pkh(&pubkeytx, &dogecoin_chainparams_test, (char*)p2pkh_pubkey_internal);

    // assert the p2pkh address we just generated matches the one from dogecoin core:
    u_assert_str_eq(internal_p2pkh_address, (char*)p2pkh_pubkey_internal);

    // validate p2pkh we will send 5 dogecoin to:
    u_assert_int_eq(verifyP2pkhAddress(external_p2pkh_address, strlen(external_p2pkh_address)), 1);

    dogecoin_pubkey_cleanse(&pubkeytx);
    // -------------------------------- transaction generation & validation --------------------------------

    // ----------------------------------------------------------------
    // test building transaction with multiple inputs and signing with sign_transaction:
    
    // instantiate a new working_transaction object by calling start_transaction()
    // which passes back index and stores in index variable
    int working_transaction_index = start_transaction();

    // add 1st input worth 2 dogecoin:
    u_assert_int_eq(add_utxo(working_transaction_index, utxo_txid_from_tx_worth_2_dogecoin, utxo_previous_output_index_from_tx_worth_2_dogecoin), 1);

    // add 2nd input worth 10 dogecoin:
    u_assert_int_eq(add_utxo(working_transaction_index, utxo_txid_from_tx_worth_10_dogecoin, utxo_previous_output_index_from_tx_worth_10_dogecoin), 1);

    // add output to transaction which is amount and address we are sending to:
    u_assert_int_eq(add_output(working_transaction_index, external_p2pkh_address, "5"), 1);
    
    // confirm total output value equals total utxo input value minus transaction fee
    // validate external p2pkh address by converting script hash to p2pkh and asserting equal:
    char* raw_hexadecimal_transaction  = finalize_transaction(working_transaction_index, external_p2pkh_address, ".00226", "12.0", internal_p2pkh_address);

    u_assert_int_eq(sign_transaction(working_transaction_index, utxo_scriptpubkey, private_key_wif), 1);
    u_assert_str_eq(raw_hexadecimal_transaction, expected_signed_raw_hexadecimal_transaction);

    // ----------------------------------------------------------------
    // test building transaction with single input and signing with sign_transaction:
    
    // instantiate a new working_transaction object by calling start_transaction()
    // which passes back index and stores in index variable
    working_transaction_index = start_transaction();

    // add 2nd input worth 10 dogecoin:
    u_assert_int_eq(add_utxo(working_transaction_index, utxo_txid_from_tx_worth_10_dogecoin, utxo_previous_output_index_from_tx_worth_10_dogecoin), 1);

    // add output to transaction which is amount and address we are sending to:
    u_assert_int_eq(add_output(working_transaction_index, external_p2pkh_address, "9.99887"), 1);
    
    // confirm total output value equals total utxo input value minus transaction fee
    // validate external p2pkh address by converting script hash to p2pkh and asserting equal:
    raw_hexadecimal_transaction  = finalize_transaction(working_transaction_index, external_p2pkh_address, ".00113", "10.0", internal_p2pkh_address);

    u_assert_int_eq(sign_transaction(working_transaction_index, utxo_scriptpubkey, private_key_wif), 1);
    u_assert_str_eq(raw_hexadecimal_transaction, expected_single_utxo_signed_transaction);

    // ----------------------------------------------------------------
    // test store_raw_transaction:

    int working_transaction_index2 = store_raw_transaction(raw_hexadecimal_transaction);
    u_assert_int_eq(working_transaction_index, working_transaction_index2 - 1);
    u_assert_str_eq(get_raw_transaction(working_transaction_index), get_raw_transaction(working_transaction_index2));
    
    // ----------------------------------------------------------------
    // test clear_transaction:

    clear_transaction(working_transaction_index2);
    u_assert_is_null(get_raw_transaction(working_transaction_index2));

    // ----------------------------------------------------------------
    // test building transaction and signing with sign_raw_transaction:

    // instantiate a new working_transaction object by calling start_transaction()
    // which passes back index and stores in index variable
    working_transaction_index = start_transaction();

    // add 1st input worth 2 dogecoin:
    u_assert_int_eq(add_utxo(working_transaction_index, utxo_txid_from_tx_worth_2_dogecoin, utxo_previous_output_index_from_tx_worth_2_dogecoin), 1);

    // get raw hexadecimal transaction to sign in the next steps
    raw_hexadecimal_transaction = get_raw_transaction(working_transaction_index);

    u_assert_str_eq(unsigned_single_utxo_hexadecimal_transaction, raw_hexadecimal_transaction);

    // add 2nd input worth 10 dogecoin:
    u_assert_int_eq(add_utxo(working_transaction_index, utxo_txid_from_tx_worth_10_dogecoin, utxo_previous_output_index_from_tx_worth_10_dogecoin), 1);

    raw_hexadecimal_transaction = get_raw_transaction(working_transaction_index);
    u_assert_str_eq(raw_hexadecimal_transaction, unsigned_double_utxo_hexadecimal_transaction);

    // add output to transaction which is amount and address we are sending to:
    u_assert_int_eq(add_output(working_transaction_index, external_p2pkh_address, "5"), 1);

    raw_hexadecimal_transaction = get_raw_transaction(working_transaction_index);
    u_assert_str_eq(raw_hexadecimal_transaction, unsigned_double_utxo_single_output_hexadecimal_transaction);
    
    // confirm total output value equals total utxo input value minus transaction fee
    // validate external p2pkh address by converting script hash to p2pkh and asserting equal:
    raw_hexadecimal_transaction = finalize_transaction(working_transaction_index, external_p2pkh_address, ".00226", "12.0", internal_p2pkh_address);

    // assert complete raw hexadecimal transaction is equal to expected unsigned_hexadecimal_transaction
    u_assert_str_eq(raw_hexadecimal_transaction, unsigned_hexadecimal_transaction);

    // sign current working transaction input index 0 of raw tx hex with script pubkey from utxo with sighash type of 1 (SIGHASH_ALL),
    // amount of 2 dogecoin represented as koinu (multiplied by 100 million) and with private key in wif format
    u_assert_int_eq(sign_raw_transaction(0, raw_hexadecimal_transaction, utxo_scriptpubkey, 1, private_key_wif), 1);

    // assert that our hexadecimal buffer (raw_hexadecimal_transaction) is equal to the expected transaction
    // with the first input signed:
    u_assert_str_eq(raw_hexadecimal_transaction, expected_single_input_signed_transaction);
    save_raw_transaction(working_transaction_index, raw_hexadecimal_transaction);
    raw_hexadecimal_transaction = get_raw_transaction(working_transaction_index);

    // sign current working transaction input index 1 of raw tx hex with script pubkey from utxo with sighash type of 1 (SIGHASH_ALL),
    // amount of 10 dogecoin represented as koinu (multiplied by 100 million) and with private key in wif format
    u_assert_int_eq(sign_raw_transaction(1, raw_hexadecimal_transaction, utxo_scriptpubkey, 1, private_key_wif), 1);

    // assert that our hexadecimal bufer (raw_hexadecimal_transaction) is equal to the expected finalized
    // transaction with both inputs signed:
    u_assert_str_eq(raw_hexadecimal_transaction, expected_signed_raw_hexadecimal_transaction);

    // ----------------------------------------------------------------
    // test conversion from p2pkh to script hash

    char* res = dogecoin_p2pkh_to_script_hash(internal_p2pkh_address);
    u_assert_str_eq(res, utxo_scriptpubkey);
    dogecoin_free(res);
    
    res = dogecoin_p2pkh_to_script_hash(external_p2pkh_address);
    u_assert_str_not_eq(res, utxo_scriptpubkey);
    dogecoin_free(res);

    // ----------------------------------------------------------------
    // test conversion from private key (wif) to script hash
    res = dogecoin_private_key_wif_to_script_hash(private_key_wif);

    u_assert_str_eq(res, utxo_scriptpubkey);
    dogecoin_free(res);

    // ----------------------------------------------------------------
    // test remove_all - *not noticeable unless running valgrind ./tests*
    // remove working transaction object from hashmap
    remove_all();
}
