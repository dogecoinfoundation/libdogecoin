/*

 The MIT License (MIT)

 Copyright (c) 2026 bluezr
 Copyright (c) 2026 The Dogecoin Foundation

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

#include <string.h>

#include <test/utest.h>

/* The installed headers and nothing else. A consumer could sign through the
   published surface but could not check a signature it was handed: the digest
   a signature covers came only from dogecoin_tx_sighash(), declared in tx.h,
   and the DER check only from dogecoin_pubkey_verify_sig(), declared in key.h,
   and neither header is installed. This pins the whole path to what ships. */
#include <dogecoin/libdogecoin.h>
#include <dogecoin/cstr.h>

/* one input, empty scriptSig, no outputs */
static const char* TX_HEX =
    "0100000001746007aed61e8531faba1af6610f10a5422c70a2a7eb6ffb51cb7a7b7b5e45b4"
    "0100000000ffffffff0000000000";

void test_sighash_surface(void)
{
    /* the runner owns the ecc context */
    unsigned char raw[128];
    size_t rawlen = 0;
    utils_hex_to_bin(TX_HEX, raw, strlen(TX_HEX), &rawlen);

    dogecoin_tx* tx = dogecoin_tx_new();
    u_assert_int_eq(dogecoin_tx_deserialize(raw, rawlen, tx, NULL), true);

    /* the script code a p2pkh input signs over */
    unsigned char spk[25] = {
        0x76, 0xa9, 0x14,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
        0x88, 0xac
    };
    cstring* script = cstr_new_buf(spk, sizeof(spk));
    u_assert_not_null(script);

    uint256_t hash;
    u_assert_int_eq(dogecoin_tx_sighash(tx, script, 0, SIGHASH_ALL, hash), true);

    /* the wrapper reports the same digest */
    uint8_t h32[32];
    u_assert_int_eq(dogecoin_tx_sighash32(tx, script, 0, SIGHASH_ALL, h32), true);
    u_assert_int_eq(memcmp(hash, h32, sizeof(h32)), 0);

    /* the script code binds: change it and the digest moves */
    unsigned char other_spk[25];
    memcpy(other_spk, spk, sizeof(spk));
    other_spk[5] ^= 0xff;
    cstring* other_script = cstr_new_buf(other_spk, sizeof(other_spk));
    uint256_t other_hash;
    u_assert_int_eq(dogecoin_tx_sighash(tx, other_script, 0, SIGHASH_ALL, other_hash), true);
    u_assert_int_eq(memcmp(hash, other_hash, sizeof(hash)) != 0, 1);

    /* sign that digest and check it back, which is what a receiving party does
       with a signature someone else produced */
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    u_assert_int_eq(dogecoin_privkey_gen(&key), true);
    dogecoin_pubkey pub;
    dogecoin_pubkey_init(&pub);
    dogecoin_pubkey_from_key(&key, &pub);
    u_assert_int_eq(dogecoin_pubkey_is_valid(&pub), true);

    unsigned char sig[80];
    size_t siglen = sizeof(sig);
    u_assert_int_eq(dogecoin_ecc_sign(key.privkey, hash, sig, &siglen), true);
    u_assert_int_eq(dogecoin_pubkey_verify_sig(&pub, hash, sig, siglen), true);

    /* a signature over a different digest must not verify */
    u_assert_int_eq(dogecoin_pubkey_verify_sig(&pub, other_hash, sig, siglen), false);

    /* nor may another key's */
    dogecoin_key key2;
    dogecoin_privkey_init(&key2);
    u_assert_int_eq(dogecoin_privkey_gen(&key2), true);
    dogecoin_pubkey pub2;
    dogecoin_pubkey_init(&pub2);
    dogecoin_pubkey_from_key(&key2, &pub2);
    u_assert_int_eq(dogecoin_pubkey_verify_sig(&pub2, hash, sig, siglen), false);

    dogecoin_privkey_cleanse(&key);
    dogecoin_privkey_cleanse(&key2);
    dogecoin_pubkey_cleanse(&pub);
    dogecoin_pubkey_cleanse(&pub2);
    cstr_free(script, true);
    cstr_free(other_script, true);
    dogecoin_tx_free(tx);
}
