/**********************************************************************
 * Copyright (c) 2016 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "utest.h"

#include <dogecoin/chainparams.h>
#include <dogecoin/base58.h>
#include <dogecoin/tool.h>
#include <dogecoin/utils.h>

void test_tool()
{
    char addr[100];
    u_assert_int_eq(addresses_from_pubkey(&dogecoin_chainparams_main, "039ca1fdedbe160cb7b14df2a798c8fed41ad4ed30b06a85ad23e03abe43c413b2", addr), true);
    u_assert_str_eq(addr, "DTwqVfB7tbwca2PzwBvPV1g1xDB2YPrCYh");

    size_t pubkeylen = 100;
    char* pubkey=dogecoin_char_vla(pubkeylen);
    u_assert_int_eq(pubkey_from_privatekey(&dogecoin_chainparams_main, "QUaohmokNWroj71dRtmPSses5eRw5SGLKsYSRSVisJHyZdxhdDCZ", pubkey, &pubkeylen), true);
    u_assert_str_eq(pubkey, "024c33fbb2f6accde1db907e88ebf5dd1693e31433c62aaeef42f7640974f602ba");

    free(pubkey);

    size_t privkeywiflen = 100;
    char* privkeywif=dogecoin_char_vla(privkeywiflen);
    char privkeyhex[100];
    u_assert_int_eq(gen_privatekey(&dogecoin_chainparams_main, privkeywif, privkeywiflen, NULL), true);
    u_assert_int_eq(gen_privatekey(&dogecoin_chainparams_main, privkeywif, privkeywiflen, privkeyhex), true);

    uint8_t* privkey_data=dogecoin_uint8_vla(strlen(privkeywif));
    dogecoin_base58_decode_check(privkeywif, privkey_data, strlen(privkeywif) * sizeof(privkey_data[0]));
    u_assert_int_eq(privkey_data[0] == dogecoin_chainparams_main.b58prefix_secret_address, true);

    char privkey_hex_or_null[DOGECOIN_ECKEY_PKEY_LENGTH * 2 + 1];
    utils_bin_to_hex(privkey_data + 1, DOGECOIN_ECKEY_PKEY_LENGTH, privkey_hex_or_null);
    u_assert_str_eq(privkeyhex, privkey_hex_or_null);

    free(privkey_data);
    free(privkeywif);
    
    size_t masterkeysize = 200;
    char* masterkey=dogecoin_char_vla(masterkeysize);
    u_assert_int_eq(hd_gen_master(&dogecoin_chainparams_main, masterkey, masterkeysize), true);
    u_assert_int_eq(hd_print_node(&dogecoin_chainparams_main, masterkey), true);

    free(masterkey);

    size_t extoutsize = 200;
    char* extout=dogecoin_char_vla(extoutsize);
    const char* privkey = "dgpv557t1z21sLCnAz3cJPW5DiVErXdAi7iWpSJwBBaeN87umwje8LuTKREPTYPTNGXGnB3oNd2z6RmFFDU99WKbiRDJKKXfHxf48puZibauJYB";

    u_assert_int_eq(hd_derive(&dogecoin_chainparams_main, privkey, "m/0", extout, extoutsize), true);
    u_assert_str_eq(extout, "dgpv544MJMFeoz5LXkwbZTWwouwFje2Yp9c1A8ReNaapDFjW44jEcLXv3B3KQg3fjWXWVC9FGRyxLaCHjN1DUeGgoYJxMYM723wrLN6BArKUxe3");

    u_assert_int_eq(hd_derive(&dogecoin_chainparams_main, "dgpv51eADS3spNJh9gCpE1AyQ9NpMGkGh6MJKxM84Tf87KVLNeodEW76V2nJJRPorYLGnvZGJKTgEgvqGCtf9VS9RqhfJaTxV7iqm86VpMUNi5G", "m/3", extout, extoutsize), true);

    free(extout);
}
