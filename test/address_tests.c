/**********************************************************************
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <test/utest.h>

#include <dogecoin/address.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/crypto/base58.h>
#include <dogecoin/utils.h>

void test_address()
{
    size_t privkeywiflen = 100;
    char privkeywif[privkeywiflen];
    char p2pkh_wif[36];
    u_assert_int_eq(generatePrivPubKeypair(privkeywif, p2pkh_wif, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif, NULL, false), true)

    size_t masterkeysize = 200;
    char masterkey[masterkeysize];
    u_assert_int_eq(generateHDMasterPubKeypair(masterkey, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, NULL), true)
    
    u_assert_int_eq(generateDerivedHDPubkey(masterkey, NULL), true)
    u_assert_int_eq(generateDerivedHDPubkey(NULL, NULL), false)
    
    size_t strsize = 128;
    char str[strsize];

    u_assert_int_eq(generateDerivedHDPubkey("dgpv51eADS3spNJhA6LG5QycrFmQQtxg7ztFJQuamYiytZ4x4FUC7pG5B7fUTHBDB7g6oGaCVwuGF2i75r1DQKyFSauAHUGBAi89NaggpdUP3yK", str), true)
    u_assert_str_eq("DEByFfUQ3AxcFFet9afr8wxxedQysRduWN", str);

    u_assert_int_eq(generateDerivedHDPubkey("tprv8ZgxMBicQKsPeM5HaRoH4AuGX2Jsf8rgQvcFGCvjQxvAn1Bv8SAx8cPQsnmKsB6WjvGWsNiNsrNS2d3quUkYpK2ofctFw87SXodGhBPHiUM", str), true)
    u_assert_str_eq("noBtVVtAvvh5oapFjHHyTSxxEUTykUZ3oR", str);
}
