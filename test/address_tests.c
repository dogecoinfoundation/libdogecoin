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
    char p2pkh_pubkey_main[DOGECOIN_ECKEY_COMPRESSED_LENGTH];
    char p2pkh_pubkey_test[DOGECOIN_ECKEY_COMPRESSED_LENGTH];

    // test generation ability
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_main, NULL, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, true), true);

    // test keypair validity and association
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, false), true);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, true), true);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, true), false);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, false), false);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_test, false), false);
    u_assert_int_eq(verifyPrivPubKeypair("QWgNKvA5LPD1HpopRFghjz6jPipHRAUrLjqTt7paxYX8cTbu5eRs", "D7AM5jDQ7xRRK7bMCZ87e4BsFxHxCdDbXd", false), true);
    u_assert_int_eq(verifyPrivPubKeypair("QWgNKvA5LPD1HpopRFghjz6jPipHRAUrLjqTt7paxYX8cTbu5eRs", "DCncxpcZW3GEyqs17KrqAfs4cR844JkimG", false), false);

    // test entropy check
    u_assert_int_eq(verifyP2pkhAddress("Dasdfasdfasdfasdfasdfasdfasdfasdfx", false), false);
    u_assert_int_eq(verifyP2pkhAddress("DP6xxxDJxxxJAaWucRfsPvXLPGRyF3DdeP", false), false);

    // test address format correctness (0.3% false negative rate)
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_main, false), true);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_test, true), true);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_main, true), false);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_test, false), false);

<<<<<<< HEAD
    // test entropy check
    u_assert_int_eq(verifyP2pkhAddress("Dasdfasdfasdfasdfasdfasdfasdfasdfx", false), false);
    u_assert_int_eq(verifyP2pkhAddress("DP6xxxDJxxxJAaWucRfsPvXLPGRyF3DdeP", false), false);

    
>>>>>>> fa40b89... added address and keypair verification along with wrappers for them
=======
    // test master key generation ability
>>>>>>> 5272c34... minor tweaks, added support for hd master keypair verification on the C side, added python wrappers for hd verification, included some extra unit tests
    size_t masterkeysize = 200;
    char masterkey_main[masterkeysize];
    char masterkey_test[masterkeysize];
    char p2pkh_master_pubkey_main[DOGECOIN_ECKEY_COMPRESSED_LENGTH];
    char p2pkh_master_pubkey_test[DOGECOIN_ECKEY_COMPRESSED_LENGTH];
    u_assert_int_eq(generateHDMasterPubKeypair(masterkey_main, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, NULL), true)
    u_assert_int_eq(generateHDMasterPubKeypair(masterkey_main, p2pkh_master_pubkey_main, false), true);
    u_assert_int_eq(generateHDMasterPubKeypair(masterkey_test, p2pkh_master_pubkey_test, true), true);
    
    // test child key derivation ability
    char child_key_main[DOGECOIN_ECKEY_COMPRESSED_LENGTH];
    char child_key_test[DOGECOIN_ECKEY_COMPRESSED_LENGTH];
    u_assert_int_eq(generateDerivedHDPubkey(masterkey_main, NULL), true)
    u_assert_int_eq(generateDerivedHDPubkey(NULL, NULL), false)
<<<<<<< HEAD
    
    size_t strsize = 128;
    char str[strsize];
=======
    u_assert_int_eq(generateDerivedHDPubkey(masterkey_main, child_key_main), true);
    u_assert_int_eq(generateDerivedHDPubkey(masterkey_test, child_key_test), true);

    // test master keypair validity and association
    u_assert_int_eq(verifyHDMasterPubKeypair(masterkey_main, child_key_main, false), true);
    // u_assert_int_eq(verifyHDMasterPubKeypair(masterkey_test, child_key_test, true), true);
    u_assert_int_eq(verifyHDMasterPubKeypair("dgpv51eADS3spNJh7z2oc8LgNLeJiwiPNgdEFcdtAhtCqDQ76SwphcQq74jZCRTZ2nF5RpmKx9P4Mm55RTopNQePWiSBfzyJ3jgRoxVbVLF6BCY", "DJt45oTXDxBiJBRZeMtXm4wu4kc5yPePYn", false), true);
    u_assert_int_eq(verifyHDMasterPubKeypair("dgpv51eADS3spNJh7z2oc8LgNLeJiwiPNgdEFcdtAhtCqDQ76SwphcQq74jZCRTZ2nF5RpmKx9P4Mm55RTopNQePWiSBfzyJ3jgRoxVbVLF6BCY", "DDDXCMUCXCFK3UHXsjqSkzwoqt79K6Rn6k", false), false);

    // test hd address format correctness
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_master_pubkey_main, false), true);
    // u_assert_int_eq(verifyP2pkhAddress(p2pkh_master_pubkey_test, true), true);

    // test derived hd address format correctness
    u_assert_int_eq(generateDerivedHDPubkey(masterkey_main, child_key_main), true);
    u_assert_int_eq(verifyP2pkhAddress(child_key_main, false), true);
    u_assert_int_eq(generateDerivedHDPubkey(masterkey_test, child_key_test), true);
    //u_assert_int_eq(verifyP2pkhAddress(child_key_test, true), true);
>>>>>>> 5272c34... minor tweaks, added support for hd master keypair verification on the C side, added python wrappers for hd verification, included some extra unit tests

    u_assert_int_eq(generateDerivedHDPubkey("dgpv51eADS3spNJhA6LG5QycrFmQQtxg7ztFJQuamYiytZ4x4FUC7pG5B7fUTHBDB7g6oGaCVwuGF2i75r1DQKyFSauAHUGBAi89NaggpdUP3yK", str), true)
    u_assert_str_eq("DEByFfUQ3AxcFFet9afr8wxxedQysRduWN", str);

    u_assert_int_eq(generateDerivedHDPubkey("tprv8ZgxMBicQKsPeM5HaRoH4AuGX2Jsf8rgQvcFGCvjQxvAn1Bv8SAx8cPQsnmKsB6WjvGWsNiNsrNS2d3quUkYpK2ofctFw87SXodGhBPHiUM", str), true)
    u_assert_str_eq("noBtVVtAvvh5oapFjHHyTSxxEUTykUZ3oR", str);
}
