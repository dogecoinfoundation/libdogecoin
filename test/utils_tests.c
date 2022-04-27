/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/
#include <test/utest.h>

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <fenv.h>
#include <tgmath.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dogecoin/utils.h>

/* test a buffer overflow protection */
static const char hash_buffer_exc[] = "28969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c128969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c1";

static const char hex2[] = "AA969cdfFFffFF3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c1";

double compute_fn(double z)  // [1]
{
        #pragma STDC FENV_ACCESS ON  // [2]

        assert(FLT_EVAL_METHOD == 0);  // [3]

        if (isnan(z))  // [4]
                puts("z is not a number");

        if (isinf(z))
                puts("z is infinite");

        long double r = 7.0 - 3.0/(z - 2.0 - 1.0/(z - 7.0 + 10.0/(z - 2.0 - 2.0/(z - 3.0)))); // [5, 6]

        feclearexcept(FE_DIVBYZERO);  // [7]

        bool raised = fetestexcept(FE_OVERFLOW);  // [8]

        if (raised)
                puts("Unanticipated overflow.");

        return r;
}

void test_utils()
{

    #ifndef __STDC_IEC_559__
    puts("Warning: __STDC_IEC_559__ not defined. IEEE 754 floating point not fully supported."); // [9]
    #endif

    #pragma STDC FENV_ACCESS ON

    #ifdef TEST_NUMERIC_STABILITY_UP
    fesetround(FE_UPWARD);                   // [10]
    #elif TEST_NUMERIC_STABILITY_DOWN
    fesetround(FE_DOWNWARD);
    #endif

    printf("%.7g\n", compute_fn(3.0));
    printf("%.7g\n", compute_fn(NAN));
    int outlen = 0;
    unsigned char data[] = {0x00, 0xFF, 0x00, 0xAA, 0x00, 0xFF, 0x00, 0xAA};
    char hash[] = "28969cdfa74a12c82f3bad960b0b000aca2ac329deea5c2328ebc6f2ba9802c1";
    char hex[sizeof(data) * 2 + 1];
    unsigned char data2[sizeof(data)];
    uint8_t* hash_bin = utils_hex_to_uint8(hash);
    char* new = utils_uint8_to_hex(hash_bin, 32);
    unsigned char data3[64];
    assert(strncmp(new, hash, 64) == 0);

    utils_clear_buffers();

    utils_bin_to_hex(data, sizeof(data), hex);
    assert(strcmp(hex, "00ff00aa00ff00aa") == 0);

    utils_hex_to_bin(hex, data2, strlen(hex), &outlen);
    assert(outlen == 8);
    assert(memcmp(data, data2, outlen) == 0);
    utils_hex_to_uint8(hash_buffer_exc);

    /* test upper and lowercase A / F */
    utils_hex_to_bin(hex2, data3, strlen(hex2), &outlen);
    utils_hex_to_uint8(hex2);
    utils_clear_buffers();

    /* stress test conversion between coins and koinu, round values */
    long double coin_amounts[] = {  1.0e-9, 1.0e-8, 
                                    1.0e-7, 1.0e-6,
                                    1.0e-5, 1.0e-4,
                                    1.0e-3, 1.0e-2,
                                    1.0e-1, 1.0,
                                    1.0e1, 1.0e2,
                                    1.0e3, 1.0e4,
                                    1.0e5, 1.0e6,
                                    1.0e7, 1.0e8,
                                    1.0e9, 1.0e10 };

    uint64_t exp_answers[] = {      0UL, 1UL,
                                    10UL, 100UL,
                                    1000UL, 10000UL,
                                    100000UL, 1000000UL,
                                    10000000UL, 100000000UL,
                                    1000000000UL, 10000000000UL,
                                    100000000000UL, 1000000000000UL,
                                    10000000000000UL, 100000000000000UL,
                                    1000000000000000UL, 10000000000000000UL,
                                    100000000000000000UL, 1000000000000000000UL};
    
    uint64_t actual_answer;
    uint64_t diff;
    for (int i=0; i<20; i++) {
        actual_answer = coins_to_koinu(coin_amounts[i]);
        debug_print("T%d\n\tcoin_amt: %.9Lf\n\texpected: %llu\n\tactual: %llu\n\n", i, coin_amounts[i], exp_answers[i], actual_answer);
        diff = exp_answers[i] - actual_answer;
        u_assert_int_eq((int)diff, 0);
    }

    /* stress test conversion between coins and koinu, random decimal values */
    long double coin_amounts2[] =  {183447094.420691168L, 410357585.329255459L,
                                    567184894.440967455L, 1560227520.732426502L,
                                    2022535766.086211412L, 2047466422.707290167L,
                                    2487544599.240327145L, 4290779746.000111747L,
                                    4586257992.471687504L, 4660625607.783409803L,
                                    4766962398.856681418L, 5123141607.642632654L,
                                    5432527055.762317749L, 5778056333.994872841L,
                                    6654278072.590832439L, 7037268658.778085185L,
                                    7237308828.705953093L, 8606987445.409636773L,
                                    9100595327.168318456L, 9674059614.504642487L};

    uint64_t exp_answers2[] =      {18344709442069116UL,  41035758532925546UL,
                                    56718489444096744UL, 156022752073242650UL,
                                    202253576608621141UL, 204746642270729017UL,
                                    248754459924032715UL, 429077974600011175UL,
                                    458625799247168750UL, 466062560778340980UL,
                                    476696239885668142UL, 512314160764263265UL,
                                    543252705576231775UL, 577805633399487284UL,
                                    665427807259083244UL, 703726865877808519UL,
                                    723730882870595309UL, 860698744540963677UL,
                                    910059532716831846UL, 967405961450464249UL};

    for (int i=0; i<20; i++) {
        printf("-----------------------------------\n");
        long double tmp;
        printf("\ncoin_amounts2:                          %.9Lf\n", coin_amounts2[i]);
        actual_answer = coins_to_koinu(coin_amounts2[i]);
        printf("actual_answer (long double):            %.9Lf\n", actual_answer);
        printf("actual_answer (long long unsigned):     %llu\n", actual_answer);
        printf("exp_answers2 (long double):             %.9Lf\n", exp_answers2[i]);
        printf("exp_answers2 (long long unsigned):      %llu\n", exp_answers2[i]);
        tmp = koinu_to_coins(actual_answer);
        printf("tmp (long double):                      %.9Lf\n", tmp);
        printf("tmp (long long unsigned):               %llu\n", tmp);
        printf("coin_amounts2[i]==tmp:                  %d\n", coin_amounts2[i]==tmp);
        diff = exp_answers2[i] - actual_answer;
        printf("diff (int):                             %d\n", (int)diff);
        printf("diff (long long unsigned):              %llu\n", diff);
        debug_print("T%d\n\tcoin_amt: %.9Lf\n\texpected: %llu\n\tactual: %llu\n\n", i, coin_amounts2[i], exp_answers2[i], actual_answer);
        u_assert_uint32_eq(tmp, coin_amounts2[i]);
        // broken on arm:
        // u_assert_int_eq(diff, 0);
    }
}
