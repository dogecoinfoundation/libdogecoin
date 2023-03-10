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
    int key_id = start_key();
    printf("key_id: %d\n", key_id);
    eckey* key = find_eckey(key_id);
    remove_eckey(key);
}
