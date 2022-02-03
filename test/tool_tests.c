/**********************************************************************
 * Copyright (c) 2016 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <dogecoin/base58.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/tool.h>

#include <dogecoin/utils.h>

#include "utest.h"

void test_tool()
{
    char addr[100];
    char addr_p2sh_p2wpkh[100];
    char addr_p2wpkh[100];
    u_assert_int_eq(addresses_from_pubkey(&dogecoin_chainparams_main, "039ca1fdedbe160cb7b14df2a798c8fed41ad4ed30b06a85ad23e03abe43c413b2", addr, addr_p2sh_p2wpkh, addr_p2wpkh), true);
    u_assert_str_eq(addr, "DTwqVfB7tbwca2PzwBvPV1g1xDB2YPrCYh");
    u_assert_str_eq(addr_p2sh_p2wpkh, "A6JS4r6BucWmrMXeTuuxbVCrS9iHPckeBf");
    u_assert_str_eq(addr_p2wpkh, "doge1qlg5uydlgue7ywqcnt6rumf8743pm5usr5rlvmd");

    size_t pubkeylen = 100;
    char pubkey[pubkeylen];
    u_assert_int_eq(pubkey_from_privatekey(&dogecoin_chainparams_main, "QUaohmokNWroj71dRtmPSses5eRw5SGLKsYSRSVisJHyZdxhdDCZ", pubkey, &pubkeylen), true);
    u_assert_str_eq(pubkey, "024c33fbb2f6accde1db907e88ebf5dd1693e31433c62aaeef42f7640974f602ba");

    size_t privkeywiflen = 100;
    char privkeywif[privkeywiflen];
    char privkeyhex[100];
    u_assert_int_eq(gen_privatekey(&dogecoin_chainparams_main, privkeywif, privkeywiflen, NULL), true);
    u_assert_int_eq(gen_privatekey(&dogecoin_chainparams_main, privkeywif, privkeywiflen, privkeyhex), true);

    uint8_t privkey_data[strlen(privkeywif)];
    size_t outlen = dogecoin_base58_decode_check(privkeywif, privkey_data, sizeof(privkey_data));
    u_assert_int_eq(privkey_data[0] == dogecoin_chainparams_main.b58prefix_secret_address, true);

    char privkey_hex_or_null[65];
    utils_bin_to_hex(privkey_data+1, DOGECOIN_ECKEY_PKEY_LENGTH, privkey_hex_or_null);
    u_assert_str_eq(privkeyhex,privkey_hex_or_null);

    size_t masterkeysize = 200;
    char masterkey[masterkeysize];
    u_assert_int_eq(hd_gen_master(&dogecoin_chainparams_main, masterkey, masterkeysize), true);
    u_assert_int_eq(hd_print_node(&dogecoin_chainparams_main, masterkey), true);

    size_t extoutsize = 200;
    char extout[extoutsize];
    const char *privkey = "dgpv51eADS3spNJh7xcFJekcZcC7xNKBJB8e8TSsa5rBoQaXZeDXANA2dSBZmqaV6NUCtLpRqbjAg8m7x6VRhe42JBGukecNMeiQnhbd2rd6cEf";

    u_assert_int_eq(hd_derive(&dogecoin_chainparams_main, privkey, "m/1", extout, extoutsize), true);
    u_assert_str_eq(extout, "dgpv557t1z21sLCn87uw6deyEZgm7v4deuLQ4W5KdWLxqahWVsfzAVWfkqUYB7rBaLgkye27AhP9HiReS8uhRiqCyvT4eK3mMSwrPe9EztPoazQ");

    u_assert_int_eq(hd_derive(&dogecoin_chainparams_main, privkey, "m/1'", extout, extoutsize), true);
    u_assert_str_eq(extout, "dgpv557t1z2ACzjkJEtxJ5zWNU1RCGDbr9XXy3hcYrReTjc1Eetncxh34pCQTSUvfzHMbLegNcDvnPbKYdqvUt6TfonJmGWAgo4qxa3HBB1XG4Y");

    u_assert_int_eq(hd_derive(&dogecoin_chainparams_main, "dgub8kXBZ7ymNWy2Rt5HLXg6R6cR1AhyF6RPT6bToGLw7BLbzHwmD23prgg1FLQtxW8u1jdJ1HuJLNLGrSgR3WrxhAont8Sn2BYZVGEQztyKLLG", "m/1", extout, extoutsize), true);

    u_assert_int_eq(hd_derive(&dogecoin_chainparams_main, privkey, "m/", extout, extoutsize), true);
    u_assert_str_eq(extout, privkey);

    u_assert_int_eq(hd_derive(&dogecoin_chainparams_main, privkey, "m", extout, extoutsize), false);
    u_assert_int_eq(hd_derive(&dogecoin_chainparams_main, privkey, "n/1", extout, extoutsize), false);
}
