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

#include <test/utest.h>

#include <dogecoin/hash.h>
#include <dogecoin/tx.h>
#include <dogecoin/version.h>

/*
   SipHash-2-4 output with
   k = 00 01 02 ...
   and
   in = (empty string)
   in = 00 (1 byte)
   in = 00 01 (2 bytes)
   in = 00 01 02 (3 bytes)
   ...
   in = 00 01 02 ... 3e (63 bytes)

   from: https://131002.net/siphash/siphash24.c
*/
uint64_t siphash_4_2_testvec[] = {
    0x726fdb47dd0e0e31, 0x74f839c593dc67fd, 0x0d6c8009d9a94f5a, 0x85676696d7fb7e2d,
    0xcf2794e0277187b7, 0x18765564cd99a68d, 0xcbc9466e58fee3ce, 0xab0200f58b01d137,
    0x93f5f5799a932462, 0x9e0082df0ba9e4b0, 0x7a5dbbc594ddb9f3, 0xf4b32f46226bada7,
    0x751e8fbc860ee5fb, 0x14ea5627c0843d90, 0xf723ca908e7af2ee, 0xa129ca6149be45e5,
    0x3f2acc7f57c29bdb, 0x699ae9f52cbe4794, 0x4bc1b3f0968dd39c, 0xbb6dc91da77961bd,
    0xbed65cf21aa2ee98, 0xd0f2cbb02e3b67c7, 0x93536795e3a33e88, 0xa80c038ccd5ccec8,
    0xb8ad50c6f649af94, 0xbce192de8a85b8ea, 0x17d835b85bbb15f3, 0x2f2e6163076bcfad,
    0xde4daaaca71dc9a5, 0xa6a2506687956571, 0xad87a3535c49ef28, 0x32d892fad841c342,
    0x7127512f72f27cce, 0xa7f32346f95978e3, 0x12e0b01abb051238, 0x15e034d40fa197ae,
    0x314dffbe0815a3b4, 0x027990f029623981, 0xcadcd4e59ef40c4d, 0x9abfd8766a33735c,
    0x0e3ea96b5304a7d0, 0xad0c42d6fc585992, 0x187306c89bc215a9, 0xd4a60abcf3792b95,
    0xf935451de4f21df2, 0xa9538f0419755787, 0xdb9acddff56ca510, 0xd06c98cd5c0975eb,
    0xe612a3cb9ecba951, 0xc766e62cfcadaf96, 0xee64435a9752fe72, 0xa192d576b245165a,
    0x0a8787bf8ecb74b2, 0x81b3e73d20b49b6f, 0x7fa8220ba3b2ecea, 0x245731c13ca42499,
    0xb78dbfaf3a8d83bd, 0xea1ad565322a1a0b, 0x60e61c23a3795013, 0x6606d7e446282b93,
    0x6ca4ecb15c5f91e1, 0x9f626da15c9625f3, 0xe51b38608ef25f57, 0x958a324ceb064572
};

