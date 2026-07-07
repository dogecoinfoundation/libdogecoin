/*

 The MIT License (MIT)

 Copyright (c) 2026 bluezr
 Copyright (c) 2026 The Dogecoin Foundation

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

/*
 * dudect constant-time test for dogecoin_mem_cmp_ct (src/mem.c) — the
 * library's core constant-time comparison primitive, used wherever secret
 * material must be compared without leaking where two values first differ.
 *
 * Unlike ct_bip38_mem_eq (which compiles a copy of a static function), this
 * test links and calls the REAL exported symbol via <dogecoin/mem.h>, so it
 * measures the exact code the library ships. There is no keep-in-sync caveat.
 *
 * dogecoin_mem_cmp_ct uses a volatile XOR-accumulate loop:
 *
 *     volatile uint8_t diff = 0;
 *     for (i = 0; i < len; i++) diff |= (uint8_t)(pa[i] ^ pb[i]);
 *     return diff;
 *
 * The volatile qualifier discourages the compiler from turning this into a
 * short-circuiting compare, but does not contractually guarantee constant
 * time — so the property is verified here on the compiled, linked binary.
 *
 * Methodology (dudect): two input classes are measured —
 *   class 0 (fixed):  the input equals the target (full-length match)
 *   class 1 (random): the input is random (differs early with high prob.)
 * A constant-time comparison shows the Welch t-statistic bounded and not
 * growing; a leaky one shows |t| climbing past ~4.5.
 *
 * Exit status: 0 if no leakage evidence within the budget, non-zero if a
 * leak is detected — usable as a CI gate.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <dogecoin/mem.h>

#define DUDECT_IMPLEMENTATION
#define DUDECT_VISIBLITY_STATIC
#include "dudect.h"

#define CMP_LEN 32

static uint8_t target[CMP_LEN];

void prepare_inputs(dudect_config_t *c, uint8_t *input_data, uint8_t *classes) {
    randombytes(input_data, c->number_measurements * c->chunk_size);
    for (size_t i = 0; i < c->number_measurements; i++) {
        classes[i] = randombit();
        if (classes[i] == 0) {
            memcpy(input_data + (size_t)i * c->chunk_size, target, CMP_LEN);
        }
    }
}

uint8_t do_one_computation(uint8_t *data) {
    /* real exported symbol; returns 0 when equal */
    return (uint8_t)dogecoin_mem_cmp_ct(data, target, CMP_LEN);
}

int main(void) {
    for (int i = 0; i < CMP_LEN; i++) target[i] = (uint8_t)(0xA5 ^ (i * 7));

    dudect_config_t config = {
        .chunk_size = CMP_LEN,
        .number_measurements = 1000000,
    };
    dudect_ctx_t ctx;
    dudect_init(&ctx, &config);

    int ret = DUDECT_NO_LEAKAGE_EVIDENCE_YET;
    while (ret == DUDECT_NO_LEAKAGE_EVIDENCE_YET) {
        ret = dudect_main(&ctx);
    }
    dudect_free(&ctx);
    return (ret == DUDECT_LEAKAGE_FOUND) ? 1 : 0;
}
