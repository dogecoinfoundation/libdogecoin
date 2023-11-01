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

dogecoin_bool uint256_cmp(const uint256 a, const uint256 b) {
    for (int i = 0; i <= 31; i++) {
        if (a[i] > b[i]) {
            return true;
        } else if (a[i] < b[i]) {
            return false;
        }
    }
    return false; // Return false if all bytes are equal
}

dogecoin_bool check_pow(uint256* hash, unsigned int nbits, const dogecoin_chainparams *params) {
    dogecoin_bool f_negative, f_overflow;
    arith_uint256* target = init_arith_uint256();
    target = set_compact(target, nbits, &f_negative, &f_overflow);
    swap_bytes((uint8_t*)target, sizeof (arith_uint256));
    if (f_negative || (const uint8_t*)target == 0 || f_overflow || uint256_cmp((const uint8_t*) arith_to_uint256(target), params->pow_limit)) {
        printf("%d:%s: f_negative: %d target == 0: %d f_overflow: %d\n",
        __LINE__, __func__, f_negative, (const uint8_t*)target == 0, f_overflow);
        dogecoin_free(target);
        return false;
    }
    if (uint256_cmp((const uint8_t*)hash, arith_to_uint256(target))) {
        char* rtn_str = utils_uint8_to_hex((const uint8_t*)hash, 32);
        char hash_str[65] = "";
        strncpy(hash_str, rtn_str, 64);
        char* target_str = utils_uint8_to_hex((const uint8_t*)target, 32);
        printf("%d:%s: hash: %s target: %s\n",
        __LINE__, __func__, hash_str, target_str);
        dogecoin_free(target);
        return false;
    }
    dogecoin_free(target);
    return true;
}
