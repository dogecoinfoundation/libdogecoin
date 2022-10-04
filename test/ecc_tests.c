/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utest.h"

#include <dogecoin/ecc.h>
#include <dogecoin/key.h>
#include <dogecoin/random.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>

void test_ecc()
{
    unsigned char r_buf[32];
    dogecoin_mem_zero(r_buf, 32);
    dogecoin_random_init();
    while (dogecoin_ecc_verify_privatekey(r_buf) == 0)
        dogecoin_random_bytes(r_buf, 32, 0);
    memset(r_buf, 0xFF, 32);
    u_assert_int_eq(dogecoin_ecc_verify_privatekey(r_buf), 0); //secp256k1 overflow
    uint8_t pub_key33[33], pub_key33_invalid[33], pub_key65[65], pub_key65_invalid[65];
    memcpy_safe(pub_key33, utils_hex_to_uint8("02fcba7ecf41bc7e1be4ee122d9d22e3333671eb0a3a87b5cdf099d59874e1940f"), 33);
    memcpy_safe(pub_key33_invalid, utils_hex_to_uint8("999999999941bc7e1be4ee122d9d22e3333671eb0a3a87b5cdf099d59874e1940f"), 33);
    memcpy_safe(pub_key65, utils_hex_to_uint8("044054fd18aeb277aeedea01d3f3986ff4e5be18092a04339dcf4e524e2c0a09746c7083ed2097011b1223a17a644e81f59aa3de22dac119fd980b36a8ff29a244"), 65);
    memcpy_safe(pub_key65_invalid, utils_hex_to_uint8("044054fd18aeb277aeedea01d3f3986ff4e5be18092a04339dcf4e524e2c0a09746c7083ed2097011b1223a17a644e81f59aa3de22dac119fd980b39999f29a244"), 65);
    u_assert_int_eq(dogecoin_ecc_verify_pubkey(pub_key33, 1), 1);
    u_assert_int_eq(dogecoin_ecc_verify_pubkey(pub_key65, 0), 1);
    u_assert_int_eq(dogecoin_ecc_verify_pubkey(pub_key33_invalid, 1), 0);
    u_assert_int_eq(dogecoin_ecc_verify_pubkey(pub_key65_invalid, 0), 0);
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    assert(dogecoin_privkey_is_valid(&key) == 0);
    dogecoin_privkey_gen(&key);
    uint8_t* hash = utils_hex_to_uint8((const char*)"26db47a48a10b9b0b697b793f5c0231aa35fe192c9d063d7b03a55e3c302850a");
    unsigned char sig[74];
    size_t outlen = 74;
    dogecoin_key_sign_hash(&key, hash, sig, &outlen);
    uint8_t sigcomp[64];
    unsigned char sigder[74];
    size_t sigderlen = 74;
    u_assert_int_eq(dogecoin_ecc_der_to_compact(sig, outlen, sigcomp), true);
    u_assert_int_eq(dogecoin_ecc_compact_to_der_normalized(sigcomp, sigder, &sigderlen), true);
    u_assert_uint32_eq(outlen, sigderlen);
    u_assert_int_eq(memcmp(sig, sigder, sigderlen), 0);
}
