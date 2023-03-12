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
    for (int i = 0; i < 10; i++) {
        // key 1:
        int key_id = start_key();
        eckey* key = find_eckey(key_id);
        char* msg = "This is a test message";
        signature* sig = signmsgwitheckey(key, msg);
        char* address = verifymessagewithsig(sig, msg);
        u_assert_str_eq(address, sig->address);
        remove_eckey(key);
        free_signature(sig);
        dogecoin_free(address);

        // key 2:
        int key_id2 = start_key();
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
    }
}
