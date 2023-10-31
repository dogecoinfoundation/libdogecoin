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

#include <dogecoin/pow.h>

dogecoin_bool check_pow(uint256* hash, unsigned int nbits, const dogecoin_chainparams *params) {
    dogecoin_bool f_negative, f_overflow;
    arith_uint256 target = init_arith_uint256();
    target = set_compact(target, nbits, &f_negative, &f_overflow);
    arith_uint256 h = init_arith_uint256();
    h = uint_to_arith((const uint256*)hash);
    char* hash_str = utils_uint8_to_hex((const uint8_t*)&h.pn[0], 32);
    char* target_str = utils_uint8_to_hex((const uint8_t*)&target.pn[0], 32);
    if (f_negative || (const uint8_t*)&target.pn[0] == 0 || f_overflow || memcmp(&target.pn[0], &params->pow_limit, 32) > 0) {
        printf("%d:%s: f_negative: %d target == 0: %d f_overflow: %d memcmp target powlimit: %d\n", 
        __LINE__, __func__, f_negative, (const uint8_t*)&target.pn == 0, f_overflow, memcmp(&target.pn[0], &params->pow_limit, 32) > 0);
        return false;
    }
    if (strcmp(hash_str, target_str) > 0)
        return false;
    return true;
}
