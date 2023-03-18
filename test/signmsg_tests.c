/**********************************************************************
 * Copyright (c) 2023 bluezr                                          *
 * Copyright (c) 2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <assert.h>

#include "utest.h"

#include <dogecoin/eckey.h>
#include <dogecoin/mem.h>
#include <dogecoin/sign.h>
#include <dogecoin/tool.h>

void test_signmsg() {
    // testcase 1:
    // sign and verify:
    char* msg = "Hello World!";
    char* privkey = "QWCcckTzUBiY1g3GFixihAscwHAKXeXY76v7Gcxhp3HUEAcBv33i";
    char* address = "D8mQ2sKYpLbFCQLhGeHCPBmkLJRi6kRoSg";
    char* sig = signmsgwithprivatekey(privkey, msg);
    printf("sig: %s\n", sig);
    u_assert_int_eq(verifymessage(sig, msg, address), 1);

    // testcase 2:
    // assert modified msg will cause verification failure:
    msg = "This is a new test message";
    u_assert_int_eq(verifymessage(sig, msg, address), 0);
    msg = "Hello World!";
    u_assert_int_eq(verifymessage(sig, msg, address), 1);
    dogecoin_free(sig);
}

void test_signmsg_ext() {
    for (int i = 0; i < 10; i++) {
        // key 1:
        int key_id = start_key(false);
        eckey* key = find_eckey(key_id);
        char* msg = "This is a test message";
        signature* sig = signmsgwitheckey(key, msg);
        char* address = verifymessagewithsig(sig, msg);
        u_assert_str_eq(address, sig->address);
        remove_eckey(key);
        free_signature(sig);
        dogecoin_free(address);

        // key 2:
        int key_id2 = start_key(false);
        eckey* key2 = find_eckey(key_id2);
        char* msg2 = "This is a test message";
        signature* sig2 = signmsgwitheckey(key2, msg2);
        char* address2 = verifymessagewithsig(sig2, msg2);
        u_assert_str_eq(address2, sig2->address);
        dogecoin_free(address2);

        // test message signature verification failure:
        msg2 = "This is an altered test message";
        address2 = verifymessagewithsig(sig2, msg2);
        u_assert_str_not_eq(address2, sig2->address);
        remove_eckey(key2);
        free_signature(sig2);
        dogecoin_free(address2);

        // key 3:
        int key_id3 = start_key(true);
        eckey* testnet_key = find_eckey(key_id3);
        char* testnet_msg = "bleh";
        signature* testnet_sig = signmsgwitheckey(testnet_key, testnet_msg);
        char* testnet_address = verifymessagewithsig(testnet_sig, testnet_msg);
        u_assert_str_eq(testnet_address, testnet_sig->address);
        remove_eckey(testnet_key);
        free_signature(testnet_sig);
        dogecoin_free(testnet_address);
    }

    // key 1:
    char* privkey = "QUtnMFjt3JFk1NfeMe6Dj5u4p25DHZA54FsvEFAiQxcNP4bZkPu2";
    eckey* key = new_eckey_from_privkey(privkey);
    char* msg = "This is a test message";
    signature* sig = signmsgwitheckey(key, msg);
    char* address = verifymessagewithsig(sig, msg);
    u_assert_str_eq(address, sig->address);
    char* sig2 = signmsgwithprivatekey(privkey, msg);
    u_assert_int_eq(verifymessage(sig2, msg, sig->address), 1);
    free_signature(sig);
    dogecoin_free(sig2);
    dogecoin_free(key);
    dogecoin_free(address);
}
