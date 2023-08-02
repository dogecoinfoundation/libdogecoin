/**********************************************************************
 * Copyright (c) 2023 bluezr                                          *
 * Copyright (c) 2023 edtubbs                                         *
 * Copyright (c) 2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <test/utest.h>

#include <dogecoin/address.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/constants.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/key.h>
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
    size_t wiflen = 53;
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
    char* child_key_main = dogecoin_char_vla(masterkeylen);
    char* child_key_test = dogecoin_char_vla(masterkeylen);
    char* str = dogecoin_char_vla(pubkeylen);
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
    
    /* test getHDNodeAndExtKeyByPath */
    dogecoin_hdnode* hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/0'/0/0", extout, true);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "09648faa2fa89d84c7eb3c622e06ed2c1c67df223bc85ee206b30178deea7927");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QNvtKnf9Qi7jCRiPNsHhvibNo6P5rSHR1zsg3MvaZVomB2J3VnAG");
    u_assert_str_eq(extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
    
    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/0'/0/0", extout, false);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "09648faa2fa89d84c7eb3c622e06ed2c1c67df223bc85ee206b30178deea7927");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QNvtKnf9Qi7jCRiPNsHhvibNo6P5rSHR1zsg3MvaZVomB2J3VnAG");
    u_assert_str_eq(extout, "dgub8vXjuDpn2sTkerBdjSfq9kmjhaQsXHxyBkYrikw84GCYz9ozcdwvYPo5SSDWqZUVT5d4jrG8CHiGsC1M7pdETPhoKiQa92znT2vG9YaytBH");
    
    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/0'/0/1", extout, true);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "fb3b5ae847c475f9d852522f454d9b9a9d2b71f7286ebcccf824fab09d067457");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QX2zKvdkWv1CbqhwTT1pmMFsxm2trhcs1C45uR8htsKTXM3Fjakd");
    u_assert_str_eq(extout, "dgpv5BeiZXttUioRRXS57Kd9ypkyWcV3vey1MgrSYmhaeBE54J8zerFV5mJSdZWQxpg55L13GWn4BWGMm1mPgzCp5btqBudQtoyepBECGS3pUT5");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/0'/0/1", extout, false);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "fb3b5ae847c475f9d852522f454d9b9a9d2b71f7286ebcccf824fab09d067457");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QX2zKvdkWv1CbqhwTT1pmMFsxm2trhcs1C45uR8htsKTXM3Fjakd");
    u_assert_str_eq(extout, "dgub8vXjuDpn2sTkiP6E9ZF86gJZyArgkmzieHdeht6ZSJH5cMFh4coFj4i6CncZQKrXobLWphRcR5fwupY9rKkR3s5L5xLAeS6WK6KyKM7pGYN");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/0'/1/0", extout, true);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "b2cb3f0536a52a7e3e2881c554fb19abb676188fbb8b3f855a2d192d5140030b");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QUcBMYx22178giKAWQxJV6qyRT5PMiRuTCW4JkKA7FNeWpj3PwZF");
    u_assert_str_eq(extout, "dgpv5B5FdsPKQH8hK3vUo5ZR9ZXktfUxv1PStiM2TfnwH9oct5nJwAUx28356eNXoUwcNwzvfVRSDVh85aV3CQdKpQo2Vm8MKyz7KsNAXTEMbeS");
    
    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/0'/1/0", extout, false);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "b2cb3f0536a52a7e3e2881c554fb19abb676188fbb8b3f855a2d192d5140030b");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QUcBMYx22178giKAWQxJV6qyRT5PMiRuTCW4JkKA7FNeWpj3PwZF");
    u_assert_str_eq(extout, "dgub8uxGyZKCxRo2buadqKBPGR5MMDrbk8RABK8EcnBv5GrdS8u1Lw2ifRSifsT3wuVRsK45b9kugWkd2cREzkJLiGvwbY5txG2dKfsY3bndC93");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/1'/0/0", extout, true);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "2aeb96a77fc89bbcb7e183ee627f2b7711ac543e5a36ecf9391db06bfb38db9e");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QQ44Mhbq9itBVntwhraNf3E9BEUYsh2paDtE5XsHjwsWnHYWQ3Yf");
    u_assert_str_eq(extout, "dgpv5Ckgu5gakCr2e4PpY8K64iGqoREfmRJAACxiX4ia6AToANXgttniNLr727cgx3ceih7xdMcejLb7bkL7AE8dWKRHCCW6Bgr4ZivSjoxTF3A");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/1'/0/0", extout, false);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "2aeb96a77fc89bbcb7e183ee627f2b7711ac543e5a36ecf9391db06bfb38db9e");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QQ44Mhbq9itBVntwhraNf3E9BEUYsh2paDtE5XsHjwsWnHYWQ3Yf");
    u_assert_str_eq(extout, "dgub8wdiEmcUJMWMvv3yaMw4BZpSFycJbYKsSojvgB7YtHWoiRePJfLV1eFkbM2up2rkvEeukK9ffXypdLscJKJH8MwTe8hvJcWhcMdwwjpLKmQ");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/1'/1/0", extout, true);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "1c88de21503a7b61911560f2ae9fa76c76983be0cd9c2f5d22efa1376f371e6a");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QPa6TYKTk5qggHa8V2PaWWJUAB6TZnwgqzEqF91oKKEuExVyrykD");
    u_assert_str_eq(extout, "dgpv5CnqDfc6af4vFmQ1afrYYH3SSM5wT1fXVNJuVzWBEPBB5X3oy8AFz88DawAtCcZDq6tDbJmdBTSgYPCHc3GB7sFdbbBAuyxn2vLAsKar9BT");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/1'/1/0", extout, false);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "1c88de21503a7b61911560f2ae9fa76c76983be0cd9c2f5d22efa1376f371e6a");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QPa6TYKTk5qggHa8V2PaWWJUAB6TZnwgqzEqF91oKKEuExVyrykD");
    u_assert_str_eq(extout, "dgub8wfrZMXz8ojFYd4AcuUWf8b2tuTaH8hEmy67f6uA2WEBdaAWNti2dRXsADFSsM26nsiaPR81pZNE3Y2ws89HK46qtGifYJTb7RGzbhr8CiC");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/1'/0/1", extout, true);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "2049b1748a10c0c33ed35f542a0905f2074254266bc5b7e9c32fc155b4803eda");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QPhPcYBCZPPc73Ldrdj6Ubc8SiiRqwRns6nuEqgzshiqJA6WEp62");
    u_assert_str_eq(extout, "dgpv5Ckgu5gakCr2g8NwFsi9aXXgBTXvzoFxwi8ybQHRmutQzYDoa8y4QD6w94EEYFtinVGD3ZzZG89t8pedriw9L8VgPYKeQsUHoZQaKcSEqwr");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/1'/0/1", extout, false);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "2049b1748a10c0c33ed35f542a0905f2074254266bc5b7e9c32fc155b4803eda");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QPhPcYBCZPPc73Ldrdj6Ubc8SiiRqwRns6nuEqgzshiqJA6WEp62");
    u_assert_str_eq(extout, "dgub8wdiEmcUJMWMxz36J7L7hP5Ge1uZpvHgEJvBkWgQa2wRYbLVyuWq3WWaiK3ZgYs893RqrgZN3QgRghPXkpRr7kdT44XVSaJuwMF1PTHi2mQ");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/1'/1/1", extout, true);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "3e9611987aa94ecee356d3fbe8cdc1e5b4fd9c143fe8f9e3637ac5b1f599c3b0");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QQiHajxrYwkCK1zkbmt2ZTKSQyy64jUPVbw4CDYJBchg975TRBJu");
    u_assert_str_eq(extout, "dgpv5CnqDfc6af4vKYLZQfyGgYYVQcgkiGwqAm1qEirxruSwXwSQJoTLjSckPkbZDXRQs7X83esTtoBEmy4zr4UgJBHb8T1EMc6HYCsWgKk4JRh");

    hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/1'/1/1", extout, false);
    u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "3e9611987aa94ecee356d3fbe8cdc1e5b4fd9c143fe8f9e3637ac5b1f599c3b0");
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
    u_assert_str_eq(privkeywif_main, "QQiHajxrYwkCK1zkbmt2ZTKSQyy64jUPVbw4CDYJBchg975TRBJu");
    u_assert_str_eq(extout, "dgub8wfrZMXz8ojFcPziSubEoQ65sB4PYPyYTMo3PqFwf2Vx5zZ6ia17Nk2Py25c3dvq1e7ZnfBrurCS5wuagzRoBCXhJ2NeGU54NBytvuUuRyA");

