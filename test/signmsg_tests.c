/**********************************************************************
 * Copyright (c) 2023 bluezr, edtubbs                                 *
 * Copyright (c) 2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <assert.h>

#include <test/utest.h>

#include <dogecoin/eckey.h>
#include <dogecoin/mem.h>
#include <dogecoin/sign.h>

void test_signmsg() {
    char* msg = "Hello World!";
    char* privkey = "QWCcckTzUBiY1g3GFixihAscwHAKXeXY76v7Gcxhp3HUEAcBv33i";
    char* address = "D8mQ2sKYpLbFCQLhGeHCPBmkLJRi6kRoSg";
    char* sig = sign_message(privkey, msg);
    u_assert_int_eq(verify_message(sig, msg, address), 1);
    msg = "This is a new test message";
    u_assert_int_eq(verify_message(sig, msg, address), 0);
    msg = "Hello World!";
    u_assert_int_eq(verify_message(sig, msg, address), 1);
    dogecoin_free(sig);
}

void test_signmsg_ext() {
    for (int i = 0; i < 10; i++) {
        int key_id = start_key(false);
        eckey* key = find_eckey(key_id);
        char* msg = "This is a test message";
        char* sig = sign_message(key->private_key_wif, msg);
        u_assert_int_eq(verify_message(sig, msg, key->address), 1);
        remove_eckey(key);
        dogecoin_free(sig);

        int key_id2 = start_key(false);
        eckey* key2 = find_eckey(key_id2);
        char* msg2 = "This is a test message";
        char* sig2 = sign_message(key2->private_key_wif, msg2);
        u_assert_int_eq(verify_message(sig2, msg2, key2->address), 1);
        msg2 = "This is an altered test message";
        u_assert_int_eq(verify_message(sig2, msg2, key2->address), 0);
        remove_eckey(key2);
        dogecoin_free(sig2);

        int key_id3 = start_key(true);
        eckey* testnet_key = find_eckey(key_id3);
        char* testnet_msg = "bleh";
        char* testnet_sig = sign_message(testnet_key->private_key_wif, testnet_msg);
        u_assert_int_eq(verify_message(testnet_sig, testnet_msg, testnet_key->address), 1);
        remove_eckey(testnet_key);
        dogecoin_free(testnet_sig);
    }

    char* privkey = "QUtnMFjt3JFk1NfeMe6Dj5u4p25DHZA54FsvEFAiQxcNP4bZkPu2";
    eckey* key = new_eckey_from_privkey(privkey);
    char* msg = "This is a test message";
    char* sig = sign_message(key->private_key_wif, msg);
    u_assert_int_eq(verify_message(sig, msg, key->address), 1);
    dogecoin_free(key);
    dogecoin_free(sig);
}
