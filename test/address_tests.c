/**********************************************************************
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <dogecoin/address.h>
#include <dogecoin/crypto/base58.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/utils.h>

#include "utest.h"

void test_address()
{
    size_t privkeywiflen = 100;
    char privkeywif[privkeywiflen];
    char privkeyhex[100];
    u_assert_int_eq(generatePrivPubKeypair(privkeywif, privkeyhex, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif, NULL, false), true)
    
    size_t masterkeysize = 200;
    char masterkey[masterkeysize];
    char p2pkh_pubkey;
    u_assert_int_eq(generateHDMasterPubKeypair(masterkey, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, NULL), true)
    
    u_assert_int_eq(generateDerivedHDPubkey(masterkey, NULL), true)
    u_assert_int_eq(generateDerivedHDPubkey(NULL, NULL), false)

    size_t strsize = 128;
    char str[strsize];
    u_assert_int_eq(generateDerivedHDPubkey(masterkey, str), true)

}
