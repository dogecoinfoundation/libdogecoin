/**
 * Copyright (c) 2009-2010 Satoshi Nakamoto
 * Copyright (c) 2009-2016 The Bitcoin Core developers
 * Copyright (c) 2023 bluezr
 * Copyright (c) 2023 The Dogecoin Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <dogecoin/arith_uint256.h>

arith_uint256 init_arith_uint256() {
    arith_uint256 x;
    x.WIDTH = 8;
    dogecoin_mem_zero(x.pn, x.WIDTH);
    return x;
}

arith_uint256 set_compact(arith_uint256 hash, uint32_t compact, dogecoin_bool *pf_negative, dogecoin_bool *pf_overflow) {
    int size = compact >> 24;
    uint32_t word = compact & 0x007fffff;
    if (size <= 3) {
        word >>= 8 * (3 - size);
        memcpy_safe(&hash, &word, sizeof word);
    } else {
        word <<= 8 * (size - 3);
        memcpy_safe(&hash, &word, sizeof word);
    }
    if (pf_negative) *pf_negative = word != 0 && (compact & 0x00800000) != 0;
    if (pf_overflow) *pf_overflow = word != 0 && ((size > 34) ||
                                                  (word > 0xff && size > 33) ||
                                                  (word > 0xffff && size > 32));
    return hash;
}

arith_uint256 uint_to_arith(const uint256* a)
{
    arith_uint256 b;
    b.WIDTH = 8;
    int x = 0;
    for(; x < b.WIDTH; ++x)
        b.pn[x] = read_le32((const unsigned char*)a + x * 4);
    return b;
}

uint256* arith_to_uint256(const arith_uint256 a) {
    uint256* b = dogecoin_uint256_vla(1);
    int x = 0;
    for(; x < a.WIDTH; ++x)
        write_le32((unsigned char*)b + x * 4, a.pn[x]);
    return b;
}

uint64_t get_low64(arith_uint256 a) {
    assert(a.WIDTH >= 2);
    return a.pn[0] | (uint64_t)a.pn[1] << 32;
}
