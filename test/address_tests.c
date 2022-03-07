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
    char privkeywif_main[privkeywiflen];
    char privkeywif_test[privkeywiflen];
    char p2pkh_pubkey_main[100];
    char p2pkh_pubkey_test[100];

    // test verifyP2pkhAddress functionality
    u_assert_int_eq(verifyP2pkhAddress("DMF7yHeKS1qfwTMtDrBQ6NikDqfWM6JS66", false), true);
    u_assert_int_eq(verifyP2pkhAddress("DMF7yHeKS1qfwTMtDrBQ6NikDqfWM6JS68", false), false);
    u_assert_int_eq(verifyP2pkhAddress("D8F7yH3KS1qfwTMtDrBQ6NikDqfWM6JS66", false), false);
    u_assert_int_eq(verifyP2pkhAddress("nXUrjZo9WqY593qZsfEHBpiXtL3iQ76ae1", true), true);
    u_assert_int_eq(verifyP2pkhAddress("nXUrjZo9WqY593qZsfEHBpiXtL3iQ76ae8", true), false);
    u_assert_int_eq(verifyP2pkhAddress("n8UrjZo9WqY593qZsfEHBpiXtL3iQ76ae1", true), false);

    // test generation ability
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_main, NULL, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_test, NULL, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, true), true);

    // test key validity and association
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, false), true);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, true), true);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, true), false);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, false), false);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_test, p2pkh_pubkey_main, true), false);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_test, false), false);
    u_assert_int_eq(verifyPrivPubKeypair("QWgNKvA5LPD1HpopRFghjz6jPipHRAUrLjqTt7paxYX8cTbu5eRs", "D7AM5jDQ7xRRK7bMCZ87e4BsFxHxCdDbXd", false), true);
    u_assert_int_eq(verifyPrivPubKeypair("QWgNKvA5LPD1HpopRFghjz6jPipHRAUrLjqTt7paxYX8cTbu5eRs", "DCncxpcZW3GEyqs17KrqAfs4cR844JkimG", false), false);

    // test address format correctness
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_main, false), true);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_test, true), true);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_main, true), false);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_test, false), false);

    // test master key generation ability
    size_t masterkeysize = 200;
    char masterkey[masterkeysize];
    u_assert_int_eq(generateHDMasterPubKeypair(masterkey, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, NULL), true)
    
    u_assert_int_eq(generateDerivedHDPubkey(masterkey, NULL), true)
    u_assert_int_eq(generateDerivedHDPubkey(NULL, NULL), false)
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
    // u_assert_int_eq(verifyP2pkhAddress(child_key_test, true), true);
}
