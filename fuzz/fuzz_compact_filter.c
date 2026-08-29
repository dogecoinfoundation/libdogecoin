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
 * libFuzzer harness for the BIP157/158 deserializers.
 *
 * These parse messages a peer sends us before anything has been verified,
 * so they are the whole attacker-reachable surface of compact filter sync:
 *
 *   cfilter    filter_type | block_hash | var_bytes(filter)
 *   cfheaders  filter_type | stop_hash | prev_header | vec<filter_hash>
 *   cfcheckpt  filter_type | stop_hash | vec<filter_header>
 *
 * and the GCS decoder underneath them, which is the one that actually walks
 * attacker-supplied bits: Golomb-Rice decoding accumulates deltas in a loop
 * driven by the encoded data, so a truncated or hostile stream is the
 * interesting case rather than a well-formed one.
 *
 * The first input byte selects the target so one corpus covers all four.
 *
 * Build:  ./configure CC=clang CFLAGS="-fsanitize=fuzzer-no-link" --enable-fuzz && make fuzz
 * Run:    ./fuzz/fuzz_compact_filter CORPUS_DIR -max_len=100000
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <dogecoin/dogecoin.h>
#include <dogecoin/buffer.h>
#include <dogecoin/compact_filter.h>
#include <dogecoin/cstr.h>
#include <dogecoin/golomb.h>
#include <dogecoin/mem.h>
#include <dogecoin/vector.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) return 0;

    uint8_t selector = data[0];
    /* Byte 1 feeds filter_type for the GCS case, where the value selects the
       parameter set (M, P) and so changes how the payload is decoded. Leaving
       it fixed would pin the fuzzer to one parameterisation. */
    uint8_t filter_type = data[1];
    struct const_buffer buf = { data + 2, size - 2 };

    switch (selector & 0x03) {
    case 0: {
        dogecoin_cfilter_msg msg;
        dogecoin_cfilter_msg_init(&msg);
        dogecoin_p2p_msg_cfilter_deser(&msg, &buf);
        dogecoin_cfilter_msg_free(&msg);
        break;
    }
    case 1: {
        dogecoin_cfheaders_msg msg;
        dogecoin_cfheaders_msg_init(&msg);
        dogecoin_p2p_msg_cfheaders_deser(&msg, &buf);
        dogecoin_cfheaders_msg_free(&msg);
        break;
    }
    case 2: {
        dogecoin_cfcheckpt_msg msg;
        dogecoin_cfcheckpt_msg_init(&msg);
        dogecoin_p2p_msg_cfcheckpt_deser(&msg, &buf);
        dogecoin_cfcheckpt_msg_free(&msg);
        break;
    }
    case 3: {
        /* GCS decode. The block hash keys the SipHash used to map elements
           into the filter's range, so it is derived from the input rather
           than fixed: a constant key would exercise one hash schedule only. */
        uint256_t blockhash;
        unsigned int i;
        for (i = 0; i < 32; i++)
            blockhash[i] = (uint8_t)(data[(i + 2) % size]);

        gcs_filter *filter = gcs_filter_new();
        if (filter) {
            gcs_filter_deserialize(filter, filter_type, blockhash, &buf);
            gcs_filter_free(filter);
        }
        break;
    }
    }
    return 0;
}
