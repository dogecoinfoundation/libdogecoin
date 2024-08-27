/**********************************************************************
 * Copyright (c) 2024 edtubbs                                         *
 * Copyright (c) 2024 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <dogecoin/arith_uint256.h>
#include <dogecoin/utils.h>

void test_init_and_negate()
{
    // Test initialization to zero
    arith_uint256* zero = init_arith_uint256();
    for (int i = 0; i < WIDTH; i++) {
        assert(zero->pn[i] == 0); // All parts should be zero-initialized
    }

    // Test negation from zero to max and max to zero
    arith_uint256* max_val = init_arith_uint256();
    for (int i = 0; i < WIDTH; i++) {
        max_val->pn[i] = UINT32_MAX;
    }
    arith_negate(max_val);
    for (int i = 0; i < WIDTH; i++) {
        assert(max_val->pn[i] == 0); // Negation should bring max value to zero
    }

    dogecoin_free(zero);
    dogecoin_free(max_val);
}

void test_shift_operations()
{
    // Shift left and right by various amounts
    arith_uint256* one = init_arith_uint256();
    one->pn[0] = 1;
    arith_uint256* shifted = init_arith_uint256();

    for (unsigned int shift = 1; shift <= 256; shift++) {
        memcpy(shifted, one, sizeof(arith_uint256)); // Reset
        arith_shift_left(shifted, shift);
        arith_shift_right(shifted, shift);
        assert(shifted->pn[0] == (shift > 256 ? 0 : (shift == 256 ? 0 : 1))); // Expect original value or zero if shifted completely out
    }

    dogecoin_free(one);
    dogecoin_free(shifted);
}

void test_set_compact()
{
    struct {
        uint32_t compact;
        const char* expectedHex;
        bool expectedNegative;
        bool expectedOverflow;
    } testCases[] = {
        {0x0, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x00123456, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x01003456, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x02000056, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x03000000, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x04000000, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x00923456, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x01803456, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x02800056, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x03800000, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x04800000, "0000000000000000000000000000000000000000000000000000000000000000", false, false},
        {0x01123456, "0000000000000000000000000000000000000000000000000000000000000012", false, false},
        {0x20123456, "1234560000000000000000000000000000000000000000000000000000000000", false, false},
        {0xff123456, "", false, true}, // This case indicates overflow, the expectedHex is empty.
    };

    for (size_t i = 0; i < sizeof(testCases) / sizeof(testCases[0]); i++) {
        dogecoin_bool pfNegative = 0;
        dogecoin_bool pfOverflow = 0;
        arith_uint256* num = set_compact(init_arith_uint256(), testCases[i].compact, &pfNegative, &pfOverflow);

        arith_uint256* expectedNum = init_arith_uint256();
        utils_uint256_sethex((char*)testCases[i].expectedHex, (uint8_t*)expectedNum->pn);

        debug_print("Test #%zu: Compact = %08x\n", i + 1, testCases[i].compact);
        if (!testCases[i].expectedOverflow) {
            debug_print("Expected: %s, Got: %s\n", utils_uint8_to_hex((const uint8_t*)expectedNum->pn, sizeof(expectedNum->pn)), utils_uint8_to_hex((const uint8_t*)num->pn, sizeof(num->pn)));
            assert(memcmp(num->pn, expectedNum->pn, sizeof(num->pn)) == 0);
        }

        assert(pfNegative == testCases[i].expectedNegative);
        assert(pfOverflow == testCases[i].expectedOverflow);

        dogecoin_free(num);
        dogecoin_free(expectedNum);
    }
}

void test_arithmetic_and_comparison_operations()
{
    struct {
        const char* a_hex;
        const char* b_hex;
        const char* sum_hex;
        const char* diff_hex; // `a_hex - b_hex`, assuming `a >= b` for simplicity
    } testVectors[] = {
        {"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE", "01", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD"},
        {"01", "01", "02", "00"},
        {"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE", "02", "00", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC"},
        {"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", "01", "00", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE"},
        {"01", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", "00", NULL},
        // Add more vectors as needed
    };

    for (size_t i = 0; i < sizeof(testVectors) / sizeof(testVectors[0]); i++) {
        arith_uint256* a = init_arith_uint256();
        utils_uint256_sethex((char*)testVectors[i].a_hex, (uint8_t*)a->pn);
        arith_uint256* b = init_arith_uint256();
        utils_uint256_sethex((char*)testVectors[i].b_hex, (uint8_t*)b->pn);

        arith_uint256* sum = add_arith_uint256(a, b);
        arith_uint256* expectedSum = init_arith_uint256();
        utils_uint256_sethex((char*)testVectors[i].sum_hex, (uint8_t*)expectedSum->pn);
        assert(memcmp(sum->pn, expectedSum->pn, sizeof(sum->pn)) == 0);

        arith_uint256* diff = sub_arith_uint256(a, b);
        arith_uint256* expectedDiff = init_arith_uint256();
        debug_print("Test #%zu: a = %s, b = %s\n", i + 1, testVectors[i].a_hex, testVectors[i].b_hex);
        if (testVectors[i].diff_hex != NULL) {
            utils_uint256_sethex((char*)testVectors[i].diff_hex, (uint8_t*)expectedDiff->pn);
            debug_print("Expected: %s, Got: %s\n", testVectors[i].diff_hex, utils_uint8_to_hex((const uint8_t*)diff->pn, sizeof(diff->pn)));
            assert(memcmp(diff->pn, expectedDiff->pn, sizeof(diff->pn)) == 0);
        } else {
            assert(diff == NULL);
        }

        dogecoin_free(a);
        dogecoin_free(b);
        dogecoin_free(sum);
        dogecoin_free(diff);
        dogecoin_free(expectedSum);
        dogecoin_free(expectedDiff);
    }
}

int test_arith_uint256() {
    test_init_and_negate();
    test_shift_operations();
    test_set_compact();
    test_arithmetic_and_comparison_operations();

    return 0;
}
