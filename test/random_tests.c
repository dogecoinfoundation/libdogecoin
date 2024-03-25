/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2024 bluezr                                          *
 * Copyright (c) 2024 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <test/utest.h>

#include <dogecoin/random.h>
#include <dogecoin/utils.h>

void test_random_init_cb(void)
{
}

dogecoin_bool test_random_bytes_cb(uint8_t* buf, uint32_t len, const uint8_t update_seed)
{
    (void)(update_seed);
    for (uint32_t i = 0; i < len; i++)
        buf[i] = 0;
    return false;
}

void test_random()
{
    unsigned char r_buf[32];
    dogecoin_mem_zero(r_buf, 32);
    dogecoin_random_init();
    dogecoin_bool ret = dogecoin_random_bytes(r_buf, 32, 0);
    u_assert_int_eq(ret, true);
    unsigned char r_buf64[64];
    dogecoin_mem_zero(r_buf64, 64);
    dogecoin_random_init();
    dogecoin_bool ret64 = dogecoin_random_bytes(r_buf64, 64, 0);
    u_assert_int_eq(ret64, true);

    fast_random_context* this = init_fast_random_context(true, (const uint256*)r_buf);
    fast_random_context* this2 = init_fast_random_context(true, (const uint256*)r_buf);
    dogecoin_mem_zero(r_buf, 32);
    this->rng->output(this->rng, r_buf, 32);
    this2->rng->output(this2->rng, r_buf, 32);

    assert(this->randbool == this2->randbool);
    u_assert_int_eq(this->randbits(this, 3), this2->randbits(this2, 3));
    u_assert_uint32_eq(this->rand32(this), this2->rand32(this2));
    u_assert_uint64_eq(this->rand64(this), this->rand64(this2));
    uint256* r256_1 = this->rand256(this);
    uint256* r256_2 = this2->rand256(this2);
    u_assert_mem_eq(r256_1, r256_2, 32);
    dogecoin_free(r256_1);
    dogecoin_free(r256_2);
    free_fast_random_context(this);
    free_fast_random_context(this2);

    fast_random_context* this3 = init_fast_random_context(false, 0);
    fast_random_context* this4 = init_fast_random_context(false, 0);
    u_assert_uint32_not_eq(this3->rand32(this3), this4->rand32(this4));
    u_assert_uint64_not_eq(this3->rand64(this3), this4->rand64(this4));
    uint256* r256_3 = this3->rand256(this3);
    uint256* r256_4 = this4->rand256(this4);
    u_assert_mem_not_eq(r256_3, r256_4, 32);
    dogecoin_free(r256_3);
    dogecoin_free(r256_4);
    free_fast_random_context(this3);
    free_fast_random_context(this4);

    dogecoin_rnd_mapper mymapper = {test_random_init_cb, test_random_bytes_cb};
    dogecoin_rnd_set_mapper(mymapper);
    u_assert_int_eq(dogecoin_random_bytes(r_buf, 32, 0), false);
    for (uint8_t i = 0; i < 32; ++i)
        u_assert_int_eq(r_buf[i], 0);
    // switch back to the default random callback mapper
    dogecoin_rnd_set_mapper_default();
}
