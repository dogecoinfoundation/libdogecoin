/**********************************************************************
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "utest.h"

#include <dogecoin/address.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/utils.h>

void test_address()
{
    /* initialize testing variables for simple keypair gen */
    char privkeywif_main[53];
    char privkeywif_test[53];
    char p2pkh_pubkey_main[35];
    char p2pkh_pubkey_test[35];
    dogecoin_mem_zero(privkeywif_main, sizeof(privkeywif_main));
    dogecoin_mem_zero(privkeywif_test, sizeof(privkeywif_test));
    // test generation ability
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_main, NULL, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, false), true)
    u_assert_int_eq(generatePrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, true), true);

    /* test keypair basic exterior validity */
    u_assert_int_eq(strncmp(privkeywif_main,   "Q", 1), 0);
    u_assert_int_eq(strncmp(p2pkh_pubkey_main, "D", 1), 0);
    u_assert_int_eq(strncmp(privkeywif_test,   "c", 1), 0);
    u_assert_int_eq(strncmp(p2pkh_pubkey_test, "n", 1), 0);

    /* test keypair association */
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, false), true);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, true), true);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_main, true), false);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_test, p2pkh_pubkey_test, false), false);
    u_assert_int_eq(verifyPrivPubKeypair(privkeywif_main, p2pkh_pubkey_test, false), false);
    u_assert_int_eq(verifyPrivPubKeypair("QWgNKvA5LPD1HpopRFghjz6jPipHRAUrLjqTt7paxYX8cTbu5eRs", "D7AM5jDQ7xRRK7bMCZ87e4BsFxHxCdDbXd", false), true);
    u_assert_int_eq(verifyPrivPubKeypair("QWgNKvA5LPD1HpopRFghjz6jPipHRAUrLjqTt7paxYX8cTbu5eRs", "DCncxpcZW3GEyqs17KrqAfs4cR844JkimG", false), false);

    /* test internal validity */
    u_assert_int_eq(verifyP2pkhAddress("Dasdfasdfasdfasdfasdfasdfasdfasdfx", strlen("Dasdfasdfasdfasdfasdfasdfasdfasdfx")), false);
    u_assert_int_eq(verifyP2pkhAddress("DP6xxxDJxxxJAaWucRfsPvXLPGRyF3DdeP", strlen("DP6xxxDJxxxJAaWucRfsPvXLPGRyF3DdeP")), false);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_main, strlen(p2pkh_pubkey_main)), true);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_pubkey_test, strlen(p2pkh_pubkey_test)), true);


    /* initialize testing variables for hd keypair gen */
    size_t masterkeylen = 200;
    size_t pubkeylen = 35;

    char* masterkey_main = dogecoin_char_vla(masterkeylen);
    char* masterkey_test=dogecoin_char_vla(masterkeylen);
    char* p2pkh_master_pubkey_main=dogecoin_char_vla(pubkeylen);
    char* p2pkh_master_pubkey_test=dogecoin_char_vla(pubkeylen);
    dogecoin_mem_zero(masterkey_main, masterkeylen);
    dogecoin_mem_zero(masterkey_test, masterkeylen);
    dogecoin_mem_zero(p2pkh_master_pubkey_main, pubkeylen);
    dogecoin_mem_zero(p2pkh_master_pubkey_test, pubkeylen);

    /* test generation ability */
    u_assert_int_eq(generateHDMasterPubKeypair(masterkey_main, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, false), true)
    u_assert_int_eq(generateHDMasterPubKeypair(NULL, NULL, NULL), true)
    u_assert_int_eq(generateHDMasterPubKeypair(masterkey_main, p2pkh_master_pubkey_main, false), true);
    u_assert_int_eq(generateHDMasterPubKeypair(masterkey_test, p2pkh_master_pubkey_test, true), true);

    /* test master keypair basic external validity */
    //TODO: public keys
    u_assert_int_eq(strncmp(masterkey_main,   "dgpv", 4), 0);
    u_assert_int_eq(strncmp(masterkey_test,   "tprv", 1), 0);

    /* test master keypair association */
    u_assert_int_eq(verifyHDMasterPubKeypair(masterkey_main, p2pkh_master_pubkey_main, false), true);
    u_assert_int_eq(verifyHDMasterPubKeypair(masterkey_test, p2pkh_master_pubkey_test, true), true);
    u_assert_int_eq(verifyHDMasterPubKeypair("dgpv51eADS3spNJh7z2oc8LgNLeJiwiPNgdEFcdtAhtCqDQ76SwphcQq74jZCRTZ2nF5RpmKx9P4Mm55RTopNQePWiSBfzyJ3jgRoxVbVLF6BCY", "DJt45oTXDxBiJBRZeMtXm4wu4kc5yPePYn", false), true);
    u_assert_int_eq(verifyHDMasterPubKeypair("dgpv51eADS3spNJh7z2oc8LgNLeJiwiPNgdEFcdtAhtCqDQ76SwphcQq74jZCRTZ2nF5RpmKx9P4Mm55RTopNQePWiSBfzyJ3jgRoxVbVLF6BCY", "DDDXCMUCXCFK3UHXsjqSkzwoqt79K6Rn6k", false), false);

    // test hd address format correctness
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_master_pubkey_main, strlen(p2pkh_master_pubkey_main)), true);
    u_assert_int_eq(verifyP2pkhAddress(p2pkh_master_pubkey_test, strlen(p2pkh_master_pubkey_test)), true);


    /* initialize testing variables for derived pubkeys */

    char* child_key_main=dogecoin_char_vla(masterkeylen);
    char* child_key_test=dogecoin_char_vla(masterkeylen);
    char* str=dogecoin_char_vla(pubkeylen);
    /* test child key derivation ability */
    u_assert_int_eq(generateDerivedHDPubkey(masterkey_main, NULL), true)
    u_assert_int_eq(generateDerivedHDPubkey(NULL, NULL), false)
    u_assert_int_eq(generateDerivedHDPubkey(masterkey_main, child_key_main), true);
    u_assert_int_eq(generateDerivedHDPubkey(masterkey_test, child_key_test), true);
    u_assert_int_eq(generateDerivedHDPubkey("dgpv51eADS3spNJhA6LG5QycrFmQQtxg7ztFJQuamYiytZ4x4FUC7pG5B7fUTHBDB7g6oGaCVwuGF2i75r1DQKyFSauAHUGBAi89NaggpdUP3yK", str), true)
    u_assert_str_eq("DEByFfUQ3AxcFFet9afr8wxxedQysRduWN", str);
    u_assert_int_eq(generateDerivedHDPubkey("tprv8ZgxMBicQKsPeM5HaRoH4AuGX2Jsf8rgQvcFGCvjQxvAn1Bv8SAx8cPQsnmKsB6WjvGWsNiNsrNS2d3quUkYpK2ofctFw87SXodGhBPHiUM", str), true)
    u_assert_str_eq("noBtVVtAvvh5oapFjHHyTSxxEUTykUZ3oR", str);

    /* test child key/master key association */
    u_assert_int_eq(verifyHDMasterPubKeypair(masterkey_main, child_key_main, false), true);
    u_assert_int_eq(verifyHDMasterPubKeypair(masterkey_test, child_key_test, true), true);
    u_assert_int_eq(verifyP2pkhAddress(child_key_main, strlen(child_key_main)), true);
    u_assert_int_eq(verifyP2pkhAddress(child_key_test, strlen(child_key_test)), true);

    /* ckd address generation */
    size_t extoutsize = 112;
    char* extout = dogecoin_char_vla(extoutsize);
    char* masterkey_main_ext = "dgpv51eADS3spNJh8h13wso3DdDAw3EJRqWvftZyjTNCFEG7gqV6zsZmucmJR6xZfvgfmzUthVC6LNicBeNNDQdLiqjQJjPeZnxG8uW3Q3gCA3e";
    int res = getDerivedHDAddress(masterkey_main_ext, 0, false, 0, extout, true);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
    res = getDerivedHDAddress(masterkey_main_ext, 0, true, 0, extout, true);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgpv5B5FdsPKQH8hK3vUo5ZR9ZXktfUxv1PStiM2TfnwH9oct5nJwAUx28356eNXoUwcNwzvfVRSDVh85aV3CQdKpQo2Vm8MKyz7KsNAXTEMbeS");
    res = getDerivedHDAddress(masterkey_main_ext, 0, false, 0, extout, false);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgub8vXjuDpn2sTkerBdjSfq9kmjhaQsXHxyBkYrikw84GCYz9ozcdwvYPo5SSDWqZUVT5d4jrG8CHiGsC1M7pdETPhoKiQa92znT2vG9YaytBH");
    res = getDerivedHDAddress(masterkey_main_ext, 0, true, 0, extout, false);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgub8uxGyZKCxRo2buadqKBPGR5MMDrbk8RABK8EcnBv5GrdS8u1Lw2ifRSifsT3wuVRsK45b9kugWkd2cREzkJLiGvwbY5txG2dKfsY3bndC93");
    res = getDerivedHDAddress(masterkey_main_ext, 1, false, 1, extout, true);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgpv5Ckgu5gakCr2g8NwFsi9aXXgBTXvzoFxwi8ybQHRmutQzYDoa8y4QD6w94EEYFtinVGD3ZzZG89t8pedriw9L8VgPYKeQsUHoZQaKcSEqwr");
    res = getDerivedHDAddress(masterkey_main_ext, 1, true, 1, extout, true);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgpv5CnqDfc6af4vKYLZQfyGgYYVQcgkiGwqAm1qEirxruSwXwSQJoTLjSckPkbZDXRQs7X83esTtoBEmy4zr4UgJBHb8T1EMc6HYCsWgKk4JRh");
    res = getDerivedHDAddress(masterkey_main_ext, 1, false, 1, extout, false);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgub8wdiEmcUJMWMxz36J7L7hP5Ge1uZpvHgEJvBkWgQa2wRYbLVyuWq3WWaiK3ZgYs893RqrgZN3QgRghPXkpRr7kdT44XVSaJuwMF1PTHi2mQ");
    res = getDerivedHDAddress(masterkey_main_ext, 1, true, 1, extout, false);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgub8wfrZMXz8ojFcPziSubEoQ65sB4PYPyYTMo3PqFwf2Vx5zZ6ia17Nk2Py25c3dvq1e7ZnfBrurCS5wuagzRoBCXhJ2NeGU54NBytvuUuRyA");

    // hardened paths (unabstracted as this is called by getDerivedHDAddress)
    res = getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/0'/0/0", extout, true);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
    res = getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/0'/0/0", extout, false);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgub8vXjuDpn2sTkerBdjSfq9kmjhaQsXHxyBkYrikw84GCYz9ozcdwvYPo5SSDWqZUVT5d4jrG8CHiGsC1M7pdETPhoKiQa92znT2vG9YaytBH");
    res = getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/0'/1/0", extout, true);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgpv5B5FdsPKQH8hK3vUo5ZR9ZXktfUxv1PStiM2TfnwH9oct5nJwAUx28356eNXoUwcNwzvfVRSDVh85aV3CQdKpQo2Vm8MKyz7KsNAXTEMbeS");
    res = getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/0'/1/0", extout, false);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgub8uxGyZKCxRo2buadqKBPGR5MMDrbk8RABK8EcnBv5GrdS8u1Lw2ifRSifsT3wuVRsK45b9kugWkd2cREzkJLiGvwbY5txG2dKfsY3bndC93");
    res = getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/1'/0/1", extout, true);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgpv5Ckgu5gakCr2g8NwFsi9aXXgBTXvzoFxwi8ybQHRmutQzYDoa8y4QD6w94EEYFtinVGD3ZzZG89t8pedriw9L8VgPYKeQsUHoZQaKcSEqwr");
    res = getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/1'/0/1", extout, false);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgub8wdiEmcUJMWMxz36J7L7hP5Ge1uZpvHgEJvBkWgQa2wRYbLVyuWq3WWaiK3ZgYs893RqrgZN3QgRghPXkpRr7kdT44XVSaJuwMF1PTHi2mQ");
    res = getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/1'/1/1", extout, true);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgpv5CnqDfc6af4vKYLZQfyGgYYVQcgkiGwqAm1qEirxruSwXwSQJoTLjSckPkbZDXRQs7X83esTtoBEmy4zr4UgJBHb8T1EMc6HYCsWgKk4JRh");
    res = getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/1'/1/1", extout, false);
    u_assert_int_eq(res, true);
    u_assert_str_eq(extout, "dgub8wfrZMXz8ojFcPziSubEoQ65sB4PYPyYTMo3PqFwf2Vx5zZ6ia17Nk2Py25c3dvq1e7ZnfBrurCS5wuagzRoBCXhJ2NeGU54NBytvuUuRyA");

    /*free up VLAs*/
    free(masterkey_main);
    free(masterkey_test);
    free(p2pkh_master_pubkey_main);
    free(p2pkh_master_pubkey_test);
    free(child_key_main);
    free(child_key_test);
    free(str);
    free(extout);
}
