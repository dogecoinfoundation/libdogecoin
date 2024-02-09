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

void arith_negate(arith_uint256* input) {
    // Inverting all bits (one's complement)
    for (int i = 0; i < WIDTH; i++) {
        input->pn[i] = ~input->pn[i];
    }
}

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

arith_uint256* div_arith_uint256(arith_uint256* a, arith_uint256* b) {
    if (arith_uint256_is_zero(b)) {
        // Handle division by zero if necessary
        return NULL;
    }

    arith_uint256* quotient = init_arith_uint256();
    arith_uint256* remainder = init_arith_uint256();

    for (int i = WIDTH * 32 - 1; i >= 0; i--) {
        // Left shift remainder by 1 bit
        arith_shift_left(remainder, 1);

        // Set the least significant bit of remainder to bit i of a
        int word_idx = i / 32;
        int bit_idx = i % 32;
        if ((a->pn[word_idx] & (1 << bit_idx)) != 0) {
            remainder->pn[0] |= 1;
        }

        // Compare remainder with b
        if (arith_uint256_greater_than_or_equal(remainder, b)) {
            // Subtract b from remainder
            arith_uint256* temp = sub_arith_uint256(remainder, b);
            memcpy(remainder, temp, sizeof(arith_uint256));
            free(temp);

            // Set bit i of quotient
            quotient->pn[word_idx] |= (1 << bit_idx);
        }
    }

    free(remainder);
    return quotient;
}

arith_uint256* add_arith_uint256(arith_uint256* a, arith_uint256* b) {
    arith_uint256* result = init_arith_uint256();
    uint64_t carry = 0;
    for (int i = WIDTH - 1; i >= 0; i--) {
        uint64_t sum = (uint64_t)a->pn[i] + b->pn[i] + carry;
        result->pn[i] = sum; // This will only keep the lower 32 bits
        carry = sum >> 32; // This will keep the upper 32 bits (carry)
    }
    return result;
}

arith_uint256* sub_arith_uint256(arith_uint256* a, arith_uint256* b) {
    arith_uint256* result = init_arith_uint256();
    int64_t carry = 0;
    for (int i = WIDTH - 1; i >= 0; i--) {
        int64_t diff = (uint64_t)a->pn[i] - b->pn[i] - carry;
        result->pn[i] = diff; // This will only keep the lower 32 bits
        carry = (diff < 0) ? 1 : 0; // If diff is negative, there's a borrow
    }
    return result;
}

dogecoin_bool arith_uint256_is_zero(const arith_uint256* a) {
    for (int i = 0; i < WIDTH; i++) {
        if (a->pn[i] != 0) return false;
    }
    return true;
}

dogecoin_bool arith_uint256_less_than(const arith_uint256* a, const arith_uint256* b) {
    for (int i = WIDTH - 1; i >= 0; i--) {
        if (a->pn[i] < b->pn[i]) return true;
        else if (a->pn[i] > b->pn[i]) return false;
    }
    return false;
}

dogecoin_bool arith_uint256_equal(const arith_uint256* a, const arith_uint256* b) {
    for (int i = 0; i < WIDTH; i++) {
        if (a->pn[i] != b->pn[i]) return false;
    }
    return true;
}

dogecoin_bool arith_uint256_less_than_or_equal(const arith_uint256* a, const arith_uint256* b) {
    return arith_uint256_less_than(a, b) || arith_uint256_equal(a, b);
}

dogecoin_bool arith_uint256_greater_than(const arith_uint256* a, const arith_uint256* b) {
    return !arith_uint256_less_than_or_equal(a, b);
}

dogecoin_bool arith_uint256_greater_than_or_equal(const arith_uint256* a, const arith_uint256* b) {
    return !arith_uint256_less_than(a, b);
}

uint64_t get_low64(arith_uint256* a) {
    return ((uint64_t)a->pn[WIDTH - 2]) | (((uint64_t)a->pn[WIDTH - 1]) << 32);
}
