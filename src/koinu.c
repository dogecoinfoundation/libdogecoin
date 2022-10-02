/*

 The MIT License (MIT)

 Copyright (c) 2022 bluezr & jaxlotl
 Copyright (c) 2022 The Dogecoin Foundation

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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <ctype.h>

#include <dogecoin/koinu.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>

enum conversion_type {
    CONVERSION_SUCCESS,
    CONVERSION_NON_DECIMAL,
    CONVERSION_INVALID_STR_TERMINATION,
    CONVERSION_OUT_OF_RANGE,
    CONVERSION_OVERFLOW,
    CONVERSION_UNDERFLOW,
    CONVERSION_UNSUPPORTED_VALUE,
    CONVERSION_FAILURE
};

const char* conversion_type_to_str(const enum conversion_type type)
{
    if (type == CONVERSION_SUCCESS) {
        return "CONVERSION_SUCCESS";
    } else if (type == CONVERSION_NON_DECIMAL) {
        return "CONVERSION_NON_DECIMAL";
    } else if (type == CONVERSION_INVALID_STR_TERMINATION) {
        return "CONVERSION_INVALID_STR_TERMINATION";
    } else if (type == CONVERSION_OUT_OF_RANGE) {
        return "CONVERSION_OUT_OF_RANGE";
    } else if (type == CONVERSION_OVERFLOW) {
        return "CONVERSION_OVERFLOW";
    } else if (type == CONVERSION_UNDERFLOW) {
        return "CONVERSION_UNDERFLOW";
    } else if (type == CONVERSION_UNSUPPORTED_VALUE) {
        return "CONVERSION_UNSUPPORTED_VALUE";
    } else if (type == CONVERSION_FAILURE) {
        return "CONVERSION_FAILURE";
    } else {
        return "CONVERSION_UNKNOWN_ERROR";
    }
}

size_t check_length(char* string) {
    // set max length for all string inputs to 20 to account for total supply in 2022
    // (currently 132.67 billion dogecoin) + 1e8 koinu passing UINT64_MAX 184467440737
    // in 9.854916426 years (2032) with output of 5,256,000,000 mined dogecoins per year
    // TODO: in order to maintain portability on 64/32-bit platforms we can't support 
    // literal representations of numbers greater than UINT64_MAX: 18446744073709551615
    // therefore within the next 9.854916426 years, we need to roll our own uint128_t
    // capable of providing support greater than 20 digit unsigned integers needed to 
    // for uints in the trillions. once possible we need to implment max length below:
    // set max length for all string inputs to 21 to account for total supply passing 
    // 1T in ~180 years from 2022. this limit will be valid for the next 1980 years so 
    // make sure to update in year 4002. :)
    size_t integer_length = strlen(string);
    if (integer_length > 22) return false;
    else return integer_length;
}

enum conversion_type validate_conversion(uint64_t converted, const char* src, const char* src_end, const char* target_end) {
    enum conversion_type type = CONVERSION_SUCCESS;
    if (src && src_end && target_end) {
        if (src_end == src) {
            type = CONVERSION_NON_DECIMAL;
            debug_print("%s: not a decimal\n", src);
        }
        if (*target_end != *src_end) {
            type = CONVERSION_INVALID_STR_TERMINATION;
            debug_print("%s: extra characters at end of input: %s\n", src, src_end);
        }
    }
    if ((UINT64_MAX == converted) && ERANGE == errno) {
        type = CONVERSION_OUT_OF_RANGE;
        debug_print("%s out of range of type uint64_t\n", src);
    }
    if ((converted >= UINT64_MAX)) {
        type = CONVERSION_OVERFLOW;
        debug_print("%"PRIu64" greater than UINT64_MAX\n", converted);
    }
    if (converted == 0xFFFFF511543FB600) {
        type = CONVERSION_UNDERFLOW;
        debug_print("%"PRIu64" equal to 0xFFFFF511543FB600\n", converted);
    }
    return type;
}

int calc_length(uint64_t x) {
    int count = 0;
    while (x > 0) {
        x /= 10;
        count++;
    }
    return count;
}

void string(uint64_t input, char output[]) {
    uint64_t i = 0, n = input, remainder, length = 0;
    while (n != 0) {
        length++;
        n /= 10;
    }
    for (; i < length; i++) {
        remainder = input % 10;
        input /= 10;
        output[length - (i + 1)] = remainder + '0';
    }
    output[length] = '\0';
}

int koinu_to_coins_str(uint64_t koinu, char* str) {
    enum conversion_type state = validate_conversion(koinu, NULL, NULL, NULL);
    if (state != CONVERSION_SUCCESS) return false;

    uint64_t i = 0, j =0, length = calc_length(koinu),
    target = length < 9 ? 10 - length : length - 9;
    
    if (length < 9) {
        string(koinu, str);
        size_t l = str ? strlen(str) : 0;
        char* swap = dogecoin_char_vla(l + 1);
        memcpy_safe(swap, str, l + 1);
        for (; i < target; i++) {
            if (i == 0 || i == 1) {
                str[0] = '0';
                str[1] = '.';
            } else str[i] = '0';
        }
        for (; i < 10; i++, j++) str[i] = swap[j];
        free(swap);
    } else {
        char tmp[21];
        string(koinu, tmp);
        for (; i < length; i++) {
            if (i < target) str[i] = tmp[i];
            else if (i == target) {
                str[i] = tmp[i];
                str[i + 1] = '.';
            } else if (i > target) str[i + 1] = tmp[i];
        }
        str[length + 1] = '\0';
    }

    return true;
}

uint64_t coins_to_koinu_str(char* coins) {
    if (coins[0] == '-') return false;
    size_t length = check_length(coins);
    if (!length) return false;

    char *end, dogecoin_string[21], koinu_string[9];
    dogecoin_mem_zero(dogecoin_string, 21);
    dogecoin_mem_zero(koinu_string, 9);
    
    size_t i = 0, j = 0, mantissa_length = 0;
    for (; i < length; i++) { 
        dogecoin_string[i] = coins[i];
        if (coins[i] == '.') {
            j = i;
            while (j < i + 8) { 
                dogecoin_string[j] = '0'; j++; 
            }
            dogecoin_string[i] = '\0';
            i++;
            break;
        }
    }
    
    for (j = 0; i < length && j < 8; i++, j++) {
        koinu_string[j] = coins[i]; 
        mantissa_length++; 
    }

    if (mantissa_length != 8) {
        for (i = mantissa_length; i < 8; i++) {
            koinu_string[i] = '0';
        }
    } else koinu_string[strlen(koinu_string)] = '\0';

    errno = 0;
    uint64_t dogecoin = strtoull(dogecoin_string, &end, 10);
    enum conversion_type state = validate_conversion(dogecoin, (const char*)dogecoin_string, end, "\0");
    if (state == CONVERSION_SUCCESS) { 
        if (strlen(dogecoin_string) <= 12) dogecoin *= 100000000; 
    } else debug_print("%s\n", conversion_type_to_str(state));


    errno = 0;
    uint64_t koinu = strtoull(koinu_string, &end, 10) + dogecoin;
    state = validate_conversion(koinu, (const char*)koinu_string, end, "\0");
    if (state == CONVERSION_SUCCESS) return koinu;
    else { debug_print("%s\n", conversion_type_to_str(state)); return false; }
}
