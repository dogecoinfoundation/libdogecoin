/*
 The MIT License (MIT)
 
 Copyright (c) 2015 Jonas Schnelli
 Copyright (c) 2022 bluezr
 Copyright (c) 2022 The Dogecoin Foundation

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utest.h"

#include <dogecoin/key.h>
#include <dogecoin/utils.h>

void test_key()
{
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    assert(dogecoin_privkey_is_valid(&key) == 0);
    dogecoin_privkey_gen(&key);
    assert(dogecoin_privkey_is_valid(&key) == 1);
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    assert(dogecoin_pubkey_is_valid(&pubkey) == 0);
    dogecoin_pubkey_from_key(&key, &pubkey);
    assert(dogecoin_pubkey_is_valid(&pubkey) == 1);
    assert(dogecoin_privkey_verify_pubkey(&key, &pubkey) == 1);
    unsigned int i;
    for (i = 33; i < DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH; ++i)
        assert(pubkey.pubkey[i] == 0);
    uint8_t* hash = utils_hex_to_uint8((const char*)"26db47a48a10b9b0b697b793f5c0231aa35fe192c9d063d7b03a55e3c302850a");
    unsigned char sig[74];
    size_t outlen = 74;
    dogecoin_key_sign_hash(&key, hash, sig, &outlen);
    unsigned char sigcmp[64];
    size_t outlencmp = 64;
    dogecoin_key_sign_hash_compact(&key, hash, sigcmp, &outlencmp);
    unsigned char sigcmp_rec[64];
    size_t outlencmp_rec = 64;
    int recid;
    dogecoin_pubkey pubkey_rec;
    dogecoin_pubkey_init(&pubkey_rec);
    dogecoin_key_sign_hash_compact_recoverable(&key, hash, sigcmp_rec, &outlencmp_rec, &recid);
    dogecoin_key_sign_recover_pubkey(sigcmp_rec, hash, recid, &pubkey_rec);
    u_assert_int_eq(dogecoin_pubkey_verify_sig(&pubkey, hash, sig, outlen), true);
    u_assert_int_eq(dogecoin_pubkey_verify_sig(&pubkey, hash, sig, outlen), true);
    u_assert_mem_eq(pubkey.pubkey, pubkey_rec.pubkey, sizeof(pubkey.pubkey));
    char str[66 + 1];
    size_t size = sizeof(str);
    int r = dogecoin_pubkey_get_hex(&pubkey, str, &size);
    u_assert_int_eq(r, true);
    u_assert_uint32_eq(size, 66);
    size = 50;
    r = dogecoin_pubkey_get_hex(&pubkey, str, &size);
    u_assert_int_eq(r, false);
    dogecoin_privkey_cleanse(&key);
    dogecoin_pubkey_cleanse(&pubkey);
    dogecoin_key key_wif;
    dogecoin_privkey_init(&key_wif);
    assert(dogecoin_privkey_is_valid(&key_wif) == 0);
    dogecoin_privkey_gen(&key_wif);
    assert(dogecoin_privkey_is_valid(&key_wif) == 1);
    char wifstr[100];
    size_t wiflen = 100;
    dogecoin_privkey_encode_wif(&key_wif, &dogecoin_chainparams_main, wifstr, &wiflen);
    wiflen = 100;
    dogecoin_key key_wif_decode;
    dogecoin_privkey_decode_wif(wifstr, &dogecoin_chainparams_main, &key_wif_decode);
    u_assert_mem_eq(key_wif_decode.privkey, key_wif.privkey, sizeof(key_wif_decode.privkey));
}
