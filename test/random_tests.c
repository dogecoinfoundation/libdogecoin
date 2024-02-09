/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022-2023 The Dogecoin Foundation                         *
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
    dogecoin_rnd_mapper mymapper = {test_random_init_cb, test_random_bytes_cb};
    dogecoin_rnd_set_mapper(mymapper);
    u_assert_int_eq(dogecoin_random_bytes(r_buf, 32, 0), false);
    for (uint8_t i = 0; i < 32; ++i)
        u_assert_int_eq(r_buf[i], 0);
    // switch back to the default random callback mapper
    dogecoin_rnd_set_mapper_default();
}
