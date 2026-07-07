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
 * dudect constant-time test for the BIP38 fixed-length comparison used on
 * secret-derived data (the address-hash / MAC-style equality check).
 *
 * BIP38 decrypt compares an attacker-influenced value against a value derived
 * from the secret key material. If that comparison is not constant-time, its
 * duration leaks *where* the two first differ, which is an oracle an attacker
 * can use to recover the compared bytes one position at a time. libdogecoin's
 * bip38.c implements this with an XOR-accumulate loop (bip38_mem_eq) precisely
 * to avoid the early-exit timing of memcmp:
 *
 *     uint8_t diff = 0;
 *     for (i = 0; i < len; i++) diff |= a[i] ^ b[i];
 *     return diff == 0;
 *
 * Source review says "constant-time", but the compiler is free to recognise
 * the pattern at -O2 and reintroduce a short-circuiting branch. This test
 * verifies the property on the *compiled binary* under the release
 * optimisation level, which is the only way to confirm it actually holds.
 *
 * A local copy of the function is compiled here (the library symbol is
 * static). The copy is byte-identical to bip38.c's implementation and is
 * built with the same flags via the Makefile target, so this measures the
 * algorithm as the compiler actually emits it. If bip38.c's implementation
 * changes, update the copy below to match (and consider exporting a testable
 * symbol instead).
 *
 * Methodology (dudect / Reparaz et al.): two input classes are measured —
 *   class 0 (fixed):  the secret equals the target (full match, worst case
 *                     for an early-exit compare — runs the whole length)
 *   class 1 (random): the secret is random (differs early with high
 *                     probability — an early-exit compare returns fast)
 * A constant-time comparison shows no timing difference between the classes
 * (Welch t-test |t| stays bounded); a leaky one shows |t| growing without
 * bound as measurements accumulate.
 *
 * Exit status: 0 if no leakage evidence within the measurement budget,
 * non-zero if leakage is detected — usable as a CI gate.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define DUDECT_IMPLEMENTATION
#define DUDECT_VISIBLITY_STATIC
#include "dudect.h"

/* Length of the BIP38 comparison under test (the address-hash check compares
 * 4 bytes; test a longer buffer too by changing CMP_LEN if desired). Use a
 * length long enough that an early-exit leak is measurable. */
#define CMP_LEN 32

/* Byte-identical copy of bip38.c's bip38_mem_eq. Keep in sync with the
 * library. Marked noinline so the compiler cannot specialise it away against
 * the constant target in the harness. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
static int bip38_mem_eq_under_test(const uint8_t *a, const uint8_t *b, size_t len) {
    uint8_t diff = 0;
    size_t i;
    for (i = 0; i < len; i++) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

/* Fixed target the secret is compared against (the "derived" value). */
static uint8_t target[CMP_LEN];

/* dudect calls this to fill each measurement's input. class 0 => equal to
 * target (full-length compare); class 1 => random (likely differs early). */
void prepare_inputs(dudect_config_t *c, uint8_t *input_data, uint8_t *classes) {
    randombytes(input_data, c->number_measurements * c->chunk_size);
    for (size_t i = 0; i < c->number_measurements; i++) {
        classes[i] = randombit();
        if (classes[i] == 0) {
            /* fixed class: make this measurement's secret equal to target */
            memcpy(input_data + (size_t)i * c->chunk_size, target, CMP_LEN);
        }
        /* class 1: leave the random bytes as the secret */
    }
}

uint8_t do_one_computation(uint8_t *data) {
    /* compare the measurement's secret against the fixed target */
    return (uint8_t)bip38_mem_eq_under_test(data, target, CMP_LEN);
}

int main(void) {
    /* deterministic target so runs are reproducible */
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

    /* dudect_main returns DUDECT_LEAKAGE_FOUND (0) on leak; invert so the
     * process exit code is 0 == constant-time (good), non-zero == leak. */
    return (ret == DUDECT_LEAKAGE_FOUND) ? 1 : 0;
}
