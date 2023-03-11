/**********************************************************************
 * Copyright (c) 2023 bluezr                                          *
 * Copyright (c) 2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <assert.h>

#include "utest.h"

#include <dogecoin/address.h>
#include <dogecoin/mem.h>
#include <dogecoin/tool.h>

void test_signmsg() {
    // testcase 1:
    // sign and verify:
    char* msg = "This is just a test message";
    char* privkey = "QUtnMFjt3JFk1NfeMe6Dj5u4p25DHZA54FsvEFAiQxcNP4bZkPu2";
    char* address = "D9HiLDU8ncvkcnajmJgxEQUJYnh3JQsaZE";
    char* sig = signmsgwithprivatekey(privkey, msg);
    u_assert_int_eq(verifymessage(address, sig, msg), 1);

    // testcase 2:
    // assert modified msg will cause verification failure:
    msg = "This is a new test message";
    u_assert_int_eq(verifymessage(address, sig, msg), 0);
    msg = "This is just a test message";
    u_assert_int_eq(verifymessage(address, sig, msg), 1);
    dogecoin_free(sig);
}

void test_signmsg_ext() {
    for (int i = 0; i < 2; i++) {
        // key 1:
        int key_id = start_key();
        printf("key_id: %d\n", key_id);
        eckey* key = find_eckey(key_id);
        printf("key: %s\n", key->private_key_wif);
        printf("pubkey: %s\n", key->public_key_hex);
        char* address = addressFromPrivkey((char*)key->private_key.privkey);
        printf("address: %s\n", address);
        char* msg = "This is a test message";
        char* sig = signmsgwitheckey(key, msg);
        u_assert_int_eq(verifymessagewitheckey(key, sig, msg), 1);

        // key 2:
        int key_id2 = start_key();
        printf("key_id: %d\n", key_id2);
        eckey* key2 = find_eckey(key_id2);
        printf("key: %s\n", key2->private_key_wif);
        printf("pubkey: %s\n", key2->public_key_hex);
        char* address2 = addressFromPrivkey((char*)key2->private_key.privkey);
        printf("address: %s\n", address2);
        char* msg2 = "This is a test message";
        char* sig2 = signmsgwitheckey(key2, msg2);
        u_assert_int_eq(verifymessagewitheckey(key2, sig2, msg2), 1);

        remove_eckey(key);
        remove_eckey(key2);

        dogecoin_free(sig);
        dogecoin_free(sig2);
    }
}
