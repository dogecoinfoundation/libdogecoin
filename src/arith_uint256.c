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

arith_uint256* init_arith_uint256() {
    arith_uint256* x = dogecoin_calloc(8, sizeof(uint32_t));
    int i = 0;
    for (; i < WIDTH; i++) {
        x->pn[i] = 0;
    }
    return x;
}

void arith_shift_left(arith_uint256* input, unsigned int shift) {
    // Temporary storage for the input as we're going to overwrite the data
    arith_uint256 temp = *input;

    // Clear the input
    for (int i = 0; i < WIDTH; i++) {
        input->pn[i] = 0;
    }

    int k = shift / 32;  // Number of full word shifts
    shift = shift % 32;  // Remaining shift

    // Perform the shift operation
    for (int i = 0; i < WIDTH; i++) {
        if (i + k + 1 < WIDTH && shift != 0)
            input->pn[i + k + 1] |= (temp.pn[i] >> (32 - shift));
        if (i + k < WIDTH)
            input->pn[i + k] |= (temp.pn[i] << shift);
    }
}

void arith_shift_right(arith_uint256* input, unsigned int shift) {
    // Temporary storage for the input as we're going to overwrite the data
    arith_uint256 temp = *input;

    // Clear the input
    for (int i = 0; i < WIDTH; i++) {
        input->pn[i] = 0;
    }

    int k = shift / 32;  // Number of full word shifts
    shift = shift % 32;  // Remaining shift

    // Perform the shift operation
    for (int i = 0; i < WIDTH; i++) {
        if (i - k - 1 >= 0 && shift != 0)
            input->pn[i - k - 1] |= (temp.pn[i] << (32 - shift));
        if (i - k >= 0)
            input->pn[i - k] |= (temp.pn[i] >> shift);
    }
}

arith_uint256* set_compact(arith_uint256* hash, uint32_t compact, dogecoin_bool *pf_negative, dogecoin_bool *pf_overflow) {
    int size = compact >> 24;
    uint32_t word = compact & 0x007fffff;
    if (size <= 3) {
        word >>= 8 * (3 - size);
        memcpy_safe(&hash->pn[0], &word, sizeof word);
    } else {
        memcpy_safe(&hash->pn[0], &word, sizeof word);
        arith_shift_left(hash, 8 * (size - 3));
    }
    if (pf_negative) *pf_negative = word != 0 && (compact & 0x00800000) != 0;
    if (pf_overflow) *pf_overflow = word != 0 && ((size > 34) ||
                                                  (word > 0xff && size > 33) ||
                                                  (word > 0xffff && size > 32));
    return hash;
}

arith_uint256* uint_to_arith(const uint256* a)
{
    static arith_uint256 b;
    memcpy_safe(b.pn, a, sizeof(b.pn));
    return &b;
}

uint8_t* arith_to_uint256(const arith_uint256* a) {
    static uint256 b = {0};
    memcpy_safe(b, a->pn, sizeof(uint256));
    return &b[0];
}

uint64_t get_low64(arith_uint256 a) {
    return a.pn[0] | (uint64_t)a.pn[1] << 32;
}
