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
<<<<<<< HEAD
    size_t privkeywiflen = 100;
    char privkeywif[privkeywiflen];
    char p2pkh_wif[36];
    u_assert_int_eq(generatePrivPubKeypair(privkeywif, p2pkh_wif, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif, NULL, false), true)

=======
    size_t privkeywiflen = 100;    char privkeywif_main[privkeywiflen];
    char privkeywif_test[privkeywiflen];
    char p2pkh_pubkey_main[100];
    char p2pkh_pubkey_test[100];

    // test generation ability
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_main, NULL, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, true), true);

    // test key validity and association
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, false), true);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, true), true);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, true), false);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, false), false);

    // test address format correctness (0.003% false negative rate)
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_main, false), true);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_test, true), true);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_main, true), false);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_test, false), false);

    // test entropy check
    u_assert_int_eq(verifyP2pkhAddress("Dasdfasdfasdfasdfasdfasdfasdfasdfx", false), false);
    u_assert_int_eq(verifyP2pkhAddress("DP6xxxDJxxxJAaWucRfsPvXLPGRyF3DdeP", false), false);

    
>>>>>>> fa40b89... added address and keypair verification along with wrappers for them
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
