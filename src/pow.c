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

dogecoin_bool check_pow(uint256* hash, unsigned int nbits, const dogecoin_chainparams *params, uint256* chainwork) {
    dogecoin_bool f_negative, f_overflow;
    arith_uint256* target = init_arith_uint256();
    target = set_compact(target, nbits, &f_negative, &f_overflow);
    swap_bytes((uint8_t*)target, sizeof (arith_uint256));
    uint8_t* target_uint256 = dogecoin_malloc(sizeof(uint256));
    memcpy(target_uint256, target, sizeof(arith_uint256));
    if (f_negative || (const uint8_t*)target == 0 || f_overflow || uint256_cmp(target_uint256, params->pow_limit)) {
        printf("%d:%s: f_negative: %d target == 0: %d f_overflow: %d\n",
        __LINE__, __func__, f_negative, (const uint8_t*)target == 0, f_overflow);
        dogecoin_free(target);
        dogecoin_free(target_uint256);
        return false;
    }
    if (uint256_cmp((const uint8_t*)hash, target_uint256)) {
        char* rtn_str = utils_uint8_to_hex((const uint8_t*)hash, 32);
        char hash_str[65] = "";
        strncpy(hash_str, rtn_str, 64);
        char* target_str = utils_uint8_to_hex((const uint8_t*)target, 32);
        printf("%d:%s: hash: %s target: %s\n",
        __LINE__, __func__, hash_str, target_str);
        dogecoin_free(target);
        dogecoin_free(target_uint256);
        return false;
    }
    dogecoin_free(target_uint256);

    // Calculate number of hashes done
    // hashes = ~target / (target + 1) + 1
    arith_uint256* neg_target = init_arith_uint256();
    memcpy(neg_target, target, sizeof(arith_uint256));
    arith_negate(neg_target);

    arith_uint256* one = init_arith_uint256();
    one->pn[0] = 1; // Set the lowest word to 1

    swap_bytes((uint8_t*)neg_target, sizeof(arith_uint256));
    swap_bytes((uint8_t*)target, sizeof(arith_uint256));

    // hashes = ~target / (target + 1)
    arith_uint256* target_plus_one = add_arith_uint256(target, one);
    arith_uint256* hashes = div_arith_uint256(neg_target, target_plus_one);

    // Add one to hashes for ~target / (target + 1) + 1
    arith_uint256* final_hashes = add_arith_uint256(hashes, one);

    if (chainwork != NULL) {
        memcpy(chainwork, (const arith_uint256*) final_hashes, sizeof(uint256));
    }

    // Clean up
    dogecoin_free(neg_target);
    dogecoin_free(one);
    dogecoin_free(target_plus_one);
    dogecoin_free(hashes);
    dogecoin_free(final_hashes);
    dogecoin_free(target);
    return true;
}
