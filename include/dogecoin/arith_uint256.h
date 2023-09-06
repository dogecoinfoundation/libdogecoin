/*

 The MIT License (MIT)

 Copyright (c) 2009-2010 Satoshi Nakamoto
 Copyright (c) 2009-2016 The Bitcoin Core developers
 Copyright (c) 2023 bluezr
 Copyright (c) 2023 The Dogecoin Foundation

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

#ifndef __LIBDOGECOIN_ARITH_UINT256_H__
#define __LIBDOGECOIN_ARITH_UINT256_H__

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <dogecoin/dogecoin.h>
#include <dogecoin/common.h>
#include <dogecoin/mem.h>

LIBDOGECOIN_BEGIN_DECL

struct uint_err {
    const char* str;
};

typedef struct base_uint_ {
    uint32_t pn[8]; // pn[WIDTH]
} base_uint_;

typedef base_uint_ arith_uint256;

arith_uint256* init_arith_uint256();
arith_uint256* set_compact(arith_uint256* hash, uint32_t compact, dogecoin_bool *pf_negative, dogecoin_bool *pf_overflow);
uint8_t* arith_to_uint256(const arith_uint256* a);
arith_uint256* uint_to_arith(const uint256* a);
uint64_t get_low64(arith_uint256 pn);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_ARITH_UINT256_H__