void test_hash()
{
    const char data[] = "cea946542b91ca50e2afecba73cf546ce1383d82668ecb6265f79ffaa07daa49abb43e21a19c6b2b15c8882b4bc01085a8a5b00168139dcb8f4b2bbe22929ce196d43532898d98a3b0ea4d63112ba25e724bb50711e3cf55954cf30b4503b73d785253104c2df8c19b5b63e92bd6b1ff2573751ec9c508085f3f206c719aa4643776bf425344348cbf63f1450389";
    const char expected[] = "52aa8dd6c598d91d580cc446624909e52a076064ffab67a1751f5758c9f76d26";
    uint256* digest_expected;
    digest_expected = (uint256*)utils_hex_to_uint8(expected);
    uint256 hashout;
    dogecoin_hash((const unsigned char*)data, strlen(data), hashout);
    assert(memcmp(hashout, digest_expected, sizeof(hashout)) == 0);

    struct siphasher* hasher = init_siphasher();
    siphasher_set(hasher, 0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    u_assert_uint64_eq(hasher->finalize(hasher), 0x726fdb47dd0e0e31ull);
    static const unsigned char t0[1] = {0};
    hasher->hash(hasher, t0, 1);
    u_assert_uint64_eq(hasher->finalize(hasher), 0x74f839c593dc67fdull);
    static const unsigned char t1[7] = {1,2,3,4,5,6,7};
    hasher->hash(hasher, t1, 7);
    u_assert_uint64_eq(hasher->finalize(hasher), 0x93f5f5799a932462ull);
    hasher->write(hasher, 0x0F0E0D0C0B0A0908ULL);
    u_assert_uint64_eq(hasher->finalize(hasher), 0x3f2acc7f57c29bdbull);
    static const unsigned char t2[2] = {16,17};
    hasher->hash(hasher, t2, 2);
    u_assert_uint64_eq(hasher->finalize(hasher), 0x4bc1b3f0968dd39cull);
    static const unsigned char t3[9] = {18,19,20,21,22,23,24,25,26};
    hasher->hash(hasher, t3, 9);
    u_assert_uint64_eq(hasher->finalize(hasher), 0x2f2e6163076bcfadull);
    static const unsigned char t4[5] = {27,28,29,30,31};
    hasher->hash(hasher, t4, 5);
    u_assert_uint64_eq(hasher->finalize(hasher), 0x7127512f72f27cceull);
    hasher->write(hasher, 0x2726252423222120ULL);
    u_assert_uint64_eq(hasher->finalize(hasher), 0x0e3ea96b5304a7d0ull);
    hasher->write(hasher, 0x2F2E2D2C2B2A2928ULL);
    u_assert_uint64_eq(hasher->finalize(hasher), 0xe612a3cb9ecba951ull);

    uint256* hash_in = dogecoin_uint256_vla(1);
    utils_uint256_sethex("1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a09080706050403020100", (uint8_t*)hash_in);
    u_assert_uint64_eq(siphash_u256(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL, hash_in), 0x7127512f72f27cceull);

    // Check test vectors from spec, one byte at a time
    struct siphasher* hasher2 = init_siphasher();
    siphasher_set(hasher2, 0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    uint8_t x=0;
    for (; x<ARRAYLEN(siphash_4_2_testvec); ++x)
    {
        u_assert_uint64_eq(hasher2->finalize(hasher2), siphash_4_2_testvec[x]);
        hasher2->hash(hasher2, &x, 1);
    }

    // Check test vectors from spec, eight bytes at a time
    struct siphasher* hasher3 = init_siphasher();
    siphasher_set(hasher3, 0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL);
    for (x = 0; x<ARRAYLEN(siphash_4_2_testvec); x += 8)
    {
        u_assert_uint64_eq(hasher3->finalize(hasher3), siphash_4_2_testvec[x]);
        hasher3->write(hasher3, (uint64_t)(x)|((uint64_t)(x+1)<<8)|((uint64_t)(x+2)<<16)|((uint64_t)(x+3)<<24)|
                     ((uint64_t)(x+4)<<32)|((uint64_t)(x+5)<<40)|((uint64_t)(x+6)<<48)|((uint64_t)(x+7)<<56));
    }

    hashwriter* hw = init_hashwriter(SER_DISK, CLIENT_VERSION);
    dogecoin_tx* tx = dogecoin_tx_new();
    // Note these tests were originally written with tx.nVersion=1
    // and the test would be affected by default tx version bumps if not fixed.
    tx->version = 1;
    dogecoin_tx_serialize(hw->cstr, tx);
    u_assert_uint64_eq(siphash_u256(1, 2, (uint256*)hw->get_hash(hw)), 0x79751e980c2a0a35ULL);

    dogecoin_free(hash_in);
    dogecoin_free(hasher);
    dogecoin_free(hasher2);
    dogecoin_free(hasher3);
    dogecoin_hashwriter_free(hw);
    dogecoin_tx_free(tx);
}
