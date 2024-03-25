/**********************************************************************
 * Copyright (c) 2017 The Bitcoin Core developers                     *
 * Copyright (c) 2024 bluezr                                          *
 * Copyright (c) 2024 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <test/utest.h>

#include <dogecoin/chacha20.h>
#include <dogecoin/utils.h>
#include <dogecoin/cstr.h>

void testchacha20(const char* hexkey, uint64_t nonce, uint64_t seek, const char* hexout)
{
    unsigned char* tmp = parse_hex(hexkey);
    size_t key_size = hexkey ? strlen(hexkey) / 2 : 0;
    cstring* key = cstr_new_sz(key_size);
    cstr_append_buf(key, tmp, key_size);
    chacha20* rng = chacha20_init((const unsigned char*)key->str, key->len);
    rng->setiv(rng, nonce);
    rng->seek(rng, seek);
    size_t out_size = hexout ? strlen(hexout) / 2 : 0;
    cstring* out = cstr_new_sz(out_size);
    dogecoin_free(tmp);
    tmp = parse_hex(hexout);
    cstr_append_buf(out, (const void*)tmp, out_size);

    cstring* outres = cstr_new(NULL);
    cstr_resize(outres, out->len);
    rng->output(rng, (unsigned char*)outres->str, outres->len);
    char* str1 = utils_uint8_to_hex((const uint8_t*)out->str, out->len),
    *str2 = utils_uint8_to_hex((const uint8_t*)outres->str, outres->len);
    u_assert_str_eq(str1, str2);
    dogecoin_free(tmp);
    cstr_free(key, true);
    cstr_free(out, true);
    cstr_free(outres, true);
    chacha20_free(rng);
}

void test_chacha20() {
    // Test vector from RFC 7539
    testchacha20("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", 0x4a000000UL, 1,
                 "224f51f3401bd9e12fde276fb8631ded8c131f823d2c06e27e4fcaec9ef3cf788a3b0aa372600a92b57974cded2b9334794cb"
                 "a40c63e34cdea212c4cf07d41b769a6749f3f630f4122cafe28ec4dc47e26d4346d70b98c73f3e9c53ac40c5945398b6eda1a"
                 "832c89c167eacd901d7e2bf363");

    // Test vectors from https://tools.ietf.org/html/draft-agl-tls-chacha20poly1305-04#section-7
    testchacha20("0000000000000000000000000000000000000000000000000000000000000000", 0, 0,
                 "76b8e0ada0f13d90405d6ae55386bd28bdd219b8a08ded1aa836efcc8b770dc7da41597c5157488d7724e03fb8d84a376a43b"
                 "8f41518a11cc387b669b2ee6586");
    testchacha20("0000000000000000000000000000000000000000000000000000000000000001", 0, 0,
                 "4540f05a9f1fb296d7736e7b208e3c96eb4fe1834688d2604f450952ed432d41bbe2a0b6ea7566d2a5d1e7e20d42af2c53d79"
                 "2b1c43fea817e9ad275ae546963");
    testchacha20("0000000000000000000000000000000000000000000000000000000000000000", 0x0100000000000000ULL, 0,
                 "de9cba7bf3d69ef5e786dc63973f653a0b49e015adbff7134fcb7df137821031e85a050278a7084527214f73efc7fa5b52770"
                 "62eb7a0433e445f41e3");
    testchacha20("0000000000000000000000000000000000000000000000000000000000000000", 1, 0,
                 "ef3fdfd6c61578fbf5cf35bd3dd33b8009631634d21e42ac33960bd138e50d32111e4caf237ee53ca8ad6426194a88545ddc4"
                 "97a0b466e7d6bbdb0041b2f586b");
    testchacha20("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", 0x0706050403020100ULL, 0,
                 "f798a189f195e66982105ffb640bb7757f579da31602fc93ec01ac56f85ac3c134a4547b733b46413042c9440049176905d3b"
                 "e59ea1c53f15916155c2be8241a38008b9a26bc35941e2444177c8ade6689de95264986d95889fb60e84629c9bd9a5acb1cc1"
                 "18be563eb9b3a4a472f82e09a7e778492b562ef7130e88dfe031c79db9d4f7c7a899151b9a475032b63fc385245fe054e3dd5"
                 "a97a5f576fe064025d3ce042c566ab2c507b138db853e3d6959660996546cc9c4a6eafdc777c040d70eaf46f76dad3979e5c5"
                 "360c3317166a1c894c94a371876a94df7628fe4eaaf2ccb27d5aaae0ad7ad0f9d4b6ad3b54098746d4524d38407a6deb3ab78"
                 "fab78c9");
}