#if WIN32 || USE_UNISTRING
    // mnemonic to HD keys and addresses
    MNEMONIC seedphrase = "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote";
    res = generateHDMasterPubKeypairFromMnemonic (extout, p2pkh_pubkey_test, seedphrase, NULL, true);
    u_assert_str_eq(extout, "tprv8ZgxMBicQKsPd66qSfNTYkdM76NsJ368nHs7r1WnKhmUbdx4Gwkhk175pvpe2A652Xzszhg2qf55w8qpRzNBwMboA3R6PoABT36eHV89dRZ");
    res = verifyHDMasterPubKeypairFromMnemonic ("tprv8ZgxMBicQKsPd66qSfNTYkdM76NsJ368nHs7r1WnKhmUbdx4Gwkhk175pvpe2A652Xzszhg2qf55w8qpRzNBwMboA3R6PoABT36eHV89dRZ", p2pkh_pubkey_test, seedphrase, NULL, true);
    u_assert_int_eq(res, 0);
    res = getDerivedHDAddressFromMnemonic(0, 0, BIP44_CHANGE_EXTERNAL, seedphrase, NULL, p2pkh_pubkey_test, true);
    u_assert_str_eq(p2pkh_pubkey_test, "naTzLkBZLpUVXykb3sSP1Wzzz9GzzM4BVU");
    res = getDerivedHDAddressFromMnemonic(0, 0, BIP44_CHANGE_EXTERNAL, seedphrase, NULL, p2pkh_pubkey_main, false);
    u_assert_str_eq(p2pkh_pubkey_main, "DTdKu8YgcxoXyjFCDtCeKimaZzsK27rcwT");
#endif

    /*free up VLAs*/
    dogecoin_hdnode_free(hdnode);
    free(masterkey_main);
    free(masterkey_test);
    free(p2pkh_master_pubkey_main);
    free(p2pkh_master_pubkey_test);
    free(child_key_main);
    free(child_key_test);
    free(str);
    free(extout);
}
