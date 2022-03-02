/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <dogecoin/buffer.h>

void test_buffer()
{
    struct const_buffer buf0 = {"data", 4};
    struct const_buffer buf0a = {"data1", 5};
    struct const_buffer buf1 = {"data", 4};
    struct buffer* buf2;
    assert(buffer_equal(&buf0, &buf1) == 1);
    assert(buffer_equal(&buf0, &buf0a) == 0);
    buf2 = buffer_copy(&buf0.p, buf0.len);
    buffer_free(buf2);
}
