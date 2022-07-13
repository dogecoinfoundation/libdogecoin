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
#include <math.h>
#include <fenv.h>
#include <errno.h>
#include <ctype.h>

#include <dogecoin/koinu.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>

long double rnd(long double v, long double digit) {
    long double _pow;
    _pow = pow(10.0, digit);
    long double t = v * _pow;
    long double r = ceil(t + 0.5);
    return r / _pow;
}

void show_fe_currentrnding_direction(void)
{
    switch (fegetround()) {
           case FE_TONEAREST:  debug_print ("FE_TONEAREST: %d\n", FE_TONEAREST);  break;
           case FE_DOWNWARD:   debug_print ("FE_DOWNWARD: %d\n", FE_DOWNWARD);   break;
           case FE_UPWARD:     debug_print ("FE_UPWARD: %d\n", FE_UPWARD);     break;
           case FE_TOWARDZERO: debug_print ("FE_TOWARDZERO: %d\n", FE_TOWARDZERO); break;
           default:            debug_print ("%s\n", "unknown");
    };
}

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

int check_length(char* string) {
    int integer_length;
    // length minus 1 representative of decimal point and 8 representative of koinu
    integer_length = strlen(string);
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
    if (integer_length > 21) {
        return false;
    }
    return integer_length;
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
    uint64_t length = calc_length(koinu), target = length - 9, i = 0;
    char tmp[21];
    string(koinu, tmp);
    for (; i < length; i++) {
        if (i < target) {
            str[i] = tmp[i];
        } else if (i == target) {
            str[i] = tmp[i];
            str[i + 1] = '.';
        } else if (i > target) {
            str[i + 1] = tmp[i];
        }
    }
    str[length + 1] = '\0';
    return true;
}

uint64_t coins_to_koinu_str(char* coins) {
    if (!check_length(coins)) return false;
    enum conversion_type state;
    char coins_string[32];
    char koinu_string[32];
    sprintf(coins_string, "%s", coins);
    char* c_ptr = coins_string;
    char* k_ptr = koinu_string;
    int i;

    // copy all digits until end of string or decimal point is reached
    while (*c_ptr != '\0') {
        if (*c_ptr =='.') {
            c_ptr++;
            break;
        }
        memcpy(k_ptr, c_ptr, 1);
        c_ptr++;
        k_ptr++;
    }

    // pad mantissa with up to 8 trailing zeros
    if (strlen(c_ptr) != 8) {
        for (i = strlen(c_ptr); i < 8; i++) {
            if (i == 8) {
                c_ptr[i] = '\0';
            } else c_ptr[i] = '0';
            
        }
    }

    //copy remaining 8 or less decimal places
    for (i = 1; i <= 8; i++) {
        if (i==8) {
            memcpy(k_ptr, c_ptr, 1);
            k_ptr++;
            memset(k_ptr, '\0', 1);
            break;
        }
        memcpy(k_ptr, c_ptr, 1);
        c_ptr++;
        k_ptr++;
    }
    errno = 0;
    uint64_t result = strtoull(koinu_string, &k_ptr, 10);
    state = validate_conversion(result, (const char*)koinu_string, k_ptr, "\0");
    if (state == CONVERSION_SUCCESS) return result;
    else {
        debug_print("%s\n", conversion_type_to_str(state));
        return false;
    }
}

long double round_ld(long double x)
{
    fenv_t save_env;
    feholdexcept(&save_env);
    long double result = rintl(x);
    if (fetestexcept(FE_INEXACT)) {
        int const save_round = fegetround();
        fesetround(FE_UPWARD);
        result = rintl(copysignl(0.5 + fabsl(x), x));
        fesetround(save_round);
    }
    feupdateenv(&save_env);
    return result;
}

long double koinu_to_coins(uint64_t koinu) {
    long double output;
#if defined(__ARM_ARCH_7A__)
    int rounding_mode = fegetround();
    int l = calc_length(koinu);
    output = (long double)koinu / (long double)1e8;
    if (l >= 9) {
        fesetround(FE_UPWARD);
        output = (long double)koinu / (long double)1e8;
    } else if (l >= 17) {
        output = rnd((long double)koinu / (long double)1e8, 8.5) + .000000005;
    }
    fesetround(rounding_mode);
#elif defined(WIN32)
    output = (long double)koinu / (long double)1e8;
#else
    output = (long double)koinu / (long double)1e8;
#endif
    return output;
}

long long unsigned coins_to_koinu(long double coins) {
    long double output;
#if defined(__ARM_ARCH_7A__)
    long double integer_length, mantissa_length;
    char* str[22];
    dogecoin_mem_zero(str, 11);
    snprintf(&str, sizeof(str), "%.8Lf", coins);
    // length minus 1 representative of decimal and 8 representative of koinu
    integer_length = strlen(str) - 9;
    mantissa_length = integer_length + (8 - integer_length);
    if (integer_length <= mantissa_length) {
        output = coins * powl(10, mantissa_length);
    } else {
        output = round_ld(coins * powl(10, mantissa_length));
    }
#else
     output = (uint64_t)round((long double)coins * (long double)1e8);
#endif
    return output;
}
