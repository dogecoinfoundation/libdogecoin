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
 * libFuzzer harness for the on-disk compact filter stores.
 *
 * The wire deserializers have their own harness (fuzz_compact_filter). This
 * one covers the other parser: cfheaders.dat and cfilters.dat, which are read
 * back at startup. Their contents are not attacker-supplied over the network,
 * but they are attacker-influenced -- every record was written from data a peer
 * sent -- and a corrupt or tampered file is exactly the case the readers have
 * to survive rather than trust.
 *
 * cfilters.dat is the more interesting of the two. Each record carries its own
 * length field, and iterate() sizes an allocation directly from it:
 *
 *   height (4) | block_hash (32) | data_len (4) | data[data_len]
 *
 * The harness is file-shaped because the readers take a path, not a buffer.
 * That caps throughput far below a buffer harness -- every execution writes and
 * reopens a file -- which is inherent to the target rather than a defect in the
 * harness.
 *
 * Build:  ./configure CC=clang CFLAGS="-fsanitize=fuzzer-no-link" --enable-fuzz && make fuzz
 * Run:    ./fuzz/fuzz_cfdb CORPUS_DIR -max_len=65536
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dogecoin/dogecoin.h>
#include <dogecoin/cfheadersdb_file.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/compact_filter.h>
#include <dogecoin/mem.h>
#include <dogecoin/vector.h>

/* Reused across executions: creating a fresh temp name per run would leave the
   fuzzer's working directory full of files and slow the loop further. */
static const char *CFDB_FUZZ_PATH = "fuzz_cfdb_input.dat";

static dogecoin_bool iter_cb(uint32_t height, const uint256_t block_hash,
                             const uint8_t *filter_data, uint32_t data_len,
                             void *ctx)
{
    /* Touch the record so a bad length or pointer is dereferenced rather than
       merely returned: a callback that ignores its arguments would let an
       out-of-bounds buffer pass through unnoticed. */
    volatile uint8_t sink = 0;
    unsigned int *count = (unsigned int *)ctx;
    (void)height;
    sink ^= block_hash[0];
    if (filter_data && data_len > 0) {
        sink ^= filter_data[0];
        sink ^= filter_data[data_len - 1];
    }
    (void)sink;
    if (++(*count) > 4096) return false;   /* bound the loop, not the parser */
    return true;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 1) return 0;

    uint8_t selector = data[0];
    FILE *f = fopen(CFDB_FUZZ_PATH, "wb");
    if (!f) return 0;
    if (size > 1) fwrite(data + 1, 1, size - 1, f);
    fclose(f);

    if (selector & 0x01) {
        /* cfilters.dat: record headers plus length-prefixed payloads. */
        dogecoin_cfilters_db *db =
            dogecoin_cfilters_db_new(&dogecoin_chainparams_main, false);
        if (db) {
            if (dogecoin_cfilters_db_load(db, CFDB_FUZZ_PATH)) {
                unsigned int count = 0;
                dogecoin_cfilters_db_iterate(db, iter_cb, &count);
            }
            dogecoin_cfilters_db_free(db);
        }
    } else {
        /* cfheaders.dat: file header, version gate, then fixed-size records
           loaded into the filter state. */
        dogecoin_cfheaders_db *db =
            dogecoin_cfheaders_db_new(&dogecoin_chainparams_main, false);
        dogecoin_compact_filter_state *state = dogecoin_compact_filter_state_new();
        if (db && state)
            dogecoin_cfheaders_db_load(db, CFDB_FUZZ_PATH, state);
        if (state) dogecoin_compact_filter_state_free(state);
        if (db) dogecoin_cfheaders_db_free(db);
    }

    unlink(CFDB_FUZZ_PATH);
    return 0;
}
